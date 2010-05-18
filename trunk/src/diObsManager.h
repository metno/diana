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
#include <diTimeFilter.h>
#include <diSetupParser.h>
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
    ofmt_hqc
  };

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
    bool noTime; //files have no time, vector<miutil::miTime> time is empty
    int timeRangeMin;
    int timeRangeMax;
    float current;
    bool synoptic;
    miutil::miString headerfile;
#ifdef ROADOBS
    miutil::miString stationfile;
    miutil::miString databasefile;
    int daysback;
#endif
    bool useFileTime;
    vector<miutil::miString> parameter;
  };

  map<miutil::miString,ProdInfo> Prod;
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
  vector< vector<miutil::miString> > hqcdiffdata;
  vector<miutil::miString> hqc_synop_parameter;
  vector<miutil::miString> hqc_ascii_parameter;
  miutil::miTime hqcTime;
  miutil::miString hqcFlag;
  miutil::miString hqcFlag_old;
  int hqc_from;
  miutil::miString hqcPlotType;
  miutil::miString hqc_mark;
  miutil::miString selectedStation;
  //  miutil::miString obsdataType;
  //--------------------------------------

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
  bool changeHqcdata(ObsData&, const vector<miutil::miString>& param,
			const vector<miutil::miString>& data);
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
  bool parseSetup(SetupParser &);
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
		   const miutil::miString& commondesc, 
		   const miutil::miString& common, 
		   const miutil::miString& desc, 
		   const vector<miutil::miString>&data);
  bool updateHqcdata(const miutil::miString& commondesc, const miutil::miString& common,
		     const miutil::miString& desc, const vector<miutil::miString>&data);
  bool sendHqcdata(ObsPlot* oplot);
  void processHqcCommand(const miutil::miString&, const miutil::miString&);
  // Added for automatic updates
  bool timeListChanged;
};

#endif


