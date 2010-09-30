/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diObsRoad.cc 1266 2009-03-24 14:09:34Z yngve.einarsson@smhi.se $

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
//#ifndef ROADOBS
//#define ROADOBS 1
//#endif
//#ifdef ROADOBS

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>
#include <iostream>
#include <diObsRoad.h>
#include <diObsPlot.h>
// from kvroadapi
#ifdef NEWARK_INC
#include <newarkAPI/diParam.h>
#include <newarkAPI/diStation.h>
#include <newarkAPI/diRoaddata.h>
#else
#include <roadAPI/diParam.h>
#include <roadAPI/diStation.h>
#include <roadAPI/diRoaddata.h>
#endif
#include <vector>

//#define DEBUGPRINT 1

using namespace road;
using namespace miutil;
using namespace std;

ObsRoad::ObsRoad(const miString &filename, const miString &databasefile, const miString &stationfile, const miString &headerfile,
		   const miTime &filetime, ObsPlot *oplot, bool breadData)
{
#ifdef DEBUGPRINT
	cerr << "++ ObsRoad::ObsRoad() ++" << endl;
#endif
  filename_ = filename;
  databasefile_ = databasefile;
  stationfile_ = stationfile;
  headerfile_ = headerfile;
  filetime_ = filetime;
	if (!breadData)
		readHeader(oplot);
	else
		readData(oplot);
}

void ObsRoad::readHeader(ObsPlot *oplot)
{
#ifdef DEBUGPRINT
  cerr << "++ ObsRoad::readHeader( headerfile: " << headerfile_ << " ) ++" << endl;
#endif
  int n,i;
  vector<miString> vstr,pstr;
  miString str;
  if (!oplot->roadobsHeader) {
	int theresult = diParam::initParameters(headerfile_);
    if (theresult)
	{
		// take a local copy
		vector<diParam> * params = NULL;
		map<miString, vector<diParam> * >::iterator itp = diParam::params_map.find(headerfile_);
		if (itp != diParam::params_map.end())
		{
			params = itp->second;
		}
		/* this should not happen if configured properly */
		if (params == NULL)
		{
			oplot->roadobsHeader = false;
			cerr << " ObsRoad::readHeader() error, parameterfile: " << headerfile_ << endl;
#ifdef DEBUGPRINT
  cerr << "++ ObsRoad::readHeader() done, error finding parameters ++" << endl;
#endif
		}

		// fill the oplot variables
		oplot->roadobsOK=   false;
		oplot->roadobsColumn.clear();
		oplot->roadobsSkipDataLines= 0;
 
		oplot->roadobsp.clear();
		oplot->roadobsColumnName.clear();
		oplot->roadobsColumnTooltip.clear();
		oplot->roadobsColumnType.clear();
		oplot->roadobsColumnHide.clear();
		oplot->roadobsColumnUndefined.clear();
		oplot->roadobsLengthMax.clear();
		bool useLabel = false;
		// EXAMPLE St.no(5):s Date:d     Time:t   Lat:lat Lon:lon  dd:dd   ff:ff   TTT:r   TdTdTd:r  PPPP:r    ppp:r    a:r   h:r   VV:r     N:r  RRR:r  ww:r    W1:r   W2:r   Nh:r    Cl:r   Cm:r   Ch:r   TxTxTx:r   TnTnTn:r   sss:r  911ff:ff
		// set the first entries
		// the station id
		oplot->roadobsColumnName.push_back("St.no(5)");
	    oplot->roadobsColumnType.push_back("s");
		oplot->roadobsColumnTooltip.push_back("");
		// date
		oplot->roadobsColumnName.push_back("Date");
	    oplot->roadobsColumnType.push_back("d");
		oplot->roadobsColumnTooltip.push_back("");
		// time
		oplot->roadobsColumnName.push_back("Time");
	    oplot->roadobsColumnType.push_back("t");
		oplot->roadobsColumnTooltip.push_back("");
		// lat
		oplot->roadobsColumnName.push_back("Lat");
	    oplot->roadobsColumnType.push_back("lat");
		oplot->roadobsColumnTooltip.push_back("");
		// long
		oplot->roadobsColumnName.push_back("Lon");
	    oplot->roadobsColumnType.push_back("lon");
		oplot->roadobsColumnTooltip.push_back("");
		// the data valu parameters
		for (i = 0; i < params->size(); i++)
		{
			miString name = (*params)[i].diananame();
			name.trim();
			oplot->roadobsColumnName.push_back(name);
			oplot->roadobsColumnType.push_back("r");
			oplot->roadobsColumnTooltip.push_back((*params)[i].unit());
		}
		oplot->roadobsColumnUndefined.push_back("-32767.0");
		// check consistency
		n= oplot->roadobsColumnType.size();
//####################################################################
//  cerr<<"     coloumns= "<<n<<endl;
//####################################################################

		oplot->roadobsKnots=false;
		for (i=0; i<n; i++) {
//####################################################################
//cerr<<"   column "<<i<<" : "<<oplot->roadobsColumnName[i]<<"  "
//		            <<oplot->roadobsColumnType[i]<<endl;
//####################################################################
		if      (oplot->roadobsColumnType[i]=="d")
			oplot->roadobsColumn["date"]= i;
		else if (oplot->roadobsColumnType[i]=="t")
			oplot->roadobsColumn["time"]= i;
		else if (oplot->roadobsColumnType[i]=="year")
			oplot->roadobsColumn["year"] = i;
		else if (oplot->roadobsColumnType[i]=="month")
			oplot->roadobsColumn["month"] = i;
		else if (oplot->roadobsColumnType[i]=="day")
			oplot->roadobsColumn["day"] = i;
		else if (oplot->roadobsColumnType[i]=="hour")
			oplot->roadobsColumn["hour"] = i;
		else if (oplot->roadobsColumnType[i]=="min")
			oplot->roadobsColumn["min"] = i;
		else if (oplot->roadobsColumnType[i]=="sec")
			oplot->roadobsColumn["sec"] = i;
		else if (oplot->roadobsColumnType[i].downcase()=="lon")
			oplot->roadobsColumn["x"]= i;
		else if (oplot->roadobsColumnType[i].downcase()=="lat")
			oplot->roadobsColumn["y"]= i;
		else if (oplot->roadobsColumnType[i].downcase()=="dd")
			oplot->roadobsColumn["dd"]= i;
		else if (oplot->roadobsColumnType[i].downcase()=="ff")    //Wind speed in m/s
			oplot->roadobsColumn["ff"]= i;
		else if (oplot->roadobsColumnType[i].downcase()=="ffk")   //Wind speed in knots
			oplot->roadobsColumn["ff"]= i;
		else if (oplot->roadobsColumnType[i].downcase()=="image")
			oplot->roadobsColumn["image"]= i;
		else if (oplot->roadobsColumnName[i].downcase()=="lon" &&  //Obsolete
				oplot->roadobsColumnType[i]=="r")                
			oplot->roadobsColumn["x"]= i;                           
		else if (oplot->roadobsColumnName[i].downcase()=="lat" &&  //Obsolete
				oplot->roadobsColumnType[i]=="r")                
			oplot->roadobsColumn["y"]= i;                           
		else if (oplot->roadobsColumnName[i].downcase()=="dd" &&   //Obsolete
				oplot->roadobsColumnType[i]=="r")                 
			oplot->roadobsColumn["dd"]= i;
		else if (oplot->roadobsColumnName[i].downcase()=="ff" &&    //Obsolete
				oplot->roadobsColumnType[i]=="r")
			oplot->roadobsColumn["ff"]= i;
		else if (oplot->roadobsColumnName[i].downcase()=="ffk" &&    //Obsolete
				oplot->roadobsColumnType[i]=="r")
			oplot->roadobsColumn["ff"]= i;
		else if (oplot->roadobsColumnName[i].downcase()=="image" && //Obsolete
				oplot->roadobsColumnType[i]=="s")
			oplot->roadobsColumn["image"]= i;

		if (oplot->roadobsColumnType[i].downcase()=="ffk" ||
			oplot->roadobsColumnName[i].downcase()=="ffk") 
			oplot->roadobsKnots=true;  

		}

		for (i=0; i<n; i++)
			oplot->roadobsLengthMax.push_back(0);
		oplot->roadobsOK= true;
		if(!useLabel) //if there are labels, the header must be read each time
			oplot->roadobsHeader = true;
	}
  }
#ifdef DEBUGPRINT
  cerr << "++ ObsRoad::readHeader()  done ++" << endl;
#endif
}

