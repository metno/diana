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
#define MILOGGER_CATEGORY "diana.VprofRTemp"
#include <miLogger/miLogging.h>

#include <diVprofRTemp.h>
#include <diVprofPlot.h>
//#include <robs/geopos.h>
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

VprofRTemp::VprofRTemp()
{
}


// land or ship station with name
VprofRTemp::VprofRTemp(const std::string& file, bool amdar,
		       const vector<std::string>& stationList,
		       const std::string & stationfile,
		       const std::string & databasefile,
		       const miutil::miTime& time)
  : amdartemp(amdar), parameterfile_(file), stationList_(stationList),
    stationfile_(stationfile), databasefile_(databasefile), time_(time)
{
}



// ship station without name
VprofRTemp::VprofRTemp(const std::string& file, bool amdar,
		       float latitude, float longitude,
		       float deltalat, float deltalong,
		       const std::string & stationfile,
		       const std::string & databasefile,
		       const miutil::miTime& time)
  : amdartemp(amdar),
    parameterfile_(file),
    stationfile_(stationfile),
    databasefile_(databasefile), time_(time)
    // unfortunately this request cannot be limited to the ship zone (99)
{
}


VprofRTemp::~VprofRTemp()
{
}


miutil::miTime VprofRTemp::getFileObsTime(){
  return time_;
}


VprofPlot* VprofRTemp::getStation(const std::string& station,
				 const miutil::miTime& time)
{
  // get the stationlist
  VprofPlot *vp= NULL;
  // This creates the stationlist
  // We must also init the connect string to mora db
  Roaddata::initRoaddata(databasefile_);
  diStation::initStations(stationfile_);
  // get the pointer to the actual station vector
  vector<diStation> * stations = NULL;
  map<std::string, vector<diStation> * >::iterator its = diStation::station_map.begin();
  its = diStation::station_map.find(stationfile_);
  if (its != diStation::station_map.end())
  {
	  stations = its->second;
  }
  if (stations == NULL)
  {
	  METLIBS_LOG_ERROR("Unable to find stationlist: " <<stationfile_);
	  return vp;
  }

  int nStations = stations->size();
  // Return if empty station list
  if (nStations<1 || time_!=time) return vp;
  
  int n= 0;
  while (n<nStations && (*stations)[n].stationID()!=miutil::to_int(station)) n++;
  // Return if station not found in station list
  if (n==nStations)
  {
	  METLIBS_LOG_ERROR("Unable to find station: " << station << " in stationlist!");
	  return vp;
  }


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
  miutil::trim(vp->text.posName);
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
  itd = raw_data_map.find((*stations)[n].stationID());
  if (itd != raw_data_map.end())
  {
	  raw_data = itd->second;
  }
  /* Sort the data */
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofRTemp::getStation: Lines returned from road.getData(): " << raw_data.size());
#endif
  // For now, we dont use the surface data!
  vector <diParam> * params = NULL;
  map<std::string, vector<diParam> * >::iterator itp = diParam::params_map.begin();
  itp = diParam::params_map.find(parameterfile_);
  if (itp != diParam::params_map.end())
  {
	  params = itp->second;
  }
  /* map the data and sort them */
  map < std::string, map<float, RDKCOMBINEDROW_2 > > data_map;
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
		//METLIBS_LOG_DEBUG("Parameter: " << (*params)[k].diananame());
	  }
  }
  /* data is now sorted */

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofRTemp::getStation: Data is now sorted!");
#endif

  /* Use TTT + 1 to detrmine no of levels */

  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator ittt = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator ittd = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itdd = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itff = data_map.begin();
  /* Siginifcant wind levels */
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itsig_4 = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itsig_7 = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itsig_12 = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itsig_13 = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itsig_14 = data_map.begin();

  /* the surface values */
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator ittts = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator ittds = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itdds = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itffs = data_map.begin();
  /* the ground pressure */
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itpps = data_map.begin();

  map<float, RDKCOMBINEDROW_2 >::iterator ittp;
  
  ittts = data_map.find("TTTs");
  float TTTs_value = -32767.0;
  if (ittts != data_map.end())
  { 
	  ittp = ittts->second.begin();
	  for (; ittp != ittts->second.end(); ittp++) {
		TTTs_value = ittp->second.floatvalue;
		//METLIBS_LOG_DEBUG("TTTs_value: " << TTTs_value);
      }
  }

  ittds = data_map.find("TdTdTds");
  float TdTdTds_value = -32767.0;

  if (ittds != data_map.end())
  {
	  ittp = ittds->second.begin();
	  for (; ittp != ittds->second.end(); ittp++) {
		TdTdTds_value = ittp->second.floatvalue;
		//METLIBS_LOG_DEBUG("TdTdTds_value: " << TdTdTds_value);
      }
  }

  itdds = data_map.find("dds");
  float dds_value = -32767.0;
  if (itdds != data_map.end())
  {
	  ittp = itdds->second.begin();
	  for (; ittp != itdds->second.end(); ittp++) {
		dds_value = ittp->second.floatvalue;
		//METLIBS_LOG_DEBUG("dds_value: " << dds_value);
	  }
  }

  itffs = data_map.find("ffs");
  float ffs_value = -32767.0;
  if (itffs != data_map.end())
  {
	  ittp = itffs->second.begin();
	  for (; ittp != itffs->second.end(); ittp++) {  
		ffs_value = ittp->second.floatvalue;
		//METLIBS_LOG_DEBUG("ffs_value: " << ffs_value);
	  }	
  }

  itpps = data_map.find("PPPP");
  float PPPPs_value = -32767.0;
  if (itpps != data_map.end())
  {
	  ittp = itpps->second.begin();
	  for (; ittp != itpps->second.end(); ittp++) {  
		PPPPs_value = ittp->second.floatvalue;
		//METLIBS_LOG_DEBUG("PPPPs_value: " << PPPPs_value);
	  }	
  }
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofRTemp::getStation: surface data TTTs: " << TTTs_value << " TdTdTds: " << TdTdTds_value << " dds: " << dds_value << " ffs: " << ffs_value << " PPPP: " << PPPPs_value);
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

  /* Significant vind levels */
  /* These may be or may be not in telegram */
  itsig_4 = data_map.find("sig_4");
  itsig_7 = data_map.find("sig_7");
  itsig_12 = data_map.find("sig_12");
  itsig_13 = data_map.find("sig_13");
  itsig_14 = data_map.find("sig_14");
  /* allocate temporary maps */
  map<float, RDKCOMBINEDROW_2 > sig_4;
  map<float, RDKCOMBINEDROW_2 > sig_7;
  map<float, RDKCOMBINEDROW_2 > sig_12;
  map<float, RDKCOMBINEDROW_2 > sig_13;
  map<float, RDKCOMBINEDROW_2 > sig_14;

  /* fill them with data if data present */
  if (itsig_4 != data_map.end())
	sig_4 = itsig_4->second;
  if (itsig_7 != data_map.end())
	sig_7 = itsig_7->second;
  if (itsig_12 != data_map.end())
	sig_12 = itsig_12->second;
  if (itsig_13 != data_map.end())
	sig_13 = itsig_13->second;
  if (itsig_14 != data_map.end())
	sig_14 = itsig_14->second;


  float p,tt,td,fff,ddd;
  int   dd,ff,bpart;
  int   ffmax= -1, kmax= -1;

