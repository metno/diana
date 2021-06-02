/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2021 met.no

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
public:
  struct Column
  {
    std::string type;
    std::string name;
    std::string tooltip;
  };

private:
  bool m_needDataRead;
  bool m_error;
  std::string m_filename;

  std::vector<std::string> lines;
  std::vector<ObsData> vObsData;
  std::string separator;
  bool knots;
  miutil::miTime plotTime;
  miutil::miTime fileTime;
  int    timeDiff;

  std::set<std::string> asciiColumnUndefined;

  // column indices (time, x,y,dd,ff etc)
  size_t idx_x;
  size_t idx_y;
  size_t idx_ff;
  size_t idx_dd;
  size_t idx_image;
  size_t idx_Name;

  size_t idx_date;
  size_t idx_year;
  size_t idx_month;
  size_t idx_day;

  size_t idx_time;
  size_t idx_hour;
  size_t idx_min;
  size_t idx_sec;

  int  asciiSkipDataLines;
  PlotCommand_cpv labels;

  std::vector<std::string> m_columnType;
  std::vector<std::string> m_columnName;
  std::vector<std::string> m_columnTooltip;
  std::vector<Column> column;
  void readDecodeData();
  void readData(const std::string &filename);
  void decodeData();

  bool checkColumn(size_t idx, const std::vector<std::string>& cv) const;

  void readHeaderInfo(const std::string& filename, const std::string& headerfile,
      const std::vector<std::string>& headerinfo);
  void decodeHeader();
  bool parseHeaderBrackets(const std::string& str);

  void addStationsToUrl(std::string& filename);

public:
  ObsAscii(const std::string& filename, const std::string& headerfile,
      const std::vector<std::string>& headerinfo);

  PlotCommand_cpv getLabels() const
    { return labels; }
  const std::vector<Column>& getColumns() const
    { return column; }

  const std::vector<ObsData>& getObsData(const miutil::miTime& filetime, const miutil::miTime& time, int timeDiff);
  bool hasError() const { return m_error; }

public:
  static bool bracketContents(std::vector<std::string>& in_out);
};

#endif
