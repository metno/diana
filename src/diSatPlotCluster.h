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

#ifndef diSatPlotCluster_h
#define diSatPlotCluster_h

#include "diPlotCluster.h"

#include "diSatTypes.h"

#include <vector>

class Area;
class Projection;
class SatPlot;
class SatManager;

class SatPlotCluster : public PlotCluster
{

public:
  SatPlotCluster(SatManager* satm);

  plottimes_t getTimes() override;

  bool MapToGrid(const Projection& plotproj, float xmap, float ymap, float& gridx, float& gridy) const;

  bool getSatArea(Area& a) const;

  /// get name++ of current channels (with calibration)
  std::vector<std::string> getCalibChannels();

  /// show pixel values in status bar
  std::vector<SatValues> showValues(float x, float y);

private:
  void processInputPE(const PlotCommand_cpv&) override;

private:
  SatManager* satm_;
};

#endif
