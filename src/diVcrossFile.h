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
#ifndef diVcrossFile_h
#define diVcrossFile_h

#include <puTools/miString.h>
#include <puTools/miTime.h>
#include <vector>
#include <diField/diGridConverter.h>
#include <diLocationPlot.h>

using namespace std;

class FtnVfile;
class VcrossPlot;


/**
  \brief Vertical Crossection prognostic data from a met.no proprietary file

   Contains misc information from file header, incl. positions
   and data pointers.
*/
class VcrossFile
{

public:
  VcrossFile(const miString& filename, const miString& modelname);
  ~VcrossFile();
  void cleanup();
  bool update();
  bool readFileHeader();
  vector<miString> getNames() { return names; }
  vector<miTime> getTimes() { return validTime; }
  vector<miString> getFieldNames();
  void getMapData(vector<LocationElement>& elements);

  VcrossPlot* getCrossection(const miString& name, const miTime& time,
			     int tgpos= -1);

private:

  static GridConverter gc;

  miString fileName;
  miString modelName;

  FtnVfile *vfile;

  long int modificationtime;

  miString modelName2; // from file
  int vcoord;
  int numCross;
  int numTime;
  int numLev;
  int numPar2d;
  int numPar1d;

  int nlvlid;

  vector<int> numPoint;
  vector<int> identPar2d;
  vector<int> identPar1d;

  int nxgPar,nygPar,nxsPar,nxdsPar;

  int    nposmap;
  float *xposmap;
  float *yposmap;

  vector<miString> names;
  vector<miString> posOptions;
  vector<miTime>   validTime;
  vector<int>      forecastHour;
  vector<float>    vrangemin;
  vector<float>    vrangemax;

  // dataAddress[2][numCross][numTime]
  int *dataAddress;

};

#endif
