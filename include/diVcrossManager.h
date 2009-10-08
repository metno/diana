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

using namespace std;

class VcrossOptions;
class VcrossFile;
class VcrossPlot;


/**
   \brief Managing Vertical Crossection data sources and plotting

*/
class VcrossManager {

private:

  struct VcrossData {
    miString    filename;
    miString    crossection;
    int         tgpos;
    miTime      time;
    VcrossPlot* vcplot;
  };

  // map<model,filename>
  map<miString,miString> filenames;
  vector<miString> modelnames;

  map<miString,VcrossFile*> vcfiles;

  vector<VcrossData> vcdata;

  SetupParser sp;

  VcrossOptions *vcopt;

  bool dataChange;

  vector<miString>    selectedModels;
  vector<miString>    selectedFields;
  vector<int>         selectedHourOffset;
  vector<PlotOptions> selectedPlotOptions;
  vector<bool>        selectedPlotShaded;
  vector<miString>    selectedVcFile;
  vector<int>         selectedVcData;
  vector<miString>    selectedLabel;

  //bool asField;

  miString masterFile;
  vector<miString> nameList;
  vector<miTime>   timeList;

  vector<miString> usedModels;

  miString plotCrossection;
  miString lastCrossection;
  miTime   plotTime;
  miTime   ztime;

  int timeGraphPos;
  int timeGraphPosMax;

  bool hardcopy;
  printOptions printoptions;
  bool hardcopystarted;

  map<miString,miString> menuConst;

  bool parseSetup();
  bool setModels();

public:
  // constructor
  VcrossManager();
  // destructor
  ~VcrossManager();

  void cleanup();

  VcrossOptions* getOptions() { return vcopt; }

  //routines from controller
  vector< vector<Colour::ColourInfo> > getMultiColourInfo(int multiNum);

  void preparePlot();
  void setCrossection(const miString& crossection);
  void setTime(const miTime& time);
  miString setCrossection(int step);
  miTime setTime(int step);
  const miTime getTime() { return plotTime; }
  void getCrossections(LocationData& locationdata);
  void getCrossectionOptions(LocationData& locationdata);
  const miString getCrossection() { return plotCrossection; }
  const miString getLastCrossection(){return lastCrossection;}
  const vector<miString>& getCrossectionList() { return nameList; }
  const vector<miTime>&   getTimeList() { return timeList; }
  vector<miString> getAllModels();
  map<miString,miString> getAllFieldOptions();
  vector<miString> getFieldNames(const miString& model);
  bool plot();
  void startHardcopy(const printOptions& po);
  void endHardcopy();
  void mainWindowTimeChanged(const miTime& time);
  bool setSelection(const vector<miString>& vstr);
  bool timeGraphOK();
  void disableTimeGraph();
  void setTimeGraphPos(int plotx, int ploty);
  void setTimeGraphPos(int incr);

  void setMenuConst(map<miString,miString> mc)
  { menuConst = mc;}

  vector<miString> writeLog();
  void readLog(const vector<miString>& vstr,
	       const miString& thisVersion, const miString& logVersion);

};

#endif
