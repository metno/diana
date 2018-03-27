/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017 met.no

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

#include "diObsReaderFile.h"

#include "diField/VcrossUtil.h"
#include "diUtilities.h"
#include "util/misc_util.h"
#include "util/time_util.h"

#include <puCtools/stat.h>
#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.ObsReaderFile"
#include <miLogger/miLogging.h>

namespace {

//! \return file modification timestamp, or -1 if file does not exist
long modificationtime(const std::string& filename)
{
  pu_struct_stat buf;
  if (pu_stat(filename.c_str(), &buf) != 0)
    return -1;
  return (long)buf.st_mtime;
}

} // namespace

// ########################################################################

ObsReaderFile::ObsReaderFile()
    : timeRangeMin_(-60)
    , timeRangeMax_(60)
{
}

bool ObsReaderFile::configure(const std::string& key, const std::string& value)
{
  if (key == "timerange") {
    std::vector<std::string> time = miutil::split(value, ",");
    if (time.size() != 2)
      return false;
    setTimeRange(miutil::to_int(time[0]), miutil::to_int(time[1]));
  } else {
    return ObsReader::configure(key, value);
  }
  return true;
}

void ObsReaderFile::setTimeRange(int tmin, int tmax)
{
  timeRangeMin_ = tmin;
  timeRangeMax_ = tmax;
}

std::set<miutil::miTime> ObsReaderFile::getTimes(bool useArchive, bool update)
{
  METLIBS_LOG_SCOPE(LOGVAL(fileInfo.size()));
  std::set<miutil::miTime> times;

  if (update || fileInfo.empty())
    checkForUpdates(useArchive);

  for (const FileInfo& fi : fileInfo) {
    if (!fi.time.undef())
      times.insert(fi.time);
  }
  return times;
}

void ObsReaderFile::addPattern(const std::string& p, bool archive)
{
  METLIBS_LOG_SCOPE(LOGVAL(p));
  patternInfo pf;
  pf.pattern = p;
  pf.archive = archive;
  miutil::remove(pf.pattern, '\"');
  pf.filter = miutil::TimeFilter(pf.pattern); // modifies pf.pattern!
  pattern.push_back(pf);
}

void ObsReaderFile::getData(ObsDataRequest_cp request, ObsDataResult_p result)
{
  METLIBS_LOG_SCOPE();
  if (fileInfo.empty())
    checkForUpdates(request->useArchive);

  std::vector<ObsData> obsdata;

  // actual timespan for request
  miutil::miTime req_min = request->obstime, req_max = request->obstime;
  if (request->timeDiff >= 0) {
    req_min.addMin(-request->timeDiff);
    req_max.addMin(+request->timeDiff);
  }

  int best_diff = -1;
  const FileInfo* best_fi = 0;

  // find file(s) where [req_min..req_max] overlaps [filetime+timeRangeMin..filetime+timeRangeMax]
  for (const FileInfo& fi : fileInfo) {
    if (fi.time.undef() || (!isSynoptic() && request->timeDiff < 0)) {
      getDataFromFile(fi, request, result);
    } else {
      const miutil::miTime file_min = miutil::addMin(fi.time, timeRangeMin());
      const miutil::miTime file_max = miutil::addMin(fi.time, timeRangeMax());
      if (req_max >= file_min && req_min <= file_max) {
        if (isSynoptic()) {
          const int diff = std::abs(miutil::miTime::minDiff(request->obstime, fi.time));
          if (!best_fi || diff < best_diff) {
            best_diff = diff;
            best_fi = &fi;
          }
        } else {
          getDataFromFile(fi, request, result);
          result->setTime(fi.time);
        }
      }
    }
  }

  if (isSynoptic() && best_fi) {
    getDataFromFile(*best_fi, request, result);
    result->setTime(best_fi->time);
  }
}

bool ObsReaderFile::checkForUpdates(bool useArchive)
{
  METLIBS_LOG_SCOPE();

  const std::vector<FileInfo> oldfileInfo = fileInfo;
  fileInfo.clear();

  for (const patternInfo& pit : pattern) {
    if (!pit.archive || useArchive) {
      METLIBS_LOG_DEBUG(LOGVAL(pit.pattern));
      bool ok = pit.filter.ok();
      const diutil::string_v matches = diutil::glob(pit.pattern);
      if (matches.empty())
        METLIBS_LOG_INFO("no files matching '" << pit.pattern << "'");
      for (const std::string& m : matches) {
        FileInfo finfo;
        finfo.filename = m;
        finfo.mtime = modificationtime(finfo.filename);
        if (ok && pit.filter.getTime(finfo.filename, finfo.time)) {
          // time from file name
        } else {
          // need to get time from file content
          finfo.time = getTimeFromFile(finfo.filename);
        }
        fileInfo.push_back(finfo);
      }
    }
  }

  // return true if timeLists are different
  if (fileInfo.size() != oldfileInfo.size())
    return true;

  for (size_t i = 0; i < fileInfo.size(); i++) {
    const FileInfo &fi = fileInfo[i], &ofi = oldfileInfo[i];
    if ((fi.time != ofi.time) || (fi.mtime != ofi.mtime))
      return true;
  }
  return false;
}

miutil::miTime ObsReaderFile::getTimeFromFile(const std::string&)
{
  return miutil::miTime();
}
