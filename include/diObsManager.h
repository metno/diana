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
#include <miString.h>
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
    miString pattern;
    bool archive;
    miString fileType;  //bufr,miobs...
  };

  struct FileInfo {
    miString filename;
    miTime   time;
    miString filetype; //bufr,miobs...
  };

  struct ProdInfo {
    ObsFormat obsformat;
    miString dialogName; // mixedcase, always Prod[lowercase]
    miString plotFormat;
    vector<patternInfo> pattern;
    vector<FileInfo> fileInfo; 
    bool noTime; //files have no time, vector<miTime> time is empty
    int timeRangeMin;    
    int timeRangeMax;
    float current;
    bool synoptic;
    miString headerfile;
    bool useFileTime;
    vector<miString> parameter;
  };
  
  map<miString,ProdInfo> Prod;
  ObsDialogInfo dialog;
  vector<ObsDialogInfo::PriorityList> priority;
  //one  criterialist pr plot type
  map<miString, vector<ObsDialogInfo::CriteriaList> > criteriaList;

  vector<int> levels;

  //  set<miString> dataTypesListed;


  //Used to find files
  bool firstTry; //false if times can't be found from filename
  vector<miTime> termin; //list of times from all files used
  miTime timeRangeMin;
  miTime timeRangeMax;

  bool useArchive; //read archive files too.
  bool mslp;
  
  //HQC - perhaps its own class?
  vector<ObsData> hqcdata;
  vector< vector<miString> > hqcdiffdata;
  vector<miString> hqc_synop_parameter;
  vector<miString> hqc_ascii_parameter;
  miTime hqcTime;
  miString hqcFlag;
  miString hqcFlag_old;
  int hqc_from;
  miString hqcPlotType;
  miString hqc_mark;
  miString selectedStation;
  //  miString obsdataType;
  //--------------------------------------

  ObsDialogInfo::Button addButton(const miString& name, const miString& tip, 
				  int low=-50, int high=50, bool def=true);
  void addType(ObsDialogInfo::PlotType& dialogInfo,
	       const vector<ObsFormat>& obsformat);
  void setActive(const vector<miString>& name, bool on, 
		 vector<bool>& active, 
		 const vector<ObsDialogInfo::Button>& b);
void setAllActive(ObsDialogInfo::PlotType& dialogInfo,
		  const vector<miString>& parameter, 
		  const miString& name,
		  const vector<ObsDialogInfo::Button>& b);
  void getFileName(vector<FileInfo>& finfo,
		   miTime& , miString dataType, ObsPlot*);
  bool updateTimesfromFile(miString obsType);
  bool updateTimes(miString obsType);


//  HQC
  bool changeHqcdata(ObsData&, const vector<miString>& param,
			const vector<miString>& data);
  Colour flag2colour(const miString& flag);


public:
  ObsManager();

  //parse PlotInfo
  bool init(ObsPlot *, const miString&);
  //read data
  bool prepare(ObsPlot *,miTime);
  ObsDialogInfo initDialog(void);
  ObsDialogInfo updateDialog(const miString& name);
  bool parseSetup(SetupParser &);
//return observation times for list of PlotInfo's
  vector<miTime> getTimes( vector<miString> pinfos);
  ///returns times where time - sat/obs-time < mindiff
  vector<miTime> timeIntersection(const vector<miString>& pinfos,
				  const vector<miTime>& times);
// return observation times for list of obsTypes
  vector<miTime> getObsTimes(const vector<miString>& obsTypes);
  bool obs_mslp(){return mslp;}
  void archiveMode( bool on ){useArchive=on;}
//  HQC
  ObsDialogInfo updateHqcDialog(const miString& plotType);
  bool initHqcdata(int from, 
		   const miString& commondesc, 
		   const miString& common, 
		   const miString& desc, 
		   const vector<miString>&data);
  bool updateHqcdata(const miString& commondesc, const miString& common,
		     const miString& desc, const vector<miString>&data);
  bool sendHqcdata(ObsPlot* oplot);
  void processHqcCommand(const miString&, const miString&);
};

#endif


