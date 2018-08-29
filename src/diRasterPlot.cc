/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2018 met.no

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

#include "diRasterPlot.h"

#include "diStaticPlot.h"

#define MILOGGER_CATEGORY "diana.RasterPlot"
#include <miLogger/miLogging.h>

RasterPlot::RasterPlot()
{
}

RasterPlot::~RasterPlot()
{
}

QImage RasterPlot::rasterPaint()
{
  METLIBS_LOG_SCOPE();

  const PlotArea& pa = rasterPlotArea();
  const diutil::PointI& size = pa.getPhysSize();
  cached_ = QImage(size.x(), size.y(), QImage::Format_ARGB32);
  cached_.fill(Qt::transparent);

  GridReprojection::instance()->reproject(size, pa.getPlotSize(), pa.getMapArea().P(), rasterArea().P(), *this);
  return cached_;
}

void RasterPlot::pixelLine(const diutil::PointI& s, const diutil::PointD& xy0, const diutil::PointD& dxy, int w)
{
  rasterPixels(w, xy0, dxy, pixels(s));
}

QRgb* RasterPlot::pixels(const diutil::PointI& s)
{
  const int x0 = s.x(), y0 = cached_.size().height() - 1 - s.y();
  return reinterpret_cast<QRgb*>(cached_.scanLine(y0)) + x0;
}
