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

#include "diana_config.h"

#include "diVprofReaderRoadobs.h"

#ifdef ROADOBS
#include "diObsRoad.h"
#endif

#define MILOGGER_CATEGORY "diana.VprofReaderRoadobs"
#include <miLogger/miLogging.h>

using namespace miutil;

VprofData_p VprofReaderRoadobs::find(const VprofSelectedModel& vsm, const std::string& stationsfilename)
{
  METLIBS_LOG_SCOPE();
#ifdef ROADOBS
  VprofDataRoadobs_p vpd = std::make_shared<VprofDataRoadobs>(vsm.model, stationsfilename);
  vpd->db_parameterfile = db_parameters[vsm.model];
  vpd->db_connectfile = db_connects[vsm.model];

  // Due to the fact that we have a database insteda of an archive,
  // we must fake the behavoir of anarchive
  // Assume that all stations report every hour
  // first, get the current time.
  //
  // We assume that tempograms are made 00Z, 06Z, 12Z and 18Z.
  miTime now = miTime::nowTime();
  miClock nowClock = now.clock();
  miDate nowDate = now.date();
  nowClock.setClock(nowClock.hour(), 0, 0);
  now.setTime(nowDate, nowClock);
  /* TBD: read from setup file */
  int daysback = 4;
  miTime starttime = now;
  if (now.hour() % 6 != 0) {
    /* Adjust start hour */
    switch (now.hour()) {
    case 1:
    case 7:
    case 13:
    case 19:
      now.addHour(-1);
      break;
    case 2:
    case 8:
    case 14:
    case 20:
      now.addHour(-2);
      break;
    case 3:
    case 9:
    case 15:
    case 21:
      now.addHour(-3);
      break;
    case 4:
    case 10:
    case 16:
    case 22:
      now.addHour(-4);
      break;
    case 5:
    case 11:
    case 17:
    case 23:
      now.addHour(-5);
      break;
    }
  }
  try {
    // Dummy filename
    std::string filename;
    METLIBS_LOG_DEBUG("Parameters: " << vpd->db_connectfile << "," << stationsfilename << "," << vpd->db_parameterfile << "," << starttime);
    // read stationlist and init the api.
    ObsRoad road = ObsRoad(filename, vpd->db_connectfile, stationsfilename, vpd->db_parameterfile, starttime, NULL, false);

  } catch (...) {
    METLIBS_LOG_ERROR("Exception: " << vpd->db_connectfile << "," << stationsfilename << "," << vpd->db_parameterfile << "," << starttime);
    return VprofData_p();
  }

  starttime.addDay(-daysback);
  int hourdiff;
  miTime time = now;
  METLIBS_LOG_DEBUG(LOGVAL(time));
  vpd->addValidTime(time);
  time.addHour(-6);
  while ((hourdiff = miTime::hourDiff(time, starttime)) > 0) {
    METLIBS_LOG_DEBUG(LOGVAL(time));
    vpd->addValidTime(time);
    time.addHour(-6);
  }
  return vpd;
#else  // !ROADOBS
  return VprofData_p();
#endif // !ROADOBS
}

VprofDataRoadobs::VprofDataRoadobs(const std::string& modelname, const std::string& stationsfilename)
    : VprofData(modelname, stationsfilename)
{
}

bool VprofDataRoadobs::updateStationList(const miutil::miTime& plotTime)
{
  setRoadObs(plotTime);
  return !mStations.empty();
}

bool VprofDataRoadobs::setRoadObs(const miutil::miTime& plotTime)
{
#ifdef ROADOBS
  mStations.clear();
  try {
    // Dummy filename
    std::string filename;
    // read stationlist and init the api.
    METLIBS_LOG_DEBUG("Parameters: " << db_connectfile << "," << getStationsFileName() << "," << db_parameterfile << "," << plotTime);
    // does nothing if already done
    ObsRoad road = ObsRoad(filename, db_connectfile, getStationsFileName(), db_parameterfile, plotTime, NULL, false);
    road.getStationList(mStations);

  } catch (...) {
    METLIBS_LOG_ERROR("Exception in ObsRoad: " << db_connectfile << "," << getStationsFileName() << "," << db_parameterfile << "," << plotTime);
    return false;
  }
#endif
  return true;
}

VprofValues_cpv VprofDataRoadobs::getValues(const VprofValuesRequest& req)
{
  VprofValues_cpv values;

  METLIBS_LOG_SCOPE(req.name << "  " << req.time << "  " << getModelName());
  METLIBS_LOG_DEBUG(LOGVAL(getTimes().size()) << LOGVAL(mStations.size()));
  METLIBS_LOG_DEBUG("Parameters: " << db_connectfile << "," << getStationsFileName() << "," << db_parameterfile << "," << req.time);

#ifdef ROADOBS
  try {
    // Dummy filename
    std::string filename;
    // read stationlist and init the api.
    // does nothing if already done
    ObsRoad road = ObsRoad(filename, db_connectfile, getStationsFileName(), db_parameterfile, req.time, NULL, false);
    VprofValues_p vv = road.getVprofPlot(getModelName(), req.name, req.time);
    values.push_back(vv);
  } catch (...) {
    METLIBS_LOG_ERROR("Exception in ObsRoad: " << db_connectfile << "," << getStationsFileName() << "," << db_parameterfile << "," << req.time);
  }
#endif
  return values;
}
