/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2021 met.no

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

#ifndef RASTERPLOT_H
#define RASTERPLOT_H 1

#include "diana_config.h"

#include "diField/diGridReprojection.h"
#include "diField/diArea.h"

#include <QImage>

class DiPainter;
class PlotArea;

class RasterPlot : protected GridReprojectionCB {
protected:
  RasterPlot();
  virtual ~RasterPlot();

  virtual const PlotArea& rasterPlotArea() = 0;
  virtual const GridArea& rasterArea() = 0;
  virtual void rasterPixels(int n, const diutil::PointD& xy0, const diutil::PointD& dxy, QRgb* pixels) = 0;

public:
  QImage rasterPaint();

protected:
  // implementations for PixelReprojectionCB
  void pixelLine(const diutil::PointI& s0, const diutil::PointD& xyf0, const diutil::PointD& dxyf, int w) override;
  QRgb* pixels(const diutil::PointI& s);

//private:
  QImage cached_;
};

#endif // RASTERPLOT_H
