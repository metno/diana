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

#ifndef diRasterRGB_h
#define diRasterRGB_h

#include "diRasterGrid.h"

class PlotOptions;

class RasterRGB : public RasterGrid
{
public:
  RasterRGB(const PlotArea& pa, const std::vector<Field*>& f, const PlotOptions& po);
  void colorizePixel(QRgb& pixel, const diutil::PointI& i) override;

private:
  const std::vector<Field*>& fields;
  const PlotOptions& poptions;
  float value_div[3], value_min[3];
};

#endif // diRasterRGB_h
