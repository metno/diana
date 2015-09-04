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
#include "EditItems/kml.h" // for struct CrossSection

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
  struct Marker {
    float position;
    std::string text;
    std::string colour;
    float x, y;

    Marker(float p, const std::string& t, const std::string& c)
      : position(p), text(t), colour(c), x(-1), y(-1) { }
    Marker(float xx, float yy, const std::string& t, const std::string& c)
      : position(-1), text(t), colour(c), x(xx), y(yy) { }
  };
  typedef std::vector<Marker> Marker_v;

  class CS {
  public:
    explicit CS(Crossection_cp sourceCS)
      : mSourceCS(sourceCS) { }
    CS(const std::string& label, const LonLat_v& requested)
      : mLabel(label), mPointsRequested(requested) { }

    const std::string& label() const
      { return mSourceCS ? mSourceCS->label() : mLabel; }

    size_t length() const
      { return mSourceCS? mSourceCS->length() : mPointsRequested.size(); }
    const LonLat& point(int i) const
      { return mSourceCS ? mSourceCS->point(i) : mPointsRequested.at(i); }

    size_t lengthRequested() const
      { return mSourceCS ? mSourceCS->lengthRequested() : mPointsRequested.size(); }
    const LonLat& pointRequested(int i) const
      { return mSourceCS ? mSourceCS->pointRequested(i) : mPointsRequested.at(i); }

    bool dynamic() const
      { return !mSourceCS || mSourceCS->dynamic(); }
    Crossection_cp sourceCS() const
      { return mSourceCS; }
    void setSourceCS(Crossection_cp scs)
      { mSourceCS = scs; }
    bool hasSourceCS() const
      { return mSourceCS.get() != 0; }

    bool operator==(const CS& other) const;

  private:
    Crossection_cp mSourceCS;
    std::string mLabel;
    LonLat_v mPointsRequested;
  };
  typedef std::vector<CS> CS_v;

public:
  typedef miutil::miTime vctime_t;
  typedef std::vector<vctime_t> vctime_v;

  typedef std::vector<std::string> string_v;
  typedef std::set<std::string> string_s;
  typedef std::map<std::string, std::string> string_string_m;
  typedef std::set< std::pair<Source_p,Time> > Source_Reftime_ps;

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

  //! lists all reference times available for the specified model
  vctime_v getModelReferenceTimes(const std::string& modelName);

  //! get plot options used previously in this session, or those configured in setup
  std::string getPlotOptions(const std::string& field, bool fromSetup) const;

  /*! get a list of plot names that can be cosen for the given model
   * \param model the model name for which plots shall be listed, from getAllModels
   * \param reftime the reference time for which plots shall be listed, from getModelReferenceTimes
   * \param includeSelected include plots that are already selected in the list
   */
  std::vector<std::string> getFieldNames(const std::string& model, const vctime_t& reftime, bool includeSelected=true);

  size_t getFieldCount() const;
  std::string getModelAt(int index) const;
  vctime_t getReftimeAt(int index) const;
  std::string getFieldAt(int index) const;
  std::string getOptionsAt(int index) const;
  bool getVisibleAt(int index) const;

  void fieldChangeStart(bool script);
  void fieldChangeDone();

  class PlotSpec {
  public:
    PlotSpec(const std::string& model, const vctime_t& reftime, const std::string& field);

    PlotSpec(const ModelReftime& mr, const std::string& field)
      : mModelReftime(mr), mField(field) { }

    const ModelReftime& modelReftime() const
      { return mModelReftime; }

    const std::string& field() const
      { return mField; }

    void setField(const std::string& f)
      { mField = f; }

  private:
    ModelReftime mModelReftime;
    std::string mField;
  };

  int addField(const PlotSpec& ps, const std::string& fieldOpts, int index, bool updateUserFieldOptions=true);
  void updateField(int index, const std::string& fieldOpts);
  void removeField(int index);
  void moveField(int indexOld, int indexNew);
  void removeAllFields();

  void setFieldVisible(int index, bool visible);

  size_t getMarkerCount() const
    { return mMarkers.size(); }
  float getMarkerPosition(size_t idx) const
    { return mMarkers.at(idx).position; }
  const std::string& getMarkerText(size_t idx) const
    { return mMarkers.at(idx).text; }
  const std::string& getMarkerColour(size_t idx) const
    { return mMarkers.at(idx).colour; }

  int getCrossectionIndex() const;
  void setCrossectionIndex(int index);
  int findCrossectionIndex(const QString& label);
  QString getCrossectionLabel(int index) const;
  QString getCrossectionLabel() const
    { return getCrossectionLabel(getCrossectionIndex()); }
  int getCrossectionCount() const;
  void addDynamicCrossection(const QString& label, const LonLat_v& points);
  void removeDynamicCrossection(const QString& label);
  const LocationData& getCrossections() const
    { return locationData; }
  LonLat_v getDynamicCrossectionPoints(const QString& label);

  bool hasPredefinedDynamicCrossections() const
    { return mHasSupportForDynamicCs && mHasPredefinedDynamicCs; }

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


  bool supportsTimeGraph();
  void switchTimeGraph(bool on);
  bool isTimeGraph() const
    { return mTimeGraphMode; }
  void setTimeGraph(const LonLat& position);


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

  //! return axis position of point (x,y), formatted as text
  QString axisPosition(int x, int y);