//####################################################################
//  if (station=="03005" || station=="03953") {
//    METLIBS_LOG_DEBUG("-------------------------------------------------");
//    METLIBS_LOG_DEBUG(station<<"   "<<time);
//    for (int k=0; k<nLevel; k++) {
//      METLIBS_LOG_DEBUG("  p="<<contents[n].data.PPPP[k]
//          <<"  dd="<<contents[n].data.dd[k]
//          <<"  ff="<<contents[n].data.ff[k]
//          <<"  t="<<contents[n].data.TTT[k]
//          <<"  td="<<contents[n].data.TdTdTd[k]
//          <<"  flags1="<<contents[n].data.flags1[k]);
//    }
//    METLIBS_LOG_DEBUG("-------------------------------------------------");
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
  int siglevels = sig_4.size() + sig_7.size() + sig_12.size() + sig_13.size() + sig_14.size();
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
			  // Only plot the significant winds
			  bpart = 0;
			  // SHOULD it always be 1 ?
			  if (sig_4.count(key) != 0)
			  {
				  bpart = 1;
			  }
			  else if (sig_7.count(key) != 0)
			  {
				  bpart = 1;
			  }
			  else if (sig_12.count(key) != 0)
			  {
				  bpart = 1;
			  }
			  else if (sig_13.count(key) != 0)
			  {
				  bpart = 1;
			  }
			  else if (sig_14.count(key) != 0)
			  {
				  bpart = 1;
			  }
			  // reduce wind plots only if there are some siglevels!
			  if (siglevels > 5)
			  {
				  if (bpart > 0)
				  {
					  vp->sigwind.push_back(bpart);
					  vp->puv.push_back(p);
					  vp->dd.push_back(dd);
					  vp->ff.push_back(ff);
					  // convert to east/west and north/south component
					  fff= float(ff);
					  ddd= (float(dd)+90.)*DEG_TO_RAD;
					  vp->uu.push_back( fff*cosf(ddd));
					  vp->vv.push_back(-fff*sinf(ddd));
					  // check B-part flag (significant level)
					  /*bpart= (contents[n].data.flags1[k] >> 14) & 1;
					  vp->sigwind.push_back(bpart);
					  if (ff>ffmax) {
					  ffmax= ff;
					  kmax= vp->sigwind.size() - 1;
					  }*/

					  if (ff>ffmax) {
						  ffmax= ff;
						  kmax = vp->sigwind.size() - 1;
					  }
				  }
			  }
			  else
			  {
				  // plot all the winds!
				  vp->sigwind.push_back(bpart);
				  vp->puv.push_back(p);
				  vp->dd.push_back(dd);
				  vp->ff.push_back(ff);
				  // convert to east/west and north/south component
				  fff= float(ff);
				  ddd= (float(dd)+90.)*DEG_TO_RAD;
				  vp->uu.push_back( fff*cosf(ddd));
				  vp->vv.push_back(-fff*sinf(ddd));
				  // check B-part flag (significant level)
				  /*bpart= (contents[n].data.flags1[k] >> 14) & 1;
				  vp->sigwind.push_back(bpart);
				  if (ff>ffmax) {
				  ffmax= ff;
				  kmax= vp->sigwind.size() - 1;
				  }*/

				  if (ff>ffmax) {
					  ffmax= ff;
					  kmax = vp->sigwind.size() - 1;
				  }
			  }
		  }
	  }
  } /* End for */
  if (kmax>=0) vp->sigwind[kmax]= 3;
  vp->prognostic= false;
  int l1= vp->ptt.size();
  int l2= vp->puv.size();
  vp->maxLevels= (l1>l2) ? l1 : l2;
  return vp;
}

#endif
