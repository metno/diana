/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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

#include "diStationInfo.h"
#include "vcross_v2/VcrossSetup.h"

#include <puTools/miTime.h>

#include <vector>
#include <map>
#include <set>

class VprofOptions;
class VprofData;
class VprofDiagram;
class DiCanvas;
class DiGLPainter;

/**
   \brief Managing Vertical Profile observation and prognostic sources.
   \brief If we use observations from road, the filename och filepath
   \brief will not be used.
*/
class VprofManager{

private:


  struct SelectedModel{
    std::string model;
    std::string reftime;
  };

  // std::map<model,filename>
  std::map<std::string,std::string> filenames;
  std::map<std::string,std::string> stationsfilenames;
  std::map<std::string,std::string> filetypes;
  std::map<std::string,std::string> db_parameters;
  std::map<std::string,std::string> db_connects;
  std::vector<std::string> computations;

  // for use in dialog (unique lists in setup order)
  std::vector<std::string> dialogModelNames;
  std::vector<std::string> dialogFileNames;

  vcross::Setup_p setup;

  VprofOptions *vpopt;
  VprofDiagram *vpdiag;
  std::vector<VprofData*> vpdata;

  std::vector <stationInfo> stationList;
  std::vector <miutil::miTime>   timeList;

  std::vector<SelectedModel> selectedModels;

  std::vector<std::string> plotStations;
  std::vector<std::string> selectedStations;
  std::string lastStation;
  miutil::miTime   plotTime;

  int realizationCount;
  int realization;

  std::map<std::string,std::string> menuConst;

  int plotw, ploth;
  DiCanvas* mCanvas;

  bool initVprofData(const SelectedModel& selectedModel);
  void initStations();
  void initTimes();


  void updateSelectedStations();

public:
  VprofManager();
  ~VprofManager();

  void setCanvas(DiCanvas* c);
  DiCanvas* canvas() const
    { return mCanvas; }

  // clenans up vhen user closes the vprof window
  void cleanup();
  // parseSetup and init obsfilelist
  void init();

  VprofOptions* getOptions()
    { return vpopt; }
  void setPlotWindow(int w, int h);

  void parseSetup();
  void setModel();
  void setRealization(int r);
  void setStation(const std::string& station);
  void setStations(const std::vector<std::string>& station);
  void setTime(const miutil::miTime& time);
  std::string setStation(int step);
  miutil::miTime setTime(int step, int dir);
  const miutil::miTime& getTime()
    { return plotTime; }
  const std::vector<std::string>& getStations()
    { return selectedStations; }
  const std::string& getLastStation()
    { return lastStation; }
  const std::vector<stationInfo>& getStationList() const
    { return stationList; }
  const std::vector<miutil::miTime>& getTimeList()
    { return timeList; }
  int getRealizationCount() const
    { return realizationCount; }
  std::vector <std::string> getModelNames();
  const std::vector<std::string>& getModelFiles()
    { return dialogFileNames; }
  std::vector <std::string> getReferencetimes(const std::string model);
  void setSelectedModels(const std::vector<std::string>& models);

  bool plot(DiGLPainter* gl);
  void mainWindowTimeChanged(const miutil::miTime& time);
  std::string getAnnotationString();

  std::vector<std::string> writeLog();
  void readLog(const std::vector<std::string>& vstr,
      const std::string& thisVersion, const std::string& logVersion);
};

#endif