Q_SIGNALS:
  void fieldChangeBegin(bool fromScript);
  void fieldAdded(int insertedIndex);
  void fieldRemoved(int removedIndex);
  void fieldOptionsChanged(int changedIndex);
  void fieldVisibilityChanged(int changedIndex);
  void fieldChangeEnd();

  void crossectionListChanged();
  void crossectionIndexChanged(int current);

  void timeListChanged();
  void timeIndexChanged(int current);

  void timeGraphModeChanged(bool on);

private:
  int insertField(const ModelReftime& model, const std::string& plot,
      const string_v& options, int idx);

  SelectedPlot_p findSelectedPlot(const PlotSpec& ps);
  int findSelectedPlotIndex(const PlotSpec& ps);

  SelectedPlot_p getSelectedPlot(int index) const;
  Source_Reftime_ps listDynamicSources() const;

  // update list of crossections, emit signal
  void handleChangedCrossectionList(const QString& oldLabel);

  // update crossection points, emit signal
  void handleChangedCrossection();

  void handleChangedTimeList(const vctime_t& oldTime);
  void handleChangedTime(int plotTimeIndex);

  void updateCrossectionsTimes();

  void preparePlot();

  void updateLocationData();

  bool goodCrossSectionLength(size_t length) const;
  CS_v loadCsFromFile(const std::string& filename);
  CS& crossection()
    { return mCrossections.at(mCrossectionCurrent); }
  const CS& crossection() const
    { return mCrossections.at(mCrossectionCurrent); }
  LonLat_v crossectionPoints() const;
  LonLat_v crossectionPointsRequested() const;
  bool isValidCsIndex(int i) const
    { return i >= 0 && i < getCrossectionCount(); }
  bool hasValidCsIndex() const
    { return isValidCsIndex(mCrossectionCurrent); }

  void saveZoom();
  void restoreZoom();

  void cleanupData();

private:
  Collector_p mCollector;
  VcrossOptions_p mOptions;
  QtPlot_p mPlot;

  enum { CHANGED_NO=0, CHANGED_TIME=1, CHANGED_CS=2, CHANGED_SEL=7 };
  int dataChange;
  int inFieldChangeGroup;

  CS_v mCrossections;
  int mCrossectionCurrent; //! mCrossections index of current cross section
  LocationData locationData; //! kind of duplicate of mCrossections in a different structure

  vctime_v mCrossectionTimes;
  int mPlotTime; //! mCrossectionTimes index of current plot time

  bool mTimeGraphMode; //! true iff in timegraph mode

  //! map filename -> crossections for cross-sections defined in kml files
  typedef std::map<std::string, CrossSection_v> cspredefined_m;
  cspredefined_m mCsPredefined;

  bool mHasSupportForDynamicCs;
  bool mHasPredefinedDynamicCs;

  typedef std::map<std::string, QtPlot::Rect> cs_zoom_t;
  cs_zoom_t mCrossectionZooms;

  //! Maps fieldname to field options set via the GUI (ie not via quickmenu). These are saved in writePlotOptions().
  string_string_m userFieldOptions;

  Marker_v mMarkers;
  float mReferencePosition;
};

typedef boost::shared_ptr<QtManager> QtManager_p;

} // namespace vcross

#endif // VCROSSQTMANAGER_H
