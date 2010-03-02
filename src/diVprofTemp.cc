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

#include <iostream>
#include <diVprofTemp.h>
#include <diVprofPlot.h>
#include <robs/geopos.h>
#include <math.h>

using namespace::miutil;

// Default constructor
VprofTemp::VprofTemp()
{
}


// land or ship station with name
VprofTemp::VprofTemp(const miString& file, bool amdar,
		     const vector<miString>& stationList)
  : temp(file,stationList), amdartemp(amdar)
{
}


// ship station without name
VprofTemp::VprofTemp(const miString& file, bool amdar,
		     float latitude, float longitude,
		     float deltalat, float deltalong)
  : temp(file,geopos(latitude-deltalat,longitude-deltalong),
      geopos(latitude+deltalat,longitude+deltalong)),
      amdartemp(amdar)
    // unfortunately this request cannot be limited to the ship zone (99)
{
}


// Destructor
VprofTemp::~VprofTemp()
{
}


miTime VprofTemp::getFileObsTime(){
  return fileObsTime();
}


VprofPlot* VprofTemp::getStation(const miString& station,
				 const miTime& time)
{
  const float rad=3.141592654/180.;

  // temp.numStations is all stations, contents may have less !!!
  int nStations= contents.size();

  VprofPlot *vp= 0;

  if (nStations<1 || fileObsTime()!=time) return vp;

  int n= 0;
  if (station != "99") {
    while (n<nStations && contents[n].stationID!=station) n++;
    if (n==nStations && station.length()==4) {
      //amdar temp..
      n= 0;
      while (n<nStations && (contents[n].stationID.length()<5 ||
      			     contents[n].stationID.substr(0,4)!=station)) n++;
    }
    if (n==nStations) return vp;
  }
  // for ship without name we take the first (until time & need)...

  vp= new VprofPlot();

  vp->text.index= -1;
  if (amdartemp)
    vp->text.modelName= "AMDAR";
  else
    vp->text.modelName= "TEMP";
//vp->text.posName= station;
//####  if (contents[n].stationID.substr(0,2)=="99")
//####    vp->text.posName=
//####	contents[n].stationID.substr(2,contents[n].stationID.length()-2);
//####  else
//####    vp->text.posName= contents[n].stationID;
  vp->text.posName= contents[n].desc.kjennetegn;
  vp->text.posName.trim();
  vp->text.prognostic= false;
  vp->text.forecastHour= 0;
  vp->text.validTime= miTime(contents[n].desc.aar,contents[n].desc.mnd,
	                     contents[n].desc.dag,contents[n].desc.kl,
                             contents[n].desc.min,0);
  vp->text.latitude=  float(contents[n].desc.bredde)*0.01;
  vp->text.longitude= float(contents[n].desc.lengde)*0.01;
  vp->text.kindexFound= false;

  int nLevel= contents[n].data.PPPP.size();

  float p,tt,td,fff,ddd;
  int   dd,ff,bpart;
  int   ffmax= -1, kmax= -1;

//####################################################################
//  if (station=="03005" || station=="03953") {
//    cerr<<"-------------------------------------------------"<<endl;
//    cerr<<station<<"   "<<time<<endl;
//    for (int k=0; k<nLevel; k++) {
//      cerr<<"  p="<<contents[n].data.PPPP[k]
//          <<"  dd="<<contents[n].data.dd[k]
//          <<"  ff="<<contents[n].data.ff[k]
//          <<"  t="<<contents[n].data.TTT[k]
//          <<"  td="<<contents[n].data.TdTdTd[k]
//          <<"  flags1="<<contents[n].data.flags1[k]<<endl;
//    }
//    cerr<<"-------------------------------------------------"<<endl;
//  }
//####################################################################

  for (int k=0; k<nLevel; k++) {
    p= contents[n].data.PPPP[k];
    if (p>0. && p<1300.) {
      tt= contents[n].data.TTT[k];
      td= contents[n].data.TdTdTd[k];
      if (tt>-30000.) {
        vp->ptt.push_back(p);
        vp->tt.push_back(tt);
	if (td>-30000.) {
          vp->ptd.push_back(p);
          vp->td.push_back(td);
          vp->pcom.push_back(p);
          vp->tcom.push_back(tt);
          vp->tdcom.push_back(td);
	}
      }
      dd= int(contents[n].data.dd[k]);
      ff= int(contents[n].data.ff[k]);
      if (dd>=0 && dd<=360 && ff>=0) {
        vp->puv.push_back(p);
        vp->dd.push_back(dd);
        vp->ff.push_back(ff);
	// convert to east/west and north/south component
	fff= float(ff);
	ddd= (float(dd)+90.)*rad;
        vp->uu.push_back( fff*cosf(ddd));
        vp->vv.push_back(-fff*sinf(ddd));
	// check B-part flag (significant level)
	bpart= (contents[n].data.flags1[k] >> 14) & 1;
	vp->sigwind.push_back(bpart);
        if (ff>ffmax) {
	  ffmax= ff;
	  kmax= vp->sigwind.size() - 1;
	}
      }
    }
  }

  if (kmax>=0) vp->sigwind[kmax]= 3;

  vp->prognostic= false;
  int l1= vp->ptt.size();
  int l2= vp->puv.size();
  vp->maxLevels= (l1>l2) ? l1 : l2;

  return vp;
}

#endif
