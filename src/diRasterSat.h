/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2019 met.no

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
#ifndef diRasterSat_h
#define diRasterSat_h

#include "diRasterPlot.h"

/**
 * \brief Make a QImage from satellite or radar data (or whatever RGBA data).
 */
class RasterSat : public RasterPlot
{
public:
  RasterSat(const PlotArea& pa, const GridArea& image_area, const unsigned char* image);
  ~RasterSat();

protected:
  const PlotArea& rasterPlotArea() override { return pa_; }
  const GridArea& rasterArea() override { return ga_; }

  void rasterPixels(int n, const diutil::PointD& xy0, const diutil::PointD& dxy, QRgb* pixels) override;

private:
  const PlotArea& pa_;
  const GridArea& ga_;
  const unsigned char* rgba_;

  const int nx, ny;

  const diutil::PointD fxy0;
  const diutil::PointD res_inv;
};

#endif
