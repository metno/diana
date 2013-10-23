/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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

#include <sys/types.h>

#include <math.h>

#include <algorithm>
#include <iostream>
#include <set>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>

#ifdef METNOOBS
#include <diObsSynop.h>
#include <diObsDribu.h>
#include <diObsMetar.h>
#include <diObsTemp.h>
#include <diObsOcea.h>
#include <diObsTide.h>
#include <diObsPilot.h>
#include <diObsAireps.h>
#include <diObsSatob.h>
#endif
#ifdef ROADOBS
// includes for road specific implementation
#include <diObsRoad.h>
#endif

#include <diObsAscii.h>

#ifdef BUFROBS
#include <diObsBufr.h>
#endif

#define MILOGGER_CATEGORY "diana.ObsManager"
#include <miLogger/miLogging.h>

#include <diObsManager.h>
#include <puCtools/puCglob.h>
#include <puCtools/glob_cache.h>
#include <puTools/miSetupParser.h>

using namespace std;
using namespace miutil;

namespace /* anonymous */ {
std::vector<std::string> split_on_comma(const std::string& txt, const char* comma = ",")
{
  std::vector<std::string> s;
  boost::algorithm::split(s, txt, boost::algorithm::is_any_of(comma));
  return s;
}
} /* anonymous namespace */

// #define DEBUGPRINT 1


// Default constructor
ObsManager::ObsManager(){
  useArchive=false;
  mslp = false;
  timeListChanged = false;
  levels.push_back(10);
  levels.push_back(30);
  levels.push_back(50);
  levels.push_back(70);
  levels.push_back(100);
  levels.push_back(150);
  levels.push_back(200);
  levels.push_back(250);
  levels.push_back(300);
  levels.push_back(400);
  levels.push_back(500);
  levels.push_back(700);
  levels.push_back(850);
  levels.push_back(925);
  levels.push_back(1000);
}

bool ObsManager::init(ObsPlot *oplot,const miString& pin){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("++ ObsManager::init() ++");
#endif


  //sending PlotInfo to obsPlot
  if(!oplot->prepare(pin)){
    METLIBS_LOG_ERROR("++ Returning false from ObsManager::init() ++");
    return false;
  }

  mslp=false;

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("++ Returning from ObsManager::init() ++");
#endif

  return true;
}


bool ObsManager::prepare(ObsPlot * oplot, miTime time){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("++ ObsManager::prepare() ++");
#endif

  oplot->clear();

  mslp = mslp || oplot->mslp();

  if( oplot->flagInfo()){
    METLIBS_LOG_INFO("ObsManager::prepare HQC");
    METLIBS_LOG_INFO("hqcTime:"<<hqcTime);
    METLIBS_LOG_INFO("Time:"<<time);
    int d = abs(miTime::minDiff(time, hqcTime));
    if( oplot->getTimeDiff()>-1 && d>oplot->getTimeDiff() ) return false;
    if( sendHqcdata(oplot))
      return true;
    return false;
  }

  termin.clear();
  miString anno_str;

  //false if any asynoptic data or moreTimes,  used for annotation
  bool anno_synoptic=!oplot->moreTimes();
  timeRangeMin=time;
  timeRangeMax=time;

  oplot->clearModificationTime();

  vector<miString> &dataType = oplot->dataTypes();

  for(unsigned int i=0; i<dataType.size(); i++){

    firstTry=true; //false if times can't be found from filename

    vector<FileInfo> finfo;
    if(!Prod[dataType[i]].timeInfo.empty() || Prod[dataType[i]].obsformat == ofmt_url){
      finfo = Prod[dataType[i]].fileInfo;
    } else {
      getFileName(finfo,time,dataType[i],oplot);
    }

    //Plot current not wind if current > 0
    oplot->setCurrent( Prod[dataType[i]].current);

    // Set modification time
    int nFiles = finfo.size();
    for( int j=0; j<nFiles; j++ )
      oplot->setModificationTime(finfo[j].filename);

    oplot->setObsTime(time);

    //Annotations
    if( nFiles>0 ){  //if there are any files of this dataType
      anno_str += miutil::to_upper(dataType[i]);
      anno_str += " ";
      // this will be used in the annnotation
      if (!Prod[dataType[i]].synoptic) {
        anno_synoptic=false;
      }
    }

    ObsFormat obsformat= ofmt_unknown;
    miutil::miString headerfile;
#ifdef ROADOBS
    miString stationfile, databasefile;
#endif
    if (Prod.find(dataType[i])!=Prod.end()) {
      obsformat= Prod[dataType[i]].obsformat;
      headerfile= Prod[dataType[i]].headerfile;
#ifdef ROADOBS
      stationfile= Prod[dataType[i]].stationfile;
      databasefile= Prod[dataType[i]].databasefile;
#endif
    }
    //get get pressure level etc from field (if needed)
    oplot->updateLevel(dataType[i]);




    for(int j=0; j<nFiles; j++){
      //Add stations and time to url
      if ( !Prod[dataType[i]].metaData.empty() ) {
        if ( !addStationsAndTimeFromMetaData( Prod[dataType[i]].metaData, finfo[j].filename, time) ) {
          break;
        }
      }

      int num=oplot->numPositions();

      if(finfo[j].filetype =="bufr"){
#ifdef BUFROBS
        ObsBufr bufr;
        oplot->setDataType(dataType[i]);
        bufr.setObsPlot(oplot);
        if(!bufr.init(finfo[j].filename,"obsplot")){
          //reset oplot
          oplot->resetObs(num);
        }
#endif
      } else if(finfo[j].filetype =="ascii" || finfo[j].filetype =="url"){
        ObsAscii obsAscii =
          ObsAscii(finfo[j].filename, headerfile, Prod[dataType[i]].headerinfo,
              finfo[j].time, oplot);
      }
#ifdef ROADOBS
      else if(finfo[j].filetype =="roadobs"){
	ObsRoad obsRoad =
	  ObsRoad(finfo[j].filename,databasefile, stationfile, headerfile,finfo[j].time,oplot,false);
	// initData inits the internal data structures in oplot, eg the roadobsp and stationlist.
	obsRoad.initData(oplot);
	// readData reads the data from road.
	// it shoukle be complemented by a method on the ObsPlot objects that reads data from a single station from road,
	// after ObsPlt has computed wich stations to plot.
	obsRoad.readData(oplot);
      }
#endif
      else {
#ifdef METNOOBS
        try {
          if ( obsformat == ofmt_synop ){
            ObsSynop Synop(finfo[j].filename);
            Synop.init(oplot);
          } else if ( obsformat == ofmt_dribu ){
            ObsDribu Dribu = ObsDribu(finfo[j].filename);
            Dribu.init(oplot);
          } else if ( obsformat == ofmt_metar ){
            ObsMetar Metar(finfo[j].filename);
            Metar.init(oplot);
          } else if ( obsformat == ofmt_temp ){
            ObsTemp Temp = ObsTemp(finfo[j].filename);
            Temp.init(oplot,levels);
          } else if ( obsformat == ofmt_ocea ){
            ObsOcea Ocea = ObsOcea(finfo[j].filename);
            Ocea.init(oplot);
          } else if ( obsformat == ofmt_tide ){
            ObsTide Tide = ObsTide(finfo[j].filename);
            Tide.init(oplot);
          } else if ( obsformat == ofmt_pilot ){
            ObsPilot Pilot = ObsPilot(finfo[j].filename);
            Pilot.init(oplot,levels);
          } else if ( obsformat == ofmt_aireps ){
            ObsAireps Aireps = ObsAireps(finfo[j].filename);
            Aireps.init(oplot,levels);
          } else if ( obsformat == ofmt_satob ){
            ObsSatob Satob = ObsSatob(finfo[j].filename);
            Satob.init(oplot);
          }
        }  // end of try

        catch (...){
          METLIBS_LOG_WARN("Exception in "<<dataType[i]<<": " <<finfo[j].filename);
          //reset oplot
          oplot->resetObs(num);
          continue;
        }
#endif
      }

    }

    //Add metadata
    if ( !Prod[dataType[i]].metaData.empty() ) {
      oplot->mergeMetaData( metaDataMap[Prod[dataType[i]].metaData]->metaData );
    }
  }


  if(oplot->setData()){

    //   if (!oplot->setData())
    //     METLIBS_LOG_DEBUG("ObsPlot::setData returned false (no data)");;

    //minTime - maxTime are used in the annotation
    //minTime/maxTime is time -/+ timeDiff (from dialog) unless
    //there are only synoptic data, and timeRange < timeDiff.
    //Then timeRange is used.
    miString timeInterval;
    if(oplot->getTimeDiff()<0 && !anno_synoptic){
      timeInterval = " (alle tider)";
    }
    else {
      miTime minTime=time;
      miTime maxTime=time;
      minTime.addMin(-1*oplot->getTimeDiff());
      maxTime.addMin(oplot->getTimeDiff());
      if( anno_synoptic){
        if(timeRangeMin > minTime || oplot->getTimeDiff()<0)
          minTime = timeRangeMin;
        if(timeRangeMax < maxTime || oplot->getTimeDiff()<0)
          maxTime = timeRangeMax;
      }
      timeInterval = " (" + minTime.format("%H:%M")
      + " - " + maxTime.format("%H:%M") + " ) ";
    }

    // The vector termin contains the "file time" from each file
    // used. bestTermin is the time closest to the time asked for

    miTime bestTermin;
    int m=termin.size();
    if(m>0){
      int best = abs(miTime::minDiff(time,termin[0]));
      int bestIndex=0;
      for(unsigned int i=1; i<termin.size(); i++){
        int d = abs(miTime::minDiff(time,termin[i]));
        if( d < best){
          best=d;
          bestIndex=i;
        }
      }
      bestTermin = termin[bestIndex];
    }

    //Annotation

    if (!anno_str.empty()) {
      if (oplot->getLevel()>0){
        if(dataType.size()>0 && dataType[0] == "ocean"){
          anno_str+= (miString(oplot->getLevel()) + "m ");
        }else{
          anno_str+= (miString(oplot->getLevel()) + "hPa ");
        }
      }
      if(!bestTermin.undef()){
        miString time0=bestTermin.format("%D %H:%M");
        anno_str += time0 + timeInterval;
      }
    }

  } else { // no data

    anno_str.clear();

  }

  oplot->setObsAnnotation(anno_str);

  // Set plotname - ADC
  oplot->setPlotName(anno_str);

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("++ End ObsManager prepare() ++");
#endif

  return true;
}
bool ObsManager::addStationsAndTimeFromMetaData( const miutil::miString& metaData,
    miutil::miString& url, const miutil::miTime& time)
{
  //METLIBS_LOG_DEBUG(__FUNCTION__);
  //read metadata
  if ( !metaDataMap.count(metaData) ) {

    ObsMetaData* pMetaData = new ObsMetaData();
    if ( !Prod.count(metaData) || !Prod[metaData].pattern.size()) {
      METLIBS_LOG_WARN("WARNING: ObsManager::prepare:  meatdata "<<metaData<<" not available");
      return false;
    }

    //read metaData
    miString fileName = Prod[metaData].pattern[0].pattern;
    miString headerfile = Prod[metaData].headerfile;
    vector<miString> headerinfo = Prod[metaData].headerinfo;
    ObsAscii obsAscii(fileName, headerfile, headerinfo, pMetaData);
    metaDataMap[metaData] = pMetaData;
  }

  //add stations
  metaDataMap[metaData]->addStationsToUrl(url);

  //add time
  miString timeString = time.format("fd=%d.%m.%Y&td=%d.%m.%Y&h=%H");
  url.replace("TIME", timeString);
  int mm = time.month()-1;
  timeString = "m=" + miString(mm);
  url.replace("MONTH", timeString);

  return true;

}

