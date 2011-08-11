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


#include <diCommonTypes.h>
#include <diSetupParser.h>
#include <diField/diPlotOptions.h>
#include <diPrintOptions.h>
#include <diLocationPlot.h>
#include <puTools/miTime.h>
#include <puTools/miString.h>
#include <vector>
#include <map>
#include <diController.h>

using namespace std;

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

  // map<model,filename>
  map<miutil::miString,miutil::miString> filenames;
  vector<miutil::miString> modelnames;
  map<miutil::miString, miutil::miString> filetypes;

  map<miutil::miString,VcrossFile*> vcfiles;
  map<miutil::miString,VcrossField*> vcfields;

  vector<VcrossData> vcdata;

  SetupParser sp;
  FieldManager *fieldm;   // field manager

  VcrossOptions *vcopt;

  bool dataChange;

  vector<miutil::miString>    selectedModels;
  vector<miutil::miString>    selectedFields;
  vector<int>         selectedHourOffset;
  vector<PlotOptions> selectedPlotOptions;
  vector<bool>        selectedPlotShaded;
  vector<miutil::miString>    selectedVcFile;
  vector<int>         selectedVcData;
  vector<miutil::miString>    selectedLabel;
  vector<miutil::miString>    plotStrings;

  //bool asField;

  miutil::miString masterFile;
  vector<miutil::miString> nameList;
  vector<miutil::miTime>   timeList;

  vector<miutil::miString> usedModels;

  miutil::miString plotCrossection;
  miutil::miString lastCrossection;
  miutil::miTime   plotTime;
  miutil::miTime   ztime;

  int timeGraphPos;
  int timeGraphPosMax;

  bool hardcopy;
  printOptions printoptions;
  bool hardcopystarted;

  bool parseSetup();
  bool setModels();

public:
  // constructor
  VcrossManager(Controller *co);
  // destructor
  ~VcrossManager();

  void cleanup();
  void cleanupDynamicCrossSections();

  VcrossOptions* getOptions() { return vcopt; }

  //routines from controller
  vector< vector<Colour::ColourInfo> > getMultiColourInfo(int multiNum);

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
  const vector<miutil::miString>& getCrossectionList() { return nameList; }
  const vector<miutil::miTime>&   getTimeList() { return timeList; }
  vector<miutil::miString> getAllModels();
  map<miutil::miString,miutil::miString> getAllFieldOptions();
  vector<miutil::miString> getFieldNames(const miutil::miString& model);
  vector<miutil::miString> getPlotStrings(){return plotStrings;}

  bool plot();
  void startHardcopy(const printOptions& po);
  void endHardcopy();
  void mainWindowTimeChanged(const miutil::miTime& time);
  bool setSelection(const vector<miutil::miString>& vstr);
  bool timeGraphOK();
  void disableTimeGraph();
  void setTimeGraphPos(int plotx, int ploty);
  void setTimeGraphPos(int incr);
  void parseQuickMenuStrings(const vector<miutil::miString>& vstr);
  vector<miutil::miString> getQuickMenuStrings();

  vector<miutil::miString> writeLog();
  void readLog(const vector<miutil::miString>& vstr,
	       const miutil::miString& thisVersion, const miutil::miString& logVersion);

};

#endif
