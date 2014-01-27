/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diVprofRTemp.h 1 2007-09-12 08:06:42Z lisbethb $

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
#ifndef diVprofRTemp_h
#define diVprofRTemp_h

//#include <robs/geopos.h>
#include <puTools/miTime.h>
#include <diVprofPlot.h>
#include <string>
#include <vector>

/**
  \brief Reading sounding from met.no obs files (temp)
   using the robs library
*/
class VprofRTemp {
public:
  // Constructors
  VprofRTemp();
  VprofRTemp(const std::string& file, bool amdar,
	     const std::vector<std::string>& stationList,
	     const std::string & stationfile,
	     const std::string & databasefile, const miutil::miTime& time);
  VprofRTemp(const std::string& file, bool amdar,
	     float latitude, float longitude,
	     float deltalat, float deltalong,
	     const std::string & stationfile,
	     const std::string & databasefile, const miutil::miTime& time);
  // Destructor
  ~VprofRTemp();

  miutil::miTime getFileObsTime();

  VprofPlot* getStation(const std::string& station,
			const miutil::miTime& time);
  int float2int(float f) {return (int)(f > 0.0 ? f + 0.5 : f - 0.5);}
  int ms2knots(float ff) {return (float2int(ff*3600.0/1852.0));}

private:
  bool amdartemp;
  std::vector<std::string> stationList_;
  std::string parameterfile_;
  std::string stationfile_;
  std::string databasefile_;
  //geopos geoposll;
  //geopos geoposur;
  miutil::miTime time_;
};

#endif
