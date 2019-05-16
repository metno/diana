/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2018 met.no

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

#include "diana_config.h"

#include "diRasterAlpha.h"

#include "diColour.h"
#include "diColourShading.h"
#include "diField/diField.h"
#include "diPlotOptions.h"
#include "diRasterUtil.h"
#include "util/misc_util.h"
#include "util/plotoptions_util.h"

#include <mi_fieldcalc/math_util.h>

#define MILOGGER_CATEGORY "diana.FieldRenderer"
#include <miLogger/miLogging.h>

RasterAlpha::RasterAlpha(const PlotArea& pa, Field* f, const PlotOptions& po)
    : RasterGrid(pa, f)
    , poptions(po)
{
  const int nx = field->area.nx, ny = field->area.ny;
  float cmax = 0;
  if (poptions.minvalue != -fieldUndef && poptions.maxvalue != fieldUndef) {
    cmin = poptions.minvalue;
    cmax = poptions.maxvalue;
  } else {
    diutil::mini_maxi(field->data, nx * ny, fieldUndef, cmin, cmax);
  }
  cdiv = 255 / (cmax - cmin);
}

void RasterAlpha::colorizePixel(QRgb& pixel, const diutil::PointI& i)
{
  const int nx = field->area.nx;
  const unsigned char red = poptions.linecolour.R(), green = poptions.linecolour.G(), blue = poptions.linecolour.B();
  const float value = field->data[i.x() + i.y() * nx];
  if (value != fieldUndef) {
    unsigned char alpha = (unsigned char)((value - cmin) * cdiv);
    pixel = qRgba(red, green, blue, alpha);
  }
}
