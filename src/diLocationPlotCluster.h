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

#ifndef LOCATIONPLOTCLUSTER_H
#define LOCATIONPLOTCLUSTER_H

#include "diPlotCluster.h"

class LocationData;

class LocationPlotCluster : public PlotCluster
{
public:
  LocationPlotCluster();

  void processInput(const PlotCommand_cpv& cmds) override;

  void putLocation(const LocationData& locationdata);
  bool deleteLocation(const std::string& name);
  void setSelectedLocation(const std::string& name, const std::string& elementname);
  std::string findLocation(int x, int y, const std::string& name);

private:
  LocationPlotCluster::Plot_xv::iterator find_lp(const std::string& name);
};

#endif // LOCATIONPLOTCLUSTER_H
