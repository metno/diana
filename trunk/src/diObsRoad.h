/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diObsAscii.h 1 2007-09-12 08:06:42Z lisbethb $

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
#ifndef diObsRoad_h
#define diObsRoad_h
// if not defined ROADOBS, just an empty include file
//#define ROADOBS 1
#ifdef ROADOBS
//#include <diObsPlot.h>
//#include <diStation.h>
#include <puTools/miString.h>
#include <puTools/miTime.h>

class ObsPlot;

/*using namespace std;
using namespace miutil;
using namespace road;*/

/**

  \brief Reading road observation data
  In-house smhi.se format

*/
class ObsRoad {
private:
  miutil::miString filename_;
  miutil::miString databasefile_;
  miutil::miString stationfile_;
  miutil::miString headerfile_;
  miutil::miTime filetime_;
  void readFile(const miutil::miString &filename, const miutil::miString &headerfile,
		const miutil::miTime &filetime, ObsPlot *oplot, bool readData);
  void readHeader(ObsPlot *oplot);
  void readRoadData(ObsPlot *oplot);
  void initRoadData(ObsPlot *oplot);


public:
  ObsRoad(const miutil::miString &filename, const miutil::miString &databasefile,
	  const miutil::miString &stationfile, const miutil::miString &headerfile,
	  const miutil::miTime &filetime, ObsPlot *oplot, bool breadData);
  void readData(ObsPlot *oplot);
  void initData(ObsPlot *oplot);
};
#endif
#endif




