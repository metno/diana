/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diVprofManager.h 4551 2014-10-21 13:10:06Z lisbethb $

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
#ifndef VPROFMANAGER_H
#define VPROFMANAGER_H

#include <diCommonTypes.h>
#include <diPrintOptions.h>
#include <diField/TimeFilter.h>

#include "vcross_v2/VcrossSetup.h"

#include <puTools/miTime.h>

#include <vector>
#include <map>
#include <set>

class VprofOptions;
class VprofData;
class VprofDiagram;

/**
   \brief Managing Vertical Profile observation and prognostic sources.
   \brief If we use observations from road, the filename och filepath
   \brief will not be used.
*/
class VprofManager{

private:

  enum obsType {
    temp,
    amdar,
    pilot
  };

  enum FileFormat {
    metnoobs,
#ifdef ROADOBS
    bufr,
    roadobs
#else
    bufr
#endif
  };

  struct ObsFile {
    std::string   filename;
    obsType    obstype;
    FileFormat fileformat;
    miutil::miTime     time;
    long       modificationTime;
#ifdef ROADOBS
    std::string parameterfile;
    std::string stationfile;
    std::string databasefile;
#endif
  };

  struct ObsFilePath {
    std::string   filepath;
    obsType    obstype;
    FileFormat fileformat;
    TimeFilter tf;
#ifdef ROADOBS
    std::string parameterfile;
    std::string stationfile;
    std::string databasefile;
#endif

  };

  struct SelectedModel{
    std::string model;
    std::string reftime;
  };

  // std::map<model,filename>
  std::map<std::string,std::string> filenames;
  std::map<std::string,std::string> stationsfilenames;
  std::map<std::string,std::string> filetypes;
  std::vector<std::string> computations;

  // for use in dialog (unique lists in setup order)
  std::vector<std::string> dialogModelNames;
  std::vector<std::string> dialogFileNames;
  std::vector<ObsFilePath>   filePaths;

  vcross::Setup_p setup;

  std::string amdarStationFile;
  bool amdarStationList;
  std::vector<float>    amdarLatitude;
  std::vector<float>    amdarLongitude;
  std::vector<std::string> amdarName;

  VprofOptions *vpopt;
  VprofDiagram *vpdiag;
  std::vector<VprofData*> vpdata;
  bool showObs;
  bool showObsTemp;
  bool showObsPilot;
  bool showObsAmdar;

  std::vector <std::string> nameList;
  std::vector <float>    latitudeList;
  std::vector <float>    longitudeList;
  std::vector <miutil::miTime>   timeList;
  std::vector <std::string> obsList;

  std::vector<ObsFile> obsfiles;
  std::vector<miutil::miTime> obsTime;
  bool onlyObs;

  std::vector<std::string> fieldModels;
  std::vector<SelectedModel> selectedModels;

  int plotw, ploth;

  std::vector<std::string> plotStations;
  std::vector<std::string> selectedStations;
  std::string lastStation;
  miutil::miTime   plotTime;
  miutil::miTime ztime;

  bool hardcopy;
  printOptions printoptions;
  bool hardcopystarted;

  std::map<std::string,std::string> menuConst;

  std::string getDefaultModel();
  void updateObsFileList();
  bool initVprofData(const SelectedModel& selectedModel);
  void initStations();
  void initTimes();
  void checkObsTime(int hour=-1);

  void renameAmdar(std::vector<std::string>& namelist,
		   std::vector<float>& latitudelist,
		   std::vector<float>& longitudelist,
		   std::vector<std::string>& obslist,
		   std::vector<miutil::miTime>& tlist,
		   std::map<std::string,int>& amdarCount);
  void readAmdarStationList();

public:
  // constructor
  VprofManager();
  // destructor
  ~VprofManager();

  // clenans up vhen user closes the vprof window
  void cleanup();
  // parseSetup and init obsfilelist
  void init();

  VprofOptions* getOptions() { return vpopt; }
  void setPlotWindow(int w, int h);

  void parseSetup();
  void setModel();
  void setStation(const std::string& station);
  void setStations(const std::vector<std::string>& station);
  void setTime(const miutil::miTime& time);
  std::string setStation(int step);
  miutil::miTime setTime(int step, int dir);
  const miutil::miTime getTime(){return plotTime;}
  const std::vector<std::string> getStations(){return selectedStations;}
  const std::string getLastStation(){return lastStation;}
  const std::vector<std::string>& getStationList() { return nameList; }
  const std::vector<float>& getLatitudes(){return latitudeList;}
  const std::vector<float>& getLongitudes(){return longitudeList;}
  const std::vector<miutil::miTime>&   getTimeList()    { return timeList; }
  std::vector <std::string> getModelNames();
  std::vector <std::string> getModelFiles(){return dialogFileNames;}
  std::vector <std::string> getReferencetimes(const std::string model);
  void setFieldModels(const std::vector<std::string>& fieldmodels);
  void setSelectedModels(const std::vector<std::string>& models, bool obs=false);

  bool plot();
  void startHardcopy(const printOptions& po);
  void endHardcopy();
  bool onlyObsState() { return onlyObs; }
  void mainWindowTimeChanged(const miutil::miTime& time);
  void updateObs();
  std::string getAnnotationString();

  std::vector<std::string> writeLog();
  void readLog(const std::vector<std::string>& vstr,
	       const std::string& thisVersion, const std::string& logVersion);
  /* Added for debug purposes */
  void printObsFiles(const ObsFile &of);
  void printObsFilePath(const ObsFilePath & ofp);
};

#endif
