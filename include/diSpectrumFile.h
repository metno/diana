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
  SpectrumFile(const miString& filename, const miString& modelname);
  ~SpectrumFile();
  void cleanup();
  bool update();
  bool readFileHeader();
  vector<miString> getNames() { return posName; }
  vector<float> getLatitudes() { return posLatitude; }
  vector<float> getLongitudes() { return posLongitude; }
  vector<miTime> getTimes() { return validTime; }

  SpectrumPlot* getData(const miString& name, const miTime& time);

private:

  miString fileName;
  miString modelName;

  FtnVfile *vfile;

  long int modificationtime;

  miString modelName2; // from file
  int numPos;
  int numTime;
  int numDirec;
  int numFreq;
  int numExtra;

  vector<float> directions;
  vector<float> frequences;
  vector<float> extraScale;

  vector<miString> posName;
  vector<float>    posLatitude;
  vector<float>    posLongitude;
  vector<miTime>   validTime;
  vector<int>      forecastHour;

  // dataAddress[2][numPos][numTime]
  int *dataAddress;

};

#endif
