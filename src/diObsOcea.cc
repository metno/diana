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

#include <diObsOcea.h>

using namespace std; using namespace miutil; 

ObsOcea::ObsOcea(const miString &file)
:ocea(file){
}

void ObsOcea::init(ObsPlot *oplot){

  //It is assumed that each station has only one entry in the ocea
  //file.

  int numStations= contents.size();

   for(int i=0; i<numStations; i++) {
     int levelIndex = getIndex(oplot->getLevel(),
			       contents[i].data.zz1,oplot->getLevelDiff()); 
     if(levelIndex > -1){
       miTime obsTime(contents[i].desc.aar,contents[i].desc.mnd,
		     contents[i].desc.dag,contents[i].desc.kl,
		     contents[i].desc.min,0);
       if(oplot->timeOK(obsTime)){
	 ObsData &d = oplot->getNextObs();
        putData(i,levelIndex,obsTime,d);
       }
     }
   }
   
}

int ObsOcea::getIndex(int depth, const vector<int16> & z, int diff)
{
  int zsize = z.size();

  if(zsize == 0) return -1;

  int i=0;
  while(i<zsize && z[i]<depth) i++;

  if(i==zsize){
    int d2=depth-z[i-1];
    if(d2<=diff)
      return (i-1);
    return -1;
  }

  int d1=z[i]-depth;
  if(i==0){
    if(d1<=diff)
      return i;
    return -1;
  }

  int d2=depth-z[i-1];
  if(d1<d2){
    if(d1<=diff)
      return i;
    return -1;
  }

  if(d2<=diff)
    return (i-1);
  return -1;

}


void ObsOcea::putData(int i, int levelIndex,miTime obsTime,ObsData &d){

  d.dataType="ocean";
  //Description
  d.id = contents[i].desc.kjennetegn;
  d.id.trim();
  miString tegn = contents[i].data.tegn;
  d.id += tegn;
  d.id.trim();
  d.xpos =  contents[i].desc.lengde/100.;
  d.ypos = contents[i].desc.bredde/100.;
  d.zone = _IDtoZone(contents[i].stationID);
  d.obsTime = obsTime;//contents[i].desccontents[i].desc.obsTime;
  
  //Data
  if(contents[i].data.PwaPwa != undef)
    d.fdata["PwaPwa"] = contents[i].data.PwaPwa;  // Periode (s)
  if(contents[i].data.HwaHwa != undef)
    d.fdata["HwaHwa"] = contents[i].data.HwaHwa;  // Høyde (m)
  int NT =contents[i].data.NT;
  int NC =contents[i].data.NC;
  if(levelIndex<NT){ 
    if(contents[i].data.zz1[levelIndex] != undef)
      d.fdata["Dyp(1)"]  = (float)contents[i].data.zz1[levelIndex];   // dyp (m)
    if(contents[i].data.TTTT[levelIndex] != undef)
      d.fdata["TTTT"] = contents[i].data.TTTT[levelIndex];  // sea temp (degrees) 
    if(contents[i].data.SSSS[levelIndex] != undef)
      d.fdata["SSSS"] = contents[i].data.SSSS[levelIndex];  // salt (promille)
  }
  if(levelIndex<NC){
    if(contents[i].data.zz2[levelIndex] != undef)
      d.fdata["Dyp(2)"]    = (float)contents[i].data.zz2[levelIndex]; //dyp (m)
    if(contents[i].data.dd[levelIndex] != undef)
      d.fdata["Retn"]    = contents[i].data.ccc[levelIndex];//direction (degrees)
    if(contents[i].data.ccc[levelIndex] != undef)
      d.fdata["Strøm"] = contents[i].data.dd[levelIndex];  //strøm (m/s)
  }
  
}

#endif
 


