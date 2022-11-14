/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no and SMHI

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

#ifndef diObsRoad_h
#define diObsRoad_h

#ifdef ROADOBS
#include "diCommonTypes.h"
#include "diObsData.h"
#include "diObsPlot.h"
#include "diObsReader.h"
#include "diStationTypes.h"
#include "diObsDataVector.h"
#include "vprof/diVprofSimpleData.h"
#include "vprof/diVprofSimpleValues.h"

#include <newarkAPI/diStation.h>

#include <puTools/miTime.h>

#include <map>
#include <set>
#include <string>
#include <vector>

/**
  \brief Reading road observation data
  In-house smhi.se format
*/
class ObsRoad
{
private:
  std::string filename_;
  std::string databasefile_;
  std::string stationfile_;
  std::string headerfile_;
  std::string datatype_;
  // This defines a set of stations, eg synop,metar,ship observations
  std::vector<road::diStation>* stationlist;
  miutil::miTime filetime_;
  void readHeader(ObsDataRequest_cp request);
  void readRoadData(ObsDataRequest_cp request);
  void initRoadData(ObsDataRequest_cp request);
  bool headerRead;

  // from ObsAscii class
  bool m_needDataRead;
  std::string m_filename;

  std::vector<std::string> lines;
  std::map<std::string, std::vector<std::string> > parameterlines;
  ObsDataVector_p vObsData;
  std::map<std::string, ObsData> mObsData;
  std::string separator;
  bool fileOK;
  bool knots;
  miutil::miTime plotTime;
  miutil::miTime fileTime;
  int timeDiff;

  std::set<std::string> asciiColumnUndefined;
  typedef std::map<std::string, size_t> string_size_m;
  string_size_m asciiColumn; // column index(time, x,y,dd,ff etc)
  string_size_m parameterColumn; // column index (Parameter, LevelParameter, ...)
  // Name of parameter column in diana/mora mapping file (parameters_mora.csv)
  std::vector<std::string> m_parameterColumnName;
  int asciiSkipDataLines;
  PlotCommand_cpv labels;

  std::vector<std::string> m_columnType;
  std::vector<std::string> m_columnName;
  std::vector<std::string> m_columnTooltip;

  void readDecodeData();
  void readData(const std::string& filename);

  void decodeData();
  string_size_m::const_iterator getColumn(const std::string& cn, const std::vector<std::string>& cv) const;
  bool getColumnValue(const std::string& cn, const std::vector<std::string>& cv, float& value) const;
  bool getColumnValue(const std::string& cn, const std::vector<std::string>& cv, int& value) const;
  bool getColumnValue(const std::string& cn, const std::vector<std::string>& cv, std::string& value) const;
  
  void decodeNewData();
  void decodeOneLine(ObsData * obsData, std::vector<std::string> & pstr, const int & obshour);
  std::string convertWmoStationId(std::string & text);
  float convertOperatingMode(std::string & text);
  int decodeOffset(std::string & text);
  
  std::string getParameterColumn(const std::string& cn, const std::vector<std::string>& cv);
  bool getParameterColumnValue(const std::string& cn, const std::vector<std::string>& cv, float& value);
  bool getParameterColumnValue(const std::string& cn, const std::vector<std::string>& cv, int& value);
  bool getParameterColumnValue(const std::string& cn, const std::vector<std::string>& cv, std::string& value);

  void readHeaderInfo(const std::string& filename, const std::string& headerfile, const std::vector<std::string>& headerinfo);
  void decodeHeader();
  bool bracketContents(std::vector<std::string>& in_out);
  void parseHeaderBrackets(const std::string& str);
  
  void decodeNewHeader();
  void parseNewHeaderFormat(const std::string& str);
  

  void addStationsToUrl(std::string& filename);
  enum VerticalAxis { PRESSURE, ALTITUDE };
  VprofSimpleValues_p vp;
  VprofSimpleData_p temperature, dewpoint_temperature, wind_dd, wind_ff, wind_sig;
  VerticalAxis vertical_axis_;
  // New obsData interface
  bool isAuto(const ObsData& obs);
  void decodeMetarCloudAmount(const int & Nsx, QString & ost);  

public:
  ObsRoad(const std::string& filename, const std::string& databasefile, const std::string& stationfile, const std::string& headerfile,
          const miutil::miTime& filetime, ObsDataRequest_cp request, bool breadData);
  void readData(ObsDataVector_p &obsdata,ObsDataRequest_cp request);
  void initData(ObsDataRequest_cp request);

  void cloud_type_string(ObsData& d, double v);
  std::string height_of_clouds_string(double height);
  // from ObsBufr
  void cloud_type(ObsData& d, double v);
  float height_of_clouds(double height);
  float convert2hft(double height);
  float ms2code4451(float v);
  float percent2oktas(float v);
  float convertWW(float ww);
  // from ObsPlot
  void amountOfClouds_1_4(ObsData& dta, bool metar);
  void amountOfClouds_1(ObsData& dta, bool metar);
  
  static std::map<std::string, std::map<std::string, std::vector<std::string>>> parameterMap;
  static std::map<std::string, ObsDataVector_p> obsDataCache;
  static std::map<std::string, time_t> obsDataTimeStamp;
  static void clearCaches(){obsDataCache.clear(); obsDataTimeStamp.clear();};

  VprofValues_p getVprofPlot(const std::string& modelName, const std::string& station, const miutil::miTime& time);

  void getStationList(std::vector<stationInfo>& stations);
  friend class ObsData;
};

#endif // ROADOBS
#endif // diObsRoad_h
