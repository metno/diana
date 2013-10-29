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
    miutil::miString   filename;
    obsType    obstype;
    FileFormat fileformat;
    miutil::miTime     time;
    long       modificationTime;
#ifdef ROADOBS
    miutil::miString parameterfile;
    miutil::miString stationfile;
    miutil::miString databasefile;
#endif
  };

  struct ObsFilePath {
    miutil::miString   filepath;
    obsType    obstype;
    FileFormat fileformat;
    TimeFilter tf;
#ifdef ROADOBS
    miutil::miString parameterfile;
    miutil::miString stationfile;
    miutil::miString databasefile;
#endif

  };

  // map<model,filename>
  map<miutil::miString,miutil::miString> filenames;
  map<miutil::miString,miutil::miString> filetypes;

  // for use in dialog (unique lists in setup order)
  vector<miutil::miString> dialogModelNames;
  vector<miutil::miString> dialogFileNames;

  vector<ObsFilePath>   filePaths;

  miutil::miString amdarStationFile;
  bool amdarStationList;
  vector<float>    amdarLatitude;
  vector<float>    amdarLongitude;
  vector<miutil::miString> amdarName;

  FieldManager *fieldm;   // field manager

  VprofOptions *vpopt;
  VprofDiagram *vpdiag;
  vector<VprofData*> vpdata;
  bool showObs;
  bool showObsTemp;
  bool showObsPilot;
  bool showObsAmdar;
  bool asField;

  vector <miutil::miString> nameList;
  vector <float>    latitudeList;
  vector <float>    longitudeList;
  vector <miutil::miTime>   timeList;
  vector <miutil::miString> obsList;

  vector<ObsFile> obsfiles;
  vector<miutil::miTime> obsTime;
  bool onlyObs;

  std::vector<std::string> fieldModels;
  std::vector<std::string> selectedModels;
  std::vector<std::string> selectedFiles;
  std::set<std::string> usemodels;

  int plotw, ploth;

  miutil::miString plotStation;
  miutil::miString lastStation;
  miutil::miTime   plotTime;
  miutil::miTime ztime;

  bool hardcopy;
  printOptions printoptions;
  bool hardcopystarted;

  map<miutil::miString,miutil::miString> menuConst;

  miutil::miString getDefaultModel();
  void updateObsFileList();
  bool initVprofData(miutil::miString file,miutil::miString model);
  void initStations();
  void initTimes();
  void checkObsTime(int hour=-1);

  void renameAmdar(vector<miutil::miString>& namelist,
		   vector<float>& latitudelist,
		   vector<float>& longitudelist,
		   vector<miutil::miString>& obslist,
		   vector<miutil::miTime>& tlist,
		   map<miutil::miString,int>& amdarCount);
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
  void setStation(const miutil::miString& station);
  void setTime(const miutil::miTime& time);
  miutil::miString setStation(int step);
  miutil::miTime setTime(int step);
  const miutil::miTime getTime(){return plotTime;}
  const miutil::miString getStation(){return plotStation;}
  const miutil::miString getLastStation(){return lastStation;}
  const vector<miutil::miString>& getStationList() { return nameList; }
  const vector<float>& getLatitudes(){return latitudeList;}
  const vector<float>& getLongitudes(){return longitudeList;}
  const vector<miutil::miTime>&   getTimeList()    { return timeList; }
  vector <miutil::miString> getModelNames();
  vector <miutil::miString> getModelFiles();
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
  miutil::miString getAnnotationString();

  void setMenuConst(map<miutil::miString,miutil::miString> mc)
  { menuConst = mc;}

  std::vector<std::string> writeLog();
  void readLog(const std::vector<std::string>& vstr,
	       const std::string& thisVersion, const std::string& logVersion);
  /* Added for debug purposes */
  void printObsFiles(const ObsFile &of);
  void printObsFilePath(const ObsFilePath & ofp);

};

#endif
