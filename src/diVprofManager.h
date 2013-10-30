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
#ifndef VPROFMANAGER_H
#define VPROFMANAGER_H

#include <puTools/miString.h>
#include <puTools/miTime.h>
#include <vector>
#include <map>
#include <set>
#include <diCommonTypes.h>
#include <diPrintOptions.h>
#include <diField/TimeFilter.h>
#include <diController.h>

using namespace std;

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

  // map<model,filename>
  map<std::string,std::string> filenames;
  map<std::string,std::string> filetypes;

  // for use in dialog (unique lists in setup order)
  vector<std::string> dialogModelNames;
  vector<std::string> dialogFileNames;

  vector<ObsFilePath>   filePaths;

  std::string amdarStationFile;
  bool amdarStationList;
  vector<float>    amdarLatitude;
  vector<float>    amdarLongitude;
  vector<std::string> amdarName;

  FieldManager *fieldm;   // field manager

  VprofOptions *vpopt;
  VprofDiagram *vpdiag;
  vector<VprofData*> vpdata;
  bool showObs;
  bool showObsTemp;
  bool showObsPilot;
  bool showObsAmdar;
  bool asField;

  vector <std::string> nameList;
  vector <float>    latitudeList;
  vector <float>    longitudeList;
  vector <miutil::miTime>   timeList;
  vector <std::string> obsList;

  vector<ObsFile> obsfiles;
  vector<miutil::miTime> obsTime;
  bool onlyObs;

  std::vector<std::string> fieldModels;
  std::vector<std::string> selectedModels;
  std::vector<std::string> selectedFiles;
  std::set<std::string> usemodels;

  int plotw, ploth;

  std::string plotStation;
  std::string lastStation;
  miutil::miTime   plotTime;
  miutil::miTime ztime;

  bool hardcopy;
  printOptions printoptions;
  bool hardcopystarted;

  map<std::string,std::string> menuConst;

  std::string getDefaultModel();
  void updateObsFileList();
  bool initVprofData(std::string file,std::string model);
  void initStations();
  void initTimes();
  void checkObsTime(int hour=-1);

  void renameAmdar(vector<std::string>& namelist,
		   vector<float>& latitudelist,
		   vector<float>& longitudelist,
		   vector<std::string>& obslist,
		   vector<miutil::miTime>& tlist,
		   map<std::string,int>& amdarCount);
  void readAmdarStationList();

public:
  // constructor
  VprofManager(Controller *co);
  // destructor
  ~VprofManager();

  VprofOptions* getOptions() { return vpopt; }
  void setPlotWindow(int w, int h);

  void parseSetup();
  void setModel();
  void setStation(const std::string& station);
  void setTime(const miutil::miTime& time);
  std::string setStation(int step);
  miutil::miTime setTime(int step);
  const miutil::miTime getTime(){return plotTime;}
  const std::string getStation(){return plotStation;}
  const std::string getLastStation(){return lastStation;}
  const vector<std::string>& getStationList() { return nameList; }
  const vector<float>& getLatitudes(){return latitudeList;}
  const vector<float>& getLongitudes(){return longitudeList;}
  const vector<miutil::miTime>&   getTimeList()    { return timeList; }
  vector <std::string> getModelNames();
  vector <std::string> getModelFiles();
  void setFieldModels(const std::vector<std::string>& fieldmodels);
  void setSelectedModels(const std::vector<std::string>& models,
			 bool field, bool obsTemp,
			 bool obsPilot, bool obsAmdar);
  void setSelectedFiles(const std::vector<std::string>& files,
			bool field, bool obsTemp,
			bool obsPilot, bool obsAmdar);

  std::vector<std::string> getSelectedModels();
  bool plot();
  void startHardcopy(const printOptions& po);
  void endHardcopy();
  bool onlyObsState() { return onlyObs; }
  void mainWindowTimeChanged(const miutil::miTime& time);
  void updateObs();
  std::string getAnnotationString();

  void setMenuConst(map<std::string,std::string> mc)
  { menuConst = mc;}

  std::vector<std::string> writeLog();
  void readLog(const std::vector<std::string>& vstr,
	       const std::string& thisVersion, const std::string& logVersion);
  /* Added for debug purposes */
  void printObsFiles(const ObsFile &of);
  void printObsFilePath(const ObsFilePath & ofp);

};

#endif