void ObsManager::getFileName(vector<FileInfo>& finfo,
    miTime &time, miString obsType,
    ObsPlot* oplot){
  //     METLIBS_LOG_DEBUG("getFileName:"<<time.isoTime());
  //     METLIBS_LOG_DEBUG("getFileName:"<<obsType);


  finfo.clear();
  obsType = obsType.downcase();

  if (Prod.find(obsType)==Prod.end())
    return;

  int rangeMin = Prod[obsType].timeRangeMin;
  int rangeMax = Prod[obsType].timeRangeMax;
  int n=Prod[obsType].fileInfo.size();
  int found = -1;

  //Find file where time is within filetime +/- timeRange (from setup)
  for( int i=0; i<n; i++ ){
    miTime t=Prod[obsType].fileInfo[i].time;
    if (t.undef()){
      METLIBS_LOG_WARN("No time defined:"<<Prod[obsType].fileInfo[i].filename);
      continue;
    }
    int d = (miTime::minDiff(time, t));
    if( d > rangeMin && d < rangeMax+1){   //file found
      found = i;
      finfo.push_back(Prod[obsType].fileInfo[i]);
      termin.push_back(t);
      //find time range for annotation
      if( Prod[obsType].synoptic && !oplot->moreTimes()){
        t.addMin(rangeMin);
        if( t<timeRangeMin )
          timeRangeMin = t;
        t.addMin(rangeMax - rangeMin);
        if( t>timeRangeMax )
          timeRangeMax = t;
      }
      break;
    }
  }

  //If the file is synoptic and not "moreTimes" , only one file is returned.
  // If not, all files with time range within time+-timeDiff are returned.
  if(!Prod[obsType].synoptic || oplot->moreTimes()) {
    bool ok;
    int timeDiff=oplot->getTimeDiff();
    for( int i=0; i<n; i++ ){
      if( i==found ) continue; // in list already
      if(timeDiff == -1) ok=true; // use all times
      else {
        ok=false;
        miTime thisTime=Prod[obsType].fileInfo[i].time;
        if(time<thisTime){
          thisTime.addMin(-1*(timeDiff-rangeMin));
          if(time>thisTime) ok=true;
        } else if( time > thisTime){
          thisTime.addMin((timeDiff+rangeMax+1));
          if(time<thisTime) ok=true;
        } else if( time == thisTime ){ //could happend if timerange overlap
          ok=true;
        }
      }
      if(ok){
        finfo.push_back(Prod[obsType].fileInfo[i]);
        termin.push_back(Prod[obsType].fileInfo[i].time);
      }
    }
  }


  //Check if time in file-time list == time written in file (only metnoobs)
#ifdef METNOOBS
  bool timeOK=true;
  int nrFiles = finfo.size();
  for( int i=0; i<nrFiles; i++ ){
    if (finfo[i].filetype == "metnoobs") {
      miTime tt;
      obs Obs;
      try{
        Obs.readFileHeader(finfo[i].filename);
        tt = Obs.fileObsTime();
      }
      catch (...){
        METLIBS_LOG_WARN("Exception in: " <<finfo[i].filename);
        continue;
      }
      if(tt!=finfo[i].time){
        timeOK=false;
        break;
      }
    }
  }

  if(!timeOK){ //Wrong time, update file-time list
    if(firstTry){           // try to find times from filename
      firstTry=false;
      if(updateTimes(obsType))
		  timeListChanged = true;
    }else {                 //if that did not work, open all files
      if (updateTimesfromFile(obsType))
		  timeListChanged = true;
      //timefilter did not work, turn it off
      miString offstr("OFF");
      for(unsigned  int j=0;j<Prod[obsType].pattern.size(); j++)
        Prod[obsType].pattern[j].filter.initFilter(offstr);
    }
    getFileName(finfo,time,obsType,oplot);
  }
#endif

}


bool ObsManager::updateTimes(miString obsType)
{
  //Making list of file names and times using file names
  //  METLIBS_LOG_DEBUG("Finner tider fra filnavn:"<<obsType);

  obsType= obsType.downcase();

  if (Prod.find(obsType)==Prod.end())
    return false;
  // make a copy of the old fileinfo
  vector<FileInfo> oldfileInfo = Prod[obsType].fileInfo;
  Prod[obsType].fileInfo.clear();
#ifdef ROADOBS
  // WMO (synop) reports 1 time per hour. ICAO (metar) reports ever 30 minutes (x.20, x.50)
  // Can we trust in obsType to select the right time intervall and obstime?
  if (Prod[obsType].obsformat == ofmt_roadobs)
  {
	// Due to the fact that we have a database instead of an archive,
	// we maust fake the behavoir of anarchive
	// Assume that all stations report every hour
	// firt, get the current time.
	miTime now = miTime::nowTime();
	miClock nowClock = now.clock();
	miDate nowDate = now.date();
	if (obsType.contains("wmo") || obsType.contains("ship"))
		nowClock.setClock(nowClock.hour(),0,0);
	else if (obsType.contains("icao"))
		// if metar select startime +20 minutes.
		nowClock.setClock(nowClock.hour(),20,0);
	now.setTime(nowDate, nowClock);
	// Check first if now already exists in oldfileInfo
	int n = oldfileInfo.size();
	for (int i = 0; i < n; i++)
	{
		if (oldfileInfo[i].time == now)
		{
			// restor the old fileinfo
			Prod[obsType].fileInfo = oldfileInfo;
			return false;
		}
	}
	int daysback = Prod[obsType].daysback;
	miTime starttime = now;
	starttime.addDay(-daysback);
	int npattern = Prod[obsType].pattern.size();
	int hourdiff;
	for( int j=0;j<npattern; j++) {
		FileInfo finfo;
		miTime time = now;
		finfo.time = time;
		finfo.filename = "ROADOBS_" + time.isoDate() + "_" + time.isoClock(true, true);
		finfo.filetype = Prod[obsType].pattern[j].fileType;
		Prod[obsType].fileInfo.push_back(finfo);
		if (obsType.contains("wmo") || obsType.contains("ship"))
			time.addHour(-1);
		else if (obsType.contains("icao"))
			time.addMin(-30);
		while ((hourdiff = miTime::hourDiff(time, starttime)) > 0) {
			finfo.time = time;
			finfo.filename = "ROADOBS_" + time.isoDate() + "_" + time.isoClock(true, true);
			finfo.filetype = Prod[obsType].pattern[j].fileType;
			Prod[obsType].fileInfo.push_back(finfo);
			if (obsType.contains("wmo") || obsType.contains("ship"))
				time.addHour(-1);
			else if (obsType.contains("icao"))
				time.addMin(-30);
		}
	 }
  }
  else
  {
#endif
  int npattern = Prod[obsType].pattern.size();
  for( int j=0;j<npattern; j++) {
    if( (!Prod[obsType].pattern[j].archive && !useArchive) ||
       (Prod[obsType].pattern[j].archive && useArchive)  ){
      bool ok = Prod[obsType].pattern[j].filter.ok();

      glob_t globBuf;
      glob_cache(Prod[obsType].pattern[j].pattern.c_str(),0,0,&globBuf);
      for (__size_t k=0; k < globBuf.gl_pathc; k++) {
        FileInfo finfo;
        finfo.filename = globBuf.gl_pathv[k];
        if(ok &&
            Prod[obsType].pattern[j].filter.getTime(finfo.filename,finfo.time)){
          //time from file name
        } else {
          //time not found from filename, open file
          if (Prod[obsType].pattern[j].fileType == "metnoobs") {
#ifdef METNOOBS
            //read time from metnoobs-file
            obs Obs;
            try{
              Obs.readFileHeader(finfo.filename);
              finfo.time = Obs.fileObsTime();
            }
            catch (...){
              METLIBS_LOG_WARN("Exception in: " <<finfo.filename);
              continue;
            }
#endif
          } else if (Prod[obsType].pattern[j].fileType == "bufr") {
#ifdef BUFROBS
            //read time from bufr-file
            ObsBufr bufr;
            if(!bufr.ObsTime(finfo.filename,finfo.time))continue;
#endif
          }
        }
        //add new file to Prod[obsType]
        finfo.filetype = Prod[obsType].pattern[j].fileType;
        Prod[obsType].fileInfo.push_back(finfo);
      }
      globfree_cache(&globBuf);
    }
  }
#ifdef ROADOBS
  } // end�if obstype == roadobs
#endif
  // Check if timeLists are equal
  if (Prod[obsType].fileInfo.size() == oldfileInfo.size())
  {
	  // Compare the list members
	  int n= Prod[obsType].fileInfo.size();
	  for (int i = 0; i < n; i++)
	  {
		if (Prod[obsType].fileInfo[i].time != oldfileInfo[i].time)
		{
			return true;
		}
	  }
	  // The lists are equal
	  return false;
  }
  return true;
}

