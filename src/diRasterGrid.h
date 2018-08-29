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

#ifndef diRasterGrid_h
#define diRasterGrid_h

#include "diRasterPlot.h"

#include "diField/diField.h"
#include "diPlotArea.h"

class RasterGrid : public RasterPlot
{
public:
  RasterGrid(const PlotArea& pa, const Field* f);
  void rasterPixels(int n, const diutil::PointD& xy0, const diutil::PointD& dxy, QRgb* pixels) override;
  const PlotArea& rasterPlotArea() override { return pa_; }
  const GridArea& rasterArea() override { return field->area; }
  virtual void colorizePixel(QRgb& pixel, const diutil::PointI& i) = 0;

protected:
  PlotArea pa_;
  const Field* field;
};

#endif // diRasterGrid_h