void ObsRoad::initData(ObsPlot *oplot)
{
#ifdef DEBUGPRINT
	cerr << "++ ObsRoad::initData( filename= " << filename_ << " databasefile= " << databasefile_ << " stationfile= " << stationfile_ << " headerfile= " << headerfile_ << " filetime= " << filetime_.isoTime() << " )++ " << endl;
#endif
	// read the headerfile if needed
	if(!oplot->roadobsHeader)
	{
		readHeader(oplot);
	}
	
	initRoadData(oplot);
#ifdef DEBUGPRINT
	cerr << "++ ObsRoad::initData()done ++ " << endl;
#endif
}

void ObsRoad::initRoadData(ObsPlot *oplot)
{
#ifdef DEBUGPRINT
	cerr << "++ ObsRoad::initRoadData( filename= " << filename_ << " databasefile= " << databasefile_ << " stationfile= " << stationfile_ << " headerfile= " << headerfile_ << " filetime= " << filetime_.isoTime() << " )++ " << endl;
#endif

  vector<miString> vstr,pstr;
// HERE COMES THE TRICKY PART!
// First, we must init the oplot...  
  // init the metadata in oplot
  miTime tplot= oplot->getObsTime();
  int    tdiff= oplot->getTimeDiff() + 1;

  int n= oplot->roadobsColumnType.size();

  int nu= oplot->roadobsColumnUndefined.size();

  bool useTime (oplot->roadobsColumn.count("time") || 
		oplot->roadobsColumn.count("hour"));
  bool isoTime (oplot->roadobsColumn.count("time"));
  bool allTime (useTime && 
		(oplot->roadobsColumn.count("date")  || 
		 (oplot->roadobsColumn.count("year") && 
		  oplot->roadobsColumn.count("month") && 
		  oplot->roadobsColumn.count("day"))));
  bool isoDate (allTime && oplot->roadobsColumn.count("date"));

  bool first=true, addstr=false, cutstr=false;
  miString taddstr, tstr, timestr;
  miTime obstime;

  miDate filedate= filetime_.date();

  oplot->filename=filename_;
  oplot->databasefile=databasefile_;
  oplot->stationfile=stationfile_;
  oplot->headerfile=headerfile_;
  oplot->filetime=filetime_;
  
   

  Roaddata road = Roaddata(databasefile_, stationfile_, headerfile_, filetime_); 
  map<miString, vector<diStation> * >::iterator its = diStation::station_map.find(stationfile_);
  if (its != diStation::station_map.end())
  {
	  oplot->stationlist = its->second;
  }
  //vector<miString> lines;
  map<int, miString> lines;
  road.initData(oplot->roadobsColumnName, lines);

  int nskip= oplot->roadobsSkipDataLines;
  int nline= 0;

  miString str;
  int stnid;
  int i;
  map<int, miString>::iterator it=lines.begin();	
  for(;it!=lines.end(); it++) {
	stnid = it->first;
	str = it->second;
	str.trim();
    nline++;
    if (nline>nskip && str.exists() && str[0]!='#') {
      pstr= str.split('"','"');
      if (pstr.size()>=n) {
	if (nu>0) {
	  for (i=0; i<n; i++) {
	    int iu= 0;
            while (iu<nu && pstr[i]!=oplot->roadobsColumnUndefined[iu]) iu++;
	    if (iu<nu) pstr[i]="X";
	  }
	}

	if (useTime) {
	  if(isoTime) {
	    tstr= pstr[oplot->roadobsColumn["time"]];
	    if (first) {
	      // allowed time formats: HH HH:MM HH:MM:SS HH:MM:SSxxx...
	      vector<miString> tv= tstr.split(':');
	      if (tv.size()==1) {
		addstr= true;
		taddstr= ":00:00";
	      } else if (tv.size()==2) {
		addstr= true;
		taddstr= ":00";
	      } else if (tv.size()>=3 && tstr.length()>8) {
		cutstr= true;
	      }
	      first= false;
	    }
	    if (addstr)
	      tstr+=taddstr;
	    else if (cutstr)
	      tstr= tstr.substr(0,8);
	  } else {
	    tstr = pstr[oplot->roadobsColumn["hour"]] + ":";
	    if(oplot->roadobsColumn.count("min")) 
	      tstr += pstr[oplot->roadobsColumn["min"]] + ":";
	    else 
	      tstr += "00:";
	    if(oplot->roadobsColumn.count("sec")) 
	      tstr += pstr[oplot->roadobsColumn["sec"]] ;
	    else 
	      tstr += "00";
	  }

	  if (allTime) {
	    if (isoDate) 
	      timestr= pstr[oplot->roadobsColumn["date"]] +" "+ tstr;
	    else 
	      timestr= pstr[oplot->roadobsColumn["year"]] + "-" 
		+ pstr[oplot->roadobsColumn["month"]] + "-" 
		+ pstr[oplot->roadobsColumn["day"]] + " "+ tstr;
	    obstime= miTime(timestr);
	  } else {
	    miClock clock= miClock(tstr);
	    obstime= miTime(filedate,clock);
	    int mdiff= miTime::minDiff(obstime,filetime_);
	    if      (mdiff<-12*60) obstime.addHour(24);
	    else if (mdiff> 12*60) obstime.addHour(-24);
	  }
	
//#################################################################
//  if (abs(miTime::minDiff(obstime,tplot))<tdiff)
//    cerr<<obstime<<" ok"<<endl;
//  else
//    cerr<<obstime<<" not ok"<<endl;
//#################################################################
	  if (oplot->getTimeDiff() <0 
		  || abs(miTime::minDiff(obstime,tplot))<tdiff){
			  map<int, vector<miString> >::iterator itc = oplot->roadobsp.find(stnid);
			  if (itc != oplot->roadobsp.end())
			  {
				  // replace the old one
				  oplot->roadobsp.erase (oplot->roadobsp.find(stnid));
				  // insert the new one
				  oplot->roadobsp[stnid]= pstr;
			  }
			  else
			  {
				  // just insert the new one
				  oplot->roadobsp[stnid]= pstr;
			  }
			  oplot->roadobsTime.push_back(obstime);
	  }

	} else {
		map<int, vector<miString> >::iterator itc = oplot->roadobsp.find(stnid);
		if (itc != oplot->roadobsp.end())
		{
			// replace the old one
			oplot->roadobsp.erase (oplot->roadobsp.find(stnid));
			// insert the new one
			oplot->roadobsp[stnid]= pstr;
		}
		else
		{
			// just insert the new one
			oplot->roadobsp[stnid]= pstr;
		}
	}
	for (i=0; i<n; i++) {
	  if (oplot->roadobsLengthMax[i]<pstr[i].length())
	      oplot->roadobsLengthMax[i]=pstr[i].length();
	}
      }
    }
  }
//####################################################################
  /*
  cerr<<"----------- at end -----------------------------"<<endl;
  cerr<<"   oplot->roadobsp.size()= "<<oplot->roadobsp.size()<<endl;
  cerr <<"     oplot->dateRoad= "<<oplot->dateRoad<<endl;
  cerr <<"     oplot->timeRoad= "<<oplot->timeRoad<<endl;
  cerr<<"     oplot->xRoad=    "<<oplot->xRoad<<endl;
  cerr<<"     oplot->yRoad=    "<<oplot->yRoad<<endl;
  cerr<<"     oplot->ddRoad=   "<<oplot->ddRoad<<endl;
  cerr<<"     oplot->ffRoad=   "<<oplot->ffRoad<<endl;
  cerr<<"     oplot->roadobsColumnName.size()= "<<oplot->roadobsColumnName.size()<<endl;
  cerr<<"     oplot->roadobsColumnType.size()= "<<oplot->roadobsColumnType.size()<<endl;
  cerr<<"     oplot->roadobsHeader= "<<oplot->roadobsHeader<<endl;
  cerr<<"------------------------------------------------"<<endl;
  */
//####################################################################

  oplot->roadobsOK= (oplot->roadobsp.size()>0);
  
//####################################################################
  /*
  cerr<<"----------- at end -----------------------------"<<endl;
  cerr<<"   oplot->roadobsp.size()= "<<oplot->roadobsp.size()<<endl;
  cerr <<"     oplot->dateRoad= "<<oplot->dateRoad<<endl;
  cerr <<"     oplot->timeRoad= "<<oplot->timeRoad<<endl;
  cerr<<"     oplot->xRoad=    "<<oplot->xRoad<<endl;
  cerr<<"     oplot->yRoad=    "<<oplot->yRoad<<endl;
  cerr<<"     oplot->ddRoad=   "<<oplot->ddRoad<<endl;
  cerr<<"     oplot->ffRoad=   "<<oplot->ffRoad<<endl;
  cerr<<"     oplot->roadobsColumnName.size()= "<<oplot->roadobsColumnName.size()<<endl;
  cerr<<"     oplot->roadobsColumnType.size()= "<<oplot->roadobsColumnType.size()<<endl;
  cerr<<"     oplot->roadobsHeader= "<<oplot->roadobsHeader<<endl;
  cerr<<"------------------------------------------------"<<endl;
  */
//####################################################################

  oplot->roadobsOK= (oplot->roadobsp.size()>0);
  // Force setting of dummy data
  oplot->setData();
  // clear plot positions
  oplot->clearPos();
  // make a dummy plot to cuompute a list of stations to be plotted
  oplot->preparePlot();

#ifdef DEBUGPRINT
	cerr << "++ ObsRoad::initRoadData()done ++ " << endl;
#endif
}

