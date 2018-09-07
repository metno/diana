/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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
#ifndef diVprofData_h
#define diVprofData_h

#include "diField/VcrossData.h"
#include "diStationTypes.h"
#include "diVprofValues.h"

#include <puTools/miTime.h>

#include <memory>
#include <set>
#include <vector>

struct VprofValuesRequest
{
  std::string name;
  miutil::miTime time;
  int realization;
  std::set<std::string> variables;
  vcross::Z_AXIS_TYPE vertical_axis;

  VprofValuesRequest();

  bool operator==(const VprofValuesRequest& other) const;
  bool operator!=(const VprofValuesRequest& other) const { return !(*this == other); }
};

/**
  \brief Vertical Profile (sounding) prognostic data

   Contains all data and misc information from one file
*/
class VprofData
{
public:
  VprofData(const std::string& modelname, const std::string& stationsfilename = "");
  ~VprofData();

  virtual VprofValues_cpv getValues(const VprofValuesRequest& request) = 0;
  virtual bool updateStationList(const miutil::miTime& plotTime) = 0;

  const std::vector<stationInfo>& getStations() const { return mStations; }
  const std::vector<miutil::miTime>& getTimes() const { return validTime; }
  int getRealizationCount() const { return numRealizations; }
  const std::string& getModelName() const { return modelName; }

  void addValidTime(const miutil::miTime& vt) { validTime.push_back(vt); }

protected:
  const std::string& getStationsFileName() const { return stationsFileName; }

  void readStationNames();
  void renameStations();
  void readStationList();

  int numRealizations;

  std::vector<stationInfo> mStations;
  std::map<std::string, std::string> stationMap;

  VprofValues_cpv cache;
  VprofValuesRequest cachedRequest;

private:
  std::string modelName;
  std::string stationsFileName;

  std::vector<miutil::miTime> validTime;

  bool stationList;
  std::vector<float> stationLatitude;
  std::vector<float> stationLongitude;
  std::vector<std::string> stationName;
};

typedef std::shared_ptr<VprofData> VprofData_p;
typedef std::shared_ptr<const VprofData> VprofData_cp;

#endif
