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
#ifndef diVprofData_h
#define diVprofData_h

#include <diVprofPlot.h>

#include <puTools/miTime.h>
#include <diField/diFieldManager.h>

#include <vector>

/**
  \brief Vertical Profile (sounding) prognostic data from a met.no file
  
   Contains all data and misc information from one file 
*/
class VprofData
{

public:
  VprofData(const std::string& filename, const std::string& modelname);
  ~VprofData();
  bool readFile();
  bool readField(std::string type, FieldManager* fieldm);
  bool readFimex(FieldManager* fieldm);
  VprofPlot* getData(const std::string& name, const miutil::miTime& time);
  std::vector<std::string> getNames() { return posName; }
  std::vector <float> getLatitudes() { return posLatitude; }
  std::vector <float> getLongitudes() { return posLongitude; }
  std::vector<std::string> getObsNames() { return obsName; }
  std::vector<miutil::miTime>   getTimes() { return validTime; }

private:

  std::string fileName;
  std::string modelName;
  bool readFromFimex;
  bool readFromField;
  FieldManager* fieldManager;

  int numPos;
  int numTime;
  int numParam;
  int numLevel;

  struct station {
    std::string id; /**< WMO number */
    std::string name; /**< name */
    float lat; /**< latitude */
    float lon; /**< longitude */
    int height; /**< station height */
    int barHeight; /**< barometer height */
  };

  std::vector<std::string> posName;
  std::vector<std::string> obsName;
  std::vector<int>      posTemp;
  std::vector<float>    posLatitude;
  std::vector<float>    posLongitude;
  std::vector<float>    posDeltaLatitude;
  std::vector<float>    posDeltaLongitude;
  std::vector<miutil::miTime>   validTime;
  std::vector<int>      forecastHour;
  std::vector<std::string> progText;
  std::vector<std::string> mainText;
  std::vector<int>      paramId;
  std::vector<float>    paramScale;
  VprofPlot        *vProfPlot;
  std::string vProfPlotName;
  miutil::miTime   vProfPlotTime;


  // dataBuffer[numPos][numTime][numParam][numLevel]
  short int *dataBuffer;
};

#endif
