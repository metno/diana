/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2014 met.no

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
#ifndef VCROSSQTMANAGER_H
#define VCROSSQTMANAGER_H

#include "diPlotOptions.h"
#include "VcrossOptions.h"
#include "VcrossSetup.h"
#include "VcrossQtPlot.h"
#include "diLocationPlot.h"

#include <puTools/miTime.h>
#include <vector>
#include <map>
#include <memory>

class QPainter;

namespace vcross {

/**
   \brief Managing Vertical Crossection data sources and plotting
*/
class QtManager {
private:
  typedef miutil::miTime vctime_t;

public:
  QtManager();
  ~QtManager();

  Setup_p getSetup() const
    { return mCollector->getSetup(); }

  // called from VcrossDialog
  std::vector<std::string> getAllModels();
  std::map<std::string,std::string> getAllFieldOptions();
  std::vector<std::string> getFieldNames(const std::string& model);
  bool setSelection(const std::vector<std::string>& vstr);
  // end of calls from VcrossDialog

  // called from VcrossWindow
  void cleanup();
  // TODO void cleanupDynamicCrossSections();
  void disableTimeGraph();
  const std::string& getCrossection() const
    { return currentCSName(); }
  const string_v& getCrossectionList() const
    { return mCrossectionLabels; }
  void getCrossections(LocationData& locationdata);

  //! list of filenames with predefined cross-sections
  const std::set<std::string>& getCrossectionPredefinitions() const
    { return mCsPredefined; }

  //! true if at least one selected model supports dynamic cross-sections
  bool supportsDynamicCrossections() const
    { return mHasSupportForDynamicCs; }

  std::vector<std::string> getQuickMenuStrings();
  vctime_t getTime() const
    { return currentTime(); }
  const std::vector<vctime_t>& getTimeList() const
    { return mCrossectionTimes; }
  void mainWindowTimeChanged(const vctime_t& time);
  void parseQuickMenuStrings(const std::vector<std::string>& vstr);
  void parseSetup(const string_v& sources, const string_v&computations, const string_v&plots);
  void readLog(const std::vector<std::string>& vstr,
	       const std::string& thisVersion, const std::string& logVersion);
  //! Add or replace a dynamic cross section
  void setDynamicCrossection(const std::string& crossection, const LonLat_v& points);
  void setCrossection(const std::string& crossection);
  void setTime(const vctime_t& time);
  void setTimeGraph(const LonLat& position);
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
  bool plot(QPainter& painter);
  void setTimeGraphPos(int plotx, int ploty);
  void setTimeGraphPos(int incr);
  void getPlotSize(float& x1, float& y1, float& x2, float& y2, Colour& rubberbandColour);
  // end of calls from VcrossWidget

  // called from VcrossSetupDialog
  VcrossOptions* getOptions()
    { return mOptions.get(); }

  //! re-read mOptions after it has been changed in VcrossSetupDialog
  void updateOptions();
  // end of calls from VcrossSetupDialog

  //! forwards call to mOptions
  void readOptions(const string_v& vstr);

private:
  void preparePlot();

  void fillLocationData(LocationData& locationdata);

  void setTimeToBestMatch(const vctime_t& time);

  bool setModels();

  const std::string& currentCSName() const;
  void updateCSPoints();
  vctime_t currentTime() const;
  bool isTimeGraph() const
    { return mTimeGraphPos >= 0; }

  void saveZoom();
  void restoreZoom();

private:
  Collector_p mCollector;
  VcrossOptions_p mOptions;
  QtPlot_p mPlot;

  enum { CHANGED_NO=0, CHANGED_TIME=1, CHANGED_CS=2, CHANGED_SEL=7 };
  int dataChange;

  string_v mCrossectionLabels;
  LonLat_v mCrossectionPoints;
  int mCrossectionCurrent; //! mCrossectionLabels index of current cross section
  int mTimeGraphPos; //! position inside current cross section for which we plot a time graph; -1 for no timegraph
  LocationData locationData;
  typedef std::vector<vctime_t> times_t;
  times_t mCrossectionTimes;
  int mPlotTime; //! mCrossectionTimes index of current plot time

  string_v mPlotStrings;

  //! filenames of predefined cross-sections
  std::set<std::string> mCsPredefined;

  bool mHasSupportForDynamicCs;

  typedef std::map<std::string, QtPlot::Rect> cs_zoom_t;
  cs_zoom_t mCrossectionZooms;
};

typedef boost::shared_ptr<QtManager> QtManager_p;

} // namespace vcross

#endif // VCROSSQTMANAGER_H
