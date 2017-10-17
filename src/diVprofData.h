/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2016 met.no

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
#ifndef diVprofData_h
#define diVprofData_h

#include "diVprofPlot.h"

#include "diStationInfo.h"

#include <puTools/miTime.h>
#include "vcross_v2/VcrossCollector.h"
#include <vector>

/**
  \brief Vertical Profile (sounding) prognostic data

   Contains all data and misc information from one file
*/
class VprofData
{

public:
  VprofData(const std::string& modelname,
      const std::string& stationsfilename="");
  ~VprofData();
  bool readRoadObs(const std::string& databasefile, const std::string& parameterfile);
  bool readBufr(const std::string& modelname, const std::string& pattern);
  bool readFimex(vcross::Setup_p setup, const std::string& reftime );
  VprofPlot* getData(const std::string& name, const miutil::miTime& time, int realization);
  bool updateStationList(const miutil::miTime& plotTime);

  const std::vector<stationInfo>& getStations() const
    { return mStations; }
  const std::vector<miutil::miTime>& getTimes() const
    { return validTime; }
  int getRealizationCount() const
    { return numRealizations; }
  const std::string& getModelName() const
    { return modelName; }
  const std::vector<std::string>& getFileNames() const
    { return fileNames; }
  void readBufrFile(int i, int j, const std::string& model, std::vector<std::string>& namelist,
      std::vector<float>& latitudelist, std::vector<float>& longitudelist,
      std::vector<miutil::miTime>& tlist);

private:
  bool setBufr(const miutil::miTime& plotTime);
  bool setRoadObs(const miutil::miTime& plotTime);
  void readStationNames(const std::string& stationsfilename);
  void renameStations();
  void readStationList();

  enum FileFormat {
    fimex,
    bufr,
    roadobs
  };

  std::string modelName;
  std::string stationsFileName;
  std::string db_parameterfile;
  std::string db_connectfile;
  FileFormat format;

  int numPos;
  int numTime;
  int numParam;
  int numLevel;
  int numRealizations;

  std::vector<stationInfo> mStations;
  std::vector<miutil::miTime>   validTime;
  std::vector<int>      forecastHour;
  std::vector<std::string> progText;
  std::unique_ptr<VprofPlot> vProfPlot;
  std::string vProfPlotName;
  miutil::miTime   vProfPlotTime;
  std::vector<std::string> currentFiles;

  std::vector<std::string> fileNames;

  vcross::Collector_p collector;
  vcross::string_v fields;
  vcross::Time reftime;

  bool stationList;
  std::vector<float>    stationLatitude;
  std::vector<float>    stationLongitude;
  std::vector<std::string> stationName;
  std::map< std::string, std::string > stationMap;
};

extern const char VP_AIR_TEMPERATURE[];
extern const char VP_DEW_POINT_TEMPERATURE[];
extern const char VP_X_WIND[];
extern const char VP_Y_WIND[];
extern const char VP_RELATIVE_HUMIDITY[];
extern const char VP_OMEGA[]; // upward air velocity in Pa/s

#endif
