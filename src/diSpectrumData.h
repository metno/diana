/*
 Diana - A Free Meteorological Visualisation Tool

 $Id: diSpectrumFile.h 3822 2013-11-01 19:42:36Z alexanderb $

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
#ifndef diSpectrumData_h
#define diSpectrumData_h

#include <puTools/miTime.h>
#include <vector>
#include <diField/VcrossSource.h>
#include "vcross_v2/VcrossCollector.h"

class SpectrumPlot;

/**
 \brief Wave Spectrum data from fimex

 Contains misc information from file header, incl. positions
 and data pointers.
 */
class SpectrumData {
public:
  SpectrumData(const std::string& modelname);
  ~SpectrumData();
  bool readFileHeader(vcross::Setup_p setup, const std::string& reftimestr);
  std::vector<std::string> getNames()
  {
    return posName;
  }
  std::vector<float> getLatitudes()
  {
    return posLatitude;
  }
  std::vector<float> getLongitudes()
  {
    return posLongitude;
  }
  std::vector<miutil::miTime> getTimes()
  {
    return validTime;
  }

  SpectrumPlot* getData(const std::string& name, const miutil::miTime& time);

private:

  std::string modelName;

  std::string modelName2; // from file
  int numPos;
  int numTime;
  int numDirec;
  int numFreq;

  std::vector<float> directions;
  std::vector<float> frequences;
  std::vector<float> extraScale;

  std::vector<std::string> posName;
  std::vector<float> posLatitude;
  std::vector<float> posLongitude;
  std::vector<miutil::miTime> validTime;
  std::vector<int> forecastHour;
  vcross::Source_p fs;
  vcross::Collector_p collector;
   vcross::Time reftime;
};

#endif
