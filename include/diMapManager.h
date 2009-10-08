/*
 Diana - A Free Meteorological Visualisation Tool

 $Id$

 Copyright (C) 2006 met.no

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
#ifndef _diMapManager_h
#define _diMapManager_h

/*
 \file

 \brief Manager of maps and predefined areas/projections
 */

#include <diField/diPlotOptions.h>
#include <diCommonTypes.h>
#include <diSetupParser.h>

using namespace std;

/**

 \brief Manager of maps and predefined areas/projections

 Manages datafiles and setup information for maps

 */

class MapManager {
private:
  static vector<Area> mapareas;
  static vector<Area> mapareas_Fkeys;
  static vector<MapInfo> mapfiles;

  // parse section containing definitions of map-areas
  bool parseMapAreas(SetupParser&);
  // parse section containing definitions of map-types
  bool parseMapTypes(SetupParser&);

public:
  MapManager()
  {
  }

  /// parse the maps section in the setup file
  bool parseSetup(SetupParser& sp);

  /// get list of predefined areas (names)
  vector<miString> getMapAreaNames();
  /// get predefined area from name
  bool getMapAreaByName(const miString&, Area&);
  /// get predefined area from accelerator
  bool getMapAreaByFkey(const miString&, Area&);
  /// get list of defined maps
  vector<MapInfo> getMapInfo();
  /// get information on one specific map
  bool getMapInfoByName(const miString&, MapInfo&);
  /// extract plot information from string
  bool fillMapInfo(const miString&, MapInfo&, PlotOptions& contopts,
      PlotOptions& landopts, PlotOptions& lonopts, PlotOptions& latopts,
      PlotOptions& ffopts);
  /// make string representation of one MapInfo
  miString MapInfo2str(const MapInfo&);
  /// get all defined maps and areas for the GUI
  MapDialogInfo getMapDialogInfo();

};

#endif
