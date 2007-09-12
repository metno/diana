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

#include <diObsDribu.h>

using namespace std; 

ObsDribu::ObsDribu(const miString &file)
:dribu(file){
}


void ObsDribu::init(ObsPlot *oplot){

  int numStations= contents.size();

  for(int i=0; i<numStations; i++) 
    if(oplot->timeOK(contents[i].desc.obsTime)){
      ObsData &d = oplot->getNextObs();
      putData(i,d);
    }

}


void ObsDribu::putData(int i, ObsData &d){


  d.dataType="dribu";
  //Description
  // Only the 5 last characters of kjennetegn should be used
  // The last character is usually blank, this should be '0' 
  d.id = contents[i].desc.kjennetegn;
  d.xpos =  contents[i].desc.pos.longitude();
  d.ypos = contents[i].desc.pos.latitude();
  d.fdata["stationHeight"] = (float)contents[i].desc.hoeyde;
  d.zone = _IDtoZone(contents[i].stationID);
  d.obsTime = contents[i].desc.obsTime;


  //Data
  if(contents[i].data.dd != undef)
    d.fdata["dd"]    = (float)contents[i].data.dd;	// grader
  if(contents[i].data.ff != undef)
    d.fdata["ff"]    = knots2ms(contents[i].data.ff);	// knop
  if(contents[i].data.TTT != undef)
    d.fdata["TTT"]   = contents[i].data.TTT;	// Celsius
  if(contents[i].data.TdTdTd != undef)
    d.fdata["TdTdTd"]= contents[i].data.TdTdTd;	// Celsius
  if(contents[i].data.PPPP != undef)
    d.fdata["PPPP"]  = contents[i].data.PPPP;	// mb
  if(contents[i].data.HHH != undef)
    d.fdata["HHH"]   = (float)contents[i].data.HHH;       // Height(1000hPa)
  if(contents[i].data.a >= 0 && contents[i].data.a < 10)
    d.fdata["a"]     = (float)contents[i].data.a;		// Trykktendens
  if(contents[i].data.ppp != undef){
   d.fdata["ppp"]   = contents[i].data.ppp;	// Tendens
     if( d.fdata.count("a") && d.fdata["a"] > 4 )
    d.fdata["ppp"] *= -1;
  }
  if(contents[i].data.TwTwTw != undef)
    d.fdata["TwTwTw"]= contents[i].data.TwTwTw;   	// Sjøtemperatur
  if(d.id.isNumber() )
    d.fdata["Id"]= d.id.toInt();   	// Sjøtemperatur
}

#endif
