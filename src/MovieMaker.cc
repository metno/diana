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

#include <sys/types.h>
#include <sys/stat.h>

#include "MovieMaker.h"

#define VIDEO_BUF_SIZE 1835008
#define VIDEO_BITRATE  6000 * 1024

using namespace std;

MovieMaker::MovieMaker(string &filename, string &format, float delay)
{
#ifdef HAVE_LOG4CXX
  logger = log4cxx::Logger::getLogger("diana.MovieMaker"); ///< LOG4CXX init
#endif
  g_strOutputVideoFile = filename;
  g_strOutputVideoFormat = format;
  this->delay = delay;

  // register all the codecs
  avcodec_register_all();
  av_register_all();

  // must be called before using avcodec lib
  avcodec_init();

  outputVideo.fileName = g_strOutputVideoFile.c_str();

  if (!initOutputStream(&outputVideo))LOG4CXX_ERROR(logger, "Cannot init output video stream");
}

MovieMaker::~MovieMaker()
{
  endOutputStream(&outputVideo);
}

bool MovieMaker::addVideoStream(OutputCtx *output)
{
  output->videoStream = av_new_stream(output->outputCtx, 0);
  if (!output->videoStream)
    return false;

  AVCodecContext *video = output->videoStream->codec;
  video->codec_id = (CodecID) output->outputCtx->oformat->video_codec;
  video->codec_type = CODEC_TYPE_VIDEO;
  video->bit_rate = VIDEO_BITRATE;
  video->sample_aspect_ratio.den = 4;
  video->sample_aspect_ratio.num = 3;
  //  video->dtg_active_format = FF_DTG_AFD_4_3; only used for decoding
  video->width = 720;
  video->height = 480;
  video->time_base.den = 30000;
  video->time_base.num = 1001;
  video->gop_size = 18;

  video->pix_fmt = PIX_FMT_YUV420P;
  video->rc_buffer_size = VIDEO_BUF_SIZE;
  video->rc_max_rate = 9 * 1024 * 1024;
  video->rc_min_rate = 0;

  return true;
}

AVFrame *MovieMaker::allocPicture(int pixFormat, int width, int height)
{
  AVFrame *frame = avcodec_alloc_frame();
  if (!frame)
    return NULL;

  int size = avpicture_get_size(pixFormat, width, height);
  uint8_t *buffer = (uint8_t*) av_malloc(size);
  if (!buffer) {
    av_free(frame);
    return NULL;
  }

  avpicture_fill((AVPicture *) frame, buffer, pixFormat, width, height);
  return frame;
}

bool MovieMaker::openVideoEncoder(OutputCtx *output)
{
  AVCodecContext *video = output->videoStream->codec;

  // find the video encoder and open it
  AVCodec *codec = avcodec_find_encoder(video->codec_id);
  if (!codec) {
    LOG4CXX_ERROR(logger, "Video codec not found");
    return false;
  }

  if (avcodec_open(video, codec) < 0) {
    LOG4CXX_ERROR(logger, "Could not open video codec");
    return false;
  }

  output->videoBuffer = (short *) av_malloc(VIDEO_BUF_SIZE);

  // allocate the encoded raw picture
  output->frame = allocPicture(video->pix_fmt, video->width, video->height);
  if (!output->frame) {
    LOG4CXX_ERROR(logger, "Could not allocate picture");
    return false;
  }

  // The following settings will prevent warning messages from FFmpeg
  float muxPreload = 0.5f;
  float muxMaxDelay = 0.7f;
  //For svcd you might set it to:
  //mux_preload= (36000+3*1200) / 90000.0; //0.44
  output->outputCtx->preload = (int) (muxPreload * AV_TIME_BASE);
  output->outputCtx->max_delay = (int) (muxMaxDelay * AV_TIME_BASE);

  return true;
}

bool MovieMaker::initOutputStream(OutputCtx *output)
{
  AVOutputFormat *outputFormat = 0;
  if (!g_strOutputVideoFormat.compare("mpg")) {
    outputFormat = guess_format("dvd", NULL, NULL);
    if (outputFormat)
        outputFormat->video_codec = CODEC_ID_MPEG2VIDEO;
  } else if (!g_strOutputVideoFormat.compare("avi")) {
      outputFormat = guess_format("avi", NULL, NULL);
      if (outputFormat)
          outputFormat->video_codec = CODEC_ID_MSMPEG4V2;
  }
  if (!outputFormat)
    return false;

  output->outputCtx = av_alloc_format_context();
  if (!output->outputCtx)
    return false;

  output->outputCtx->oformat = outputFormat;
  snprintf(output->outputCtx->filename, sizeof(output->outputCtx->filename),
      "%s", output->fileName);

  // add video and audio streams
  if (!addVideoStream(output))
    return false;

  if (av_set_parameters(output->outputCtx, NULL ) < 0)
    return false;

  output->outputCtx->packet_size = 2048;
  dump_format(output->outputCtx, 0, g_strOutputVideoFile.c_str(), 1);

  // open the audio and video codecs and allocate the necessary encode buffers
  if (!openVideoEncoder(output))
    return false;

  // open the output file
  if (url_fopen(&output->outputCtx->pb, g_strOutputVideoFile.c_str(),
      URL_WRONLY) < 0) {
    ostringstream msg;
    msg << "Could not open " << g_strOutputVideoFile.c_str() << " for writing"
        << endl;
    LOG4CXX_ERROR(logger, msg.str());
    return false;
  }

  // write the stream header
  output->outputCtx->packet_size = 2048;
  output->outputCtx->mux_rate = 10080000;
  av_write_header(output->outputCtx);

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
  url_fclose (output->outputCtx->pb);
#else
  url_fclose(&output->outputCtx->pb);
#endif

  // free the stream
  av_free(output->outputCtx);
  output->outputCtx = NULL;
}

#define rgbtoyuv(r, g, b, y, u, v) \
  y=(uint8_t)( (((int)(66*r)   +(int)(129*g) +(int)(25*b) + 128) >> 8 ) + 16 ); \
  u=(uint8_t)( (((int)(-38*r)  -(int)(74*g) +(int)(112*b) + 128) >> 8 ) + 128 ); \
  v=(uint8_t)( (((int)(112*r)   -(int)(94*g) -(int)(18*b) + 128) >> 8 ) + 128 );

void MovieMaker::RGBtoYUV420P(const uint8_t *RGB, uint8_t *YUV,
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
      pkt.flags |= PKT_FLAG_KEY;

    pkt.stream_index = output->videoStream->index;
    pkt.data = (uint8_t *) output->videoBuffer;
    pkt.size = out_size;

    // write the compressed frame in the media file
    int ret = av_write_frame(output->outputCtx, &pkt);
    if (ret != 0) {
      LOG4CXX_ERROR(logger, "Error while writing video frame");
      return false;
    }
  }
  return true;
}

/*bool MovieMaker::addImage(QImage *image)
{
  if (!image)
      return false;

  // scale image to fit video format size
  QImage imageScaled = image->scaled(720, 480, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

  addImage(&outputVideo, imageScaled);

  return true;

}*/

bool MovieMaker::addImage(QImage *image)
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
  RGBtoYUV420P(image->bits(), buffer, image->depth() / 8, true, width, height,
      false);

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
