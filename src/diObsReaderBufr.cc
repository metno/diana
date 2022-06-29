/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2022 met.no

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

#include "diObsReaderBufr.h"

#include "util/misc_util.h"

#include <puTools/miStringFunctions.h>

#ifdef BUFROBS
#include "diObsBufr.h"
#endif // BUFROBS

#define MILOGGER_CATEGORY "diana.ObsReaderBufr"
#include <miLogger/miLogging.h>

ObsReaderBufr::ObsReaderBufr()
{
  METLIBS_LOG_SCOPE();
}

bool ObsReaderBufr::configure(const std::string& key, const std::string& value)
{
  METLIBS_LOG_SCOPE(LOGVAL(key) << LOGVAL(value));
  if (key == "bufr" || key == "archive_bufr") {
    addPattern(value, key == "archive_bufr");
  } else {
    return ObsReaderFile::configure(key, value);
  }
  return true;
}

bool ObsReaderBufr::getDataFromFile(const FileInfo& fi, ObsDataRequest_cp request, ObsDataResult_p result)
{
  METLIBS_LOG_SCOPE(LOGVAL(fi.filename));
  bool success = false;
#ifdef BUFROBS
  ObsDataBufr bufr(request->level, request->obstime, request->timeDiff);
  if (auto obsp = bufr.getObsData(fi.filename)) {
    success = true;
    for (size_t i = 0; i < obsp->size(); ++i)
      obsp->basic(i).dataType = dataType();
    result->add(obsp);
  }
#else // !BUFROBS
  METLIBS_LOG_WARN("Diana compiled without BUFR reader");
#endif
  return success;
}

miutil::miTime ObsReaderBufr::getTimeFromFile(const std::string& filename)
{
  METLIBS_LOG_SCOPE(LOGVAL(filename));
  miutil::miTime time;
#ifdef BUFROBS
  if (ObsBufr::ObsTime(filename, time)) {
    METLIBS_LOG_DEBUG(LOGVAL(time));
  }
#else // !BUFROBS
  METLIBS_LOG_WARN("Diana compiled without BUFR reader");
#endif
  return time;
}
