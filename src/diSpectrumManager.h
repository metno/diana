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

#include "diCommonTypes.h"
#include "diPrintOptions.h"
#include "vcross_v2/VcrossSetup.h"

#include <puTools/miTime.h>

#include <vector>
#include <map>
#include <set>

class SpectrumOptions;
class SpectrumFile;
class SpectrumData;
class SpectrumPlot;

/**
   \brief Managing Wave Spectrum data sources and plotting
*/
class SpectrumManager
{
private:
  struct StationPos {
    float latitude;
    float longitude;
    std::string obs;
  };

  struct SelectedModel{
    std::string model;
    std::string reftime;
  };

  // map<model,filename>
  std::map<std::string,std::string> filenames;
  std::map<std::string,std::string> filetypes;
  std::map<std::string,std::string> filesetup;
  vcross::Setup_p setup;

  // for use in dialog (unique lists in setup order)
  std::vector<std::string> dialogModelNames;
  std::vector<std::string> dialogFileNames;

  SpectrumOptions *spopt;
  std::vector<SpectrumFile*> spfile;
  std::vector<SpectrumData*> spdata;

  std::vector<std::string> nameList;
  std::vector<float> latitudeList;
  std::vector<float> longitudeList;
  std::vector<miutil::miTime>   timeList;

  std::vector<SelectedModel> selectedModels;

  int plotw, ploth;

  std::string plotStation;
  std::string lastStation;
  miutil::miTime   plotTime;
  miutil::miTime   ztime;

  bool dataChange;
  std::vector<SpectrumPlot*> spectrumplots;

  bool hardcopy;
  printOptions printoptions;
  bool hardcopystarted;

  std::map<std::string,std::string> menuConst;

  std::string getDefaultModel();
  bool initSpectrumFile(const SelectedModel& selectedModel);
  void initStations();
  void initTimes();
  void preparePlot();

public:
  SpectrumManager();
  ~SpectrumManager();

  void parseSetup();
  SpectrumOptions* getOptions() { return spopt; }
  void setPlotWindow(int w, int h);

  //routines from controller
  std::vector<std::string> getLineThickness();

  void setModel();
  void setStation(const std::string& station);
  void setTime(const miutil::miTime& time);
  std::string setStation(int step);
  miutil::miTime setTime(int step);

  const miutil::miTime& getTime(){return plotTime;}
  const std::string& getStation() { return plotStation; }
  const std::string& getLastStation() { return lastStation; }
  const std::vector<std::string>& getStationList() { return nameList; }
  const std::vector<float>& getLatitudes() { return latitudeList; }
  const std::vector<float>& getLongitudes() { return longitudeList; }
  const std::vector<miutil::miTime>& getTimeList() { return timeList; }

  std::vector<std::string> getModelNames();
  std::vector<std::string> getModelFiles();
  std::vector <std::string> getReferencetimes(const std::string& model);
  void setSelectedModels(const std::vector<std::string>& models);

  bool plot();
  void startHardcopy(const printOptions& po);
  void endHardcopy();
  void mainWindowTimeChanged(const miutil::miTime& time);
  std::string getAnnotationString();

  void setMenuConst(const std::map<std::string,std::string>& mc)
    { menuConst = mc;}

  std::vector<std::string> writeLog();
  void readLog(const std::vector<std::string>& vstr,
      const std::string& thisVersion, const std::string& logVersion);
};

#endif
