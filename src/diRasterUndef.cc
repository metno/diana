/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2020 met.no

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

#include "diRasterUndef.h"

#include "diana_config.h"

#include "diColour.h"
#include "diGLPainter.h"
#include "diPlotOptions.h"

#define MILOGGER_CATEGORY "diana.RasterUndef"
#include <miLogger/miLogging.h>

RasterUndef::RasterUndef(const PlotArea& pa, const Field_pv& f, const PlotOptions& po)
    : RasterGrid(pa, f.front())
    , fields(f)
{
  const Colour& u = po.undefColour;
  pixelUndef = qRgba(u.R(), u.G(), u.B(), u.A());
}

void RasterUndef::colorizePixel(QRgb& pixel, const diutil::PointI& i)
{
  const int nx = fields[0]->area.nx;
  for (size_t k = 0; k < fields.size(); k++) {
    const float v = fields[k]->data[i.x() + i.y() * nx];
    if (v == fieldUndef) {
      pixel = pixelUndef;
      return;
    }
  }
}
