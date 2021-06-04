/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2019-2021 met.no

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

#ifndef TRAJECTORYPLOTCLUSTER_H
#define TRAJECTORYPLOTCLUSTER_H

#include "diPlotCluster.h"

class TrajectoryPlot;

class TrajectoryPlotCluster : public PlotCluster
{
public:
  TrajectoryPlotCluster();

  void processInput(const PlotCommand_cpv& cmds) override;

  TrajectoryPlot* getPlot(size_t idx);

  /// handle command
  /// \returns true if plots changed
  bool trajPos(const std::vector<std::string>& vstr);

  bool hasTrajectories() { return !plots_.empty(); }
};

#endif // TRAJECTORYPLOTCLUSTER_H
