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

#include "diRasterRGB.h"

#include "diColour.h"
#include "diColourShading.h"
#include "diGLPainter.h"
#include "diPlotOptions.h"
#include "diRasterUtil.h"
#include "util/math_util.h"
#include "util/misc_util.h"
#include "util/plotoptions_util.h"
#include <diField/VcrossUtil.h>

#define MILOGGER_CATEGORY "diana.RasterRGB"
#include <miLogger/miLogging.h>

RasterRGB::RasterRGB(const PlotArea& pa, const std::vector<Field*>& f, const PlotOptions& po)
    : RasterGrid(pa, f.front())
    , fields(f)
    , poptions(po)
{
  const int nx = fields[0]->area.nx, ny = fields[0]->area.ny;
  for (int k = 0; k < 3; k++) {
    float v_min = 0, v_max = 1;
    if (poptions.minvalue != -fieldUndef && poptions.maxvalue != fieldUndef) {
      v_min = poptions.minvalue;
      v_max = poptions.maxvalue;
    } else {
      diutil::mini_maxi(fields[k]->data, nx * ny, fieldUndef, v_min, v_max);
    }

    float c_min = v_min, c_max = v_max;
    diutil::mincut_maxcut(fields[k]->data, nx * ny, fieldUndef, poptions.colourcut, c_min, c_max);

    float v_div = 255 / (v_max - v_min);
    float c_div = 255 / (c_max - c_min);
    value_min[k] = v_min + c_min / v_div;
    value_div[k] = c_div * v_div;
  }
}

void RasterRGB::colorizePixel(QRgb& pixel, const diutil::PointI& i)
{
  const int nx = fields[0]->area.nx;
  unsigned char ch[3];
  for (int k = 0; k < 3; k++) {
    const float v0 = fields[k]->data[i.x() + i.y() * nx];
    if (v0 != fieldUndef) {
      const long val = std::lround((v0 - value_min[k]) * value_div[k]);
      const int col = vcross::util::constrain_value(val, 0L, 255L);
      ch[k] = (unsigned char)col;
    } else {
      ch[k] = 0;
    }
  }
  pixel = qRgb(ch[0], ch[1], ch[2]);
}
