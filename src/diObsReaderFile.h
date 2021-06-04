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

#ifndef DIOBSREADERFILE_H
#define DIOBSREADERFILE_H

#include "diObsReader.h"

#include <puTools/TimeFilter.h>

#include <vector>

class ObsReaderFile : public ObsReader
{
public:
  struct patternInfo
  {
    miutil::TimeFilter filter;
    std::string pattern;
    bool archive;
  };
  struct FileInfo
  {
    std::string filename;
    miutil::miTime time;
    long mtime; //! modification time
  };

  ObsReaderFile();

  void setTimeRange(int tmin, int tmax);
  int timeRangeMax() const { return timeRangeMax_; }
  int timeRangeMin() const { return timeRangeMin_; }

  bool configure(const std::string& key, const std::string& value) override;
  bool checkForUpdates(bool useArchive) override;

  std::set<miutil::miTime> getTimes(bool useArchive, bool update) override;
  /**
   * @brief getData Read observation data from files.
   *
   * Search for data in a two-step procedure:
   * 1. select files:
   *   - files without filetime
   *   - if not synoptic and reqtime+-timediff < 0, all files
   *   - files with [filetime+-timerange] in [reqtime+-timediff]
   *     - if synoptic: select only the file with minimum |filetime - reqtime|
   * 2. select all obs from these files with obstime in [reqtime+-timediff]
   *   (\see ObsReader::getData)
   *
   * Allways update file/time list
   *
   * @param request
   * @param result
   */
  void getData(ObsDataRequest_cp request, ObsDataResult_p result) override;

protected:
  void addPattern(const std::string& pattern, bool archive);

  virtual miutil::miTime getTimeFromFile(const std::string& filename);

  //! \returns true if no errors were encountered
  virtual bool getDataFromFile(const FileInfo& fi, ObsDataRequest_cp request, ObsDataResult_p result) = 0;

protected:
  std::vector<patternInfo> pattern;
  std::vector<FileInfo> fileInfo;
  int timeRangeMin_;
  int timeRangeMax_;
};

#endif // DIOBSREADERFILE_H
