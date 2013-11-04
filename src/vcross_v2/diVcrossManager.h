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

#include "diPlotOptions.h"
#include "diPrintOptions.h"
#include "diVcrossSetup.h"

#include <diField/diVcrossData.h>

#include <puTools/miTime.h>
#include <vector>
#include <map>
#include <memory>

class Controller;
class FieldManager;
class LocationData;
class VcrossOptions;
class VcrossFile;
class VcrossPlot;
class VcrossField;
class VcrossSource;

/**
   \brief Managing Vertical Crossection data sources and plotting
*/
class VcrossManager {
private:
  typedef std::vector<std::string> strings_t;
  typedef std::map<std::string, std::string> string2string_t;
  typedef miutil::miTime vctime_t;

public:
  VcrossManager(Controller *co);
  ~VcrossManager();

  // called from VcrossDialog
  std::vector<std::string> getAllModels();
  std::map<std::string,std::string> getAllFieldOptions();
  std::vector<std::string> getFieldNames(const std::string& model);
  bool setSelection(const std::vector<std::string>& vstr);
  // end of calls from VcrossDialog

  // called from VcrossWindow
  void cleanup();
  void cleanupDynamicCrossSections();
  void disableTimeGraph();
  const std::string& getCrossection() const
    { return currentCSName(); }
  const std::vector<std::string>& getCrossectionList() const
    { return nameList; }
  void getCrossections(LocationData& locationdata);
  std::vector<std::string> getQuickMenuStrings();
  vctime_t getTime() const
    { return currentTime(); }
  const std::vector<vctime_t>& getTimeList() const
    { return timeList; }
  void mainWindowTimeChanged(const vctime_t& time);
  void parseQuickMenuStrings(const std::vector<std::string>& vstr);
  void parseSetup();
  void readLog(const std::vector<std::string>& vstr,
	       const std::string& thisVersion, const std::string& logVersion);
  bool setCrossection(float lat, float lon);
  void setCrossection(const std::string& crossection);
  void setTime(const vctime_t& time);
  bool timeGraphOK();
  std::vector<std::string> writeLog();
  // end of calls from VcrossWindow

  // called from VcrossWindow and VcrossWidget
  std::string setCrossection(int step);
  vctime_t setTime(int step);
  // end of calls from VcrossWindow and VcrossWidget

  // called from VcrossWidget
  void setPlotWindow(int w, int h);
  void movePart(int pxmove, int pymove);
  void decreasePart(int px1, int py1, int px2, int py2);
  void increasePart();
  void standardPart();
  void endHardcopy();
  bool plot();
  void setTimeGraphPos(int plotx, int ploty);
  void setTimeGraphPos(int incr);
  void startHardcopy(const printOptions& po);
  void getPlotSize(float& x1, float& y1, float& x2, float& y2, Colour& rubberbandColour);
  // end of calls from VcrossWidget

  // called from VcrossDialog
  VcrossOptions* getOptions()
    { return mOptions.get(); }
  // end of calls from VcrossDialog

  //! forwards call to mOptions
  void readOptions(const std::vector<std::string>& vstr);

private:
  void preparePlot();

  void fillLocationData(LocationData& locationdata);

  void setTimeToBestMatch(const vctime_t& time);

  VcrossField* getVcrossField(const std::string& modelname);
  VcrossFile* getVcrossFile(const std::string& modelname);
  VcrossSource* getVcrossSource(const std::string& modelname);

  bool addComputedValues(VcrossData* data, const std::vector<std::string>& plotArguments,
      const VcrossData::Parameters_t& csPar);
  VcrossData::Parameters_t calculateCSParameters(const VcrossData::Cut::lonlat_t& cut);

  bool setModels();

  const std::string& currentCSName() const;
  vctime_t currentTime() const;

private:
  std::auto_ptr<VcrossOptions> mOptions;
  std::auto_ptr<VcrossSetup> mSetup;
  std::auto_ptr<VcrossPlot> mPlot;
  FieldManager *fieldm; //! pointer to field manager, not owned by this

  typedef std::map<std::string, VcrossFile*> vcfiles_t;
  vcfiles_t vcfiles;
  typedef std::map<std::string, VcrossField*> vcfields_t;
  vcfields_t vcfields;
  typedef std::map<std::string, FileContents> filecontents_t;
  filecontents_t vcfilecontents;

  enum { CHANGED_NO=0, CHANGED_TIME=1, CHANGED_CS=2, CHANGED_SEL=7 };
  int dataChange;

  struct VcrossSelected {
    std::string model;
    std::string field;
    int hourOffset;
    PlotOptions plotOptions;
    bool plotShaded;
  };
  std::vector<VcrossSelected> selected;

  strings_t selectedLabel;
  strings_t plotStrings;

  strings_t nameList;
  int plotCrossection; //! nameList index of current cross section
  int timeGraphPos; //! position inside current cross section for which we plot a time graph; -1 for no timegraph

  typedef std::vector<vctime_t> times_t;
  times_t timeList;
  int plotTime; //! timeList index of current plot time

  printOptions printoptions;
  bool hardcopy;
  bool hardcopystarted;
};

#endif
