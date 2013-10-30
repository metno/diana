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
#ifndef SPECTRUMMANAGER_H
#define SPECTRUMMANAGER_H

#include <puTools/miString.h>
#include <puTools/miTime.h>
#include <vector>
#include <map>
#include <set>
#include <diCommonTypes.h>
#include <diPrintOptions.h>

using namespace std;

class SpectrumOptions;
class SpectrumFile;
class SpectrumPlot;


/**
   \brief Managing Wave Spectrum data sources and plotting

*/
class SpectrumManager{

private:

  enum obsType {
    obsAAA,
    obsBBB
  };

  struct ObsFile {
    std::string filename;
    obsType  obstype;
    miutil::miTime   time;
    long     modificationTime;
  };

  struct StationPos {
    float latitude;
    float longitude;
    std::string obs;
  };

  // map<model,filename>
  map<std::string,std::string> filenames;

  // for use in dialog (unique lists in setup order)
  vector<std::string> dialogModelNames;
  vector<std::string> dialogFileNames;

  // temp file paths  ("/dir/xxxx??.dat*")
  vector<std::string> obsAaaPaths;
  // pilot file paths ("/dir/xxxx??.dat*")
  vector<std::string> obsBbbPaths;

  SpectrumOptions *spopt;
  vector<SpectrumFile*> spfile;
  bool showObs;
  bool asField;

  vector<std::string> nameList;
  vector<float> latitudeList;
  vector<float> longitudeList;
  vector<std::string> obsList;
  vector<miutil::miTime>   timeList;

  vector<ObsFile> obsfiles;
  vector<miutil::miTime> obsTime;
  bool onlyObs;

  std::vector<std::string> fieldModels;
  std::vector<std::string> selectedModels;
  std::vector<std::string> selectedFiles;
  set<std::string> usemodels;

  int plotw, ploth;

  std::string plotStation;
  std::string lastStation;
  miutil::miTime   plotTime;
  miutil::miTime   ztime;

  bool dataChange;
  vector<SpectrumPlot*> spectrumplots;

  bool hardcopy;
  printOptions printoptions;
  bool hardcopystarted;

  map<std::string,std::string> menuConst;

  std::string getDefaultModel();
  void updateObsFileList();
  bool initSpectrumFile(std::string file,std::string model);
  void initStations();
  void initTimes();
  void checkObsTime(int hour=-1);
  void preparePlot();

public:
  // constructor
  SpectrumManager();
  // destructor
  ~SpectrumManager();

  void parseSetup();
  SpectrumOptions* getOptions() { return spopt; }
  void setPlotWindow(int w, int h);

  //routines from controller
  vector<std::string> getLineThickness();

  void setModel();
  void setStation(const std::string& station);
  void setTime(const miutil::miTime& time);
  std::string setStation(int step);
  miutil::miTime setTime(int step);

  const miutil::miTime getTime(){return plotTime;}
  const std::string getStation() { return plotStation; }
  const std::string getLastStation() { return lastStation; }
  const vector<std::string>& getStationList() { return nameList; }
  const vector<float>& getLatitudes() { return latitudeList; }
  const vector<float>& getLongitudes() { return longitudeList; }
  const vector<miutil::miTime>& getTimeList() { return timeList; }

  vector<std::string> getModelNames();
  vector<std::string> getModelFiles();
  void setFieldModels(const std::vector<std::string>& fieldmodels);
  void setSelectedModels(const std::vector<std::string>& models, bool obs, bool field);
  void setSelectedFiles(const std::vector<std::string>& files, bool obs, bool field);

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

};


#endif
