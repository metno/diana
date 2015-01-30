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
#ifndef VCROSSQTMANAGER_H
#define VCROSSQTMANAGER_H

#include "diPlotOptions.h"
#include "VcrossOptions.h"
#include "VcrossSetup.h"
#include "VcrossQtPlot.h"
#include "diLocationData.h"

#include <puTools/miTime.h>

#include <QObject>

#include <vector>
#include <map>
#include <memory>

class QPainter;

namespace vcross {

/**
   \brief Managing Vertical Crossection data sources and plotting
*/
class QtManager : public QObject {
  Q_OBJECT;
  
private:
  typedef miutil::miTime vctime_t;

public:
  typedef std::vector<std::string> string_v;
  typedef std::set<std::string> string_s;
  typedef std::map<std::string, std::string> string_string_m;
  typedef std::set<Source_p> Source_ps;

  QtManager();
  ~QtManager();


  void cleanup();


  Setup_p getSetup() const
    { return mCollector->getSetup(); }
  void parseSetup(const string_v& sources, const string_v&computations, const string_v&plots);


  VcrossOptions* getOptions() const
    { return mOptions.get(); }

  //! re-read mOptions after it has been changed in VcrossSetupDialog
  void updateOptions();


  //! lists all model names configured in setup
  string_v getAllModels();

  //! get plot options used previously in this session, or those configured in setup
  std::string getPlotOptions(const std::string& model, const std::string& field, bool fromSetup) const;

  /*! get a list of plot names that can be cosen for the given model 
   * \param model the model name for which plots shall be listed
   * \param includeSelected include plots that are already selected in the list
   */
  std::vector<std::string> getFieldNames(const std::string& model, bool includeSelected=true);

  size_t getFieldCount() const;
  std::string getFieldAt(int index) const;
  std::string getModelAt(int index) const;
  std::string getOptionsAt(int index) const;
  bool getVisibleAt(int index) const;

  SelectedPlot_p findSelectedPlot(const std::string& model, const std::string& field);
  int findSelectedPlotIndex(const std::string& model, const std::string& field);

  void fieldChangeStart(bool script);
  void fieldChangeDone();

  void addField(const std::string& model, const std::string& field, const std::string& fieldOpts,
      int index, bool updateUserFieldOptions=true);
  void updateField(const std::string& model, const std::string& field, const std::string& fieldOpts);
  void removeField(const std::string& model, const std::string& field);
  void removeAllFields();

  void setFieldVisible(int index, bool visible);


  int getCrossectionIndex() const;
  void setCrossectionIndex(int index);
  int findCrossectionIndex(const QString& label);
  QString getCrossectionLabel(int index) const;
  QString getCrossectionLabel() const
    { return getCrossectionLabel(getCrossectionIndex()); }
  int getCrossectionCount() const;
  void addDynamicCrossection(const QString& label, const LonLat_v& points);
  void removeDynamicCrossection(const QString& label);
  void getCrossections(LocationData& locationdata);

  //! list of filenames with predefined cross-sections
  const std::set<std::string>& getCrossectionPredefinitions() const
    { return mCsPredefined; }

  //! true if at least one selected model supports dynamic cross-sections
  bool supportsDynamicCrossections() const
    { return mHasSupportForDynamicCs; }


  int getTimeIndex() const;
  void setTimeIndex(int index);
  int getTimeCount() const;
  vctime_t getTimeValue(int index) const;
  vctime_t getTimeValue() const
    { return getTimeValue(getTimeIndex()); }
  void setTimeToBestMatch(const vctime_t& time);


  void setTimeGraph(const LonLat& position);
  bool timeGraphOK();
  void disableTimeGraph();
  void setTimeGraphPos(int plotx, int ploty);
  void setTimeGraphPos(int incr);


  void selectFields(const string_v& fields);

  void readVcrossOptions(const string_v& settings, const std::string& thisVersion, const std::string& logVersion);
  string_v writeVcrossOptions();

  void readPlotOptions(const string_v& settings, const std::string& thisVersion, const std::string& logVersion);
  string_v writePlotOptions();


  void setPlotWindow(int w, int h);
  void movePart(int pxmove, int pymove);
  void decreasePart(int px1, int py1, int px2, int py2);
  void increasePart();
  void standardPart();
  void getPlotSize(float& x1, float& y1, float& x2, float& y2, Colour& rubberbandColour);
  bool plot(QPainter& painter);

Q_SIGNALS:
  void fieldChangeBegin(bool fromScript);
  void fieldAdded(const std::string& model, const std::string& field, int index);
  void fieldRemoved(const std::string& model, const std::string& field, int index);
  void fieldOptionsChanged(const std::string& model, const std::string& field, int index);
  void fieldVisibilityChanged(const std::string& model, const std::string& field, int index);
  void fieldChangeEnd();

  void crossectionListChanged();
  void crossectionIndexChanged(int current);

  void timeListChanged();
  void timeIndexChanged(int current);

  void timeGraphMode(bool on);

private:
  SelectedPlot_p getSelectedPlot(int index) const;
  Source_ps listDynamicSources() const;

  // update list of crossections, emit signal
  void handleChangedCrossectionList(const QString& oldLabel);

  // update crossection points, emit signal
  void handleChangedCrossection();

  void handleChangedTimeList(const vctime_t& oldTime);
  void handleChangedTime();

  void updateCrossectionsTimes();

  void preparePlot();

  void fillLocationData(LocationData& locationdata);

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
  bool inFieldChangeGroup;

  string_v mCrossectionLabels;
  LonLat_v mCrossectionPoints;
  int mCrossectionCurrent; //! mCrossectionLabels index of current cross section
  int mTimeGraphPos; //! position inside current cross section for which we plot a time graph; -1 for no timegraph
  LocationData locationData;
  typedef std::vector<vctime_t> vctime_v;
  vctime_v mCrossectionTimes;
  int mPlotTime; //! mCrossectionTimes index of current plot time

  //! filenames of predefined cross-sections
  std::set<std::string> mCsPredefined;

  bool mHasSupportForDynamicCs;

  typedef std::map<std::string, QtPlot::Rect> cs_zoom_t;
  cs_zoom_t mCrossectionZooms;

  //! Maps fieldname to field options set via the GUI (ie not via quickmenu). These are saved in writePlotOptions().
  string_string_m userFieldOptions;
};

typedef boost::shared_ptr<QtManager> QtManager_p;

} // namespace vcross

#endif // VCROSSQTMANAGER_H
