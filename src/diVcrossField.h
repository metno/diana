/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diVcrossField.h 1 2007-09-12 08:06:42Z lisbethb $

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
#ifndef diVcrossField_h
#define diVcrossField_h

#include <puTools/miString.h>
#include <puTools/miTime.h>
#include <vector>
#include <diField/diFieldManager.h>
#include <diLocationPlot.h>

using namespace std;

class VcrossPlot;


/**
  \brief Vertical Crossection prognostic data from a field source
*/
class VcrossField
{

public:
  VcrossField(const std::string& modelname, FieldManager* fieldm);
  ~VcrossField();
  void cleanup();
  bool update();
  bool getInventory();
  bool setLatLon(float lat,float lon);
  vector<std::string> getNames() { return names; }
  vector<miutil::miTime> getTimes() { return validTime; }
  vector<std::string> getFieldNames();
  void getMapData(vector<LocationElement>& elements);

  VcrossPlot* getCrossection(const std::string& name,
			     const miutil::miTime& time, int tgpos);

  void cleanupCache();
  void cleanupTGCache();

private:

  std::string modelName;
  FieldManager* fieldManager;

  vector<std::string> names;
  vector<std::string> posOptions;
  vector<miutil::miTime>   validTime;
  vector<int>      forecastHour;
  vector<std::string> params;

  // Holds active crossections
  vector<LocationElement> crossSections;
  miutil::miTime lastVcrossTime;
  map<int,vector<float*> > VcrossDataMap;
  map<int,vector<bool> > VcrossMultiLevelMap;
  vector<VcrossPlot*> VcrossPlotVector;

  // Holds the last timeGraph
  int lastVcross;
  int lastTgpos;
  vector<float*> lastVcrossData;
  vector<bool> lastVcrossMultiLevel;
  VcrossPlot* lastVcrossPlot;

  // Holds positions clicked on the map
  float startLatitude;
  float startLongitude;
  float stopLatitude;
  float stopLongitude;
};

#endif
