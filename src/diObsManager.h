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
#ifndef diObsManager_h
#define diObsManager_h

#include <diObsPlot.h>
#include <diObsMetaData.h>
#include <diField/TimeFilter.h>
#include <diCommonTypes.h>
#include <puTools/miString.h>
#include <set>

class ObsData;
struct ObsDialogInfo;

/**

  \brief Managing observations
  
  - parse setup
  - send plot info strings to ObsPlot objects
  - managing file/time info
  - read data

*/
class ObsManager {

private:

  enum ObsFormat {
#ifdef ROADOBS
    ofmt_roadobs,
#endif
    ofmt_unknown,
    ofmt_synop,
    ofmt_aireps,
    ofmt_satob,
    ofmt_dribu,
    ofmt_temp,
    ofmt_ocea,
    ofmt_tide,
    ofmt_pilot,
    ofmt_metar,
    ofmt_ascii,
    ofmt_hqc,
    ofmt_url
  };

public:
  struct patternInfo {
    TimeFilter filter;
    miutil::miString pattern;
    bool archive;
    miutil::miString fileType;  //bufr,miobs...
  };

  struct FileInfo {
    miutil::miString filename;
    miutil::miTime   time;
    miutil::miString filetype; //bufr,miobs...
  };

  struct ProdInfo {
    ObsFormat obsformat;
    miutil::miString dialogName; // mixedcase, always Prod[lowercase]
    miutil::miString plotFormat;
    vector<patternInfo> pattern;
    vector<FileInfo> fileInfo;
    vector<miutil::miString> headerinfo;
    miutil::miString timeInfo;// noTime or timeInfo from setup: from=;to=;interval
    int timeRangeMin;
    int timeRangeMax;
    float current;
    bool synoptic;
    miutil::miString headerfile;
    miutil::miString metaData;
#ifdef ROADOBS
    miutil::miString stationfile;
    miutil::miString databasefile;
    int daysback;
#endif
    bool useFileTime;
    vector<miutil::miString> parameter;
  };

private:
  map<miutil::miString,ProdInfo> Prod;
  map< miutil::miString, ObsMetaData*> metaDataMap;
  ObsDialogInfo dialog;
  vector<ObsDialogInfo::PriorityList> priority;
  //one  criterialist pr plot type
  map<miutil::miString, vector<ObsDialogInfo::CriteriaList> > criteriaList;

  vector<int> levels;

  //  set<miutil::miString> dataTypesListed;


  //Used to find files
  bool firstTry; //false if times can't be found from filename
  vector<miutil::miTime> termin; //list of times from all files used
  miutil::miTime timeRangeMin;
  miutil::miTime timeRangeMax;

  bool useArchive; //read archive files too.
  bool mslp;
  ObsPositions obsPositions;

  //HQC - perhaps its own class?
  vector<ObsData> hqcdata;
  vector<miutil::miString> hqc_synop_parameter;
  miutil::miTime hqcTime;
  miutil::miString hqcFlag;
  miutil::miString hqcFlag_old;
  int hqc_from;
  miutil::miString hqcPlotType;
  miutil::miString hqc_mark;
  miutil::miString selectedStation;
  //  miutil::miString obsdataType;
  //--------------------------------------

  bool addStationsAndTimeFromMetaData( const miutil::miString& metaData, miutil::miString& url, const miutil::miTime& time);
  ObsDialogInfo::Button addButton(const miutil::miString& name, const miutil::miString& tip, 
				  int low=-50, int high=50, bool def=true);
  void addType(ObsDialogInfo::PlotType& dialogInfo,
	       const vector<ObsFormat>& obsformat);
  void setActive(const vector<miutil::miString>& name, bool on, 
		 vector<bool>& active, 
		 const vector<ObsDialogInfo::Button>& b);
void setAllActive(ObsDialogInfo::PlotType& dialogInfo,
		  const vector<miutil::miString>& parameter, 
		  const miutil::miString& name,
		  const vector<ObsDialogInfo::Button>& b);
  void getFileName(vector<FileInfo>& finfo,
		   miutil::miTime& , miutil::miString dataType, ObsPlot*);
  bool updateTimesfromFile(miutil::miString obsType);
  bool updateTimes(miutil::miString obsType);


//  HQC
  bool changeHqcdata(ObsData&, const vector<std::string>& param,
			const vector<std::string>& data);
  Colour flag2colour(const miutil::miString& flag);

  void printProdInfo(const ProdInfo & pinfo);

public:
  ObsManager();

  //parse PlotInfo
  bool init(ObsPlot *, const miutil::miString&);
  //read data
  bool prepare(ObsPlot *,miutil::miTime);
  ObsDialogInfo initDialog(void);
  ObsDialogInfo updateDialog(const miutil::miString& name);
  bool parseSetup();
//return observation times for list of PlotInfo's
  vector<miutil::miTime> getTimes( vector<miutil::miString> pinfos);
  ///returns union or intersection of plot times from all pinfos
  void getCapabilitiesTime(vector<miutil::miTime>& normalTimes,
			   miutil::miTime& constTime,
			   int& timediff,
			   const miutil::miString& pinfo);
// return observation times for list of obsTypes
  vector<miutil::miTime> getObsTimes(const vector<miutil::miString>& obsTypes);
  bool obs_mslp(){return mslp;}
  void updateObsPositions(const vector<ObsPlot*> oplot);
  ObsPositions& getObsPositions(){  return obsPositions;}
  void clearObsPositions();
  void calc_obs_mslp(const vector<ObsPlot*> oplot);
  void archiveMode( bool on ){useArchive=on;}
//  HQC
  ObsDialogInfo updateHqcDialog(const miutil::miString& plotType);
  bool initHqcdata(int from, 
		   const std::string& commondesc,
		   const std::string& common,
		   const std::string& desc,
		   const vector<std::string>&data);
  bool updateHqcdata(const std::string& commondesc, const std::string& common,
		     const std::string& desc, const vector<std::string>&data);
  bool sendHqcdata(ObsPlot* oplot);
  void processHqcCommand(const miutil::miString&, const miutil::miString&);
  // Added for automatic updates
  bool timeListChanged;

  map<miutil::miString,ProdInfo> getProductsInfo() const;
};

#endif
