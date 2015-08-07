/*
 Diana - A Free Meteorological Visualisation Tool

 $Id: qtMainWindow.h 477 2008-05-06 09:53:22Z lisbethb $

 Copyright (C) 2006 met.no

 Contact information:
 Norwegian Meteorological Institute
 Box 43 Blindern
 0313 OSLO
 NORWAY
 email: diana@met.no

 This file is part of Diana

 Diana is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 Diana is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Diana; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "MovieMaker.h"

extern "C" {
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mathematics.h>
}

#include <QImage>

#include <sstream>
#include <unistd.h>

#include <sys/types.h>

#define MILOGGER_CATEGORY "diana.MovieMaker"
#include <miLogger/miLogging.h>

//#define VIDEO_BUF_SIZE 1835008
//#define VIDEO_BITRATE  6000 * 1024

#define VIDEO_BUF_SIZE 700835008
#define VIDEO_BITRATE  12960000

using namespace std;

namespace avhelpers {

#define rgbtoyuv(r, g, b, y, u, v) \
  y=(uint8_t)( (((int)(66*r)   +(int)(129*g) +(int)(25*b) + 128) >> 8 ) + 16 ); \
  u=(uint8_t)( (((int)(-38*r)  -(int)(74*g) +(int)(112*b) + 128) >> 8 ) + 128 ); \
  v=(uint8_t)( (((int)(112*r)   -(int)(94*g) -(int)(18*b) + 128) >> 8 ) + 128 );

void RGBtoYUV420P(const uint8_t *RGB, uint8_t *YUV,
    uint RGBIncrement, bool swapRGB, int width, int height, bool flip)
{
  const unsigned planeSize = width * height;
  const unsigned halfWidth = width >> 1;

  // get pointers to the data
  uint8_t *yplane = YUV;
  uint8_t *uplane = YUV + planeSize;
  uint8_t *vplane = YUV + planeSize + (planeSize >> 2);
  const uint8_t *RGBIndex = RGB;
  int RGBIdx[3];
  RGBIdx[0] = 0;
  RGBIdx[1] = 1;
  RGBIdx[2] = 2;
  if (swapRGB) {
    RGBIdx[0] = 2;
    RGBIdx[2] = 0;
  }

  for (int y = 0; y < (int) height; y++) {
    uint8_t *yline = yplane + (y * width);
    uint8_t *uline = uplane + ((y >> 1) * halfWidth);
    uint8_t *vline = vplane + ((y >> 1) * halfWidth);

    if (flip) // Flip horizontally
      RGBIndex = RGB + (width * (height - 1 - y) * RGBIncrement);

    for (int x = 0; x < width; x += 2) {
      rgbtoyuv ( RGBIndex[RGBIdx[0]], RGBIndex[RGBIdx[1]], RGBIndex[RGBIdx[2]],
          *yline, *uline, *vline );
      RGBIndex += RGBIncrement;
      yline++;
      rgbtoyuv ( RGBIndex[RGBIdx[0]], RGBIndex[RGBIdx[1]], RGBIndex[RGBIdx[2]],
          *yline, *uline, *vline );
      RGBIndex += RGBIncrement;
      yline++;
      uline++;
      vline++;
    }
  }
}

AVFrame* allocPicture(PixelFormat pixFormat, int width, int height)
{
  AVFrame *frame = avcodec_alloc_frame();
  if (!frame)
    return NULL;

  int size = avpicture_get_size(static_cast<PixelFormat>(pixFormat), width, height);
  uint8_t *buffer = (uint8_t*) av_malloc(size);
  if (!buffer) {
    av_free(frame);
    return NULL;
  }

  avpicture_fill((AVPicture *) frame, buffer, static_cast<PixelFormat>(pixFormat), width, height);
  return frame;
}

} // namespace avhelpers

MovieMaker::MovieMaker(const string &filename, const string &format,
    float delay, const QSize& frameSize)
  : mFrameSize(frameSize)
{
  METLIBS_LOG_SCOPE();
  g_strOutputVideoFile = filename;
  g_strOutputVideoFormat = format;
  this->delay = delay;

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(53, 0, 0)
  static bool is_avcodec_initialised = false;
  if (!is_avcodec_initialised) {
    // must be called before using avcodec lib
    avcodec_init();
    is_avcodec_initialised = true;
  }
#endif

  // register all the codecs
  avcodec_register_all();
  av_register_all();

  outputVideo.fileName = g_strOutputVideoFile.c_str();

  if (!initOutputStream(&outputVideo))
    METLIBS_LOG_ERROR("Cannot init output video stream '" << filename << "'");
}

MovieMaker::~MovieMaker()
{
  endOutputStream(&outputVideo);
}

std::string MovieMaker::outputFile() const
{
  return g_strOutputVideoFile;
}

std::string MovieMaker::outputFormat() const
{
  return g_strOutputVideoFormat;
}

bool MovieMaker::addVideoStream(OutputCtx *output)
{
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(54, 0, 0)
  output->videoStream = avformat_new_stream(output->outputCtx, 0);
#else
  output->videoStream = av_new_stream(output->outputCtx, 0);
#endif
  if (!output->videoStream)
    return false;

  AVCodecContext *video = output->videoStream->codec;
  // not sure which version replaced CodecID -> AVCodecID
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(54, 0, 0)
  video->codec_id = (AVCodecID) output->outputCtx->oformat->video_codec;
#else
  video->codec_id = (CodecID) output->outputCtx->oformat->video_codec;
#endif
  video->codec_type = AVMEDIA_TYPE_VIDEO;
  video->bit_rate = VIDEO_BITRATE;
  //video->sample_aspect_ratio.den = 16;
  //video->sample_aspect_ratio.num = 9;
  //output->videoStream->sample_aspect_ratio.den = 16;
  //output->videoStream->sample_aspect_ratio.num = 9;
  //  video->dtg_active_format = FF_DTG_AFD_4_3; only used for decoding
  video->width = mFrameSize.width();
  video->height = mFrameSize.height();
  video->time_base.den = 30000;
  video->time_base.num = 1001;
  //video->gop_size = 18;
  video->gop_size = 100;

  video->pix_fmt = PIX_FMT_YUV420P;
  video->rc_buffer_size = VIDEO_BUF_SIZE;
  video->rc_max_rate = VIDEO_BUF_SIZE;
  video->rc_min_rate = 0;

  // params from chris (Martin)
  video->mb_decision = 2;
  video->qblur = 1.0;
  video->compression_level = 2;
  video->me_sub_cmp = 2;
  video->dia_size = 2;
  //video->mv0_threshold = ???;
  video->last_predictor_count = 3;

  return true;
}

bool MovieMaker::openVideoEncoder(OutputCtx *output)
{
  AVCodecContext *video = output->videoStream->codec;

  // find the video encoder and open it
  AVCodec *codec = avcodec_find_encoder(video->codec_id);
  if (!codec) {
    METLIBS_LOG_ERROR("Video codec not found");
    return false;
  }

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53, 0, 0)
  const int ret_avco = avcodec_open2(video, codec, NULL);
#else
  const int ret_avco = avcodec_open(video, codec);
#endif
  if (ret_avco < 0) {
    METLIBS_LOG_ERROR("Could not open video codec");
    return false;
  }

  output->videoBuffer = (short *) av_malloc(VIDEO_BUF_SIZE);

  // allocate the encoded raw picture
  output->frame = avhelpers::allocPicture(video->pix_fmt, video->width, video->height);
  if (!output->frame) {
    METLIBS_LOG_ERROR("Could not allocate picture");
    return false;
  }

  // The following settings will prevent warning messages from FFmpeg
  //For svcd you might set it to:
  //mux_preload= (36000+3*1200) / 90000.0; //0.44
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(54, 0, 0)
  float muxPreload = 0.5f;
  output->outputCtx->preload = (int) (muxPreload * AV_TIME_BASE);
#endif
  float muxMaxDelay = 0.7f;
  output->outputCtx->max_delay = (int) (muxMaxDelay * AV_TIME_BASE);

  return true;
}

bool MovieMaker::initOutputStream(OutputCtx *output)
{
#if 1
  AVOutputFormat *outputFormat = 0;
  if (!g_strOutputVideoFormat.compare("mpg")) {
    outputFormat = av_guess_format("dvd", NULL, NULL);
    if (outputFormat)
        outputFormat->video_codec = CODEC_ID_MPEG2VIDEO;
  } else if (!g_strOutputVideoFormat.compare("avi")) {
      outputFormat = av_guess_format("avi", NULL, NULL);
      if (outputFormat)
          outputFormat->video_codec = CODEC_ID_MSMPEG4V2;
  }
#else
  // this does not seem to work on precise
  AVOutputFormat *outputFormat = av_guess_format(NULL, output->outputCtx->filename, NULL);
  if (!outputFormat)
    outputFormat = av_guess_format(g_strOutputVideoFormat.c_str(), NULL, NULL);
#endif
  if (!outputFormat)
    return false;

  output->outputCtx = avformat_alloc_context();
  if (!output->outputCtx)
    return false;

  output->outputCtx->oformat = outputFormat;
  snprintf(output->outputCtx->filename, sizeof(output->outputCtx->filename),
      "%s", output->fileName);

  // add video and audio streams
  if (!addVideoStream(output))
    return false;

  output->outputCtx->packet_size = 2048;
  av_dump_format(output->outputCtx, 0, g_strOutputVideoFile.c_str(), 1);

  // open the audio and video codecs and allocate the necessary encode buffers
  if (!openVideoEncoder(output))
    return false;

  // open the output file
  if (avio_open(&output->outputCtx->pb, g_strOutputVideoFile.c_str(), AVIO_FLAG_WRITE) < 0) {
    ostringstream msg;
    msg << "Could not open " << g_strOutputVideoFile.c_str() << " for writing"
        << endl;
    METLIBS_LOG_ERROR(msg.str());
    return false;
  }

  // write the stream header
  output->outputCtx->packet_size = 2048;
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(54, 0, 0)
  output->outputCtx->mux_rate = 10080000;
#endif
  avformat_write_header(output->outputCtx, NULL);

  return true;
}

void MovieMaker::closeVideoEncoder(OutputCtx *output)
{
  if (!output->videoStream)
    return;

  avcodec_close(output->videoStream->codec);

  if (output->frame) {
    if (output->frame->data[0])
      av_free(output->frame->data[0]);
    av_free(output->frame);
    output->frame = NULL;
  }

  if (output->videoBuffer)
    av_free(output->videoBuffer);
  output->videoBuffer = NULL;
  output->videoStream = NULL;
}

void MovieMaker::endOutputStream(OutputCtx *output)
{
  closeVideoEncoder(output);

  if (!output->outputCtx)
    return;

  // write the trailer
  av_write_trailer(output->outputCtx);

  // free the streams
  unsigned int t;
  for (t = 0; t < output->outputCtx->nb_streams; t++) {
    av_freep(&output->outputCtx->streams[t]->codec);
    av_freep(&output->outputCtx->streams[t]);
  }

  // close the output file
#if LIBAVFORMAT_VERSION_INT >= (52<<16)
  avio_close (output->outputCtx->pb);
#else
  url_fclose(&output->outputCtx->pb);
#endif

  // free the stream
  av_free(output->outputCtx);
  output->outputCtx = NULL;
}

bool MovieMaker::writeVideoFrame(OutputCtx *output)
{
  AVCodecContext *video = output->videoStream->codec;

  // encode the image
  int out_size = avcodec_encode_video(video, (uint8_t *) output->videoBuffer,
      VIDEO_BUF_SIZE, output->frame);
  if (out_size < 0)
    return false;
  // if zero size, it means the image was buffered
  if (out_size > 0) {
    AVPacket pkt;
    av_init_packet(&pkt);

    pkt.pts = av_rescale_q(video->coded_frame->pts, video->time_base,
        output->videoStream->time_base);
    if (video->coded_frame->key_frame)
      pkt.flags |= AV_PKT_FLAG_KEY;

    pkt.stream_index = output->videoStream->index;
    pkt.data = (uint8_t *) output->videoBuffer;
    pkt.size = out_size;

    // write the compressed frame in the media file
    int ret = av_write_frame(output->outputCtx, &pkt);
    if (ret != 0) {
      METLIBS_LOG_ERROR("Error while writing video frame");
      return false;
    }
  }
  return true;
}

bool MovieMaker::addImage(const QImage &image)
{
  const QImage::Format FORMAT = QImage::Format_RGB32;

  QImage imageScaled;
  if (image.size() == mFrameSize)
    imageScaled = image;
  else
    imageScaled = image.scaled(mFrameSize);

  if (imageScaled.format() != FORMAT)
    imageScaled = imageScaled.convertToFormat(FORMAT);

  return makeVideoFrame(&imageScaled);
}

bool MovieMaker::makeVideoFrame(const QImage *image)
{
  OutputCtx *output = &outputVideo;

  int frames = (int) (delay * 29.97);

  AVCodecContext *video = output->videoStream->codec;

  // Allocate buffer for FFMPeg ...
  int width, height;
  int size = image->width() * image->height();
  uint8_t *buffer = new uint8_t[((size * 3) / 2) + 100]; // 100 bytes extra buffer
  width = image->width();
  height = image->height();

  output->frame->data[0] = buffer;
  output->frame->data[1] = output->frame->data[0] + size;
  output->frame->data[2] = output->frame->data[1] + size / 4;
  output->frame->linesize[0] = width;
  output->frame->linesize[1] = width / 2;
  output->frame->linesize[2] = output->frame->linesize[1];

  // Copy data over from the QImage. Convert from 32bitRGB to YUV420P
  avhelpers::RGBtoYUV420P(image->bits(), buffer, image->depth() / 8, true, width, height, false);

  double duration = ((double) output->videoStream->pts.val)
      * output->videoStream->time_base.num / output->videoStream->time_base.den
      + ((double) frames) * video->time_base.num / video->time_base.den;

  while (true) {
    double deltaVideo = (double) output->videoStream->time_base.num
        / output->videoStream->time_base.den;
    double videoPts = ((double) output->videoStream->pts.val) * deltaVideo;

    if (!output->videoStream || videoPts >= duration)
      break;

    // write video frames
    if (!writeVideoFrame(output))
      return false;
  }

  return true;
}
