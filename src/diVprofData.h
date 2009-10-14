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

#include <puTools/miString.h>
#include <puTools/miTime.h>
#include <vector>

using namespace std;

/**

  \brief Vertical Profile (sounding) prognostic data from a met.no file
  
   Contains all data and misc information from one file 

*/
class VprofData
{

public:
  VprofData(const miString& filename, const miString& modelname);
  ~VprofData();
  bool readFile();
  VprofPlot* getData(const miString& name, const miTime& time);
  vector<miString> getNames() { return posName; }
  vector <float> getLatitudes() { return posLatitude; }
  vector <float> getLongitudes() { return posLongitude; }
  vector<miString> getObsNames() { return obsName; }
  vector<miTime>   getTimes() { return validTime; }

private:

  miString fileName;
  miString modelName;

  int numPos;
  int numTime;
  int numParam;
  int numLevel;

  vector<miString> posName;
  vector<miString> obsName;
  vector<int>      posTemp;
  vector<float>    posLatitude;
  vector<float>    posLongitude;
  vector<float>    posDeltaLatitude;
  vector<float>    posDeltaLongitude;
  vector<miTime>   validTime;
  vector<int>      forecastHour;
  vector<miString> progText;
  vector<miString> mainText;
  vector<int>      paramId;
  vector<float>    paramScale;

  // dataBuffer[numPos][numTime][numParam][numLevel]
  short int *dataBuffer;

};

#endif