void ObsRoad::readData(ObsPlot *oplot)
{
#ifdef DEBUGPRINT
	cerr << "++ ObsRoad::readData( filename= " << filename_ << " databasefile= " << databasefile_ << " stationfile= " << stationfile_ << " headerfile= " << headerfile_ << " filetime= " << filetime_.isoTime() << " )++ " << endl;
#endif
	// read the headerfile if needed
	if(!oplot->roadobsHeader)
	{
		readHeader(oplot);
	}

	readRoadData(oplot);
#ifdef DEBUGPRINT
	cerr << "++ ObsRoad::readData()done ++ " << endl;
#endif
}

void ObsRoad::readRoadData(ObsPlot *oplot)
{
#ifdef DEBUGPRINT
	cerr << "++ ObsRoad::readRoadData( filename= " << filename_ << " databasefile= " << databasefile_ << " stationfile= " << stationfile_ << " headerfile= " << headerfile_ << " filetime= " << filetime_.isoTime() << " )++ " << endl;
#endif

  vector<miString> vstr,pstr;
// HERE COMES THE TRICKY PART!
// First, we must init the oplot...  
  // init the metadata in oplot
  miTime tplot= oplot->getObsTime();
  int    tdiff= oplot->getTimeDiff() + 1;

  int n= oplot->roadobsColumnType.size();

  int nu= oplot->roadobsColumnUndefined.size();

  bool useTime (oplot->roadobsColumn.count("time") || 
		oplot->roadobsColumn.count("hour"));
  bool isoTime (oplot->roadobsColumn.count("time"));
  bool allTime (useTime && 
		(oplot->roadobsColumn.count("date")  || 
		 (oplot->roadobsColumn.count("year") && 
		  oplot->roadobsColumn.count("month") && 
		  oplot->roadobsColumn.count("day"))));
  bool isoDate (allTime && oplot->roadobsColumn.count("date"));

  bool first=true, addstr=false, cutstr=false;
  miString taddstr, tstr, timestr;
  miTime obstime;

  miDate filedate= filetime_.date();

  oplot->filename=filename_;
  oplot->databasefile=databasefile_;
  oplot->stationfile=stationfile_;
  oplot->headerfile=headerfile_;
  oplot->filetime=filetime_;
  
   

  Roaddata road = Roaddata(databasefile_, stationfile_, headerfile_, filetime_); 
  map<miString, vector<diStation> * >::iterator its = diStation::station_map.find(stationfile_);
  if (its != diStation::station_map.end())
  {
	  oplot->stationlist = its->second;
  }
  
  road.open();
  
  //vector<miString> lines;
  map<int, miString> lines;
  // Members in the global stationlist that is not in the stations_to_plot list are not read from road
  // This should improve performance
  road.getData(oplot->stations_to_plot, lines);

  int nskip= oplot->roadobsSkipDataLines;
  int nline= 0;

  miString str;
  int stnid;
  int i;
  map<int, miString>::iterator it=lines.begin();	
  for(;it!=lines.end(); it++) {
	stnid = it->first;
	str = it->second;
	str.trim();
	//cerr << str << endl;

    nline++;
    if (nline>nskip && str.exists() && str[0]!='#') {
      pstr= str.split('"','"');
      if (pstr.size()>=n) {
	if (nu>0) {
	  for (i=0; i<n; i++) {
	    int iu= 0;
            while (iu<nu && pstr[i]!=oplot->roadobsColumnUndefined[iu]) iu++;
	    if (iu<nu) pstr[i]="X";
	  }
	}

	if (useTime) {
	  if(isoTime) {
	    tstr= pstr[oplot->roadobsColumn["time"]];
	    if (first) {
	      // allowed time formats: HH HH:MM HH:MM:SS HH:MM:SSxxx...
	      vector<miString> tv= tstr.split(':');
	      if (tv.size()==1) {
		addstr= true;
		taddstr= ":00:00";
	      } else if (tv.size()==2) {
		addstr= true;
		taddstr= ":00";
	      } else if (tv.size()>=3 && tstr.length()>8) {
		cutstr= true;
	      }
	      first= false;
	    }
	    if (addstr)
	      tstr+=taddstr;
	    else if (cutstr)
	      tstr= tstr.substr(0,8);
	  } else {
	    tstr = pstr[oplot->roadobsColumn["hour"]] + ":";
	    if(oplot->roadobsColumn.count("min")) 
	      tstr += pstr[oplot->roadobsColumn["min"]] + ":";
	    else 
	      tstr += "00:";
	    if(oplot->roadobsColumn.count("sec")) 
	      tstr += pstr[oplot->roadobsColumn["sec"]] ;
	    else 
	      tstr += "00";
	  }

	  if (allTime) {
	    if (isoDate) 
	      timestr= pstr[oplot->roadobsColumn["date"]] +" "+ tstr;
	    else 
	      timestr= pstr[oplot->roadobsColumn["year"]] + "-" 
		+ pstr[oplot->roadobsColumn["month"]] + "-" 
		+ pstr[oplot->roadobsColumn["day"]] + " "+ tstr;
	    obstime= miTime(timestr);
	  } else {
	    miClock clock= miClock(tstr);
	    obstime= miTime(filedate,clock);
	    int mdiff= miTime::minDiff(obstime,filetime_);
	    if      (mdiff<-12*60) obstime.addHour(24);
	    else if (mdiff> 12*60) obstime.addHour(-24);
	  }
	
//#################################################################
//  if (abs(miTime::minDiff(obstime,tplot))<tdiff)
//    cerr<<obstime<<" ok"<<endl;
//  else
//    cerr<<obstime<<" not ok"<<endl;
//#################################################################
	  if (oplot->getTimeDiff() <0 
		  || abs(miTime::minDiff(obstime,tplot))<tdiff){
			  map<int, vector<miString> >::iterator itc = oplot->roadobsp.find(stnid);
			  if (itc != oplot->roadobsp.end())
			  {
				  // replace the old one
				  oplot->roadobsp.erase (oplot->roadobsp.find(stnid));
				  // insert the new one
				  oplot->roadobsp[stnid]= pstr;
			  }
			  else
			  {
				  // just insert the new one
				  oplot->roadobsp[stnid]= pstr;
			  }
			  oplot->roadobsTime.push_back(obstime);
	  }

	} else {
		map<int, vector<miString> >::iterator itc = oplot->roadobsp.find(stnid);
		if (itc != oplot->roadobsp.end())
		{
			// replace the old one
			oplot->roadobsp.erase (oplot->roadobsp.find(stnid));
			// insert the new one
			oplot->roadobsp[stnid]= pstr;
		}
		else
		{
			// just insert the new one
			oplot->roadobsp[stnid]= pstr;
		}
	}
	for (i=0; i<n; i++) {
	  if (oplot->roadobsLengthMax[i]<pstr[i].length())
	      oplot->roadobsLengthMax[i]=pstr[i].length();
	}
      }
    }
  }
//####################################################################
  /*
  cerr<<"----------- at end -----------------------------"<<endl;
  cerr<<"   oplot->roadobsp.size()= "<<oplot->roadobsp.size()<<endl;
  cerr <<"     oplot->dateRoad= "<<oplot->dateRoad<<endl;
  cerr <<"     oplot->timeRoad= "<<oplot->timeRoad<<endl;
  cerr<<"     oplot->xRoad=    "<<oplot->xRoad<<endl;
  cerr<<"     oplot->yRoad=    "<<oplot->yRoad<<endl;
  cerr<<"     oplot->ddRoad=   "<<oplot->ddRoad<<endl;
  cerr<<"     oplot->ffRoad=   "<<oplot->ffRoad<<endl;
  cerr<<"     oplot->roadobsColumnName.size()= "<<oplot->roadobsColumnName.size()<<endl;
  cerr<<"     oplot->roadobsColumnType.size()= "<<oplot->roadobsColumnType.size()<<endl;
  cerr<<"     oplot->roadobsHeader= "<<oplot->roadobsHeader<<endl;
  cerr<<"------------------------------------------------"<<endl;
  */
//####################################################################

  road.close();
  oplot->roadobsOK= (oplot->roadobsp.size()>0);

#ifdef DEBUGPRINT
	cerr << "++ ObsRoad::readRoadData()done ++ " << endl;
#endif
}

