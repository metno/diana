/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diVprofRTemp.cc 1 2007-09-12 08:06:42Z lisbethb $

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

#ifdef ROADOBS

//#define DEBUGPRINT 1

#include <iostream>
#include <diVprofRTemp.h>
#include <diVprofPlot.h>
#include <robs/geopos.h>
#include <math.h>
#include <vector>
#include <map>
#ifdef NEWARK_INC
#include <newarkAPI/diRoaddata.h>
#include <newarkAPI/diStation.h>
#include <newarkAPI/rdkESQLTypes.h>
#else
#include <roadAPI/diRoaddata.h>
#include <roadAPI/diStation.h>
#include <rdkAPI/rdkESQLTypes.h>
#endif

using namespace std;
using namespace road;

// Default constructor
VprofRTemp::VprofRTemp()
{
}


// land or ship station with name
VprofRTemp::VprofRTemp(const miutil::miString& file, bool amdar,
		       const vector<miutil::miString>& stationList,
		       const miutil::miString & stationfile,
		       const miutil::miString & databasefile,
		       const miutil::miTime& time)
  : amdartemp(amdar), parameterfile_(file), stationList_(stationList),
    stationfile_(stationfile), databasefile_(databasefile), time_(time)
{
}



// ship station without name
VprofRTemp::VprofRTemp(const miutil::miString& file, bool amdar,
		       float latitude, float longitude,
		       float deltalat, float deltalong,
		       const miutil::miString & stationfile,
		       const miutil::miString & databasefile,
		       const miutil::miTime& time)
  : amdartemp(amdar),
    parameterfile_(file), geoposll(latitude-deltalat,longitude-deltalong),
    geoposur(latitude+deltalat,longitude+deltalong), stationfile_(stationfile),
    databasefile_(databasefile), time_(time)
    // unfortunately this request cannot be limited to the ship zone (99)
{
}


// Destructor
VprofRTemp::~VprofRTemp()
{
}


miutil::miTime VprofRTemp::getFileObsTime(){
  return time_;
}


