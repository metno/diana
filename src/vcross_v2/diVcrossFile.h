/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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

#include "diVcrossSource.h"

class FtnVfile;

/**
  \brief Vertical Crossection prognostic data from a met.no proprietary file

   Contains misc information from file header, incl. positions
   and data pointers.
*/
class VcrossFile : public VcrossSource
{
public:
  VcrossFile(const std::string& filename, const std::string& modelname);
  ~VcrossFile();
  virtual void cleanup();
  virtual bool update();
  virtual std::vector<std::string> getParameterIds() const
    { return mParameterIds; }

  const crossections_t& getCrossections() const
    { return mCrossections; }

  const std::vector<miutil::miTime>& getTimes() const
    { return validTime; }

  virtual VcrossData* getCrossData(const std::string& csname, const std::set<std::string>& parameters, const miutil::miTime& time);
  virtual VcrossData* getTimeData(const std::string& csname, const std::set<std::string>& parameters, int csPositionIndex);

private:
  bool readFileHeader();

private:
  std::string fileName;
  std::string modelName;

  FtnVfile *vfile;

  long int modificationtime;

  std::string modelName2; // from file
  int vcoord;
  int numCross;
  int numTime;
  int numLev;
  int numPar2d;
  int numPar1d;

  int nlvlid;

  std::vector<int> numPoint;
  std::vector<int> identPar2d;
  std::vector<int> identPar1d;

  int nxgPar,nygPar,nxsPar,nxdsPar;

  int    nposmap;
  float *xposmap;
  float *yposmap;

  crossections_t mCrossections;
  std::vector<std::string> posOptions;
  std::vector<miutil::miTime>   validTime;
  std::vector<int>      forecastHour;
  std::vector<float>    vrangemin;
  std::vector<float>    vrangemax;

  // dataAddress[2][numCross][numTime]
  int *dataAddress;

  std::vector<std::string> mParameterIds;
};

#endif // diVcrossFile_h
