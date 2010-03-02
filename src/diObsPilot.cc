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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef METNOOBS

#include <diObsPilot.h>

using namespace::miutil;

ObsPilot::ObsPilot(const miString &file)
:pilot(file){
}

void ObsPilot::init(ObsPlot *oplot, vector<int> &levels){

  //It is assumed that each station has only one entry in the pilot
  //file.

  int level = oplot->getLevel();

  //Find <min,max> level
  float pmin=0,pmax=2000;
  int n= levels.size();

  if(n>0){

    int j=0;
    while (j<n && level != levels[j]) j++;
    if (j==n){   //level is not in the list
      j=0;
      while (j<n && level > levels[j]) j++;
      if( j > 0 )
	pmin = levels[j-1];
      if( j < n )
	pmax = levels[j];
    } else {    //level is in the list
      if( j >0 )
	pmin = (levels[j-1] + levels[j])/2. - 0.1;
      if( j < n-1 )
	pmax = (levels[j] + levels[j+1])/2. + 0.1;

    }
  }

  // temp.numStations is all stations, contents may have less !!!
  int numStations= contents.size();

  //NBNBNBNBNBNBNBNBN
  //Observatins where abs(time of file - time of obs) < minTimeDiff
  //are used even if abs(time  - time of obs) < timeDiff

  miTime termin = fileObsTime();
  int minTimeDiff = 75;

  for(int i=0; i<numStations; i++) {
    int levelIndex = getIndex(level,pmin,pmax,contents[i].data.PPPP);
    if(levelIndex>-1){
      miTime obsTime(contents[i].desc.aar,contents[i].desc.mnd,
		     contents[i].desc.dag,contents[i].desc.kl,
		     contents[i].desc.min,0);
      miTime tmpTime=obsTime;
      if(abs(miTime::minDiff(obsTime,termin))<minTimeDiff)
	tmpTime=termin;
      if(oplot->timeOK(tmpTime)){
	ObsData &d = oplot->getNextObs();
	putData(i,levelIndex,obsTime,d);
      }
    }
  }

}

int ObsPilot::getIndex(int& level,
		      float& pmin,
		      float& pmax,
		      vector<float> & PPPP)
{

  int psize = PPPP.size();
  int j = 0;
  while(j<psize  && PPPP[j]>level) j++;

  // level found
  if(j<psize && PPPP[j] == level) return j;

  // find best level
  bool p1=false,p2=false;
  if(j< psize && PPPP[j]>pmin)
    p1=true;
  if(j>0 && PPPP[j-1]<pmax)
    p2=true;
  if(p1 && p2)
    return (level-PPPP[j]<PPPP[j-1]-level ? j:j-1);
  if(p1) return j;
  if(p2) return j-1;

  //no level found
  return -1;
}


void
ObsPilot::putData(int i, int levelIndex,miTime obsTime, ObsData &d){

  d.dataType="pilot";
  //Description
  d.id = contents[i].desc.kjennetegn;
  d.id.trim();
  d.xpos =  contents[i].desc.lengde/100.;
  d.ypos = contents[i].desc.bredde/100.;
  d.zone = _IDtoZone(contents[i].stationID);
  d.obsTime = obsTime;//contents[i].desccontents[i].desc.obsTime;

    //Data
  if(contents[i].data.dd[levelIndex] != undef)
    d.fdata["dd"]    = (float)contents[i].data.dd[levelIndex];		// grader
  if(contents[i].data.ff[levelIndex] != undef)
    d.fdata["ff"]    = knots2ms(contents[i].data.ff[levelIndex]);		// knop
  if(contents[i].data.TTT[levelIndex] != undef)
    d.fdata["TTT"]   = contents[i].data.TTT[levelIndex];	// Celsius
  if(contents[i].data.TdTdTd[levelIndex] != undef)
    d.fdata["TdTdTd"]= contents[i].data.TdTdTd[levelIndex];	// Celsius
  if(contents[i].data.PPPP[levelIndex] != undef)
    d.fdata["PPPP"]  = contents[i].data.PPPP[levelIndex];		// mb
  if(contents[i].data.HHH[levelIndex] != undef)
    d.fdata["HHH"]   = (float)contents[i].data.HHH[levelIndex];	// m

}

#endif


