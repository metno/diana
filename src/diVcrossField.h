/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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

#include <puTools/miTime.h>

#include <map>
#include <vector>

class VcrossPlot;
class LocationElement;
class FieldManager;

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
  bool setLatLon(float lat, float lon);
  const std::vector<std::string>& getNames() const
    { return names; }

  const std::vector<miutil::miTime>& getTimes() const
    { return validTime; }

  std::vector<std::string> getFieldNames();
  void getMapData(std::vector<LocationElement>& elements);

  VcrossPlot* getCrossection(const std::string& name,
			     const miutil::miTime& time, int tgpos);

  void cleanupCache();
  void cleanupTGCache();

private:
  std::string modelName;
  FieldManager* fieldManager;

  std::vector<std::string> names;
  std::vector<std::string> posOptions;
  std::vector<miutil::miTime> validTime;
  std::vector<int> forecastHour;
  std::vector<std::string> params;

  // Holds active crossections
  std::vector<LocationElement> crossSections;
  miutil::miTime lastVcrossTime;
  std::map<int,std::vector<float*> > VcrossDataMap;
  std::map<int,std::vector<bool> > VcrossMultiLevelMap;
  std::vector<VcrossPlot*> VcrossPlotVector;

  // Holds the last timeGraph
  int lastVcross;
  int lastTgpos;
  std::vector<float*> lastVcrossData;
  std::vector<bool> lastVcrossMultiLevel;
  VcrossPlot* lastVcrossPlot;

  // Holds positions clicked on the map
  float startLatitude;
  float startLongitude;
  float stopLatitude;
  float stopLongitude;
};

#endif
