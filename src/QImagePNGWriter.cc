/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018 met.no

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

#include "QImagePNGWriter.h"

#include <QIODevice>
#include <QImage>

#include <png.h>
#include <pngconf.h>

#define MILOGGER_CATEGORY "diana.QImagePNGWriter"
#include <miLogger/miLogging.h>

extern "C" {
static void QImagePNGWriter_error(png_structp /*png_ptr*/, png_const_charp message)
{
  METLIBS_LOG_ERROR("libpng: " << message);
}

static void QImagePNGWriter_warning(png_structp /*png_ptr*/, png_const_charp message)
{
  METLIBS_LOG_WARN("libpng: " << message);
}

static void QImagePNGWriter_write(png_structp png_ptr, png_bytep data, png_size_t length)
{
  QImagePNGWriter* w = (QImagePNGWriter*)png_get_io_ptr(png_ptr);
  QIODevice* device = w->device();

  uint n = device->write((char*)data, length);
  if (n != length) {
    png_error(png_ptr, "Write Error");
  }
}

static void QImagePNGWriter_flush(png_structp /*png_ptr*/)
{
}
} // extern "C"

QImagePNGWriter::QImagePNGWriter(QIODevice* device)
    : io_(device)
    , compressionlevel_(-1)
{
}

QImagePNGWriter::~QImagePNGWriter()
{
}

void QImagePNGWriter::setCompressionLevel(int cl)
{
  if (cl >= -1 && cl <= 9)
    compressionlevel_ = cl;
}

bool QImagePNGWriter::write(const QImage& img)
{
  METLIBS_LOG_SCOPE();

  const QImage image = img.convertToFormat(QImage::Format_ARGB32);

  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, QImagePNGWriter_error, QImagePNGWriter_warning);
  if (!png_ptr) {
    METLIBS_LOG_ERROR("could not create write struct");
    return false;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_write_struct(&png_ptr, 0);
    METLIBS_LOG_ERROR("could not create info struct");
    return false;
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    METLIBS_LOG_ERROR("setjmp failed");
    return false;
  }

  if (compressionlevel_ >= 0)
    png_set_compression_level(png_ptr, compressionlevel_);

#if 0
  png_set_compression_strategy(png_ptr, Z_RLE); // huge output, factor 10 larger than default
#endif

  // see https://stackoverflow.com/questions/7948901/how-can-i-decrease-the-amount-of-time-it-takes-to-save-a-png-using-qimage
  png_set_filter(png_ptr, 0, PNG_FILTER_NONE);

  png_set_write_fn(png_ptr, (void*)this, QImagePNGWriter_write, QImagePNGWriter_flush);

  const int height = image.height();
  const int width = image.width();
  int color_type = PNG_COLOR_TYPE_RGB_ALPHA;
  int bit_depth = 8;
  png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  png_set_bgr(png_ptr);

  png_write_info(png_ptr, info_ptr);

  //  png_set_packing(png_ptr);
  //  png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);

  png_bytep* row_pointers(new png_bytep[height]);
  for (int y = 0; y < height; y++)
    row_pointers[y] = const_cast<png_bytep>(image.constScanLine(y));
  png_write_image(png_ptr, row_pointers);
  delete[] row_pointers;

  png_write_end(png_ptr, info_ptr);

  png_destroy_write_struct(&png_ptr, &info_ptr);

  return true;
}
