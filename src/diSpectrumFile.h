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
#ifndef diSpectrumFile_h
#define diSpectrumFile_h

#include <puTools/miString.h>
#include <puTools/miTime.h>
#include <vector>

using namespace std;

class FtnVfile;
class SpectrumPlot;

/**
  \brief Wave Spectrum data from a met.no proprietary file

   Contains misc information from file header, incl. positions
   and data pointers.
*/
class SpectrumFile
{

public:
  SpectrumFile(const std::string& filename, const std::string& modelname);
  ~SpectrumFile();
  void cleanup();
  bool update();
  bool readFileHeader();
  vector<std::string> getNames() { return posName; }
  vector<float> getLatitudes() { return posLatitude; }
  vector<float> getLongitudes() { return posLongitude; }
  vector<miutil::miTime> getTimes() { return validTime; }

  SpectrumPlot* getData(const std::string& name, const miutil::miTime& time);

private:

  std::string fileName;
  std::string modelName;

  FtnVfile *vfile;

  long int modificationtime;

  std::string modelName2; // from file
  int numPos;
  int numTime;
  int numDirec;
  int numFreq;
  int numExtra;

  vector<float> directions;
  vector<float> frequences;
  vector<float> extraScale;

  vector<std::string> posName;
  vector<float>    posLatitude;
  vector<float>    posLongitude;
  vector<miutil::miTime>   validTime;
  vector<int>      forecastHour;

  // dataAddress[2][numPos][numTime]
  int *dataAddress;

};

#endif
