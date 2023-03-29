/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2022 met.no

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

#include "diana_config.h"

#include "diVprofData.h"

#ifdef ROADOBS
#include "diObsRoad.h"
#endif // ROADOBS

#include "diField/VcrossData.h"
#include "diField/VcrossUtil.h"
#include "diLocalSetupParser.h"
#include "diUtilities.h"
#include "util/charsets.h"
#include "util/string_util.h"
#include "vcross_v2/VcrossComputer.h"
#include "vcross_v2/VcrossEvaluate.h"

#include <mi_fieldcalc/math_util.h>

#include <puTools/TimeFilter.h>
#include <puTools/miStringFunctions.h>

#include <fstream>
#include <iomanip>
#include <sstream>

#define MILOGGER_CATEGORY "diana.VprofData"
#include <miLogger/miLogging.h>

using namespace miutil;

VprofValuesRequest::VprofValuesRequest()
    : vertical_axis(vcross::Z_TYPE_PRESSURE)
    , realization(-1)
{
}

bool VprofValuesRequest::operator==(const VprofValuesRequest& other) const
{
  return name == other.name
      && time == other.time
      && realization == other.realization
      && variables == other.variables
      && vertical_axis == other.vertical_axis;
}

// ========================================================================

VprofData::VprofData(const std::string& modelname, const std::string& stationsfilename)
    : numRealizations(1)
    , modelName(modelname)
    , stationsFileName(stationsfilename)
    , stationList(false)
{
  METLIBS_LOG_SCOPE();
}

VprofData::~VprofData()
{
  METLIBS_LOG_SCOPE();
}

void VprofData::readStationNames()
{
  METLIBS_LOG_SCOPE();
  std::ifstream stationfile(stationsFileName.c_str());
  if (!stationfile) {
    METLIBS_LOG_ERROR("Unable to open file '" << stationsFileName << "'");
    return;
  }

  mStations.clear();
  std::string line;
  diutil::GetLineConverter convertline("#");
  while (convertline(stationfile, line)) {
    // just skip the first line if present.
    if (miutil::contains(line, "obssource"))
      continue;
    if (!line.empty() && line[0] == '#')
      continue;
    std::vector<std::string> stationVector;
    int baseIdx;
    if (miutil::contains(line, ";")) {
      // the new format
      stationVector = miutil::split(line, ";", false);
      if (stationVector.size() == 7) {
        baseIdx = 2;
      } else if (stationVector.size() == 6) {
        baseIdx = 1;
      } else {
        METLIBS_LOG_ERROR("Something is wrong with: '" << line << "'");
        continue;
      }
    } else {
      // the old format
      stationVector = miutil::split(line, ",", false);
      if (stationVector.size() == 7) {
        baseIdx = 1;
      } else {
        METLIBS_LOG_ERROR("Something is wrong with: '" << line << "'");
        continue;
      }
    }
    const std::string& name = stationVector[baseIdx];
    const float lat = miutil::to_double(stationVector[baseIdx + 1]);
    const float lon = miutil::to_double(stationVector[baseIdx + 2]);
    mStations.push_back(stationInfo(name, lon, lat));
  }
}

void VprofData::renameStations()
{
  if (stationsFileName.empty())
    return;

  if (!stationList)
    readStationList();

  const int n = mStations.size(), m = stationName.size();

  std::multimap<std::string, int> sortlist;

  for (int i = 0; i < n; i++) {
    int jmin = -1;
    float smin = 0.05 * 0.05 + 0.05 * 0.05;
    for (int j = 0; j < m; j++) {
      float dx = mStations[i].lon - stationLongitude[j];
      float dy = mStations[i].lat - stationLatitude[j];
      float s = miutil::absval2(dx, dy);
      if (s < smin) {
        smin = s;
        jmin = j;
      }
    }
    std::string newname;
    if (jmin >= 0) {
      newname = stationName[jmin];
    } else {
      // TODO use formatLongitude/Latitude from diutil
      std::string slat = miutil::from_number(fabsf(mStations[i].lat));
      if (mStations[i].lat >= 0.)
        slat += "N";
      else
        slat += "S";
      std::string slon = miutil::from_number(fabsf(mStations[i].lon));
      if (mStations[i].lon >= 0.)
        slon += "E";
      else
        slon += "W";
      newname = slat + "," + slon;
      jmin = m;
    }

    std::ostringstream ostr;
    ostr << std::setw(4) << std::setfill('0') << jmin
         << newname
         // << validTime[i].isoTime() // FIXME index i may not be used here
         << mStations[i].name;
    sortlist.insert(std::make_pair(ostr.str(), i));

    stationMap[newname] = mStations[i].name;
    METLIBS_LOG_DEBUG(LOGVAL(newname) << LOGVAL(mStations[i].name));
    mStations[i].name = newname;
  }

  // gather amdars from same stations (in station list sequence)
  std::vector<stationInfo> stations;
  std::map<std::string, int> stationCount;
  for (std::multimap<std::string, int>::iterator pt = sortlist.begin(); pt != sortlist.end(); pt++) {
    int i = pt->second;

    std::string newname = mStations[i].name;
    int c;
    std::map<std::string, int>::iterator p = stationCount.find(newname);
    if (p == stationCount.end())
      stationCount[newname] = c = 1;
    else
      c = ++(p->second);
    newname += " (" + miutil::from_number(c) + ")";

    stationMap[newname] = stationMap[mStations[i].name];

    stations.push_back(stationInfo(newname, mStations[i].lon, mStations[i].lat));
  }

  std::swap(stations, mStations);
}

void VprofData::readStationList()
{
  METLIBS_LOG_SCOPE(LOGVAL(stationsFileName));
  stationList = true;

  if (stationsFileName.empty())
    return;

  std::ifstream file(stationsFileName.c_str());
  if (file.bad()) {
    METLIBS_LOG_ERROR("Unable to open station list '" << stationsFileName << "'");
    return;
  }

  const float notFound = 9999.;
  std::string str;
  diutil::GetLineConverter convertline("#");
  while (convertline(file, str)) {
    diutil::remove_comment_and_trim(str);
    if (str.empty())
      continue;

    float latitude = notFound, longitude = notFound;
    std::string name;
    for (const std::string& vs : miutil::split_protected(str, '"', '"')) {
      const std::vector<std::string> kv = miutil::split(vs, "=");
      if (kv.size() == 2) {
        const std::string key = miutil::to_lower(kv[0]);
        if (key == "latitude")
          latitude = miutil::to_float(kv[1]);
        else if (key == "longitude")
          longitude = miutil::to_float(kv[1]);
        else if (key == "name") {
          name = diutil::quote_removed(kv[1]);
        }
      }
    }
    if (latitude != notFound && longitude != notFound && not name.empty()) {
      stationLatitude.push_back(latitude);
      stationLongitude.push_back(longitude);
      stationName.push_back(name);
    }
  }
}
