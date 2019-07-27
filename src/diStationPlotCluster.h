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

#ifndef diStationPlotCluster_h
#define diStationPlotCluster_h

#include "diPlotCluster.h"

#include "diPlotCommand.h"
#include "diStationTypes.h"

#include <QString>

struct Station;
class StationManager;
class StationPlot;

class StationPlotCluster : public PlotCluster
{
public:
  StationPlotCluster(StationManager* stam);
  ~StationPlotCluster();

  bool hasData() override { return false; }

  /// put StationPlot in list of StationPlots
  void putStations(StationPlot*);
  /// make StationPlot and put it in list of StationPlots
  void makeStationPlot(const std::string& commondesc, const std::string& common, const std::string& description, int from,
                       const std::vector<std::string>& data);
  /// Returns the first station close to position x,y found in the StationPlots
  /// held by this object
  Station* findStation(int x, int y);
  /// Returns all stations close to position x,y found in the StationPlots
  /// held by this object
  std::vector<Station*> findStations(int x, int y);
  /// find station in position x,y in StationPlot with name and id
  std::string findStation(int x, int y, const std::string& name, int id = -1);
  /// find stations close to position x,y in StationPlot with name and id
  std::vector<std::string> findStations(int x, int y, const std::string& name, int id = -1);
  /// look for station in position x,y in all StationPlots
  void findStations(int x, int y, bool add, std::vector<std::string>& name, std::vector<int>& id, std::vector<std::string>& station);
  /// send command to StationPlot with name and id
  void stationCommand(const std::string& Command, const std::vector<std::string>& data, const std::string& name = "", int id = -1,
                      const std::string& misc = "");
  /// send command to StationPlot with name and id
  void stationCommand(const std::string& Command, const std::string& name = "", int id = -1);

  std::vector<StationPlot*> getStationPlots() const;

  QString getStationsText(int x, int y);

protected:
  void processInputPE(const PlotCommand_cpv& inp) override;

private:
  Station* parseSMHI(const std::string& miLine, const std::string& url);
  Plot_xv::iterator findStationPlot(const std::string& name, int id, Plot_xv::iterator begin);

private:
  StationManager* stam_;
};

#endif
