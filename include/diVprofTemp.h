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
#ifndef diVprofTemp_h
#define diVprofTemp_h

#include <temp.h>
#include <miTime.h>

using namespace std;

class VprofPlot;


/**

  \brief Reading sounding from met.no obs files (temp)
  
   using the robs library 

*/
class VprofTemp : public temp {
public:
  // Constructors
  VprofTemp();
  VprofTemp(const miString& file, bool amdar,
	    const vector<miString>& stationList);
  VprofTemp(const miString& file, bool amdar,
	    float latitude, float longitude,
	    float deltalat, float deltalong);
  // Destructor
  ~VprofTemp();

  miTime getFileObsTime();

  VprofPlot* getStation(const miString& station,
			const miTime& time);
			
private:
  bool amdartemp;
};

#endif
