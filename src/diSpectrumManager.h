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
#include <diSetupParser.h>
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
    miString filename;
    obsType  obstype;
    miTime   time;
    long     modificationTime;
  };

  struct StationPos {
    float latitude;
    float longitude;
    miString obs;
  };

  // map<model,filename>
  map<miString,miString> filenames;

  // for use in dialog (unique lists in setup order)
  vector<miString> dialogModelNames;
  vector<miString> dialogFileNames;

  // temp file paths  ("/dir/xxxx??.dat*")
  vector<miString> obsAaaPaths;
  // pilot file paths ("/dir/xxxx??.dat*")
  vector<miString> obsBbbPaths;

  SetupParser sp;

  SpectrumOptions *spopt;
  vector<SpectrumFile*> spfile;
  bool showObs;
  bool asField;

  vector<miString> nameList;
  vector<float> latitudeList;
  vector<float> longitudeList;
  vector<miString> obsList;
  vector<miTime>   timeList;

  vector<ObsFile> obsfiles;
  vector<miTime> obsTime;
  bool onlyObs;

  vector<miString> fieldModels;
  vector<miString> selectedModels;
  vector<miString> selectedFiles;
  set<miString> usemodels;

  int plotw, ploth;

  miString plotStation;
  miString lastStation;
  miTime   plotTime;
  miTime   ztime;

  bool dataChange;
  vector<SpectrumPlot*> spectrumplots;

  bool hardcopy;
  printOptions printoptions;
  bool hardcopystarted;

  map<miString,miString> menuConst;

  void parseSetup();
  miString getDefaultModel();
  void updateObsFileList();
  bool initSpectrumFile(miString file,miString model);
  void initStations();
  void initTimes();
  void checkObsTime(int hour=-1);
  void preparePlot();

public:
  // constructor
  SpectrumManager();
  // destructor
  ~SpectrumManager();

  SpectrumOptions* getOptions() { return spopt; }
  void setPlotWindow(int w, int h);

  //routines from controller
  vector<miString> getLineThickness();

  void setModel();
  void setStation(const miString& station);
  void setTime(const miTime& time);
  miString setStation(int step);
  miTime setTime(int step);

  const miTime getTime(){return plotTime;}
  const miString getStation() { return plotStation; }
  const miString getLastStation() { return lastStation; }
  const vector<miString>& getStationList() { return nameList; }
  const vector<float>& getLatitudes() { return latitudeList; }
  const vector<float>& getLongitudes() { return longitudeList; }
  const vector<miTime>& getTimeList() { return timeList; }

  vector<miString> getModelNames();
  vector<miString> getModelFiles();
  void setFieldModels(const vector<miString>& fieldmodels);
  void setSelectedModels(const vector<miString>& models,
			 bool obs, bool field);
  void setSelectedFiles(const vector<miString>& files,
			bool obs, bool field);

  vector<miString> getSelectedModels();

  bool plot();
  void startHardcopy(const printOptions& po);
  void endHardcopy();
  bool onlyObsState() { return onlyObs; }
  void mainWindowTimeChanged(const miTime& time);
  void updateObs();
  miString getAnnotationString();

  void setMenuConst(map<miString,miString> mc)
  { menuConst = mc;}

  vector<miString> writeLog();
  void readLog(const vector<miString>& vstr,
	       const miString& thisVersion, const miString& logVersion);

};


#endif