bool ObsManager::updateTimesfromFile(miString obsType)
{
  //Making list of file names and times, opening all files

  obsType= obsType.downcase();

  if (Prod.find(obsType)==Prod.end())
    return false;
  // make a copy of the old fileinfo
  vector<FileInfo> oldfileInfo = Prod[obsType].fileInfo;
  Prod[obsType].fileInfo.clear();
#ifdef ROADOBS
if (Prod[obsType].obsformat == ofmt_roadobs)
  {
	// Due to the fact that we have a database instead of an archive,
	// we must fake the behavoir of anarchive
	// Assume that all stations report every hour
	// first, get the current time.
	miTime now = miTime::nowTime();
	miClock nowClock = now.clock();
	miDate nowDate = now.date();
	nowClock.setClock(nowClock.hour(),0,0);
	now.setTime(nowDate, nowClock);
	// Check first if now already exists in oldfileInfo
	int n = oldfileInfo.size();
	for (int i = 0; i < n; i++)
	{
		if (oldfileInfo[i].time == now)
		{
			// restor the old fileinfo
			Prod[obsType].fileInfo = oldfileInfo;
			return false;
		}
	}
	int daysback = Prod[obsType].daysback;
	miTime starttime = now;
	starttime.addDay(-daysback);
	int npattern = Prod[obsType].pattern.size();
	int hourdiff;
	for( int j=0;j<npattern; j++) {
		FileInfo finfo;
		miTime time = now;
		finfo.time = time;
		finfo.filename = "ROADOBS_" + time.isoDate() + "_" + time.isoClock(true, true);
		finfo.filetype = Prod[obsType].pattern[j].fileType;
		Prod[obsType].fileInfo.push_back(finfo);
		time.addHour(-1);
		while ((hourdiff = miTime::hourDiff(time, starttime)) > 0) {
			finfo.time = time;
			finfo.filename = "ROADOBS_" + time.isoDate() + "_" + time.isoClock(true, true);
			finfo.filetype = Prod[obsType].pattern[j].fileType;
			Prod[obsType].fileInfo.push_back(finfo);
			time.addHour(-1);
		}
	 }
  }
  else
  {
#endif
  vector<miString> fname;

  for(unsigned int j=0;j<Prod[obsType].pattern.size(); j++) {
    if( (!Prod[obsType].pattern[j].archive && !useArchive ) ||
      ( Prod[obsType].pattern[j].archive && useArchive ) ){
      glob_t globBuf;
      glob_cache(Prod[obsType].pattern[j].pattern.c_str(),0,0,&globBuf);
      for (__size_t k=0; k < globBuf.gl_pathc; k++) {
        FileInfo finfo;
        finfo.filename = globBuf.gl_pathv[k];
        if (Prod[obsType].pattern[j].fileType == "metnoobs") {
#ifdef METNOOBS
          obs Obs;
          try{
            Obs.readFileHeader(finfo.filename);
            finfo.time = Obs.fileObsTime();
          }
          catch (...){
            METLIBS_LOG_WARN("Exception in: " <<finfo.filename);
            continue;
          }
#endif
        } else if (Prod[obsType].pattern[j].fileType == "bufr") {
#ifdef BUFROBS
          //read time from bufr-file
          ObsBufr bufr;
          if(!bufr.ObsTime(finfo.filename,finfo.time)) continue;
#endif
        }
        if(finfo.time.undef()&& Prod[obsType].timeInfo != "notime") continue;
        finfo.filetype = Prod[obsType].pattern[j].fileType;
        Prod[obsType].fileInfo.push_back(finfo);
      }
      globfree_cache(&globBuf);
    }
  }
#ifdef ROADOBS
}
#endif
  // Check if timeLists are equal
  if (Prod[obsType].fileInfo.size() == oldfileInfo.size())
  {
	  // Compare the list members
	  int n= Prod[obsType].fileInfo.size();
	  for (int i = 0; i < n; i++)
	  {
		if (Prod[obsType].fileInfo[i].time != oldfileInfo[i].time)
		{
			return true;
		}
	  }
	  // The lists are equal
	  return false;
  }
  return true;
}


//return observation times for list of PlotInfo's
vector<miTime> ObsManager::getObsTimes(const vector<miString>& pinfos)
{
  int m,nn= pinfos.size();
  vector<miString> tokens;
  vector<miString> obsTypes;

  for (int i=0; i<nn; i++){
    //     METLIBS_LOG_DEBUG("Processing: " << pinfos[i].infoStr());
    tokens= pinfos[i].split();
    m= tokens.size();
    if (m<2) continue;
    for (int k=1; k<m; k++){
      vector<miString> stokens = tokens[k].split('=');
      if( stokens.size() == 2) {
        if (stokens[0].downcase() == "data"){
          vector<miString> obsT = stokens[1].split(",");
          obsTypes.insert(obsTypes.end(),obsT.begin(),obsT.end());
          break;
        }
      }
    }
  }

  return getTimes(obsTypes);

}


void ObsManager::getCapabilitiesTime(vector<miTime>& normalTimes,
    miTime& constTime,
    int& timediff,
    const miString& pinfo)
{
  //Finding times from pinfo

  timediff=0;

  vector<miString> tokens= pinfo.split('"','"');
  int m= tokens.size();
  if (m<3) return;
  vector<miString> obsTypes;
  for(unsigned int j=0; j<tokens.size();j++){
    vector<miString> stokens= tokens[j].split("=");
    if(stokens.size()==2 && stokens[0].downcase()=="data"){
      obsTypes = stokens[1].split(",");
    }
    else if(stokens.size()==2 && stokens[0].downcase()=="timediff"){
      timediff=stokens[1].toInt();
    }
  }

  normalTimes = getTimes(obsTypes);

}



// return observation times for list of obsTypes
vector<miTime> ObsManager::getTimes( vector<miString> obsTypes)
{
  set<miTime> timeset;

  int n=obsTypes.size();
  for(int i=0; i<n; i++ ){

    miString obsType= obsTypes[i].downcase();
    if ( Prod[obsType].obsformat == ofmt_url ) {
      // TODO move some of this to parseSetup
      FileInfo finfo;
      if( !Prod[obsType].pattern.size() ) continue;
      finfo.filename = Prod[obsType].pattern[0].pattern;
      finfo.filetype = Prod[obsType].pattern[0].fileType;
      Prod[obsType].fileInfo.push_back(finfo);


      vector<miString> tokens = Prod[obsType].timeInfo.split(";");
      miTime from = miTime::nowTime();
      miTime to = miTime::nowTime();
      int interval = 3;
      for ( size_t j = 0; j< tokens.size(); ++j ){
        vector<miString> stokens = tokens[j].split("=");
        if ( stokens.size() != 2 ) continue;
        if ( stokens[0] == "from" ) {
          from = miTime(stokens[1]);
        } else if ( stokens[0] == "to" ) {
            to = miTime(stokens[1]);
        } else if ( stokens[0] == "interval" ) {
            interval = atoi(stokens[1].c_str());
        }
      }
      while ( from < to ) {
        timeset.insert(from);
        from.addHour( interval );
      }

    } else {

        if(not obsType.contains("hqc")) {
            if (updateTimes(obsType)) {
                timeListChanged = true;
            }
            
            if(Prod[obsType].timeInfo == "notime") continue;
        }

        vector<FileInfo>::iterator p= Prod[obsType].fileInfo.begin();
        for (; p!=Prod[obsType].fileInfo.end(); p++)
            timeset.insert((*p).time);
    }

  }

  return vector<miTime>(timeset.begin(), timeset.end());
}

void ObsManager::updateObsPositions(const vector<ObsPlot*> oplot)
{

  if(!mslp) return;

  clearObsPositions();

  vector<float> xpos;
  vector<float> ypos;
  for(unsigned int i=0; i<oplot.size();i++){
    oplot[i]->getPositions(xpos,ypos);
  }

  obsPositions.numObs=xpos.size();
  obsPositions.xpos = new float[obsPositions.numObs];
  obsPositions.ypos = new float[obsPositions.numObs];
  obsPositions.values = new float[obsPositions.numObs];
  for( int i=0; i<obsPositions.numObs;i++){
    obsPositions.xpos[i]=xpos[i];
    obsPositions.ypos[i]=ypos[i];
  }

  if(oplot.size()){
    obsPositions.obsArea = oplot[0]->getMapArea();
  }

  //new conversion  needed
  obsPositions.convertToGrid = true;

}

void ObsManager::clearObsPositions()
{

  obsPositions.numObs=0;
  delete[] obsPositions.xpos;
  obsPositions.xpos=0;
  delete[] obsPositions.ypos;
  obsPositions.ypos=0;
  delete[] obsPositions.values;
  obsPositions.values=0;

}


void ObsManager::calc_obs_mslp(const vector<ObsPlot*> oplot)
{

  if(!mslp) return;
  ObsPlot::clearPos();
  int m= oplot.size();
  for (int i=0; i<m; i++){
    oplot[i]->obs_mslp(obsPositions.values);
  }

}

