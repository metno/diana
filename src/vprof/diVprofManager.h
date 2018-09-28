/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#include "diPlotCommand.h"
#include "diStationTypes.h"
#include "diTimeTypes.h"
#include "diVprofReaderBufr.h"
#include "diVprofReaderFimex.h"
#include "diVprofReaderRoadobs.h"
#include "vcross_v2/VcrossSetup.h"

#include <puTools/miTime.h>

#include <QSize>

#include <map>
#include <set>
#include <vector>

class VprofOptions;
class VprofData;
typedef std::shared_ptr<VprofData> VprofData_p;
class VprofDiagram;
class DiCanvas;
class DiGLPainter;

/**
   \brief Managing Vertical Profile observation and prognostic sources.
   \brief If we use observations from road, the filename och filepath
   \brief will not be used.
*/
class VprofManager
{
private:
  // std::map<model,filename>
  std::map<std::string, std::string> stationsfilenames;
  std::map<std::string, std::string> filetypes;
  VprofReaderBufr_p reader_bufr;
  VprofReaderFimex_p reader_fimex;
  VprofReaderRoadobs_p reader_roadobs;

  // for use in dialog (unique lists in setup order)
  std::vector<std::string> dialogModelNames;
  std::vector<std::string> dialogFileNames;

  VprofOptions* vpopt;
  VprofDiagram* vpdiag;
  std::vector<VprofData_p> vpdata;

  std::vector<stationInfo> stationList;
  plottimes_t timeList;

  VprofSelectedModel_v selectedModels;

  std::vector<std::string> plotStations;
  std::vector<std::string> selectedStations;
  std::string lastStation;
  miutil::miTime plotTime;

  int realizationCount;
  int realization;

  std::map<std::string, std::string> menuConst;

  QSize plotsize;
  DiCanvas* mCanvas;

  VprofReader_p getReader(const std::string& modelName);
  bool initVprofData(const VprofSelectedModel& selectedModel);
  void initStations();
  void initTimes();

  void updateSelectedStations();

  VprofValuesRequest prepareValuesRequest();

public:
  VprofManager();
  ~VprofManager();

  void setCanvas(DiCanvas* c);
  DiCanvas* canvas() const { return mCanvas; }

  // cleans up vhen user closes the vprof window
  void cleanup();
  // parseSetup and init obsfilelist
  void init();

  VprofOptions* getOptions() { return vpopt; }
  void setPlotWindow(const QSize& size);
  const QSize& plotWindow() const { return plotsize; }

  void applyPlotCommands(const PlotCommand_cpv& vstr);

  void parseSetup();
  void setModel();
  void setRealization(int r);
  void setStation(const std::string& station);
  void setStations(const std::vector<std::string>& station);
  void setTime(const miutil::miTime& time);
  std::string stepStation(int step);
  miutil::miTime setTime(int step, int dir);
  const miutil::miTime& getTime() { return plotTime; }
  const std::vector<std::string>& getStations() { return selectedStations; }
  const std::string& getLastStation() { return lastStation; }
  const std::vector<stationInfo>& getStationList() const { return stationList; }
  const plottimes_t& getTimeList() { return timeList; }
  int getRealizationCount() const { return realizationCount; }
  const std::vector<std::string>& getModelNames();
  const std::vector<std::string>& getModelFiles() { return dialogFileNames; }
  std::set<std::string> getReferencetimes(const std::string& model);
  void setSelectedModels(const VprofSelectedModel_v& models);
  const std::vector<VprofSelectedModel>& getSelectedModels() const { return selectedModels; }

  bool plot(DiGLPainter* gl);
  void mainWindowTimeChanged(const miutil::miTime& time);
  std::string getAnnotationString();

  std::vector<std::string> writeLog();
  void readLog(const std::vector<std::string>& vstr, const std::string& thisVersion, const std::string& logVersion);
};

#endif
