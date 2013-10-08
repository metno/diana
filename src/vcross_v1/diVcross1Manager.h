/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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
#include "diVcross1Plot.h"

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
    std::string    filename;
    std::string    crossection;
    int         tgpos;
    miutil::miTime      time;
    VcrossPlot* vcplot;
  };

  // std::map<model,filename>
  std::map<std::string,std::string> filenames;
  std::vector<std::string> modelnames;
  std::map<std::string, std::string> filetypes;

  std::map<std::string,VcrossFile*> vcfiles;
  std::map<std::string,VcrossField*> vcfields;

  std::vector<VcrossData> vcdata;

  FieldManager *fieldm;   // field manager

  VcrossOptions *vcopt;

  bool dataChange;

  std::vector<std::string>    selectedModels;
  std::vector<std::string>    selectedFields;
  std::vector<int>         selectedHourOffset;
  std::vector<PlotOptions> selectedPlotOptions;
  std::vector<bool>        selectedPlotShaded;
  std::vector<std::string>    selectedVcFile;
  std::vector<int>         selectedVcData;
  std::vector<std::string>    selectedLabel;
  std::vector<std::string>    plotStrings;

  //bool asField;

  std::string masterFile;
  std::vector<std::string> nameList;
  std::vector<miutil::miTime>   timeList;

  std::vector<std::string> usedModels;

  std::string plotCrossection;
  std::string lastCrossection;
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
  void setPlotWindow(int xs, int ys);
  void movePart(int pxmove, int pymove)
    { VcrossPlot::movePart(pxmove, pymove); }
  void getPlotSize(float& x1, float& y1, float& x2, float& y2, Colour& rubberbandColour)
    { VcrossPlot::getPlotSize(x1, y1, x2, y2, rubberbandColour); }
  void increasePart()
    { VcrossPlot::increasePart(); }
  void decreasePart(int px1, int py1, int px2, int py2)
    { VcrossPlot::decreasePart(px1, py1, px2, py2); }
  void standardPart()
    { VcrossPlot::standardPart(); }

  void preparePlot();
  void setCrossection(const std::string& crossection);
  void setTime(const miutil::miTime& time);
  std::string setCrossection(int step);
  bool setCrossection(float lat, float lon);
  miutil::miTime setTime(int step);
  const miutil::miTime getTime() { return plotTime; }
  void getCrossections(LocationData& locationdata);
  void getCrossectionOptions(LocationData& locationdata);
  const std::string getCrossection() { return plotCrossection; }
  const std::string getLastCrossection(){return lastCrossection;}
  const std::vector<std::string>& getCrossectionList() { return nameList; }
  const std::vector<miutil::miTime>&   getTimeList() { return timeList; }
  std::vector<std::string> getAllModels();
  std::map<std::string,std::string> getAllFieldOptions();
  std::vector<std::string> getFieldNames(const std::string& model);
  std::vector<std::string> getPlotStrings(){return plotStrings;}

  bool plot();
  void startHardcopy(const printOptions& po);
  void endHardcopy();
  void mainWindowTimeChanged(const miutil::miTime& time);
  bool setSelection(const std::vector<std::string>& vstr);
  bool timeGraphOK();
  void disableTimeGraph();
  void setTimeGraphPos(int plotx, int ploty);
  void setTimeGraphPos(int incr);
  void parseQuickMenuStrings(const std::vector<std::string>& vstr);
  std::vector<std::string> getQuickMenuStrings();

  std::vector<std::string> writeLog();
  void readLog(const std::vector<std::string>& vstr,
	       const std::string& thisVersion, const std::string& logVersion);

private:
  VcrossField* getVcrossField(const std::string& modelname);
};

#endif
