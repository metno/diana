/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015 MET Norway

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

#include "WebMapPainting.h"

#define MILOGGER_CATEGORY "diana.WebMapPainting"
#include <miLogger/miLogging.h>

namespace diutil {

static Qt::ImageConversionFlags icf = Qt::DiffuseAlphaDither | Qt::NoOpaqueDetection;

void ensureImageFormat(QImage& image, QImage::Format format)
{
  if (image.format() != format)
    image = image.convertToFormat(format, icf);
}

QImage convertImage(QImage image, float alpha_offset, float alpha_scale, bool make_grey)
{
  const bool change_alpha = (alpha_offset != 0 || alpha_scale != 1);
  if (change_alpha || make_grey) {
    ensureImageFormat(image, QImage::Format_ARGB32);
    for (int i=0; i<image.height(); ++i) {
      QRgb* rgb = reinterpret_cast<QRgb*>(image.scanLine(i));
      for (int j=0; j<image.width(); ++j) {
        QRgb& p = rgb[j];
        int r = qRed(p), g = qGreen(p), b = qBlue(p), a = qAlpha(p);
        if (make_grey)
          r = g = b = (r+g+b)/3;
        if (change_alpha)
          a = alpha_offset + alpha_scale*a;
        p = qRgba(r, g, b, a);
      }
    }
  }

  ensureImageFormat(image, QImage::Format_ARGB32_Premultiplied);
  return image;
}

} // namespace diutil