VprofPlot* VprofRTemp::getStation(const miutil::miString& station,
				 const miutil::miTime& time)
{
  const float rad=3.141592654/180.;

  // get the stationlist
  VprofPlot *vp= NULL;
  // This creates the stationlist
  diStation::initStations(stationfile_);
  // get the pointer to the actual station vector
  vector<diStation> * stations = NULL;
  map<miutil::miString, vector<diStation> * >::iterator its = diStation::station_map.begin();
  its = diStation::station_map.find(stationfile_);
  if (its != diStation::station_map.end())
  {
	  stations = its->second;
  }
  if (stations == NULL)
  {
	  cerr<<"Unable to find stationlist: " <<stationfile_ << endl;
	  return vp;
  }

  int nStations = stations->size();
  // Return if empty station list
  if (nStations<1 || time_!=time) return vp;
  
  int n= 0;
  while (n<nStations && (*stations)[n].name()!=station) n++;
  // Return if station not found in station list
  if (n==nStations) return vp;


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
  vp->text.posName= (*stations)[n].name();
  vp->text.posName.trim();
  vp->text.prognostic= false;
  vp->text.forecastHour= 0;
  vp->text.validTime= time_;
  vp->text.latitude=  (*stations)[n].lat();
  vp->text.longitude= (*stations)[n].lon();
  vp->text.kindexFound= false;


  /* HERE we should get the data from road */

  Roaddata road = Roaddata(databasefile_, stationfile_, parameterfile_, time_);

  road.open();
  
  vector<diStation> stations_to_plot;
  /* only get data for one station */
  stations_to_plot.push_back((*stations)[n]);
  /* get the data */
  map<int, vector<RDKCOMBINEDROW_2 > > raw_data_map;
  
  road.getData(stations_to_plot, raw_data_map);

  road.close();
  vector<RDKCOMBINEDROW_2 > raw_data;
  map<int, vector<RDKCOMBINEDROW_2 > >::iterator itd = raw_data_map.begin();
  itd = raw_data_map.find((*stations)[n].wmonr());
  if (itd != raw_data_map.end())
  {
	  raw_data = itd->second;
  }
  /* Sort the data */
#ifdef DEBUGPRINT
  cerr << "VprofRTemp::getStation: Lines returned from road.getData(): " << raw_data.size() << endl;
#endif
  // For now, we dont use the surface data!
  vector <diParam> * params = NULL;
  map<miutil::miString, vector<diParam> * >::iterator itp = diParam::params_map.begin();
  itp = diParam::params_map.find(parameterfile_);
  if (itp != diParam::params_map.end())
  {
	  params = itp->second;
  }
  /* map the data and sort them */
  map < miutil::miString, map<float, RDKCOMBINEDROW_2 > > data_map;
  int no_of_data_rows = raw_data.size();
  for (int k = 0; k < params->size(); k++)
  {
	  map<float, RDKCOMBINEDROW_2 > tmpresult;

	  for (int i = 0; i < no_of_data_rows; i++)
	  {
		  if ((*params)[k].isMapped(raw_data[i]))
		  {
			  tmpresult[raw_data[i].altitudefrom] = raw_data[i];
		  }
	  }
      if (tmpresult.size() != 0)
	  {
		data_map[(*params)[k].diananame()] = tmpresult;
		//cerr << "Parameter: " << (*params)[k].diananame() << endl;
	  }
  }
  /* data is now sorted */

#ifdef DEBUGPRINT
  cerr << "VprofRTemp::getStation: Data is now sorted!" << endl;
#endif

  /* Use TTT + 1 to detrmine no of levels */

  map< miutil::miString, map<float, RDKCOMBINEDROW_2 > >::iterator ittt = data_map.begin();
  map< miutil::miString, map<float, RDKCOMBINEDROW_2 > >::iterator ittd = data_map.begin();
  map< miutil::miString, map<float, RDKCOMBINEDROW_2 > >::iterator itdd = data_map.begin();
  map< miutil::miString, map<float, RDKCOMBINEDROW_2 > >::iterator itff = data_map.begin();

  /* the surface values */
  map< miutil::miString, map<float, RDKCOMBINEDROW_2 > >::iterator ittts = data_map.begin();
  map< miutil::miString, map<float, RDKCOMBINEDROW_2 > >::iterator ittds = data_map.begin();
  map< miutil::miString, map<float, RDKCOMBINEDROW_2 > >::iterator itdds = data_map.begin();
  map< miutil::miString, map<float, RDKCOMBINEDROW_2 > >::iterator itffs = data_map.begin();
  /* the ground pressure */
  map< miutil::miString, map<float, RDKCOMBINEDROW_2 > >::iterator itpps = data_map.begin();

  map<float, RDKCOMBINEDROW_2 >::iterator ittp;
  
  ittts = data_map.find("TTTs");
  float TTTs_value = -32767.0;
  if (ittts != data_map.end())
  { 
	  ittp = ittts->second.begin();
	  for (; ittp != ittts->second.end(); ittp++) {
		TTTs_value = ittp->second.floatvalue;
		//cerr << "TTTs_value: " << TTTs_value << endl;
      }
  }

  ittds = data_map.find("TdTdTds");
  float TdTdTds_value = -32767.0;

  if (ittds != data_map.end())
  {
	  ittp = ittds->second.begin();
	  for (; ittp != ittds->second.end(); ittp++) {
		TdTdTds_value = ittp->second.floatvalue;
		//cerr << "TdTdTds_value: " << TdTdTds_value << endl;
      }
  }

  itdds = data_map.find("dds");
  float dds_value = -32767.0;
  if (itdds != data_map.end())
  {
	  ittp = itdds->second.begin();
	  for (; ittp != itdds->second.end(); ittp++) {
		dds_value = ittp->second.floatvalue;
		//cerr << "dds_value: " << dds_value << endl;
	  }
  }

  itffs = data_map.find("ffs");
  float ffs_value = -32767.0;
  if (itffs != data_map.end())
  {
	  ittp = itffs->second.begin();
	  for (; ittp != itffs->second.end(); ittp++) {  
		ffs_value = ittp->second.floatvalue;
		//cerr << "ffs_value: " << ffs_value << endl;
	  }	
  }

  itpps = data_map.find("PPPP");
  float PPPPs_value = -32767.0;
  if (itpps != data_map.end())
  {
	  ittp = itpps->second.begin();
	  for (; ittp != itpps->second.end(); ittp++) {  
		PPPPs_value = ittp->second.floatvalue;
		//cerr << "PPPPs_value: " << PPPPs_value << endl;
	  }	
  }
#ifdef DEBUGPRINT
  cerr << "VprofRTemp::getStation: surface data TTTs: " << TTTs_value << " TdTdTds: " << TdTdTds_value << " dds: " << dds_value << " ffs: " << ffs_value << " PPPP: " << PPPPs_value << endl;
#endif
  
  ittt = data_map.find("TTT");
  /* Only surface observations */

  if (ittt == data_map.end())
  {
	 vp->prognostic= false;
     vp->maxLevels= 0;
	 return vp;
  }

  ittd = data_map.find("TdTdTd");
  /* Only surface observations */

  if (ittd == data_map.end())
  {
	 vp->prognostic= false;
     vp->maxLevels= 0;
	 return vp;
  }

  itdd = data_map.find("dd");
  /* Only surface observations */

  if (itdd == data_map.end())
  {
	 vp->prognostic= false;
     vp->maxLevels= 0;
	 return vp;
  }

  itff = data_map.find("ff");
  /* Only surface observations */

  if (itff == data_map.end())
  {
	 vp->prognostic= false;
     vp->maxLevels= 0;
	 return vp;
  }

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
/* Can we trust that all parameters, has the same number of levels ? */
  ittp = ittt->second.begin();
  // Here should we sort!
  std::set <float> keys;
  /* Check if PPPP is in telegram */
  if (PPPPs_value != -32767.0)
	  keys.insert(PPPPs_value * 100.0);
  for (; ittp != ittt->second.end(); ittp++) {
	  // insert altitudefrom in the set
      keys.insert(ittp->second.altitudefrom);
  }

  // Iterate over the sorted set */
  int d = 0;
  std::set<float>::iterator it=keys.begin();
  for (; it != keys.end(); it++)
  {
	  float key = *it;
	  p= key * 0.01;
	  // check with ground level pressure !
	  if ((p > PPPPs_value) && (PPPPs_value != -32767.0))
		  continue;
	  if (p>0. && p<1300.) {
		  if (key == (PPPPs_value * 100.0))
		  {
			  tt = TTTs_value;
			  td = TdTdTds_value;
		  }
		  else
		  {
			tt= ittt->second[key].floatvalue;
			td= ittd->second[key].floatvalue;
		  }
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
		  if (key == (PPPPs_value * 100.0))
		  {
			  dd = dds_value;
			  ff = ffs_value;
		  }
		  else
		  {
			dd= int(itdd->second[key].floatvalue);
			ff= int(itff->second[key].floatvalue);
		  }
		  /* Wind should always be plotted in knots,
		     convert from m/s as they are stored in road */
                  ff = ms2knots(ff);
		  if (dd>=0 && dd<=360 && ff>=0) {
			  vp->puv.push_back(p);
			  vp->dd.push_back(dd);
			  vp->ff.push_back(ff);
			  // convert to east/west and north/south component
			  fff= float(ff);
			  ddd= (float(dd)+90.)*rad;
			  vp->uu.push_back( fff*cosf(ddd));
			  vp->vv.push_back(-fff*sinf(ddd));
			  vp->sigwind.push_back(0);
			  if (ff>ffmax) {
				  ffmax= ff;
				  kmax = d;
			  }
			  d++;
		  }
	  }
  } /* End for */

  vp->prognostic= false;
  int l1= vp->ptt.size();
  int l2= vp->puv.size();
  vp->maxLevels= (l1>l2) ? l1 : l2;
  return vp;
}

#endif
