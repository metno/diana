/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

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

#include <diObsData.h>
#include <puTools/miString.h>
#include <puTools/miTime.h>
#include <map>

class ObsPlot;
class ObsMetaData;

using namespace std;


/**

  \brief Reading ascii observation files 
  
  In-house met.no format

*/
class ObsAscii {
private:

  std::vector<std::string> lines;
  vector<ObsData> vObsData;
  map< std::string, ObsData > mObsData;
  std::string separator;
  bool fileOK;
  bool knots;
  miutil::miTime plotTime;
  miutil::miTime fileTime;
  int    timeDiff;

  vector<std::string> columnType;
  vector<std::string> asciiColumnUndefined;
  map<std::string,int> asciiColumn; //column index(time, x,y,dd,ff etc)
  int  asciiSkipDataLines;
  std::vector<std::string> labels;

  void readHeaderInfo(const std::string& filename, const std::string& headerfile,
      const std::vector<std::string>& headerinfo);

  void readData(const std::string &filename);

  void decodeHeader();
  void decodeData();

  void addStationsToUrl(std::string& filename);

public:
  vector<std::string> columnName;
  vector<std::string> columnTooltip;

  ObsAscii(const std::string& filename, const std::string& headerfile,
      const std::vector<std::string> headerinfo);

  static bool getFromFile(const std::string& filename, std::vector<std::string>& lines);
  static bool getFromHttp(const std::string &url, std::vector<std::string>& lines);
  ObsAscii(const std::string& filename, const std::string& headerfile,
      const std::vector<std::string>& headerinfo, const miutil::miTime &filetime,
      ObsPlot *oplot);

  ObsAscii(const std::string& filename, const std::string& headerfile,
      const std::vector<std::string>& headerinfo, ObsMetaData *metaData);

  bool asciiOK() { return fileOK;}

  bool parameterType(std::string param) { return asciiColumn.count(param); }
};

#endif