ObsDialogInfo ObsManager::initDialog()
{

  map<miString,ProdInfo>::iterator prbegin= Prod.begin();
  map<miString,ProdInfo>::iterator prend=   Prod.end();
  map<miString,ProdInfo>::iterator pr;


  //+++++++++Plot type = Synop+++++++++++++++

  ObsDialogInfo::PlotType psynop;
  psynop.name = "Synop";
  psynop.misc = "dev_field_button=true tempPrecision=true more_times qualityflag wmoflag";
  psynop.criteriaList = criteriaList["synop"];

  psynop.button.push_back(addButton("Wind","Wind (direction and speed)"));
  psynop.button.push_back(addButton("TTT","Temperature (2m)",-50,50));
  psynop.button.push_back(addButton("TdTdTd","Dew point temperature",-50,50));
  psynop.button.push_back(addButton("PPPP","Pressure ",100,1050));
  psynop.button.push_back(addButton("ppp"," 3 hour pressure change",-10,10));
  psynop.button.push_back(addButton("a","Characteristic of pressure tendency",0,9));
  psynop.button.push_back(addButton("h","height of base of cloud",1,9));
  psynop.button.push_back(addButton("VV" ,"horizontal visibility",0,10000));
  psynop.button.push_back(addButton("N","total cloud cover",0,8));
  psynop.button.push_back(addButton("RRR","precipitation",-1,100));
  psynop.button.push_back(addButton("ww","present weather",1,100));
  psynop.button.push_back(addButton("W1","past weather (1)",3,9));
  psynop.button.push_back(addButton("W2","past weather (2)",3,9));
  psynop.button.push_back(addButton("Nh","cloud amount",0,9));
  psynop.button.push_back(addButton("Cl","cloud type, low",1,9));
  psynop.button.push_back(addButton("Cm","cloud type, medium",1,9));
  psynop.button.push_back(addButton("Ch","cloud type, high",1,9));
  psynop.button.push_back(addButton("vs","speed of motion of moving observing platform",0,9));
  psynop.button.push_back(addButton("ds","direction of motion of moving observing platform",1,360));
  psynop.button.push_back(addButton("TwTwTw","sea/water temperature",-50,50));
  psynop.button.push_back(addButton("PwaHwa","period/height of waves",0,0));
  psynop.button.push_back(addButton("dw1dw1","direction of swell waves",0,360));
  psynop.button.push_back(addButton("Pw1Hw1","period/height of swell waves",0,0));
  psynop.button.push_back(addButton("TxTn","max/min temperature at 2m",-50,50));
  psynop.button.push_back(addButton("sss","Snow depth",0,100));
  psynop.button.push_back(addButton("911ff","maximum wind speed (gusts)",0,100));
  psynop.button.push_back(addButton("s","state of the sea",0,20));
  psynop.button.push_back(addButton("fxfx","maximum wind speed (10 min mean wind)",0,100));
  psynop.button.push_back(addButton("Id","buoy/platform, ship or mobile land station identifier",0,0));
  psynop.button.push_back(addButton("St.no(3)","WMO station number",0,0));
  psynop.button.push_back(addButton("St.no(5)","WMO block and station number",0,0));
  psynop.button.push_back(addButton("Time","hh.mm",0,0));

  vector<ObsFormat> obsformat;
  obsformat.push_back(ofmt_synop);
  obsformat.push_back(ofmt_dribu);
  obsformat.push_back(ofmt_metar);
  obsformat.push_back(ofmt_satob);
  obsformat.push_back(ofmt_aireps);
  addType(psynop,obsformat);

  dialog.plottype.push_back(psynop);

  //+++++++++Plot type = Metar+++++++++++++++

  ObsDialogInfo::PlotType pmetar;

  pmetar.name="Metar";
  pmetar.misc = "more_times tempPrecision=true";
  pmetar.criteriaList = criteriaList["metar"];

  pmetar.button.push_back(addButton("Wind","Wind (direction and speed)"));
  pmetar.button.push_back(addButton("dndx","limit of variable wind direction"));
  pmetar.button.push_back(addButton("fmfm","Wind gust",0,36));
  pmetar.button.push_back(addButton("TTT","Temperature",-50,50));
  pmetar.button.push_back(addButton("TdTdTd","Dew point temperature",-50,50));
  pmetar.button.push_back(addButton("ww","Significant weather",0,0));
  pmetar.button.push_back(addButton("REww","Recent weather",0,0));
  pmetar.button.push_back(addButton("VVVV/Dv","Visibility (worst)",0,0));
  pmetar.button.push_back(addButton("VxVxVxVx/Dvx","Visibility (best)",0,0));
  pmetar.button.push_back(addButton("Clouds","Clouds",0,0));
  pmetar.button.push_back(addButton("PHPHPHPH","PHPHPHPH",0,100));
  pmetar.button.push_back(addButton("Id","Identifier",0,0));

  ObsDialogInfo::DataType type;

  obsformat.clear();
  obsformat.push_back(ofmt_metar);
  addType(pmetar,obsformat);

  dialog.plottype.push_back(pmetar);


  //+++++++++Plot type = List+++++++++++++++

  ObsDialogInfo::PlotType plist;

  plist.name="List";
  plist.misc =
    "dev_field_button tempPrecision markerboxVisible orientation more_times qualityflag wmoflag";
  plist.criteriaList = criteriaList["list"];

  plist.button.push_back(addButton("Pos","Position",0,0,true));
  plist.button.push_back(addButton("dd","wind direction",0,360,true));
  plist.button.push_back(addButton("ff","wind speed)",0,100,true));
  plist.button.insert(plist.button.end(),
      psynop.button.begin(),psynop.button.end());
  plist.button.pop_back();
  plist.button.pop_back();
  plist.button.pop_back();
  plist.button.push_back(addButton("T_red","Potential temperature",-50,50));
  plist.button.push_back(addButton("Date","Date(mm-dd)",0,100));
  plist.button.push_back(addButton("Time","hh.mm  ",0,0,true));
  plist.button.push_back(addButton("Height","height of station",0,5000));
  plist.button.push_back(addButton("Zone","Zone",1,99));
  plist.button.push_back(addButton("Name","Name of station",0,0));
  plist.button.push_back(addButton("RRR_1","precipitation past hour",-1,100));
  plist.button.push_back(addButton("RRR_6","precipitation past 6 hours",-1,100));
  plist.button.push_back(addButton("RRR_12","precipitation past 12 hours",-1,100));
  plist.button.push_back(addButton("RRR_24","precipitation past 24 hours",-1,100));
  plist.button.push_back(addButton("quality","quality",-1,100));


  obsformat.clear();
  obsformat.push_back(ofmt_synop);
  obsformat.push_back(ofmt_dribu);
  obsformat.push_back(ofmt_metar);
  obsformat.push_back(ofmt_satob);
  obsformat.push_back(ofmt_aireps);
  addType(plist,obsformat);

  dialog.plottype.push_back(plist);


  //+++++++++Plot type = Pressure levels+++++++++++++++

  ObsDialogInfo::PlotType ppressure;
  ppressure.name="Pressure";
  ppressure.misc =
    "markerboxVisible asFieldButton orientation more_times";
  ppressure.criteriaList = criteriaList["pressure"];

  ppressure.pressureLevels = levels;

  ppressure.button.push_back(addButton("Pos","Position",0,0,true));
  ppressure.button.push_back(addButton("Wind","Wind (direction and speed)"));
  ppressure.button.push_back(addButton("dd","wind direction",0,360,true));
  ppressure.button.push_back(addButton("ff","wind speed)",0,100,true));
  ppressure.button.push_back(addButton("TTT","Temperature",-50,50));
  ppressure.button.push_back(addButton("TdTdTd","Dew point temperature",-50,50));
  ppressure.button.push_back(addButton("PPPP","Pressure",950,1050,true));
  ppressure.button.push_back(addButton("Id","Identification",0,0,true));
  ppressure.button.push_back(addButton("Date","Date(mm-dd)",0,0));
  ppressure.button.push_back(addButton("Time","hh.mm",0,0,true));
  ppressure.button.push_back(addButton("HHH","geopotential",true));
  ppressure.button.push_back(addButton("QI","Percent confidence",0,100,true));
  ppressure.button.push_back(addButton("QI_NM","Percent confidence no model",0,100,true));
  ppressure.button.push_back(addButton("QI_RFF","Percent confidence recursive filter flag",0,100,true));

  obsformat.clear();
  obsformat.push_back(ofmt_temp);
  obsformat.push_back(ofmt_pilot);
  addType(ppressure,obsformat);

  dialog.plottype.push_back(ppressure);

  //+++++++++Plot type = Synop/List (one datatype)+++++++++++++++

  for (pr=prbegin; pr!=prend; pr++) {

    if (pr->second.plotFormat=="synop" ||
        pr->second.plotFormat=="list" ) {

      ObsDialogInfo::PlotType psingle;
      if( pr->second.obsformat==ofmt_synop ||
          pr->second.obsformat==ofmt_dribu ||
          pr->second.obsformat==ofmt_metar ||
          pr->second.obsformat==ofmt_satob){
        if( pr->second.plotFormat=="synop" ){
          psingle.misc = "dev_field_button=true tempPrecision=true more_times";
          psingle.criteriaList = criteriaList["synop"];
          setAllActive(psingle,pr->second.parameter,pr->second.dialogName,
              psynop.button);
        } else {
          psingle.misc =
            "dev_field_button tempPrecision markerboxVisible orientation more_times";
          psingle.criteriaList = criteriaList["list"];
          setAllActive(psingle,pr->second.parameter,pr->second.dialogName,
              plist.button);
        }
      } else {
        psingle.misc =
          "markerboxVisible asFieldButton orientation more_times";
        psingle.criteriaList = criteriaList["pressure"];

        psingle.pressureLevels = levels;
        setAllActive(psingle,pr->second.parameter,pr->second.dialogName,
            ppressure.button);
      }

      psingle.name= pr->second.plotFormat + ": " + pr->second.dialogName;

      dialog.plottype.push_back(psingle);
    }
  }

  //+++++++++Plot type = Ascii-files+++++++++++++++

  // buttons made when plottype activated the first time...

  for (pr=prbegin; pr!=prend; pr++) {
    if (pr->second.obsformat==ofmt_ascii || pr->second.obsformat==ofmt_url) {

      ObsDialogInfo::PlotType pascii;

      pascii.misc =
        "markerboxVisible orientation  parameterName=true";

      pascii.criteriaList = criteriaList["ascii"];

      pascii.name= pr->second.dialogName;
      pascii.button.clear();

      type.active.clear();
      type.name= pr->second.dialogName; // same name as the plot type
      pascii.datatype.push_back(type);
      pascii.criteriaList = criteriaList["ascii"];

      dialog.plottype.push_back(pascii);
    }
  }

  //+++++++++Plot type = Road-obs +++++++++++++++

   // buttons made when plottype activated the first time...
#ifdef ROADOBS
  for (pr=prbegin; pr!=prend; pr++) {
    if (pr->second.obsformat==ofmt_roadobs) {

      ObsDialogInfo::PlotType proad;

      proad.misc =
	"markerboxVisible orientation  parameterName=true";

      proad.criteriaList = criteriaList["roadobs"];

      //proad.name= pr->second.dialogName;
	  //proad.name= pr->second.plotFormat + ": " + pr->second.dialogName;
	  proad.name= pr->second.plotFormat + ":" + pr->second.dialogName;
      proad.button.clear();

      type.active.clear();
      type.name= pr->second.dialogName; // same name as the plot type
	  //type.name = pr->second.plotFormat + ":" + pr->second.dialogName;
      proad.datatype.push_back(type);
      proad.criteriaList = criteriaList["roadobs"];

      dialog.plottype.push_back(proad);
    }
  }
#endif

  //+++++++++Plot type = Ocean+++++++++++++++

  ObsDialogInfo::PlotType pocea;
  pocea.name="Ocean";
  pocea.misc =
    "markerboxVisible asFieldButton  orientation more_times";
  pocea.criteriaList = criteriaList["ocean"];

  pocea.pressureLevels.push_back(0);
  pocea.pressureLevels.push_back(10);
  pocea.pressureLevels.push_back(20);
  pocea.pressureLevels.push_back(30);
  pocea.pressureLevels.push_back(50);
  pocea.pressureLevels.push_back(75);
  pocea.pressureLevels.push_back(100);
  pocea.pressureLevels.push_back(125);
  pocea.pressureLevels.push_back(150);
  pocea.pressureLevels.push_back(200);
  pocea.pressureLevels.push_back(250);
  pocea.pressureLevels.push_back(300);
  pocea.pressureLevels.push_back(400);
  pocea.pressureLevels.push_back(500);
  pocea.pressureLevels.push_back(600);
  pocea.pressureLevels.push_back(700);
  pocea.pressureLevels.push_back(800);
  pocea.pressureLevels.push_back(900);
  pocea.pressureLevels.push_back(1000);
  pocea.pressureLevels.push_back(1200);
  pocea.pressureLevels.push_back(1500);
  pocea.pressureLevels.push_back(2000);
  pocea.pressureLevels.push_back(3000);
  pocea.pressureLevels.push_back(4000);

  pocea.button.push_back(addButton("Id","Identifcation",0,0,true));
  pocea.button.push_back(addButton("PwaPwa","period of waves"));
  pocea.button.push_back(addButton("HwaHwa","height of waves"));
  pocea.button.push_back(addButton("depth","depth",0,100,true));
  pocea.button.push_back(addButton("TTTT","sea/water temperature",-50,50,true));
  pocea.button.push_back(addButton("SSSS","Salt",0,50,true));
  pocea.button.push_back(addButton("Date","Date(mm-dd)",0,0));
  pocea.button.push_back(addButton("Time","hh.mm  ",0,0,true));

  type.active.resize(pocea.button.size(),true);

  for (pr=prbegin; pr!=prend; pr++) {
    if (pr->second.obsformat==ofmt_ocea) {
      type.name= pr->second.dialogName;
      pocea.datatype.push_back(type);
    }
  }

  dialog.plottype.push_back(pocea);


  //+++++++++Plot type = Tide +++++++++++++++

  ObsDialogInfo::PlotType ptide;
  ptide.name="Tide";
  ptide.misc = "markerboxVisible orientation more_times";
  ptide.criteriaList = criteriaList["tide"];

  ptide.button.push_back(addButton("Pos","Position",0,0,true));
  ptide.button.push_back(addButton("TE","Tide",-10,10,true));
  ptide.button.push_back(addButton("Id","Identification",0,0,true));
  ptide.button.push_back(addButton("Date","Date(mm-dd)",0,0));
  ptide.button.push_back(addButton("Time","hh.mm  ",0,0,true));

  type.active.resize(ptide.button.size(),true);

  for (pr=prbegin; pr!=prend; pr++) {
    if (pr->second.obsformat==ofmt_tide) {
      type.name= pr->second.dialogName;
      ptide.datatype.push_back(type);
    }
  }

  dialog.plottype.push_back(ptide);


  //+++++++++++++++++++Plot Type = Hqc ++++++++++++++++++++++++++++++++

  //buttons made each time initHqcData() is called

  for (pr=prbegin; pr!=prend; pr++) {
    if (pr->second.obsformat==ofmt_hqc) {

      ObsDialogInfo::PlotType phqc;

      phqc.name= pr->second.dialogName;
      phqc.misc = "tempPrecision=true criteria=false";
      phqc.button.clear();

      type.active.clear();
      type.name= pr->second.dialogName; // same name as the plot type
      phqc.datatype.push_back(type);
      phqc.criteriaList = criteriaList["hqc"];

      dialog.plottype.push_back(phqc);
    }
  }

  return dialog;
}

