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
#ifndef VCROSSMANAGER_H
#define VCROSSMANAGER_H


#include "diCommonTypes.h"
#include "diController.h"
#include "diPlotOptions.h"
#include "diPrintOptions.h"
#include "diLocationPlot.h"

#include <puTools/miString.h>
#include <puTools/miTime.h>
#include <vector>
#include <map>

class VcrossOptions;
class VcrossFile;
class VcrossPlot;
class VcrossField;

/**
   \brief Managing Vertical Crossection data sources and plotting

*/
class VcrossManager {

private:

  struct VcrossData {
    miutil::miString    filename;
    miutil::miString    crossection;
    int         tgpos;
    miutil::miTime      time;
    VcrossPlot* vcplot;
  };

  // std::map<model,filename>
  std::map<miutil::miString,miutil::miString> filenames;
  std::vector<miutil::miString> modelnames;
  std::map<miutil::miString, miutil::miString> filetypes;

  std::map<miutil::miString,VcrossFile*> vcfiles;
  std::map<miutil::miString,VcrossField*> vcfields;

  std::vector<VcrossData> vcdata;

  FieldManager *fieldm;   // field manager

  VcrossOptions *vcopt;

  bool dataChange;

  std::vector<miutil::miString>    selectedModels;
  std::vector<miutil::miString>    selectedFields;
  std::vector<int>         selectedHourOffset;
  std::vector<PlotOptions> selectedPlotOptions;
  std::vector<bool>        selectedPlotShaded;
  std::vector<miutil::miString>    selectedVcFile;
  std::vector<int>         selectedVcData;
  std::vector<miutil::miString>    selectedLabel;
  std::vector<miutil::miString>    plotStrings;

  //bool asField;

  miutil::miString masterFile;
  std::vector<std::string> nameList;
  std::vector<miutil::miTime>   timeList;

  std::vector<miutil::miString> usedModels;

  miutil::miString plotCrossection;
  miutil::miString lastCrossection;
  miutil::miTime   plotTime;
  miutil::miTime   ztime;

  int timeGraphPos;
  int timeGraphPosMax;

  bool hardcopy;
  printOptions printoptions;
  bool hardcopystarted;

  bool setModels();

public:
  // constructor
  VcrossManager(Controller *co);
  // destructor
  ~VcrossManager();

  void cleanup();
  void cleanupDynamicCrossSections();
  bool parseSetup();

  VcrossOptions* getOptions() { return vcopt; }

  //routines from controller
  std::vector< std::vector<Colour::ColourInfo> > getMultiColourInfo(int multiNum);

  void preparePlot();
  void setCrossection(const miutil::miString& crossection);
  void setTime(const miutil::miTime& time);
  miutil::miString setCrossection(int step);
  bool setCrossection(float lat, float lon);
  miutil::miTime setTime(int step);
  const miutil::miTime getTime() { return plotTime; }
  void getCrossections(LocationData& locationdata);
  void getCrossectionOptions(LocationData& locationdata);
  const miutil::miString getCrossection() { return plotCrossection; }
  const miutil::miString getLastCrossection(){return lastCrossection;}
  const std::vector<std::string>& getCrossectionList() { return nameList; }
  const std::vector<miutil::miTime>&   getTimeList() { return timeList; }
  std::vector<miutil::miString> getAllModels();
  std::map<miutil::miString,miutil::miString> getAllFieldOptions();
  std::vector<std::string> getFieldNames(const miutil::miString& model);
  std::vector<miutil::miString> getPlotStrings(){return plotStrings;}

  bool plot();
  void startHardcopy(const printOptions& po);
  void endHardcopy();
  void mainWindowTimeChanged(const miutil::miTime& time);
  bool setSelection(const std::vector<miutil::miString>& vstr);
  bool timeGraphOK();
  void disableTimeGraph();
  void setTimeGraphPos(int plotx, int ploty);
  void setTimeGraphPos(int incr);
  void parseQuickMenuStrings(const std::vector<miutil::miString>& vstr);
  std::vector<miutil::miString> getQuickMenuStrings();

  std::vector<miutil::miString> writeLog();
  void readLog(const std::vector<miutil::miString>& vstr,
	       const miutil::miString& thisVersion, const miutil::miString& logVersion);

private:
  VcrossField* getVcrossField(const std::string& modelname);
};

#endif
