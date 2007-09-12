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

#ifdef METNOOBS

#include <diObsAireps.h>

ObsAireps::ObsAireps(const miString &file)
:aireps(file){
}

void ObsAireps::init(ObsPlot *oplot, vector<int> &levels){

  //It is assumed that each station has only one entry in the aireps
  //file, and that fileObsTime == ObsTime

  //Find <min,max> level
  float pmin,pmax;
  int level;

  if (oplot->AllAirepsLevels()) {

    pmin=    0.0;
    pmax= 2000.0;

  } else {

    int level = oplot->getLevel();
    int n= levels.size();
    int j= 0;

    while (j<n && level != levels[j]) j++;

    if (j==n){
      j= n/2;
      cerr <<"Level: "<<level<<" is not in the list, using "
	   <<levels[j]<<" hPa"<<endl;
    }

    if( j-1 < 0 )
      pmin = 0;
    else
      pmin = (levels[j-1] + levels[j])/2. - 0.1;

    if( j == n-1 )
      pmax = 2000;
    else
      pmax = (levels[j] + levels[j+1])/2. + 0.1;

  }

  int numStations= contents.size();

  for(int i=0; i<numStations; i++) {
    if (contents[i].data.PPPP > pmin && contents[i].data.PPPP < pmax
	&& oplot->timeOK(contents[i].desc.obsTime)){
      ObsData &d = oplot->getNextObs();
      putData(i,d);
    }
  }

}


void ObsAireps::putData(int i, ObsData &d){

  d.dataType="aireps";
  //Description
  d.id = contents[i].desc.kjennetegn;
  d.id.trim();
  d.xpos =  contents[i].desc.pos.longitude();
  d.ypos = contents[i].desc.pos.latitude();
  d.zone = _IDtoZone(contents[i].stationID);
  d.obsTime = contents[i].desc.obsTime;

  //Data
  if(contents[i].data.dd != undef)
    d.fdata["dd"]    = (float)contents[i].data.dd;		// degrees
  if(contents[i].data.ff != undef)
    d.fdata["ff"]    = knots2ms(contents[i].data.ff);		// knots

  // AF: have seen some "strange" data, maybe not proper fixup...
  if (d.fdata.count("dd") && d.fdata.count("ff")) {
    if(d.fdata["dd"]==0 && d.fdata["ff"]>0) 
      d.fdata["dd"]= 360;
    if (d.fdata["ff"]==0 && d.fdata["dd"]>0 && d.fdata["dd"]<361) 
      d.fdata["ff"]= 1;
  }
  if(contents[i].data.TTT != undef)
   d.fdata["TTT"]   =  contents[i].data.TTT;	// Celsius
  if(contents[i].data.TdTdTd != undef)
   d.fdata["TdTdTd"]= contents[i].data.TdTdTd;	// Celsius
  if(contents[i].data.PPPP != undef)
   d.fdata["PPPP"]  = contents[i].data.PPPP;		// mb
  
}

#endif


