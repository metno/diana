/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2011 met.no

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

#include <diCommonTypes.h>
#include <puTools/miString.h>

class StationPlot;
class Station;

class StationManager
{
public:
  StationManager();

  bool init(const vector<std::string>& inp);
  stationDialogInfo initDialog();
  bool parseSetup();

  StationPlot* importStations(miutil::miString& name, miutil::miString& url);

  // Stations
  /**
   * This method is only sound as long as all Stations in all StationPlots have the same scale.
   * @return Current scale for the first Station in the first StationPlot
   */
  float getStationsScale();

  /**
   * Set new scale for all Stations.
   * @param new_scale New scale (1.0 original size)
   */
  void setStationsScale(float new_scale);

  ///put StationPlot in list of StationPlots
  void putStations(StationPlot*);
  ///make StationPlot and put it in list of StationPlots
  void makeStationPlot(const std::string& commondesc, const std::string& common,
           const std::string& description, int from,
           const  vector<std::string>& data);
  ///Returns the first station close to position x,y found in the StationPlots
  ///held by this object
  Station* findStation(int x, int y);
  ///Returns all stations close to position x,y found in the StationPlots
  ///held by this object
  vector<Station*> findStations(int x, int y);
  ///find station in position x,y in StationPlot with name and id
  miutil::miString findStation(int x, int y,miutil::miString name,int id=-1);
  ///look for station in position x,y in all StationPlots
  void findStations(int x, int y, bool add,
        vector<miutil::miString>& name,vector<int>& id,
        vector<miutil::miString>& station);
  ///get editable stations, returns name/id of StationPlot and stations
  bool getEditStation(int step, miutil::miString& name, int& id,
          vector<miutil::miString>& stations);
  void getStationData(vector<std::string>& data);
  ///send command to StationPlot with name and id
  void stationCommand(const std::string& Command,
          const vector<std::string>& data,
          const std::string& name="", int id=-1,
          const std::string& misc="");
  ///send command to StationPlot with name and id
  void stationCommand(const std::string& Command,
          const std::string& name="", int id=-1);

  ///Returns a vector containing the plots held by the manager.
  vector <StationPlot*> plots();

private:
  stationDialogInfo m_info;
  //stations to be plotted
  map <miutil::miString,StationPlot*> stationPlots;

  Station* parseSMHI(miutil::miString& miLine, miutil::miString& url);

};

#endif
