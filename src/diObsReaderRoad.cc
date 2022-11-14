/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2021 met.no

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


#include "diObsReaderRoad.h"

#include "diObsDialogInfo.h"

#include "diObsDataVector.h"

// includes for road specific implementation
#include <diObsRoad.h>

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.ObsReaderRoad"
#include <miLogger/miLogging.h>

using namespace miutil;

ObsReaderRoad::ObsReaderRoad()
{
  setTimeRange(-180, 180);
}

bool ObsReaderRoad::configure(const std::string& key, const std::string& value)
{
  if (key == "roadobs" || key == "archive_roadobs") {
    addPattern(value, key == "archive_roadobs");
  } else if (key == "headerfile") {
    headerfile = value;
  } else if (key == "databasefile") {
    databasefile = value;
  } else if (key == "stationfile") {
    stationfile = value;
  } else if (key == "daysback") {
    daysback = miutil::to_int(value);
  } else {
    return ObsReaderFile::configure(key, value);
  }
  return true;
}

bool ObsReaderRoad::getDataFromFile(const FileInfo& fi, ObsDataRequest_cp request, ObsDataResult_p result)
{
#ifdef ROADOBS
  METLIBS_LOG_DEBUG(fi.filename << ", " << databasefile << ", " << stationfile << ", " << headerfile << ", " << fi.time);
  // The constructor with last argument false init the internal datastructures, but reads no data.
  ObsRoad obsRoad(fi.filename, databasefile, stationfile, headerfile, fi.time, request, false);
  // readData reads the data from the SMHI database.
  ObsDataVector_p obsdata;
  //std::vector<ObsData> obsdata;
  obsRoad.readData(obsdata,request);
  for (size_t i = 0; i < obsdata->size(); ++i)
      obsdata->basic(i).dataType = dataType();
  result->add(obsdata);
#endif
  return true; // FIXME check for errors
}
