/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2011-2019 met.no

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
#ifndef diStationManager_h
#define diStationManager_h

#include "diPlotCommand.h"
#include "diStationTypes.h"

#include <QString>

class StationPlot;
struct Station;

class StationManager
{
private:
  typedef std::vector<StationPlot*> stationPlots_t;

public:
  StationManager();
  ~StationManager();

  stationDialogInfo& initDialog();
  bool parseSetup();

  StationPlot* importStations(const std::string& name, const std::string& url);

private:
  Station* parseSMHI(const std::string& miLine, const std::string& url);

private:
  stationDialogInfo m_info;
};

#endif