void ObsManager::addType(ObsDialogInfo::PlotType& dialogInfo,
    const vector<ObsFormat>& obsformat)
{
  map<miString,ProdInfo>::iterator prbegin= Prod.begin();
  map<miString,ProdInfo>::iterator prend=   Prod.end();
  map<miString,ProdInfo>::iterator pr;

  int n = obsformat.size();
  for (int i=0; i<n; i++) {
    for (pr=prbegin; pr!=prend; pr++) {
      if (pr->second.obsformat==obsformat[i]) {
        ObsDialogInfo::DataType type;
        type.active.resize(dialogInfo.button.size(),false);
        setActive(pr->second.parameter,true,type.active,dialogInfo.button);
        type.name= pr->second.dialogName;
        dialogInfo.datatype.push_back(type);
      }
    }
  }
}

void ObsManager::setActive(const vector<miString>& name, bool on,
    vector<bool>& active,
    const vector<ObsDialogInfo::Button>& b)
{
  int nname=name.size();
  unsigned int nr=b.size();
  if(active.size() != nr) return;

  for (int j=0; j<nname; j++)
    for (unsigned int i=0; i<nr; i++)
      if (name[j] == b[i].name) {
        active[i]=on;
        break;
      }
}

void ObsManager::setAllActive(ObsDialogInfo::PlotType& dialogInfo,
    const vector<miString>& parameter,
    const miString& name,
    const vector<ObsDialogInfo::Button>& b)
{
  int n=b.size();
  int m=parameter.size();
  for( int j=0; j<m; j++)
    for( int i=0; i<n; i++)
      if(parameter[j] == b[i].name){
        dialogInfo.button.push_back(b[i]);
      }

  ObsDialogInfo::DataType type;
  type.name = name;
  type.active.resize(dialogInfo.button.size(),true);
  dialogInfo.datatype.push_back(type);
}

ObsDialogInfo::Button  ObsManager::addButton(const miString& name,
    const miString& tip,
    int low,
    int high,
    bool def)
{
  ObsDialogInfo::Button b;
  b.name = name;
  b.tooltip = tip;
  b.Default = def;
  b.low = low;
  b.high = high;
  return b;
}

ObsDialogInfo ObsManager::updateDialog(const miString& name)
{
  // called the first time an ascii dialog is activated
  //  METLIBS_LOG_DEBUG("updateDialog:"<< name);

  if(name == "Hqc_synop" || name == "Hqc_list")
    return updateHqcDialog(name);

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("updateDialog: " << name);
#endif
  int nd= dialog.plottype.size();
  int id= 0;
  while (id<nd && dialog.plottype[id].name!=name) id++;
  if (id==nd) {
    METLIBS_LOG_ERROR(name << " not found in dialog.plottype");
    return dialog;
  }
  miString oname= name.downcase();
  // BAD hack...
  if (oname.contains(":")) {
    std::string::size_type pos = oname.find(":");
    oname = oname.substr(pos+1);
  }
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("updateDialog: " << oname);
  map<miString,ProdInfo>::iterator p= Prod.begin();
  while (p != Prod.end()) {
    METLIBS_LOG_DEBUG(p->first);
    p++;
}
#endif

  map<miString,ProdInfo>::iterator pr= Prod.find(oname);
  if (pr==Prod.end()) {
    METLIBS_LOG_ERROR(oname << " not found in Prod");
    return dialog;
  }
#ifdef ROADOBS
  // We must also support the ascii format until the
  // lightning is stored in road
  if (pr->second.obsformat!=ofmt_roadobs)
  {
	  if (pr->second.obsformat!=ofmt_ascii)
	  {
		return dialog;
	  }
  }
  // TDB: the ofmt_roadobs must be added
  // **************************************
  bool headerfound= false;

  ObsPlot *roplot= new ObsPlot();

  // The road format must have a header file, defined in prod
  // This file, defines the parameters as well as the mapping
  // between diana and road parameter space.
  miString headerfile= Prod[oname].headerfile;
  miString databasefile = Prod[oname].databasefile;
  miString stationfile = Prod[oname].stationfile;
  miString filename; // just dummy here
  miTime filetime; // just dummy here
  ObsRoad obsRoad = ObsRoad(filename,databasefile,stationfile,headerfile,filetime,roplot,false);
  headerfound= roplot->roadobsOK;
  if (roplot->roadobsOK && roplot->roadobsColumn.count("time")
	 && !roplot->roadobsColumn.count("date"))
	Prod[oname].useFileTime= true;
  if (headerfound) {
    int nc= roplot->roadobsColumnName.size();
    bool addWind= false;
    if (roplot->roadobsColumn.count("dd") && roplot->roadobsColumn.count("ff")) {
      addWind= (roplot->roadobsColumn["dd"] < roplot->roadobsColumn["ff"]) ?
                roplot->roadobsColumn["dd"] : roplot->roadobsColumn["ff"];
    }
	else if (roplot->roadobsColumn.count("dd") && roplot->roadobsColumn.count("ffk")) {
      addWind= (roplot->roadobsColumn["dd"] < roplot->roadobsColumn["ffk"]) ?
                roplot->roadobsColumn["dd"] : roplot->roadobsColumn["ffk"];
    }
	if (addWind) {
        dialog.plottype[id].button.push_back(addButton("Wind",""));
        dialog.plottype[id].datatype[0].active.push_back(true);  // only one datatype, yet!
    }
    for (int c=0; c<nc; c++) {
        dialog.plottype[id].button.push_back(addButton(roplot->roadobsColumnName[c],
		     roplot->roadobsColumnTooltip[c],
		     -100,100,true));
        dialog.plottype[id].datatype[0].active.push_back(true);  // only one datatype, yet!
    }
  }
delete roplot;
#endif
  if (pr->second.obsformat!=ofmt_ascii && pr->second.obsformat!=ofmt_url) return dialog;

  // open one file and find the available data parameters

  unsigned int j= 0;

  vector<miutil::miString> columnName;
  vector<miutil::miString> columnTooltip;

  while (j<Prod[oname].pattern.size()) {
    if (!Prod[oname].pattern[j].archive || useArchive ) {
      miString headerfile= Prod[oname].headerfile;
      if(Prod[oname].pattern[j].pattern.contains("http")) {

        ObsAscii obsAscii = ObsAscii(Prod[oname].pattern[j].pattern, headerfile, Prod[oname].headerinfo);
        if (obsAscii.asciiOK() && obsAscii.parameterType("time")
            && !obsAscii.parameterType("date"))
          Prod[oname].useFileTime= true;
        if (obsAscii.parameterType("dd") && obsAscii.parameterType("ff")) {
          dialog.plottype[id].button.push_back(addButton("Wind",""));
          dialog.plottype[id].datatype[0].active.push_back(true);  // only one datatype, yet!
        }
        int nc= obsAscii.columnName.size();
        for (int c=0; c<nc; c++) {
          dialog.plottype[id].button.push_back
          (addButton(obsAscii.columnName[c], obsAscii.columnTooltip[c], -100,100,true));
          dialog.plottype[id].datatype[0].active.push_back(true);  // only one datatype, yet!
        }

      } else {

        bool found= false;
        glob_t globBuf;
        glob_cache(Prod[oname].pattern[j].pattern.c_str(),0,0,&globBuf);
        __size_t k= 0;
        while (!found && k < globBuf.gl_pathc) {
          miString filename = globBuf.gl_pathv[k];
          ObsAscii obsAscii = ObsAscii(filename, headerfile, Prod[oname].headerinfo);
          found= obsAscii.asciiOK();
          if (obsAscii.asciiOK() && obsAscii.parameterType("time")
              && !obsAscii.parameterType("date"))
            Prod[oname].useFileTime= true;
          k++;
          if (obsAscii.parameterType("dd") && obsAscii.parameterType("ff")) {
            dialog.plottype[id].button.push_back(addButton("Wind",""));
            dialog.plottype[id].datatype[0].active.push_back(true);  // only one datatype, yet!
          }
          int nc= obsAscii.columnName.size();
          for (int c=0; c<nc; c++) {
            dialog.plottype[id].button.push_back
            (addButton(obsAscii.columnName[c], obsAscii.columnTooltip[c], -100,100,true));
            dialog.plottype[id].datatype[0].active.push_back(true);  // only one datatype, yet!
          }
        }

        globfree_cache(&globBuf);
      }
    }
    j++;
  }

  return dialog;
}