void ObsRoad::readFile(const miString &filename, const miString &headerfile,
			const miTime &filetime, ObsPlot *oplot, bool readData)
{

#ifdef DEBUGPRINT
	cerr << "++ ObsRoad::readFile( filename= " << filename << " headerfile= " << headerfile << " filetime= " << filetime.isoTime() << " )++ " << endl;
#endif
//####################################################################
//  cerr<<"ObsRoad::readFile  filename= "<<filename
//      <<"   filetime= "<<filetime<<endl;
//####################################################################
  int n,i;
  vector<miString> vstr,pstr;
  miString str;

  // open filestream
  ifstream file;
  if (headerfile.empty() || oplot->roadobsHeader) {
    file.open(filename.c_str());
    if (file.bad()) {
      cerr << "ObsRoad: " << filename << " not found" << endl;
#ifdef DEBUGPRINT
	cerr << " ++ ObsRoad::readFile() ++ " << endl;
#endif
      return;
    }
  } else if (!oplot->roadobsHeader) {
    file.open(headerfile.c_str());
    if (file.bad()) {
      cerr << "ObsRoad: " << headerfile << " not found" << endl;
#ifdef DEBUGPRINT
	cerr << " ++ ObsRoad::readFile() ++ " << endl;
#endif
      return;
    }
  }


  if (!oplot->roadobsHeader || headerfile.empty()) {
    // read header
    size_t p;

    while (getline(file,str)) {
      str.trim();
      if (str.exists()) {
        p= str.find('#');
        if (p==string::npos) {
          if (str=="[DATA]") break;  // end of header, start data
	  vstr.push_back(str);
        } else if (p>0) {
          pstr= str.split("#");
          if (pstr[0]=="[DATA]") break;  // end of header, start data
	  vstr.push_back(pstr[0]);
        }
      }
    }
  }
//####################################################################
//  cerr<<"----------- at start -----------------------------"<<endl;
//  cerr<<"   oplot->roadobsp.size()= "<<oplot->roadobsp.size()<<endl;
//  cerr<<"     oplot->dateRoad= "<<oplot->dateRoad<<endl;
//  cerr<<"     oplot->timeRoad= "<<oplot->timeRoad<<endl;
//  cerr<<"     oplot->xRoad=    "<<oplot->xRoad<<endl;
//  cerr<<"     oplot->yRoad=    "<<oplot->yRoad<<endl;
//  cerr<<"     oplot->ddRoad=   "<<oplot->ddRoad<<endl;
//  cerr<<"     oplot->ffRoad=   "<<oplot->ffRoad<<endl;
//  cerr<<"     oplot->roadobsColumnName.size()= "<<oplot->roadobsColumnName.size()<<endl;
//  cerr<<"     oplot->roadobsColumnType.size()= "<<oplot->roadobsColumnType.size()<<endl;
//  cerr<<"     oplot->roadobsHeader= "<<oplot->roadobsHeader<<endl;
//  cerr<<"--------------------------------------------------"<<endl;
//####################################################################


  if (!oplot->roadobsHeader) {

    oplot->roadobsOK=   false;
    oplot->roadobsColumn.clear();
    oplot->roadobsSkipDataLines= 0;
 
    oplot->roadobsp.clear();
    oplot->roadobsColumnName.clear();
    oplot->roadobsColumnTooltip.clear();
    oplot->roadobsColumnType.clear();
    oplot->roadobsColumnHide.clear();
    oplot->roadobsColumnUndefined.clear();
    oplot->roadobsLengthMax.clear();
    bool useLabel = false; 

    // parse header

//     const miString key_name=      "NAME";
//     const miString key_mainTime=  "MAINTIME";
//     const miString key_startTime= "STARTTIME";
//     const miString key_endTime=   "ENDTIME";
    const miString key_columns=   "COLUMNS";
//     const miString key_hide=      "HIDE";
    const miString key_undefined= "UNDEFINED";
    const miString key_skiplines= "SKIP_DATA_LINES";
    const miString key_label=     "LABEL";
  //const miString key_plot=      "PLOT";
  //const miString key_format=    "FORMAT";

    bool ok= true;
    n= vstr.size();
//####################################################################
//cerr<<"HEADER:"<<endl;
//for (int j=0; j<n; j++)
//  cerr<<vstr[j]<<endl;
//cerr<<"-----------------"<<endl;
//####################################################################
    size_t p1,p2;
    i= 0;

    while (i<n) {
      str= vstr[i];
      p1= str.find('[');
      if (p1!=string::npos) {
        p2= str.find(']');
        i++;
        while (p2==string::npos && i<n) {
	  str+=(" " + vstr[i]);
          p2= str.find(']');
	  i++;
        }
        if (p2==string::npos) {
	  ok= false;
	  break;
        }
        str= str.substr(p1+1,p2-p1-1);
        pstr= str.split('"','"'," ",true);
        int j,m= pstr.size();

        if (m>1) {
	    if (pstr[0]==key_columns) {
	    vector<miString> vs;
            for (j=1; j<m; j++) {
	      pstr[j].remove('"');
	      vs= pstr[j].split(':');
	      if (vs.size()>1) {
	        oplot->roadobsColumnName.push_back(vs[0]);
	        oplot->roadobsColumnType.push_back(vs[1].downcase());
		if (vs.size()>2) {
		  oplot->roadobsColumnTooltip.push_back(vs[2]);
		}else{
		  oplot->roadobsColumnTooltip.push_back("");
		}
	      }
            }
// 	  } else if (pstr[0]==key_hide) {
//             for (j=1; j<m; j++)
// 	      oplot->roadobsColumnHide.push_back(pstr[j]);
	  } else if (pstr[0]==key_undefined && m>1) {
	    vector<miString> vs= pstr[1].split(',');
	    int nu= vs.size();
	    // sort with longest undefined strings first
	    vector<int> len;
            for (j=0; j<nu; j++)
	      len.push_back(vs[j].length());
            for (int k=0; k<nu; k++) {
	      int lmax=0, jmax=0;
              for (j=0; j<nu; j++) {
	        if (len[j]>lmax) {
	          lmax= len[j];
		  jmax= j;
	        }
	      }
              len[jmax]= 0;
	      oplot->roadobsColumnUndefined.push_back(vs[jmax]);
            }
	  } else if (pstr[0]==key_skiplines && m>1) {
	    oplot->roadobsSkipDataLines= atoi(pstr[1].cStr());

	  } else if (pstr[0]==key_label) {
	    oplot->setLabel(str);
	    useLabel=true;
	//} else if (pstr[0]==key_plot) {

	//} else if (pstr[0]==key_format) {

          }
        }
      } else {
        i++;
      }
    }

    if (!ok) {
//####################################################################
//    cerr<<"   bad header !!!!!!!!!"<<endl;
//####################################################################
      file.close();
#ifdef DEBUGPRINT
	cerr << " ++ ObsRoad::readFile() done ++ " << endl;
	cerr<<"   bad header !!!!!!!!!"<<endl;
#endif
      return;
    }

    n= oplot->roadobsColumnType.size();
//####################################################################
//  cerr<<"     coloumns= "<<n<<endl;
//####################################################################

    oplot->roadobsKnots=false;
    for (i=0; i<n; i++) {
//####################################################################
//cerr<<"   column "<<i<<" : "<<oplot->roadobsColumnName[i]<<"  "
//		            <<oplot->roadobsColumnType[i]<<endl;
//####################################################################
      if      (oplot->roadobsColumnType[i]=="d")
        oplot->roadobsColumn["date"]= i;
      else if (oplot->roadobsColumnType[i]=="t")
        oplot->roadobsColumn["time"]= i;
      else if (oplot->roadobsColumnType[i]=="year")
        oplot->roadobsColumn["year"] = i;
      else if (oplot->roadobsColumnType[i]=="month")
        oplot->roadobsColumn["month"] = i;
      else if (oplot->roadobsColumnType[i]=="day")
         oplot->roadobsColumn["day"] = i;
      else if (oplot->roadobsColumnType[i]=="hour")
         oplot->roadobsColumn["hour"] = i;
      else if (oplot->roadobsColumnType[i]=="min")
        oplot->roadobsColumn["min"] = i;
      else if (oplot->roadobsColumnType[i]=="sec")
        oplot->roadobsColumn["sec"] = i;
      else if (oplot->roadobsColumnType[i].downcase()=="lon")
        oplot->roadobsColumn["x"]= i;
      else if (oplot->roadobsColumnType[i].downcase()=="lat")
        oplot->roadobsColumn["y"]= i;
      else if (oplot->roadobsColumnType[i].downcase()=="dd")
        oplot->roadobsColumn["dd"]= i;
      else if (oplot->roadobsColumnType[i].downcase()=="ff")    //Wind speed in m/s
        oplot->roadobsColumn["ff"]= i;
      else if (oplot->roadobsColumnType[i].downcase()=="ffk")   //Wind speed in knots
        oplot->roadobsColumn["ff"]= i;
      else if (oplot->roadobsColumnType[i].downcase()=="image")
        oplot->roadobsColumn["image"]= i;
      else if (oplot->roadobsColumnName[i].downcase()=="lon" &&  //Obsolete
	       oplot->roadobsColumnType[i]=="r")                
        oplot->roadobsColumn["x"]= i;                           
      else if (oplot->roadobsColumnName[i].downcase()=="lat" &&  //Obsolete
	       oplot->roadobsColumnType[i]=="r")                
        oplot->roadobsColumn["y"]= i;                           
      else if (oplot->roadobsColumnName[i].downcase()=="dd" &&   //Obsolete
	       oplot->roadobsColumnType[i]=="r")                 
        oplot->roadobsColumn["dd"]= i;
      else if (oplot->roadobsColumnName[i].downcase()=="ff" &&    //Obsolete
	       oplot->roadobsColumnType[i]=="r")
        oplot->roadobsColumn["ff"]= i;
      else if (oplot->roadobsColumnName[i].downcase()=="ffk" &&    //Obsolete
	       oplot->roadobsColumnType[i]=="r")
        oplot->roadobsColumn["ff"]= i;
      else if (oplot->roadobsColumnName[i].downcase()=="image" && //Obsolete
	       oplot->roadobsColumnType[i]=="s")
        oplot->roadobsColumn["image"]= i;

      if (oplot->roadobsColumnType[i].downcase()=="ffk" ||
	  oplot->roadobsColumnName[i].downcase()=="ffk") 
	oplot->roadobsKnots=true;  

    }

    for (i=0; i<n; i++)
      oplot->roadobsLengthMax.push_back(0);

    if (!oplot->roadobsColumn.count("x") || !oplot->roadobsColumn.count("y")) {
//####################################################################
//    cerr<<"   bad header, missing lat,lon !!!!!!!!!"<<endl;
//####################################################################
      file.close();
#ifdef DEBUGPRINT
	cerr << " ++ ObsRoad::readFile() done ++ " << endl;
	cerr<<"   bad header, missing lat,lon !!!!!!!!!"<<endl;
#endif
      return;
    }

    if (!readData) {
      file.close();
      oplot->roadobsOK= true;
      return;
    }

    if(!useLabel) //if there are labels, the header must be read each time
      oplot->roadobsHeader = true;

    if (headerfile.exists()) {
      file.close();
      file.open(filename.c_str());
      if (file.bad()) {
        cerr << "ObsRoad: " << filename << " not found" << endl;
#ifdef DEBUGPRINT
	cerr << " ++ ObsRoad::readFile() done ++ " << endl;
#endif
        return;
      }
    }

  }

  // read data....................................................

  miTime tplot= oplot->getObsTime();
  int    tdiff= oplot->getTimeDiff() + 1;

  n= oplot->roadobsColumnType.size();

  int nu= oplot->roadobsColumnUndefined.size();

  bool useTime (oplot->roadobsColumn.count("time") || 
		oplot->roadobsColumn.count("hour"));
  bool isoTime (oplot->roadobsColumn.count("time"));
  bool allTime (useTime && 
		(oplot->roadobsColumn.count("date")  || 
		 (oplot->roadobsColumn.count("year") && 
		  oplot->roadobsColumn.count("month") && 
		  oplot->roadobsColumn.count("day"))));
  bool isoDate (allTime && oplot->roadobsColumn.count("date"));

  bool first=true, addstr=false, cutstr=false;
  miString taddstr, tstr, timestr;
  miTime obstime;

  miDate filedate= filetime.date();

  int nskip= oplot->roadobsSkipDataLines;
  int nline= 0;

  while (getline(file,str)) {
    str.trim();
    nline++;
    if (nline>nskip && str.exists() && str[0]!='#') {
      pstr= str.split('"','"');
      if (pstr.size()>=n) {
	if (nu>0) {
	  for (i=0; i<n; i++) {
	    int iu= 0;
            while (iu<nu && pstr[i]!=oplot->roadobsColumnUndefined[iu]) iu++;
	    if (iu<nu) pstr[i]="X";
	  }
	}

	if (useTime) {
	  if(isoTime) {
	    tstr= pstr[oplot->roadobsColumn["time"]];
	    if (first) {
	      // allowed time formats: HH HH:MM HH:MM:SS HH:MM:SSxxx...
	      vector<miString> tv= tstr.split(':');
	      if (tv.size()==1) {
		addstr= true;
		taddstr= ":00:00";
	      } else if (tv.size()==2) {
		addstr= true;
		taddstr= ":00";
	      } else if (tv.size()>=3 && tstr.length()>8) {
		cutstr= true;
	      }
	      first= false;
	    }
	    if (addstr)
	      tstr+=taddstr;
	    else if (cutstr)
	      tstr= tstr.substr(0,8);
	  } else {
	    tstr = pstr[oplot->roadobsColumn["hour"]] + ":";
	    if(oplot->roadobsColumn.count("min")) 
	      tstr += pstr[oplot->roadobsColumn["min"]] + ":";
	    else 
	      tstr += "00:";
	    if(oplot->roadobsColumn.count("sec")) 
	      tstr += pstr[oplot->roadobsColumn["sec"]] ;
	    else 
	      tstr += "00";
	  }

	  if (allTime) {
	    if (isoDate) 
	      timestr= pstr[oplot->roadobsColumn["date"]] +" "+ tstr;
	    else 
	      timestr= pstr[oplot->roadobsColumn["year"]] + "-" 
		+ pstr[oplot->roadobsColumn["month"]] + "-" 
		+ pstr[oplot->roadobsColumn["day"]] + " "+ tstr;
	    obstime= miTime(timestr);
	  } else {
	    miClock clock= miClock(tstr);
	    obstime= miTime(filedate,clock);
	    int mdiff= miTime::minDiff(obstime,filetime);
	    if      (mdiff<-12*60) obstime.addHour(24);
	    else if (mdiff> 12*60) obstime.addHour(-24);
	  }
	
//#################################################################
//  if (abs(miTime::minDiff(obstime,tplot))<tdiff)
//    cerr<<obstime<<" ok"<<endl;
//  else
//    cerr<<obstime<<" not ok"<<endl;
//#################################################################
	  if (oplot->getTimeDiff() <0 
	      || abs(miTime::minDiff(obstime,tplot))<tdiff){
	    //oplot->roadobsp.push_back(pstr);
	    oplot->roadobsTime.push_back(obstime);
	  }
	  
	} else {
	  //oplot->roadobsp.push_back(pstr);
	}
	for (i=0; i<n; i++) {
	  if (oplot->roadobsLengthMax[i]<pstr[i].length())
	      oplot->roadobsLengthMax[i]=pstr[i].length();
	}
      }
    }
  }
//####################################################################
//  cerr<<"----------- at end -----------------------------"<<endl;
//  cerr<<"   oplot->roadobsp.size()= "<<oplot->roadobsp.size()<<endl;
//  cerr<<"     oplot->dateRoad= "<<oplot->dateRoad<<endl;
//  cerr<<"     oplot->timeRoad= "<<oplot->timeRoad<<endl;
//  cerr<<"     oplot->xRoad=    "<<oplot->xRoad<<endl;
//  cerr<<"     oplot->yRoad=    "<<oplot->yRoad<<endl;
//  cerr<<"     oplot->ddRoad=   "<<oplot->ddRoad<<endl;
//  cerr<<"     oplot->ffRoad=   "<<oplot->ffRoad<<endl;
//  cerr<<"     oplot->roadobsColumnName.size()= "<<oplot->roadobsColumnName.size()<<endl;
//  cerr<<"     oplot->roadobsColumnType.size()= "<<oplot->roadobsColumnType.size()<<endl;
//  cerr<<"     oplot->roadobsHeader= "<<oplot->roadobsHeader<<endl;
//  cerr<<"------------------------------------------------"<<endl;
//####################################################################

  file.close();

  oplot->roadobsOK= (oplot->roadobsp.size()>0);
#ifdef DEBUGPRINT
	cerr << " ++ ObsRoad::readFile() done ++ " << endl;
#endif
}
//#endif //ROADOBS
