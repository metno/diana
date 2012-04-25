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

  vector<miutil::miString> lines;
  vector<ObsData> vObsData;
  map< miutil::miString, ObsData > mObsData;
  miutil::miString separator;
  bool fileOK;
  bool knots;
  miutil::miTime plotTime;
  miutil::miTime fileTime;
  int    timeDiff;

  vector<miutil::miString> columnType;
  vector<miutil::miString> asciiColumnUndefined;
  map<miutil::miString,int> asciiColumn; //column index(time, x,y,dd,ff etc)
  int  asciiSkipDataLines;
  vector<miutil::miString> labels;

  void readHeaderInfo(const miutil::miString &filename, const miutil::miString &headerfile,
      const vector<miutil::miString> headerinfo);

  void readData(const miutil::miString &filename);

  void decodeHeader();
  void decodeData();

  void addStationsToUrl(miutil::miString& filename);

public:
  vector<miutil::miString> columnName;
  vector<miutil::miString> columnTooltip;

  ObsAscii(const miutil::miString &filename, const miutil::miString &headerfile,
     const vector<miutil::miString> headerinfo);

  static bool getFromFile(const miutil::miString &filename, vector<miutil::miString>& lines);
  static bool getFromHttp(const miutil::miString &url, vector<miutil::miString>& lines);
  ObsAscii(const miutil::miString &filename, const miutil::miString &headerfile,
     const vector<miutil::miString> headerinfo, const miutil::miTime &filetime,
     ObsPlot *oplot);

  ObsAscii(const miutil::miString &filename, const miutil::miString &headerfile,
     const vector<miutil::miString> headerinfo,
     ObsMetaData *metaData);

  bool asciiOK() { return fileOK;}

  bool parameterType(miutil::miString param) { return asciiColumn.count(param); }
};

#endif