ObsDialogInfo ObsManager::updateHqcDialog(const std::string& plotType)
{
  // called each time initHqcData() is called
  //  METLIBS_LOG_DEBUG("updateHqcDialog");

  int nd= dialog.plottype.size();
  int id= 0;
  while (id<nd && dialog.plottype[id].name!=plotType) {
    id++;
  }
  if (id == nd)
    return dialog;

  //   miString oname= name.downcase();

  //   map<miString,ProdInfo>::iterator pr= Prod.find(oname);
  //   if (pr==Prod.end()) return dialog;

  //   if (pr->second.obsformat==ofmt_hqc) {
  dialog.plottype[id].button.clear();
  dialog.plottype[id].datatype[0].active.clear();
  if (plotType == "Hqc_synop"){
    int wind=0;
    BOOST_FOREACH(const std::string& p, hqc_synop_parameter) {
      if (p=="dd" || p=="ff") {
        wind++;
        continue;
      }
      if (p == "lon" or p == "lat" or p == "auto")
        continue;
      dialog.plottype[id].button.push_back(addButton(p," ",0,0,true));
      dialog.plottype[id].datatype[0].active.push_back(true);
    }
    if (wind == 2) {
      dialog.plottype[id].button.push_back(addButton("Wind"," ",0,0,true));
      dialog.plottype[id].datatype[0].active.push_back(true);
    }
    if (dialog.plottype[id].button.size()) {
      dialog.plottype[id].button.push_back(addButton("Flag"," ",0,0,true));
      dialog.plottype[id].datatype[0].active.push_back(true);
    }
  } else if (plotType == "Hqc_list") {
    int wind=0;
    BOOST_FOREACH(const std::string& p, hqc_synop_parameter) {
      if (p == "auto")
        continue;
      if (p == "DD" or p == "FF") {
        wind++;
      } else {
        dialog.plottype[id].button.push_back(addButton(p," ",0,0,true));
        dialog.plottype[id].datatype[0].active.push_back(true);
      }
      if (wind == 2) {
        wind = 0;
        dialog.plottype[id].button.push_back(addButton("Wind"," ",0,0,true));
        dialog.plottype[id].datatype[0].active.push_back(true);
      }
    }
  }
  return dialog;
}

void ObsManager::printProdInfo(const ProdInfo & pinfo)
{
  unsigned int i;
  METLIBS_LOG_INFO("***** ProdInfo ******");
#ifdef ROADOBS
  if (pinfo.obsformat == ofmt_roadobs)
    METLIBS_LOG_INFO("obsformat: ofmt_roadobs");
#endif
  if (pinfo.obsformat == ofmt_unknown)
    METLIBS_LOG_INFO("obsformat: ofmt_unknown");
  if (pinfo.obsformat == ofmt_synop)
    METLIBS_LOG_INFO("obsformat: ofmt_synop");
  if (pinfo.obsformat == ofmt_aireps)
    METLIBS_LOG_INFO("obsformat: ofmt_aireps");
  if (pinfo.obsformat == ofmt_satob)
    METLIBS_LOG_INFO("obsformat: ofmt_satob");
  if (pinfo.obsformat == ofmt_dribu)
    METLIBS_LOG_INFO("obsformat: ofmt_dribu");
  if (pinfo.obsformat == ofmt_temp)
    METLIBS_LOG_INFO("obsformat: ofmt_temp");
  if (pinfo.obsformat == ofmt_ocea)
    METLIBS_LOG_INFO("obsformat: ofmt_ocea");
  if (pinfo.obsformat == ofmt_tide)
    METLIBS_LOG_INFO("obsformat: ofmt_tide");
  if (pinfo.obsformat == ofmt_pilot)
    METLIBS_LOG_INFO("obsformat: ofmt_pilot");
  if (pinfo.obsformat == ofmt_metar)
    METLIBS_LOG_INFO("obsformat: ofmt_metar");
  if (pinfo.obsformat == ofmt_ascii)
    METLIBS_LOG_INFO("obsformat: ofmt_ascii");
  if (pinfo.obsformat == ofmt_hqc)
    METLIBS_LOG_INFO("obsformat: ofmt_hqc");
  METLIBS_LOG_INFO("dialogname: " << pinfo.dialogName);
  METLIBS_LOG_INFO("plotFormat: " << pinfo.plotFormat);
  METLIBS_LOG_INFO("pattern: ");
  for(i = 0; i < pinfo.pattern.size(); i++)
    {
      // TDB: timefiler
      METLIBS_LOG_INFO("\tpattern: " << pinfo.pattern[i].pattern << " archive: " << pinfo.pattern[i].archive << " fileType: " << pinfo.pattern[i].fileType);
    }
  METLIBS_LOG_INFO("fileInfo: ");
  for(i = 0; i < pinfo.fileInfo.size(); i++)
    {
      METLIBS_LOG_INFO("\tfilename: " << pinfo.fileInfo[i].filename << " time: " << pinfo.fileInfo[i].time.isoTime() << " filetype: " << pinfo.fileInfo[i].filetype);
    }
  METLIBS_LOG_INFO("timeInfo"
      ": " << pinfo.timeInfo);
  METLIBS_LOG_INFO("timeRangeMin: " << pinfo.timeRangeMin);
  METLIBS_LOG_INFO("timeRangeMax: " << pinfo.timeRangeMax);
  METLIBS_LOG_INFO("current: " << pinfo.current);
  METLIBS_LOG_INFO("synoptic: " << pinfo.synoptic);
  METLIBS_LOG_INFO("headerfile: " << pinfo.headerfile);
  for(i = 0; i < pinfo.headerinfo.size(); i++)
    {
      METLIBS_LOG_INFO("\t headerinfo: " << pinfo.headerinfo[i]);
    }
#ifdef ROADOBS
  METLIBS_LOG_INFO("databasefile: " << pinfo.databasefile);
  METLIBS_LOG_INFO("stationfile: " << pinfo.stationfile);
  METLIBS_LOG_INFO("daysback: " << pinfo.daysback);
#endif
  METLIBS_LOG_INFO("useFileTime: " << pinfo.useFileTime);
  METLIBS_LOG_INFO("\tparameters: ");
  for(i = 0; i < pinfo.parameter.size(); i++)
    {
      METLIBS_LOG_INFO(pinfo.parameter[i] << ",");
    }
  METLIBS_LOG_INFO("**** end ProdInfo *****");
}

