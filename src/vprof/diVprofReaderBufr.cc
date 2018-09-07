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

#include "diUtilities.h"
#include "diVprofReaderBufr.h"

#include "puTools/TimeFilter.h"

#ifdef BUFROBS
#include "diObsBufr.h"
#endif // BUFROBS

#define MILOGGER_CATEGORY "diana.VprofReaderBufr"
#include <miLogger/miLogging.h>

VprofData_p VprofReaderBufr::find(const VprofSelectedModel& vsm, const std::string& stationsfilename)
{
  return VprofDataBufr::createData(filenames[vsm.model], vsm, stationsfilename);
}

// static
VprofData_p VprofDataBufr::createData(const std::string& pattern_, const VprofSelectedModel& vsm, const std::string& stationsfilename)
{
  METLIBS_LOG_SCOPE();
  VprofDataBufr_p vpd = std::make_shared<VprofDataBufr>(vsm.model, stationsfilename);

#ifdef BUFROBS
  miutil::TimeFilter tf;
  std::string pattern = pattern_;
  tf.initFilter(pattern);

  diutil::string_v matches = diutil::glob(pattern);
  for (diutil::string_v::const_iterator it = matches.begin(); it != matches.end(); ++it) {
    std::string filename = *it;
    miutil::miTime time;
    if (!tf.ok() || !tf.getTime(filename, time)) {
      METLIBS_LOG_DEBUG("TimeFilter not ok" << LOGVAL(filename));
      if (!ObsBufr::ObsTime(filename, time))
        continue;
    }
    METLIBS_LOG_DEBUG(LOGVAL(filename) << LOGVAL(time));
    vpd->addValidTime(time);
    vpd->fileNames.push_back(filename);
  }
#endif
  return vpd;
}

VprofDataBufr::VprofDataBufr(const std::string& modelname, const std::string& stationsfilename)
    : VprofData(modelname, stationsfilename)
{
}

bool VprofDataBufr::updateStationList(const miutil::miTime& plotTime)
{
  METLIBS_LOG_SCOPE();
  currentFiles.clear();
  mStations.clear();
#ifdef BUFROBS
  std::vector<miutil::miTime> tlist;

  const std::vector<miutil::miTime> vt = getTimes();
  for (size_t j = 0; j < vt.size(); ++j) {
    if (vt[j] == plotTime) {
      currentFiles.push_back(fileNames[j]);
      // TODO: include files with time+-timediff, this is just a hack to include files with time = plottime - 1 hour
      if (j > 0 && abs(miutil::miTime::minDiff(vt[j], plotTime)) <= 60) {
        currentFiles.push_back(fileNames[j - 1]);
      }
      StationBufr bufr;
      bufr.readStationInfo(currentFiles, mStations, tlist);
      renameStations();
      break;
    }
  }
#endif
  return true;
}

VprofValues_cpv VprofDataBufr::getValues(const VprofValuesRequest& req)
{
  METLIBS_LOG_SCOPE(req.name << "  " << req.time << "  " << getModelName());
  METLIBS_LOG_DEBUG(LOGVAL(getTimes().size()) << LOGVAL(mStations.size()));

#ifdef BUFROBS
  if (cachedRequest.time.undef() || cachedRequest != req) {
    cache.clear();
    cachedRequest = req;

    VprofBufr::VerticalAxis zax = VprofBufr::PRESSURE;
    if (req.vertical_axis == vcross::Z_TYPE_ALTITUDE)
      zax = VprofBufr::ALTITUDE;
    std::map<std::string, std::string>::const_iterator itS = stationMap.find(req.name);
    const std::string& name_ = (itS != stationMap.end()) ? itS->second : req.name;

    VprofBufr bufr;
    if (VprofValues_p vv = bufr.getVprofPlot(currentFiles, name_, zax)) {
      vv->text.modelName.clear();
      cache.push_back(vv);
    }
  }
#endif // !BUFROBS

  return cache;
}
