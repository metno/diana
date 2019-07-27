/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2019 met.no

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

#ifndef MAPPLOTCLUSTER_H
#define MAPPLOTCLUSTER_H

#include "diPlotCluster.h"

class MapPlotCluster : public PlotCluster
{
public:
  MapPlotCluster();
  ~MapPlotCluster();

  void addPlotElements(std::vector<PlotElement>& pel) override;
  bool enablePlotElement(const PlotElement& pe) override;

  const std::string& getBackColour() const;

protected:
  void processInputPE(const PlotCommand_cpv& cmds) override;

private:
  std::string bgcolourname_;
};

#endif // MAPPLOTCLUSTER_H