bool ObsManager::parseSetup()
{

  //  METLIBS_LOG_DEBUG("Obs: parseSetup");
  const miString obs_name = "OBSERVATION_FILES";
  vector<miString> sect_obs;

  if (!SetupParser::getSection(obs_name,sect_obs)){
    METLIBS_LOG_ERROR(obs_name << " section not found");
    return true;
  }

  // ********  Common to all plot types **********************
  Prod.clear();
  dialog.plottype.clear();
  dialog.priority.clear();

  // Sliders and LCD-numbers
  dialog.density.minValue = 5;
  dialog.density.maxValue = 25;
  dialog.density.value = 10;
  dialog.density.scale = 0.1;

  dialog.size.minValue = 1;
  dialog.size.maxValue = 35;
  dialog.size.value = 10;
  dialog.size.scale = 0.1;

  //timer
  dialog.timediff.minValue = 0;
  dialog.timediff.maxValue = 48;
  //   dialog.timediff.value = 1;
  //   dialog.timediff.scale = 15.;

  dialog.defValues="colour=black devcolour1=red devcolour2=blue";


  // defaults for all observation types
  map<miString,ProdInfo> defProd;
  miString parameter;
  defProd["synop"].obsformat= ofmt_synop;
  defProd["synop"].timeRangeMin=-30;
  defProd["synop"].timeRangeMax= 30;
  defProd["synop"].synoptic= true;
  parameter= "Wind,TTT,TdTdTd,PPPP,ppp,a,h,VV,N,RRR,ww,W1,W2,Nh,Cl,Cm,Ch,vs,ds,TwTwTw,PwaHwa,dw1dw1,Pw1Hw1,TxTn,sss,911ff,s,fxfx,Id,Name,St.no(3),St.no(5),Pos,dd,ff,T_red,Date,Time,Height,Zone,RRR_1,RRR_6,RRR_12,RRR_24,quality";
  defProd["synop"].parameter= parameter.split(",");
  defProd["aireps"].obsformat= ofmt_aireps;
  defProd["aireps"].timeRangeMin=-30;
  defProd["aireps"].timeRangeMax= 30;
  defProd["aireps"].synoptic= false;
  parameter= "Pos,dd,ff,Wind,TTT,TdTdTd,PPPP,Id,Date,Time,HHH";
  defProd["aireps"].parameter= parameter.split(",");
  defProd["satob"].obsformat= ofmt_satob;
  defProd["satob"].timeRangeMin=-180;
  defProd["satob"].timeRangeMax= 180;
  defProd["satob"].synoptic= false;
  parameter= "Pos,dd,ff,Wind,Id,Date,Time";
  defProd["satob"].parameter= parameter.split(",");
  defProd["dribu"].obsformat= ofmt_dribu;
  defProd["dribu"].timeRangeMin=-90;
  defProd["dribu"].timeRangeMax= 90;
  defProd["dribu"].synoptic= false;
  parameter= "Pos,dd,ff,Wind,TTT,TdTdTd,PPPP,ppp,a,TwTwTw,Id,Date,Time";
  defProd["dribu"].parameter= parameter.split(",");
  defProd["temp"].obsformat= ofmt_temp;
  defProd["temp"].timeRangeMin=-30;
  defProd["temp"].timeRangeMax= 30;
  defProd["temp"].synoptic= false;
  parameter= "Pos,dd,ff,Wind,TTT,TdTdTd,PPPP,Id,Date,Time,HHH,QI,QI_NM,QI_RFF";
  defProd["temp"].parameter= parameter.split(",");
  defProd["ocea"].obsformat= ofmt_ocea;
  defProd["ocea"].timeRangeMin=-180;
  defProd["ocea"].timeRangeMax= 180;
  defProd["ocea"].synoptic= true;
  defProd["tide"].obsformat= ofmt_tide;
  defProd["tide"].timeRangeMin=-180;
  defProd["tide"].timeRangeMax= 180;
  defProd["tide"].synoptic= true;
  defProd["pilot"].obsformat= ofmt_pilot;
  defProd["pilot"].timeRangeMin=-30;
  defProd["pilot"].timeRangeMax= 30;
  defProd["pilot"].synoptic= false;
  parameter= "Pos,dd,ff,Wind,TTT,TdTdTd,PPPP,Id,Date,Time,HHH";
  defProd["pilot"].parameter= parameter.split(",");
  defProd["metar"].obsformat= ofmt_metar;
  defProd["metar"].timeRangeMin=-15;
  defProd["metar"].timeRangeMax= 15;
  defProd["metar"].synoptic= true;
  parameter= "Pos,dd,ff,Wind,dndx,fmfm,TTT,TdTdTd,ww,REww,VVVV/Dv,VxVxVxVx/Dvx,Clouds,PHPHPHPH,Id,Date,Time";
  defProd["metar"].parameter= parameter.split(",");
  defProd["ascii"].obsformat= ofmt_ascii;
  defProd["ascii"].timeRangeMin=-180;
  defProd["ascii"].timeRangeMax= 180;
  defProd["ascii"].synoptic= false;
  defProd["hqc"].obsformat= ofmt_hqc;
  defProd["hqc"].timeRangeMin=-180;
  defProd["hqc"].timeRangeMax= 180;
  defProd["hqc"].synoptic= false;
  defProd["url"].obsformat= ofmt_url;
#ifdef ROADOBS
  defProd["roadobs"].obsformat= ofmt_roadobs;
  defProd["roadobs"].timeRangeMin=-180;
  defProd["roadobs"].timeRangeMax= 180;
  defProd["roadobs"].synoptic= true;
#endif

  miString format,prod,dialogName;
  miString file;
  miString key,value;
  bool newprod= true;

  for(unsigned int i=0; i<sect_obs.size(); i++) {

    vector<miString> token = sect_obs[i].split('"','"',"=",true);

    if(token.size() != 2){
      miString errmsg="Line must contain '='";
      SetupParser::errorMsg(obs_name,i,errmsg);
      continue;
    }

    key = token[0].downcase();
    value = token[0];
    if( key == "prod" ){
      vector<miString> stoken= token[1].split(":");
      miString plotFormat;
      if (stoken.size()==1) {
        format=     stoken[0].downcase();
        dialogName= stoken[0];
        prod=       stoken[0].downcase();
        if (format=="qscat") format= "satob";  // remove this later !!!!
      } else {
        format=     stoken[0].downcase();
        dialogName= stoken[1];
        prod=       stoken[1].downcase();
        if (stoken.size()==3)
          plotFormat=       stoken[2].downcase();
      }
      newprod= (Prod.find(prod)==Prod.end());
      if (newprod) {
        if (defProd.find(format)==defProd.end()) {
          miString errmsg="Unknown obs data format: " + format;
          SetupParser::errorMsg(obs_name,i,errmsg);
          continue;
        }
        Prod[prod]= defProd[format];
        Prod[prod].current=-1;
        Prod[prod].dialogName= dialogName;
        Prod[prod].useFileTime= false;
        //Prod[prod].noTime= false;
      }
#ifdef ROADOBS
      if (Prod[prod].obsformat == ofmt_roadobs) {
	Prod[prod].plotFormat= "roadobs";
      }
      else {
	Prod[prod].plotFormat= plotFormat;
      }
#else
      Prod[prod].plotFormat= plotFormat;
#endif
    } else if( key == "file"
        || key == "archivefile"
        || key == "url"
        || key == "ascii"
        || key == "archive_ascii"
#ifdef METNOOBS
	       || key == "metnoobs"
	       || key == "archive_metnoobs"
#endif
#ifdef BUFROBS
	       || key == "bufr"
	       || key == "archive_bufr"
#endif
#ifdef ROADOBS
	       || key == "roadobs"
	       || key == "archive_roadobs"
#endif
    ){
      if(prod.empty() ){
        miString errmsg="You must give prod before file";
        SetupParser::errorMsg(obs_name,i,errmsg);
        continue;
      }
      token[1].remove('\"');
      TimeFilter tf;
      // init time filter and replace yyyy etc. with *
      tf.initFilter(token[1]);
      patternInfo pf;
      pf.filter=tf;
      pf.pattern=token[1];
      // obsolete
      if(key == "file" || key == "archivefile") //obsolete
        if(Prod[prod].obsformat == ofmt_ascii)
          pf.fileType="ascii";
        else
          pf.fileType="metnoobs";
      else if(key == "metnoobs" || key == "archive_metnoobs")
        pf.fileType="metnoobs";
      else if(key == "ascii" || key == "archive_ascii")
        pf.fileType="ascii";
      else if(key == "bufr" || key == "archive_bufr")
        pf.fileType="bufr";
      else if(key == "url") {
        pf.fileType="url";
      }
#ifdef ROADOBS
      else if(key == "roadobs" || key == "archive_roadobs")
	pf.fileType="roadobs";
#endif
      if( key.contains("archive") )
        pf.archive = true;
      else
        pf.archive = false;
      // Ascii files without timefilter has no time
      if (Prod[prod].obsformat==ofmt_ascii && !pf.filter.ok()) {
        Prod[prod].timeInfo="notime";
      }
      Prod[prod].pattern.push_back(pf);
    } else if( key == "headerfile"){
      if(prod.empty() ){
        miString errmsg="You must give prod before file/headerfile";
        SetupParser::errorMsg(obs_name,i,errmsg);
        continue;
      }
      Prod[prod].headerfile= token[1];
    }
#ifdef ROADOBS
    else if( key == "databasefile"){
      if(prod.empty() ){
	miString errmsg="You must give prod before file/databasefile";
	SetupParser::errorMsg(obs_name,i,errmsg);
	continue;
      }
      Prod[prod].databasefile= token[1];
    }else if( key == "stationfile"){
      if(prod.empty() ){
	miString errmsg="You must give prod before file/stationfile";
	SetupParser::errorMsg(obs_name,i,errmsg);
	continue;
      }
      Prod[prod].stationfile= token[1];
    }else if( key == "daysback"){
      if(prod.empty() ){
	miString errmsg="You must give prod before file/stationfile";
	SetupParser::errorMsg(obs_name,i,errmsg);
	continue;
      }
      Prod[prod].daysback= token[1].toInt();
    }
#endif
    else if( key == "timerange" && newprod ){
      vector<miString> time = token[1].split(",");
      if(time.size()==2){
        Prod[prod].timeRangeMin=atoi(time[0].c_str());
        Prod[prod].timeRangeMax=atoi(time[1].c_str());
      }
    } else if( key == "synoptic" && newprod ){
      if(token[1]=="true")
        Prod[prod].synoptic=true;
      else
        Prod[prod].synoptic=false;
    } else if( key == "current" && newprod ){
      Prod[prod].current=atof(token[1].c_str());
    } else if( key == "headerinfo" && newprod ){
      Prod[prod].headerinfo.push_back(token[1]);
    } else if( key == "metadata" && newprod ){
      Prod[prod].metaData = token[1];
    } else if( key == "timeinfo" && newprod ){
      Prod[prod].timeInfo = token[1];
      Prod[prod].timeInfo.remove('"');

    }
#ifdef DEBUGPRINT
  printProdInfo(Prod[prod]);
#endif
  }
  // *******  Priority List ********************

  const miString key_name= "name";
  const miString key_file= "file";

  vector<miString> tokens,stokens;
  miString name;
  ObsDialogInfo::PriorityList pri;

  dialog.priority.clear();

  const miString pri_name = "OBSERVATION_PRIORITY_LISTS";
  vector<miString> sect_pri;

  if (SetupParser::getSection(pri_name,sect_pri)){

    for (unsigned int i=0; i<sect_pri.size(); i++){
      name= "";
      file= "";

      tokens= sect_pri[i].split('"','"'," ",true);
      for (unsigned int j=0; j<tokens.size(); j++){
        stokens= tokens[j].split('=');
        if (stokens.size()>1) {
          key= stokens[0].downcase();
          value= stokens[1];
          value.remove('"');

          if (key==key_name){
            name= value;
          } else if (key==key_file){
            file= value;
          }
        }
      }

      if (name.exists() && file.exists()){
        pri.name= name;
        pri.file= file;
        dialog.priority.push_back(pri);
      } else {
        SetupParser::errorMsg(pri_name,i,"Incomplete observation priority specification");
        continue;
      }
    }
  }

  // *********  Criteria  **************

  const miString obs_crit_name = "OBSERVATION_CRITERIA";
  vector<miString> sect_obs_crit;


  if (SetupParser::getSection(obs_crit_name,sect_obs_crit)){

    int ncrit=sect_obs_crit.size();

    ObsDialogInfo::CriteriaList critList;
    miString plottype;
    for(int i=0; i<ncrit; i++) {
      vector<miString> token = sect_obs_crit[i].split("=");
      if(token.size() == 2  && token[0].downcase()=="plottype"){
        if(critList.criteria.size()){
          criteriaList[plottype].push_back(critList);
          critList.criteria.clear();
        }
        plottype=token[1].downcase();
      } else if(token.size() == 2  && token[0].downcase()=="name"){
        if(critList.criteria.size()){
          criteriaList[plottype].push_back(critList);
          critList.criteria.clear();
        }
        critList.name = token[1];
      } else {
        critList.criteria.push_back(sect_obs_crit[i]);
      }
    }
    if(critList.criteria.size())
      criteriaList[plottype].push_back(critList);
  }


  return true;
}

