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

#include "diRasterGrid.h"

#include "diana_config.h"

#include "diColour.h"
#include "diColourShading.h"
#include "diField/diField.h"
#include "diGLPainter.h"
#include "diGlUtilities.h"
#include "diPaintGLPainter.h"
#include "diPlotOptions.h"
#include "util/misc_util.h"
#include "util/plotoptions_util.h"
#include "util/string_util.h"

#include <mi_fieldcalc/math_util.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#define MILOGGER_CATEGORY "diana.RasterGrid"
#include <miLogger/miLogging.h>

RasterGrid::RasterGrid(const PlotArea& pa, const Field* f)
    : pa_(pa)
    , field(f)
{
}

inline diutil::PointI rnd(const diutil::PointD& p)
{
  const double OX = -0.5, OY = 0.5;
  return diutil::PointI(int(p.x() + OX), int(p.y() + OY));
}

void RasterGrid::rasterPixels(int n, const diutil::PointD& xy0, const diutil::PointD& dxy, QRgb* pixels)
{
  const int nx = field->area.nx, ny = field->area.ny;
  const diutil::PointD fxy0(field->area.R().x1, field->area.R().y1);
  const diutil::PointD res(field->area.resolutionX, field->area.resolutionY);
  const diutil::PointD step = dxy / res;
  diutil::PointD ixy = (xy0 - fxy0) / res;
  diutil::PointI lxy;
  int li = -1;
  for (int i = 0; i < n; ++i, ixy += step) {
    const diutil::PointI rxy = rnd(ixy);
    if (rxy.x() < 0 || rxy.x() >= nx || rxy.y() < 0 || rxy.y() >= ny)
      continue;
    if (li >= 0 && rxy == lxy) {
      pixels[i] = pixels[li];
    } else {
      colorizePixel(pixels[i], rxy);
      li = i;
      lxy = rxy;
    }
  }
}
