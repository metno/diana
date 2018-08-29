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

#ifndef diRasterAlarmBox_h
#define diRasterAlarmBox_h

#include "diRasterPlot.h"

#include <diField/diField.h>

class PlotOptions;

class RasterAlarmBox : public RasterPlot
{
public:
  RasterAlarmBox(const PlotArea& pa, Field* f, const PlotOptions& po);
  void rasterPixels(int, const diutil::PointD&, const diutil::PointD&, QRgb*) override { /* never called */}
  void pixelQuad(const diutil::PointI& s, const diutil::PointD& pxy00, const diutil::PointD& pxy10, const diutil::PointD& pxy01, const diutil::PointD& pxy11,
                 int w) override;
  const PlotArea& rasterPlotArea() override { return pa_; }
  const GridArea& rasterArea() override { return field->area; }

private:
  const PlotArea& pa_;
  const Field* field;
  const PlotOptions& poptions;
  float vmin, vmax;
  bool valid;
  QRgb c_alarm;
};

#endif // diRasterAlarmBox_h