bool ObsManager::initHqcdata(int from,
    const string& commondesc, const string& common,
    const string& desc, const std::vector<std::string>& data)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(commondesc) << LOGVAL(common) << LOGVAL(desc));

  //   if(desc == "remove" && from != hqc_from)
  //     return false;

  if (common == "remove") {
    hqcdata.clear();
    hqc_synop_parameter.clear();
    return true;
  }

  if (desc == "time") {
    int n = data.size();

    Prod["hqc_synop"].fileInfo.clear();
    Prod["hqc_list"].fileInfo.clear();

    for (int i=0; i<n; i++) {
      METLIBS_LOG_DEBUG("time = " << data[i]);
      FileInfo finfo;
      finfo.time = miTime(data[i]);
      Prod["hqc_synop"].fileInfo.push_back(finfo);
      Prod["hqc_list"].fileInfo.push_back(finfo);
    }
    return true;
  }


  hqc_from = from;
  const std::vector<std::string> descstr = split_on_comma(commondesc), commonstr = split_on_comma(common);
  if(commonstr.size() != descstr.size()){
    METLIBS_LOG_ERROR("ObsManager::initHqcdata: different size of commondesc and common");
    return false;
  }

  for (size_t i=0; i<descstr.size(); i++){
    const std::string value = miutil::to_lower(descstr[i]);
    if (value == "time") {
      hqcTime = miTime(commonstr[i]);
    }
  }

  hqcdata.clear();
  hqc_synop_parameter = split_on_comma(desc);
  METLIBS_LOG_DEBUG(LOGVAL(desc) << LOGVAL(hqc_synop_parameter.size()));
  BOOST_FOREACH(const std::string& d, data) {
    const std::vector<std::string> tokens = split_on_comma(d);

    ObsData obsd;
    if (!changeHqcdata(obsd, hqc_synop_parameter, tokens))
      return false;

    hqcdata.push_back(obsd);
  }
  return true;
}

bool ObsManager::sendHqcdata(ObsPlot* oplot)
{
  //  METLIBS_LOG_DEBUG("sendHqcData");
  oplot->addObsVector(hqcdata);
  oplot->setSelectedStation(selectedStation);
  oplot->setHqcFlag(hqcFlag);
  //  oplot->flaginfo=true;
  oplot->changeParamColour(hqcFlag_old,false);
  oplot->changeParamColour(hqcFlag,true);
  if(oplot->setData()){
    miString time=hqcTime.format("%D %H%M");
    miString anno="Hqc " + time;
    oplot->setObsAnnotation(anno);
    oplot->setPlotName(anno);
    return true;
  }
  return false;
}

Colour ObsManager::flag2colour(const miString& flag)
{
  Colour col;

  if (flag.contains("7") || flag.contains("8") || flag.contains("9")) {
    col = Colour("red");
  } else if(flag.contains("0") || flag.contains("2") || flag.contains("3") ||
      flag.contains("4") || flag.contains("5") || flag.contains("6"))
  {
    col = Colour("gulbrun");
  } else {
    col = Colour("green3");
  }

  return col;
}

bool ObsManager::updateHqcdata(const string& commondesc, const string& common,
    const string& desc, const vector<string>& data)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(commondesc) << LOGVAL(common) << LOGVAL(desc));

  const std::vector<std::string> descstr = split_on_comma(commondesc), commonstr = split_on_comma(common);
  if (commonstr.size() != descstr.size()) {
    METLIBS_LOG_ERROR("different size of commondesc and common");
    return false;
  }

  for (size_t i=0; i<descstr.size(); i++) {
    const std::string value = boost::algorithm::to_lower_copy(descstr[i]);
    if (value == "time") {
      const miutil::miTime t(commonstr[i]);
      hqcTime = t;
      //  if( t != hqcTime ) return false; //time doesn't match
    }
  }

  const std::vector<std::string> param = split_on_comma(desc);
  if (param.size() < 2)
    return false;
  
  BOOST_FOREACH(const std::string& d, data) {
    METLIBS_LOG_DEBUG(LOGVAL(d));
    
    const std::vector<std::string> datastr = split_on_comma(d);

    // find station
    size_t i;
    for (i=0; i<hqcdata.size() && hqcdata[i].id != datastr[0]; ++i) { }
    if (i == hqcdata.size())
      continue; // station not found
    METLIBS_LOG_DEBUG(LOGVAL(i) << LOGVAL(datastr[0]));

    if (!changeHqcdata(hqcdata[i], param, datastr))
      return false;
  }
  return true;
}

bool ObsManager::changeHqcdata(ObsData& odata, const vector<string>& param, const vector<string>& data)
{
  METLIBS_LOG_SCOPE();
  if (param.size() != data.size()) {
    METLIBS_LOG_ERROR("No. of parameters: "<<param.size()<<" != no. of data: " <<data.size());
    return false;
  }

  for(unsigned int i=0; i<param.size(); i++) {
    const std::string& key = param[i];
    std::string value = miutil::to_lower(data[i]);
    
    if (key == "id") {
      odata.id = data[i]; // no lower case!
    } else if (key == "lon") {
      odata.xpos = miutil::to_double(value);
    } else if(key == "lat") {
      odata.ypos = miutil::to_double(value);
    } else if (key == "auto") {
      if (value == "a")
        odata.fdata["ix"] = 4;
      else if (value == "n")
        odata.fdata["ix"] = -1;
    } else if (key == "St.type") {
      if (value != "none" && value != "")
        odata.dataType = value;
    } else {
      const std::vector<std::string> vstr = split_on_comma(data[i], ";");
      if (vstr.empty())
        continue;
      
      value = vstr[0];
      const float fvalue = miutil::to_double(value);
      if (vstr.size() >= 2) {
        odata.flag[key] = vstr[1];
        if (vstr.size() == 3)
          odata.flagColour[key] = Colour(vstr[2]);
      }
      
      const char* simple_keys[] =  {
        "h", "VV", "N", "dd", "ff", "TTT", "TdTdTd", "PPPP", "ppp", "RRR", "Rt", "ww", "Nh",
        "TwTwTw", "PwaPwa", "HwaHwa", "Pw1Pw1", "Hw1Hw1", "s", "fxfx", "TxTn", "sss"
      };
      if (std::find(simple_keys, boost::end(simple_keys), key) != boost::end(simple_keys)) {
        odata.fdata[key] = fvalue;
        continue;
      }

      if (key == "a") {
        if (fvalue >= 0 && fvalue < 10)
          odata.fdata["a"] = fvalue;
        // FIXME else { do not keep old value }
      } else if (key == "W1" or key == "W2") {
        if (fvalue > 2 && fvalue < 10)
          odata.fdata[key] = fvalue;
        // FIXME else { do not keep old value }
      } else if (key == "Cl" or key == "Cm" or key == "Ch") {
        if (fvalue > 0 && fvalue < 10)
          odata.fdata[key] = fvalue;
        // FIXME else { do not keep old value }
      } else if(key == "vs") {
        if (fvalue >= 0 && fvalue < 10)
          odata.fdata[key] = fvalue;
        // FIXME else { do not keep old value }
      } else if(key == "ds") {
        if (fvalue > 0 && fvalue < 9)
          odata.fdata[key] = fvalue;
        // FIXME else { do not keep old value }
      } else if(key == "dw1dw1") {
        if (fvalue > 0 && fvalue < 37)
          odata.fdata[key] = fvalue;
        // FIXME else { do not keep old value }
      } else if(key == "TxTxTx" or key == "TnTnTn") {
        odata.fdata["TxTn"] = fvalue;;
      } else if (key == "911ff" or key == "ff_911") {
        odata.fdata["ff_911"] = fvalue;;
      } else {
        METLIBS_LOG_INFO("unknown key '" << key << '\'');
      }
    }
  }
  return true;
}


void ObsManager::processHqcCommand(const std::string& command, const std::string& str)
{
  if (command == "remove") {
    hqcdata.clear();
    hqc_synop_parameter.clear();
  } else if (command == "flag") {
    hqcFlag_old = hqcFlag;
    hqcFlag = str;
    //    hqcFlag = str.downcase();
  } else if (command == "station") {
    const std::vector<std::string> vstr = split_on_comma(str);
    if (not vstr.empty())
      selectedStation = vstr[0];
  }
}

map<miutil::miString,ObsManager::ProdInfo> ObsManager::getProductsInfo() const
{
  return Prod;
}
