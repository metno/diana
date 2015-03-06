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

#include "diCommonTypes.h"
#include "diPlot.h"
#include "diObsData.h"

#include <diField/TimeFilter.h>

#include <string>
#include <set>
#include <vector>

class ObsData;
class ObsMetaData;
class ObsPlot;
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
    std::string pattern;
    bool archive;
    std::string fileType;  //bufr,miobs...
  };

  struct FileInfo {
    std::string filename;
    miutil::miTime   time;
    std::string filetype; //bufr,miobs...
  };

  struct ProdInfo {
    ObsFormat obsformat;
    std::string dialogName; // mixedcase, always Prod[lowercase]
    std::string plotFormat;
    std::vector<patternInfo> pattern;
    std::vector<FileInfo> fileInfo;
    std::vector<std::string> headerinfo;
    std::string timeInfo;// noTime or timeInfo from setup: from=;to=;interval
    int timeRangeMin;
    int timeRangeMax;
    float current;
    bool synoptic;
    std::string headerfile;
    std::string metaData;
#ifdef ROADOBS
    std::string stationfile;
    std::string databasefile;
    int daysback;
#endif
    bool useFileTime;
    std::vector<std::string> parameter;
  };

private:
  typedef std::map<std::string, ProdInfo> string_ProdInfo_m;
  string_ProdInfo_m defProd;
  string_ProdInfo_m Prod;
  typedef std::map<std::string, ObsMetaData*> string_ObsMetaData_m;
  string_ObsMetaData_m metaDataMap;
  ObsDialogInfo dialog;
  std::vector<ObsDialogInfo::PriorityList> priority;
  //one  criterialist pr plot type
  std::map<std::string, std::vector<ObsDialogInfo::CriteriaList> > criteriaList;

  //  set<std::string> dataTypesListed;

  std::vector<std::string> popupSpec;  // Parameter data from setupfil

  bool useArchive; //read archive files too.
  bool mslp;
  ObsPositions obsPositions;

  //HQC - perhaps its own class?
  std::vector<ObsData> hqcdata;
  std::vector<std::string> hqc_synop_parameter;
  miutil::miTime hqcTime;
  std::string hqcFlag;
  std::string hqcFlag_old;
  int hqc_from;
  std::string selectedStation;
  //--------------------------------------

  bool addStationsAndTimeFromMetaData( const std::string& metaData,
      std::string& url, const miutil::miTime& time);
  ObsDialogInfo::Button addButton(const std::string& name, const std::string& tip,
      int low=-50, int high=50, bool def=true);
  void addType(ObsDialogInfo::PlotType& dialogInfo,
      const std::vector<ObsFormat>& obsformat);
  void setActive(const std::vector<std::string>& name, bool on,
      std::vector<bool>& active,
      const std::vector<ObsDialogInfo::Button>& b);
  void setAllActive(ObsDialogInfo::PlotType& dialogInfo,
      const std::vector<std::string>& parameter,
      const std::string& name,
      const std::vector<ObsDialogInfo::Button>& b);
  std::vector<FileInfo> getFileName(const miutil::miTime& , const ProdInfo& pi,
      std::vector<miutil::miTime>& termin,
      miutil::miTime& timeRangeMin, miutil::miTime& timeRangeMax, bool moretimes, int timeDiff);
  bool updateTimesfromFile(std::string obsType);
  bool updateTimes(std::string obsType);


//  HQC
  bool changeHqcdata(ObsData&, const std::vector<std::string>& param,
      const std::vector<std::string>& data);
  Colour flag2colour(const std::string& flag);

  void printProdInfo(const ProdInfo & pinfo);

public:
  ObsManager();

  //parse PlotInfo
  ObsPlot* createObsPlot(const std::string&);

  //read data
  bool prepare(ObsPlot *,miutil::miTime);
  ObsDialogInfo initDialog(void);
  ObsDialogInfo updateDialog(const std::string& name);
  bool parseSetup();
//return observation times for list of PlotInfo's
  std::vector<miutil::miTime> getTimes(std::vector<std::string> pinfos);
  ///returns union or intersection of plot times from all pinfos
  void getCapabilitiesTime(std::vector<miutil::miTime>& normalTimes,
      int& timediff, const std::string& pinfo);

// return observation times for list of obsTypes
  std::vector<miutil::miTime> getObsTimes(const std::vector<std::string>& obsTypes);
  bool obs_mslp() const
    { return mslp; }
  void updateObsPositions(const std::vector<ObsPlot*> oplot);
  ObsPositions& getObsPositions()
    { return obsPositions; }
  void clearObsPositions();
  void calc_obs_mslp(Plot::PlotOrder porder,
      const std::vector<ObsPlot*>& oplot);
  void archiveMode(bool on)
    { useArchive=on; }

//  HQC
  ObsDialogInfo updateHqcDialog(const std::string& plotType);
  bool initHqcdata(int from,
      const std::string& commondesc, const std::string& common,
      const std::string& desc, const std::vector<std::string>&data);
  bool updateHqcdata(const std::string& commondesc, const std::string& common,
      const std::string& desc, const std::vector<std::string>&data);
  bool sendHqcdata(ObsPlot* oplot);
  void processHqcCommand(const std::string&, const std::string&);
  // Added for automatic updates
  bool timeListChanged;

  std::map<std::string, ProdInfo> getProductsInfo() const;
};

#endif
