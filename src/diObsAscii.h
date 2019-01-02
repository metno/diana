/*
  Diana - A Free Meteorological Visualisation Tool

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

#ifndef diObsAscii_h
#define diObsAscii_h

#include "diObsData.h"
#include "diPlotCommand.h"

#include <puTools/miTime.h>

#include <map>
#include <set>
#include <string>
#include <vector>

/**
  \brief Reading ascii observation files 

  In-house met.no format
*/
class ObsAscii {
private:
  bool m_needDataRead;
  std::string m_filename;

  std::vector<std::string> lines;
  std::vector<ObsData> vObsData;
  std::string separator;
  bool fileOK;
  bool knots;
  miutil::miTime plotTime;
  miutil::miTime fileTime;
  int    timeDiff;

  std::set<std::string> asciiColumnUndefined;
  typedef std::map<std::string, size_t> string_size_m;
  string_size_m asciiColumn; //column index(time, x,y,dd,ff etc)
  int  asciiSkipDataLines;
  PlotCommand_cpv labels;

  std::vector<std::string> m_columnType;
  std::vector<std::string> m_columnName;
  std::vector<std::string> m_columnTooltip;

  void readDecodeData();
  void readData(const std::string &filename);
  void decodeData();
  string_size_m::const_iterator getColumn(const std::string& cn, const std::vector<std::string>& cv) const;
  bool getColumnValue(const std::string& cn, const std::vector<std::string>& cv, float& value) const;
  bool getColumnValue(const std::string& cn, const std::vector<std::string>& cv, int& value) const;
  bool getColumnValue(const std::string& cn, const std::vector<std::string>& cv, std::string& value) const;

  void readHeaderInfo(const std::string& filename, const std::string& headerfile,
      const std::vector<std::string>& headerinfo);
  void decodeHeader();
  bool bracketContents(std::vector<std::string>& in_out);
  void parseHeaderBrackets(const std::string& str);

  void addStationsToUrl(std::string& filename);

public:
  ObsAscii(const std::string& filename, const std::string& headerfile,
      const std::vector<std::string>& headerinfo);

  PlotCommand_cpv getLabels() const
    { return labels; }
  const std::vector<std::string>& getColumnNames() const
    { return m_columnName; }

  const std::vector<ObsData> &getObsData(const miutil::miTime &filetime, const miutil::miTime &time, int timeDiff);
};

#endif
