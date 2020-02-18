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

#include "VcrossQtManager.h"

#include "VcrossComputer.h"
#include "VcrossOptions.h"
#include "VcrossQtPlot.h"
#include "diFieldUtil.h"
#include "diKVListPlotCommand.h"
#include "diPlotOptions.h"
#include "diUtilities.h"
#include "miSetupParser.h"
#include "util/string_util.h"
#include "diField/VcrossUtil.h"

#include <mi_fieldcalc/math_util.h>

#include <puTools/miStringFunctions.h>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/join.hpp>

#define MILOGGER_CATEGORY "diana.VcrossManager"
#include <miLogger/miLogging.h>

namespace /* anonymous */ {

const std::string EMPTY_STRING;
const miutil::KeyValue_v EMPTY_OPTIONS;

struct lt_LocationElement : public std::binary_function<bool, LocationElement, LocationElement>
{
  bool operator() (const LocationElement& a, const LocationElement& b) const;
};
bool lt_LocationElement::operator() (const LocationElement& a, const LocationElement& b) const
{
  if (a.name < b.name)
    return true;
  if (a.name > b.name)
    return false;
  if (a.xpos < b.xpos)
    return true;
  if (a.xpos > b.xpos)
    return false;
  if (a.ypos < b.ypos)
    return true;
  return false;
}

bool operator!=(const LonLat& a, const LonLat& b)
{
  return a.lon() != b.lon() || a.lat() != b.lat();
}

} // namespace anonymous

const char LOCATIONS_VCROSS[] = "vcross"; // declared in diVcrossInterface.h

namespace vcross {

QtManager::PlotSpec::PlotSpec(const std::string& model, const vctime_t& reftime, const std::string& field)
  : mModelReftime(model, util::from_miTime(reftime))
  , mField(field)
{
}

// ########################################################################

bool QtManager::CS::operator==(const CS& other) const
{
  if (hasSourceCS() != other.hasSourceCS())
    return false;
  if (label() != other.label())
    return false;
  if (length() != other.length())
    return false;
  for (size_t i = 0; i<length(); ++i) {
    if (point(i) != other.point(i))
      return false;
  }
  return true;
}

// ########################################################################

QtManager::QtManager()
  : mCollector(new Collector(std::make_shared<Setup>()))
  , mOptions(new VcrossOptions())
  , mPlot(new QtPlot(mOptions))
  , dataChange(0)
  , inFieldChangeGroup(0)
  , mCrossectionCurrent(-1)
  , mPlotTime(-1)
  , mRealizationCount(1)
  , mRealization(0)
  , mTimeGraphMode(false)
  , mHasSupportForDynamicCs(false)
  , mHasPredefinedDynamicCs(false)
{
  METLIBS_LOG_SCOPE();
  string_v sources, computations, plots;
  miutil::SetupParser::getSection("VERTICAL_CROSSECTION_FILES", sources);
  miutil::SetupParser::getSection("VERTICAL_CROSSECTION_COMPUTATIONS", computations);
  miutil::SetupParser::getSection("VERTICAL_CROSSECTION_PLOTS", plots);
  parseSetup(sources, computations, plots);
}


QtManager::~QtManager()
{
  METLIBS_LOG_SCOPE();
  cleanup();
}


void QtManager::cleanup()
{
  METLIBS_LOG_SCOPE();
  mCrossectionZooms.clear();
  mCsPredefined.clear(); // clear kml-file cache
  removeAllFields();
  cleanupData();
}


void QtManager::cleanupData()
{
  METLIBS_LOG_SCOPE();
  mCrossectionCurrent = -1;
  mCrossections.clear();
  mCrossectionTimes.clear();

  mPlotTime = -1;
  mRealizationCount = 1;
  mRealization = 0;

  mMarkers.clear();
  mReferencePosition = -1;

  dataChange = CHANGED_SEL;
  mPlot->clear();
}


void QtManager::parseSetup(const string_v& sources, const string_v&computations, const string_v&plots)
{
  METLIBS_LOG_SCOPE();
  cleanup();
  vc_configure(mCollector->getSetup(), sources, computations, plots);
  mCollector->setupChanged();

  // TODO assume everything changed (sources, fields, crossections, times)
}


void QtManager::updateOptions()
{
  Q_EMIT vcrossOptionsChanged();

  mCollector->setUpdateRequiredNeeded(); // might (no longer) need to request inflight
  dataChange |= CHANGED_CS | CHANGED_SEL | CHANGED_TIME;
}


int QtManager::getCrossectionIndex() const
{
  return mCrossectionCurrent;
}


void QtManager::setCrossectionIndex(int index)
{
  METLIBS_LOG_SCOPE(LOGVAL(index));
  if (index == mCrossectionCurrent && index >= 0 && mCrossectionCurrent >= 0)
    return;

  mCrossectionCurrent = index;
  if (isValidCsIndex(mCrossectionCurrent)) {
    CS& mcs = crossection();
    if (!mcs.hasSourceCS()) {
      const ModelReftime model1 = mCollector->getFirstModel();
      if (!model1.model.empty()) {
        if (Source_p src = mCollector->getSetup()->findSource(model1.model)) {
          if (src->supportsDynamicCrossections(model1.reftime)) {
            Crossection_cp scs = src->addDynamicCrossection(model1.reftime, mcs.label(),
                crossectionPointsRequested());
            mcs.setSourceCS(scs);
            updateLocationData();
            Q_EMIT crossectionListChanged();
          }
        }
      }
    }
  }
  handleChangedCrossection();
}


int QtManager::findCrossectionIndex(const QString& label)
{
  METLIBS_LOG_SCOPE(LOGVAL(label.toStdString()));
  if (!label.isEmpty()) {
    for (int i = 0; i < getCrossectionCount(); ++i)
      if (label == getCrossectionLabel(i))
        return i;
  }
  return -1;
}


QString QtManager::getCrossectionLabel(int index) const
{
  if (isValidCsIndex(index))
    return QString::fromStdString(mCrossections.at(index).label());
  else
    return QString();
}


int QtManager::getCrossectionCount() const
{
  return mCrossections.size();
}


void QtManager::addDynamicCrossection(const QString& label, const LonLat_v& points)
{
  METLIBS_LOG_SCOPE(LOGVAL(label.toStdString()));
  const std::string lbl = label.toStdString();
  const Source_Reftime_ps dynSources = listDynamicSources();
  for (Source_Reftime_ps::iterator it = dynSources.begin(); it != dynSources.end(); ++it) {
    Source_p src = it->first;
    const Time& reftime = it->second;
    src->addDynamicCrossection(reftime, lbl, points);
    mCrossectionZooms.erase(lbl);
  }

  handleChangedCrossectionList(label);
}

void QtManager::removeDynamicCrossection(const QString& label)
{
  const QString before = getCrossectionLabel();
  const Source_Reftime_ps dynSources = listDynamicSources();
  for (Source_Reftime_ps::iterator it = dynSources.begin(); it != dynSources.end(); ++it) {
    Source_p src = it->first;
    const Time& reftime = it->second;
    if (Inventory_cp inv = src->getInventory(reftime)) {
      if (Crossection_cp cs = inv->findCrossectionByLabel(label.toStdString()))
        src->dropDynamicCrossection(reftime, cs);
    }
  }

  handleChangedCrossectionList(before);
}

LonLat_v QtManager::getDynamicCrossectionPoints(const QString& label)
{
  LonLat_v points;
  const ModelReftime& mr = mCollector->getFirstModel();
  if (Source_p src = mCollector->getSetup()->findSource(mr.model)) {
    if (Inventory_cp inv = src->getInventory(mr.reftime))
      if (Crossection_cp cs = inv->findCrossectionByLabel(label.toStdString()))
        if (cs->dynamic()) {
          points.reserve(cs->lengthRequested());
          for (size_t i = 0; i<cs->lengthRequested(); ++i)
            points.push_back(cs->pointRequested(i));
        }
  }
  return points;
}

void QtManager::handleChangedCrossectionList(const QString& oldLabel)
{
  METLIBS_LOG_SCOPE(LOGVAL(oldLabel.toStdString()));

  const bool hadSupportForDynamicCs = mHasSupportForDynamicCs;
  mHasSupportForDynamicCs = false;
  mHasPredefinedDynamicCs = false;

  CS_v newCrossections;
  typedef std::map<std::string, size_t> csindex_m;
  csindex_m newCrossectionIndex;

  if (mCollector->hasVisiblePlot()) {
    const ModelReftime model1 = mCollector->getFirstModel();
    METLIBS_LOG_DEBUG(LOGVAL(model1));
    if (!model1.model.empty()) {
      if (Source_p src = mCollector->getSetup()->findSource(model1.model)) {
        // load predefined cross sections from a file, if given; this
        // should be done before listing cross sections from the source
        // in order to keep the order of cross sections from the file
        if (src->supportsDynamicCrossections(model1.reftime)) {
          mHasSupportForDynamicCs = true;
          const std::map<std::string,std::string>& options = mCollector->getSetup()->getModelOptions(model1.model);
          const std::map<std::string,std::string>::const_iterator it = options.find("predefined_cs");
          if (it != options.end() && !it->second.empty()) {
            newCrossections = loadCsFromFile(it->second);
            for (size_t i=0; i<newCrossections.size(); ++i)
              newCrossectionIndex[newCrossections[i].label()] = i;
            mHasPredefinedDynamicCs = !newCrossections.empty();
          }
        }
        // first list the cross sections known by the source
        if (Inventory_cp inv = src->getInventory(model1.reftime)) {
          for (Crossection_cpv::const_iterator itCS = inv->crossections.begin(); itCS!= inv->crossections.end(); ++itCS) {
            Crossection_cp scs = *itCS;
            if (!goodCrossSectionLength(scs->length()))
              continue;
            csindex_m::iterator itI = newCrossectionIndex.find(scs->label());
            if (itI != newCrossectionIndex.end()) {
              CS& mcs = newCrossections[itI->second];
              mcs.setSourceCS(scs);
            } else {
              const CS mcs(scs);
              newCrossectionIndex[mcs.label()] = newCrossections.size();
              newCrossections.push_back(mcs);
            }
          }
        }
      }
    }
  }

  const bool changed = (hadSupportForDynamicCs != mHasSupportForDynamicCs
      || mCrossections != newCrossections);
  if (changed) {
    dataChange |= CHANGED_CS;
    std::swap(mCrossections, newCrossections);
    mCrossectionCurrent = -1;

    updateLocationData();

    Q_EMIT crossectionListChanged();
    if (getCrossectionCount() > 0)
      setCrossectionIndex(findCrossectionIndex(oldLabel));
  }
}

void QtManager::handleChangedCrossection()
{
  if (!hasValidCsIndex())
    mCrossectionCurrent = -1;

  dataChange |= CHANGED_CS;
  Q_EMIT crossectionIndexChanged(mCrossectionCurrent);
}


QtManager::Source_Reftime_ps QtManager::listDynamicSources() const
{
  Source_Reftime_ps dynSources;
  const SelectedPlot_pv& spv = mCollector->getSelectedPlots();
  for (SelectedPlot_pv::const_iterator it = spv.begin(); it != spv.end(); ++it) {
    SelectedPlot_cp sp = *it;
    if (Source_p src = mCollector->getSetup()->findSource(sp->model.model)) {
      if (src->supportsDynamicCrossections(sp->model.reftime))
        dynSources.insert(std::make_pair(src, sp->model.reftime));
    }
  }
  return dynSources;
}


void QtManager::updateLocationData()
{
  METLIBS_LOG_SCOPE();

  const Area geoArea(Projection::geographic(), Rectangle(0, 0, 90, 360));

  locationData.name =              LOCATIONS_VCROSS;
  locationData.locationType =      location_line;
  locationData.area =              geoArea;
  locationData.annotation =        "Vertikalsnitt";
  locationData.colour =            mOptions->vcOnMapColour;
  locationData.linetype =          mOptions->vcOnMapLinetype;
  locationData.linewidth =         mOptions->vcOnMapLinewidth;
  locationData.colourSelected =    mOptions->vcSelectedOnMapColour;
  locationData.linetypeSelected =  mOptions->vcSelectedOnMapLinetype;
  locationData.linewidthSelected = mOptions->vcSelectedOnMapLinewidth;
  locationData.elements.clear();
  for (const CS& cs : mCrossections) {
    LocationElement el;
    el.name = cs.label();
    for (size_t i = 0; i<cs.length(); ++i) {
      el.xpos.push_back(cs.point(i).lonDeg());
      el.ypos.push_back(cs.point(i).latDeg());
    }
    locationData.elements.push_back(el);
  }
}


bool QtManager::goodCrossSectionLength(size_t length) const
{
  if (isTimeGraph())
    return length == 1;
  else
    return length >= 2;
}


QtManager::CS_v QtManager::loadCsFromFile(const std::string& kmlfile)
{
  METLIBS_LOG_TIME();
  cspredefined_m::iterator itP = mCsPredefined.find(kmlfile);
  if (itP == mCsPredefined.end()) {
    const CrossSection_v fromfile = KML::loadCrossSections(QString::fromStdString(kmlfile));
    itP = mCsPredefined.insert(std::make_pair(kmlfile, fromfile)).first;
  }
  CS_v crossections;
  for (CrossSection_v::const_iterator itCS = itP->second.begin(); itCS != itP->second.end(); ++itCS) {
    if (goodCrossSectionLength(itCS->mPoints.size()))
      crossections.push_back(CS(itCS->mLabel, itCS->mPoints));
  }
  return crossections;
}


LonLat_v QtManager::crossectionPoints() const
{
  LonLat_v lls;
  if (hasValidCsIndex()) {
    const CS& cs = crossection();
    for (size_t i=0; i<cs.length(); ++i)
      lls.push_back(cs.point(i));
  }
  return lls;
}

LonLat_v QtManager::crossectionPointsRequested() const
{
  LonLat_v lls;
  if (hasValidCsIndex()) {
    const CS& cs = crossection();
    for (size_t i=0; i<cs.lengthRequested(); ++i)
      lls.push_back(cs.pointRequested(i));
  }
  return lls;
}

void QtManager::setTimeIndex(int index)
{
  if (isTimeGraph())
    return;
  if (index != mPlotTime) {
    if (index >= 0 && index < (int)mCrossectionTimes.size())
      mRequestedTime = getTimeValue(index);
    else
      mRequestedTime = miutil::miTime();
  }
  handleChangedTime(index);
}


void QtManager::setTimeToBestMatch(const QtManager::vctime_t& time)
{
  METLIBS_LOG_SCOPE(LOGVAL(time));
  mRequestedTime = time;
  setTimeIndexToBestMatch();
}

void QtManager::setTimeIndexToBestMatch()
{
  int bestTime = -1;
  if (getTimeCount() > 0 && !mRequestedTime.undef()) {
    int bestDiff = -1;
    for (int i = 0; i < (int)mCrossectionTimes.size(); i++) {
      const int diff = std::abs(miutil::miTime::secDiff(mCrossectionTimes[i], mRequestedTime));
      if (bestDiff < 0 || diff < bestDiff) {
        bestDiff = diff;
        bestTime = i;
      }
    }
  }
  handleChangedTime(bestTime);
}

void QtManager::setRealization(int r)
{
  mRealization = std::max(std::min(r, mRealizationCount-1), 0);
  dataChange |= CHANGED_CS|CHANGED_SEL|CHANGED_TIME;
  Q_EMIT timeIndexChanged(mPlotTime); // FIXME
}

int QtManager::getTimeIndex() const
{
  return mPlotTime;
}


int QtManager::getTimeCount() const
{
  if (isTimeGraph())
    return 0;
  else
    return mCrossectionTimes.size();
}


QtManager::vctime_t QtManager::getTimeValue(int index) const
{
  if (!isTimeGraph() && index >= 0 && index < (int)mCrossectionTimes.size())
    return mCrossectionTimes[index];
  else
    return vctime_t();
}


void QtManager::handleChangedTimeList()
{
  METLIBS_LOG_SCOPE();
  const ModelReftime model1 = mCollector->getFirstModel();

  std::set<miutil::miTime> times;
  if (!model1.model.empty()) {
    if (Source_p src = mCollector->getSetup()->findSource(model1.model)) {
      if (Inventory_cp inv = src->getInventory(model1.reftime)) {
        for (const auto& tv : inv->times.values)
          times.insert(util::to_miTime(inv->times.unit, tv));
        miutil::maximize(mRealizationCount, inv->realizationCount);
      }
    }
  }
  vctime_v newTimes;
  util::from_set(newTimes, times);

  if (mCrossectionTimes != newTimes) {
    std::swap(mCrossectionTimes, newTimes);
    mPlotTime = -1;

    if (!supportsTimeGraph())
      switchTimeGraph(false);

    dataChange |= CHANGED_TIME;
    Q_EMIT timeListChanged();
    setTimeIndexToBestMatch();
  }
}


void QtManager::handleChangedTime(int plotTimeIndex)
{
  if (isTimeGraph())
    return;
  if (plotTimeIndex != mPlotTime || plotTimeIndex < 0 || mPlotTime < 0) {
    mPlotTime = plotTimeIndex;
    dataChange |= CHANGED_TIME;
    Q_EMIT timeIndexChanged(mPlotTime);
  }
}

void QtManager::setPlotWindow(const QSize& size)
{
  mPlot->viewSetWindow(size);
}

const QSize& QtManager::plotWindow()
{
  return mPlot->viewGetWindow();
}

void QtManager::movePart(int pxmove, int pymove)
{
  mPlot->viewPan(pxmove, pymove);
  saveZoom();
}

void QtManager::decreasePart(int px1, int py1, int px2, int py2)
{
  mPlot->viewZoomIn(px1, py1, px2, py2);
  saveZoom();
}

void QtManager::increasePart()
{
  mPlot->viewZoomOut();
  saveZoom();
}

void QtManager::standardPart()
{
  mPlot->setVerticalAxis();
  mPlot->viewStandard();
  saveZoom();
}

void QtManager::saveZoom()
{
  const std::string id = isTimeGraph() ? "" : getCrossectionLabel().toStdString();
  mCrossectionZooms[id] = mPlot->viewGetCurrentZoom();
}

void QtManager::restoreZoom()
{
  const std::string id = isTimeGraph() ? "" : getCrossectionLabel().toStdString();
  cs_zoom_t::const_iterator it = mCrossectionZooms.find(id);
  if (it != mCrossectionZooms.end())
    mPlot->viewSetCurrentZoom(it->second);
  else
    standardPart();
}

// ------------------------------------------------------------------------

void QtManager::getPlotSize(float& x1, float& y1, float& x2, float& y2, Colour& rubberbandColour)
{
  return mPlot->getPlotSize(x1, y1, x2, y2, rubberbandColour);
}

// ------------------------------------------------------------------------

QString QtManager::axisPosition(int x, int y)
{
  return mPlot->axisPosition(x, y);
}

// ------------------------------------------------------------------------

bool QtManager::plot(QPainter& painter)
{
  METLIBS_LOG_SCOPE();

  try {
    if (dataChange) {
      preparePlot();
      dataChange = CHANGED_NO;
    }

    mPlot->plot(painter);
    return true;
  } catch (std::exception& e) {
    METLIBS_LOG_ERROR("exception: " << e.what());
  } catch (...) {
    METLIBS_LOG_ERROR("unknown exception");
  }
  return false;
}

// ------------------------------------------------------------------------

void QtManager::preparePlot()
{
  METLIBS_LOG_SCOPE(LOGVAL(mCrossectionCurrent) << LOGVAL(isTimeGraph()));

  const ModelReftime model1 = mCollector->getFirstModel();
  if (!hasValidCsIndex() || (!isTimeGraph() && getTimeIndex() < 0) || model1.model.empty()) {
    mPlot->clear();
    return;
  }

  const std::string cs = getCrossectionLabel().toStdString();
  const vcross::Z_AXIS_TYPE zType = mOptions->getVerticalType();

  mPlot->clear((dataChange == CHANGED_NO or dataChange == CHANGED_TIME), (dataChange != CHANGED_SEL));

  if (mCollector->updateRequired()) {
    mCollector->requireVertical(zType);

    if (!model1.model.empty()) {
      vc_require_surface(mCollector, model1);

      if (mOptions->pInflight) {
        vc_require_unit(mCollector, model1, VC_INFLIGHT_PRESSURE, "hPa");
        vc_require_unit(mCollector, model1, VC_INFLIGHT_ALTITUDE, "m");
      }
    }
  }

  model_values_m model_values;
  if (!isTimeGraph()) {
    const Time user_time = util::from_miTime(getTimeValue(getTimeIndex()));
    METLIBS_LOG_DEBUG(LOGVAL(user_time.unit) << LOGVAL(user_time.value));
    model_values = vc_fetch_crossection(mCollector, cs, user_time, mRealization);
    mPlot->setHorizontalCross(cs, getTimeValue(), crossectionPoints(),
        crossectionPointsRequested());
  } else {
    const LonLat& ll = crossection().point(0);
    model_values = vc_fetch_timegraph(mCollector, ll, mRealization);
    mPlot->setHorizontalTime(ll, mCrossectionTimes);
  }

  if (mPlot->setVerticalAxis()) {
    mPlot->viewStandard();
    mCrossectionZooms.clear();
  }

  const EvaluatedPlot_cpv evaluated_plots = vc_evaluate_plots(mCollector, model_values, zType);
  for (EvaluatedPlot_cpv::const_iterator it = evaluated_plots.begin(); it != evaluated_plots.end(); ++it)
    mPlot->addPlot(*it);

  vc_evaluate_surface(mCollector, model_values, model1);
  { std::string surface_id, surface_unit;
    if (zType == vcross::Z_TYPE_PRESSURE) {
      surface_id = VC_SURFACE_PRESSURE;
    } else if (zType == vcross::Z_TYPE_ALTITUDE) {
      surface_id = VC_SURFACE_ALTITUDE;
    }
    surface_unit = zAxisUnit(zType);
    if (not surface_id.empty()) {
      if (InventoryBase_cp surface = mCollector->getResolvedField(model1, surface_id)) {
        METLIBS_LOG_DEBUG(LOGVAL(surface->unit()) << LOGVAL(surface_unit));
        mPlot->setSurface(util::unitConversion(model_values.at(model1).at(surface_id),
                surface->unit(), surface_unit));
      }
    }
  }

  if (mOptions->pInflight) {
    std::string inflight_id, inflight_unit;
    if (zType == vcross::Z_TYPE_PRESSURE) {
      inflight_id = VC_INFLIGHT_PRESSURE;
    } else if (zType == vcross::Z_TYPE_ALTITUDE) {
      inflight_id = VC_INFLIGHT_ALTITUDE;
    }
    inflight_unit = zAxisUnit(zType);
    if (not inflight_id.empty()) {
      if (InventoryBase_cp inflight = mCollector->getResolvedField(model1, inflight_id)) {
        METLIBS_LOG_DEBUG("resolved '" << inflight_id << "'");
        if (Values_cp linevalues = vc_evaluate_field(model_values, model1, inflight)) {
          METLIBS_LOG_DEBUG(LOGVAL(inflight->unit()) << LOGVAL(inflight_unit));
          mPlot->addLine(util::unitConversion(linevalues, inflight->unit(), inflight_unit),
              mOptions->inflightColour, mOptions->inflightLinetype, mOptions->inflightLinewidth);
        }
      }
    }
  }

  for (Marker_v::const_iterator im = mMarkers.begin(); im != mMarkers.end(); ++im) {
    if (im->position >= 0)
      mPlot->addMarker(im->position, im->text, im->colour);
    else
      mPlot->addMarker(im->x, im->y, im->text, im->colour);
  }
  mPlot->setReferencePosition(mReferencePosition);

  mPlot->prepare();
  restoreZoom();
}

// ------------------------------------------------------------------------

std::vector<std::string> QtManager::getAllModels()
{
  METLIBS_LOG_SCOPE();
  return mCollector->getSetup()->getAllModelNames();
}


QtManager::vctime_v QtManager::getModelReferenceTimes(const std::string& modelName)
{
  METLIBS_LOG_SCOPE();
  if (Source_p s = mCollector->getSetup()->findSource(modelName)) {
    s->update();
    const Time_s reftimes = s->getReferenceTimes();
    vctime_v rt;
    rt.reserve(reftimes.size());
    for (Time_s::const_iterator it=reftimes.begin(); it != reftimes.end(); ++it)
      rt.push_back(util::to_miTime(*it));
    return rt;
  }
  return vctime_v();
}


string_v QtManager::getFieldNames(const std::string& model, const vctime_t& reftime, bool includeSelected)
{
  METLIBS_LOG_SCOPE(LOGVAL(model));
  const ModelReftime mr(model, util::from_miTime(reftime));
  const ResolvedPlot_cpv& available_plots = mCollector->getAllResolvedPlots(mr);

  string_v plot_names;
  for (ResolvedPlot_cpv::const_iterator it = available_plots.begin(); it != available_plots.end(); ++it) {
    const std::string& field = (*it)->name();
    if (!includeSelected && findSelectedPlot(PlotSpec(mr, field)))
      continue;
    plot_names.push_back(field);
  }

  return plot_names;
}

miutil::KeyValue_v QtManager::getPlotOptions(const std::string& field, bool fromSetup) const
{
  if (!fromSetup) {
    const string_kv_m::const_iterator itU = userFieldOptions.find(field);
    if (itU != userFieldOptions.end())
      return itU->second;
  }
  return mCollector->getSetup()->getPlotOptions(field);
}


SelectedPlot_p QtManager::getSelectedPlot(int index) const
{
  if (index >= 0 && index < (int)mCollector->getSelectedPlots().size())
    return mCollector->getSelectedPlots()[index];
  else
    return SelectedPlot_p();
}


SelectedPlot_p QtManager::findSelectedPlot(const PlotSpec& ps)
{
  return getSelectedPlot(findSelectedPlotIndex(ps));
}


int QtManager::findSelectedPlotIndex(const PlotSpec& ps)
{
  const SelectedPlot_pv& sps = mCollector->getSelectedPlots();
  for (size_t i=0; i<sps.size(); ++i) {
    SelectedPlot_p sp = sps[i];
    if (sp->model == ps.modelReftime() && sp->name() == ps.field())
      return i;
  }
  return -1;
}


size_t QtManager::getFieldCount() const
{
  return mCollector->getSelectedPlots().size();
}


std::string QtManager::getFieldAt(int position) const
{
  if (vcross::SelectedPlot_p sp = getSelectedPlot(position))
    return sp->name();
  else
    return EMPTY_STRING;
}


std::string QtManager::getModelAt(int position) const
{
  if (vcross::SelectedPlot_p sp = getSelectedPlot(position))
    return sp->model.model;
  else
    return EMPTY_STRING;
}


QtManager::vctime_t QtManager::getReftimeAt(int position) const
{
  if (vcross::SelectedPlot_p sp = getSelectedPlot(position))
    return util::to_miTime(sp->model.reftime);
  else
    return vctime_t();
}


const miutil::KeyValue_v& QtManager::getOptionsAt(int position) const
{
  if (vcross::SelectedPlot_p sp = getSelectedPlot(position))
    return sp->options;
  else
    return EMPTY_OPTIONS;
}


bool QtManager::getVisibleAt(int position) const
{
  if (vcross::SelectedPlot_p sp = getSelectedPlot(position))
    return sp->visible;
  else
    return false;
}


void QtManager::fieldChangeStart(bool script)
{
  inFieldChangeGroup += 1;
  if (inFieldChangeGroup == 1)
    Q_EMIT fieldChangeBegin(script);
}


void QtManager::fieldChangeDone()
{
  if (inFieldChangeGroup > 0) {
    updateCrossectionsTimes();
    if (inFieldChangeGroup == 1)
      Q_EMIT fieldChangeEnd();
    inFieldChangeGroup -= 1;
  } else {
    METLIBS_LOG_ERROR("wrong call to fieldChangeDone, inFieldChangeGroup == " << inFieldChangeGroup);
  }
}


void QtManager::updateCrossectionsTimes()
{
  if (dataChange & CHANGED_SEL) {
    // FIXME getCrossectionLabel call is too late, we have to remember the crossection label before changing the crossection list
    handleChangedCrossectionList(getCrossectionLabel());
    handleChangedTimeList();
  }
}


int QtManager::addField(const PlotSpec& ps, const miutil::KeyValue_v& fieldOpts,
    int idx, bool updateUserFieldOptions)
{
  idx = insertField(ps.modelReftime(), ps.field(), fieldOpts, idx);
  if (idx >= 0 && updateUserFieldOptions)
    userFieldOptions[ps.field()] = fieldOpts;
  return idx;
}


int QtManager::insertField(const ModelReftime& model, const std::string& plot,
    const miutil::KeyValue_v& options, int idx)
{
  idx = mCollector->insertPlot(model, plot, options, idx);
  if (idx >= 0) {
    dataChange |= CHANGED_SEL;

    Q_EMIT fieldAdded(idx);
    if (inFieldChangeGroup == 0)
      updateCrossectionsTimes();
    // TODO trigger plot update
  } else {
    METLIBS_LOG_WARN("could not add plot '" << plot << "' for model '" << model.model
        << "' with reference time " << util::to_miTime(model.reftime));
  }
  return idx;
}


void QtManager::updateField(int idx, const miutil::KeyValue_v& fieldOpts)
{
  METLIBS_LOG_SCOPE(LOGVAL(idx));
  if (idx < 0 || idx >= (int)mCollector->getSelectedPlots().size())
    return;

  SelectedPlot_p sp = getSelectedPlot(idx);
  if (not sp)
    return;

  userFieldOptions[sp->name()] = fieldOpts;
  if (sp->options != fieldOpts) {
    sp->options = fieldOpts;

    dataChange |= CHANGED_SEL;

    Q_EMIT fieldOptionsChanged(idx);
    // TODO trigger plot update
  }
}


void QtManager::removeField(int idx)
{
  if (idx < 0 || idx >= mCollector->countSelectedPlots())
    return;

  mCollector->removePlot(idx);

  dataChange |= CHANGED_SEL;

  Q_EMIT fieldRemoved(idx);
  if (inFieldChangeGroup == 0)
    updateCrossectionsTimes();
  else if (mCollector->countSelectedPlots() == 0)
    cleanupData();
  // TODO trigger plot update
}


void QtManager::moveField(int indexOld, int indexNew)
{
  METLIBS_LOG_SCOPE(LOGVAL(indexOld) << LOGVAL(indexNew));
  if (indexOld < 0 || indexOld >= mCollector->countSelectedPlots()
      || indexNew < 0 || indexNew >= mCollector->countSelectedPlots()
      || indexOld == indexNew)
  {
    return;
  }

  fieldChangeStart(true);
  SelectedPlot_p plot = mCollector->getSelectedPlots().at(indexOld);
  removeField(indexOld);
  insertField(plot->model, plot->name(), plot->options, indexNew);
  fieldChangeDone();
}


void QtManager::removeAllFields()
{
  METLIBS_LOG_SCOPE();
  fieldChangeStart(true);
  while (getFieldCount() > 0) {
    mCollector->removePlot(0);
    dataChange |= CHANGED_SEL;
    Q_EMIT fieldRemoved(0);
    // TODO trigger plot update
  }
  cleanupData();
  fieldChangeDone();
}


void QtManager::setFieldVisible(int index, bool visible)
{
  if (SelectedPlot_p sp = getSelectedPlot(index)) {
    if (visible != sp->visible) {
      sp->visible = visible;
      mCollector->setUpdateRequiredNeeded();

      dataChange |= CHANGED_SEL;

      Q_EMIT fieldVisibilityChanged(index);
      if (inFieldChangeGroup == 0)
        updateCrossectionsTimes();
      // TODO trigger update
    }
  }
}


void QtManager::selectFields(const std::vector<miutil::KeyValue_v>& to_plot)
{
  METLIBS_LOG_SCOPE();

  // TODO this method should be removed from QtManager

  fieldChangeStart(true);
  removeAllFields();

  for (const miutil::KeyValue_v& m_p_o : to_plot) {
    miutil::KeyValue_v poptions;
    std::string model, field;
    vctime_t reftime;
    int refhour = -1, refoffset = 0;

    const bool isMarkerLine = (miutil::find(m_p_o, "MARKER") != size_t(-1));
    const bool isReferenceLine = (miutil::find(m_p_o, "REFERENCE") != size_t(-1));
    std::string markerText, markerColour = "black";
    float markerPosition = -1, markerX = -1, markerY = -1;

    for (const miutil::KeyValue& kv : m_p_o) {
      if (!kv.hasValue())
        continue;
      const std::string& key = kv.key();
      if (isMarkerLine) {
        if (key == "text") {
          markerText = kv.value();
        } else if (key == "colour") {
          markerColour = kv.value();
        } else if (key == "position") {
          markerPosition = kv.toFloat();
        } else if (key == "position.x") {
          markerX = kv.toFloat();
        } else if (key == "position.y") {
          markerY = kv.toFloat();
        }
      } else if (isReferenceLine) {
        if (key == "position") {
          mReferencePosition = kv.toFloat();
        }
      } else {
        if (key == "model") {
          model = kv.value();
        } else if (key == "field") {
          field = kv.value();
        } else if (key == "reftime") {
          reftime = vctime_t(kv.value());
        } else if (key == "refhour") {
          refhour = kv.toInt();
        } else if (key == "refoffset") {
          refoffset = kv.toInt();
        } else if (key == "elevel") {
          mRealization = kv.toInt();
        } else {
          poptions.push_back(kv);
        }
      }
    }
    if (isMarkerLine) {
      if (!markerText.empty()) {
        if (markerPosition >= 0)
          mMarkers.push_back(Marker(markerPosition, markerText, markerColour));
        else if (markerX >= 0 && markerY >= 0)
          mMarkers.push_back(Marker(markerX, markerY, markerText, markerColour));
      }
    } else {
      if (reftime.undef() && !model.empty()) {
        const vctime_v reftimes = getModelReferenceTimes(model);
        if (reftimes.empty())
          METLIBS_LOG_WARN("empty reference time list for model '" << model << "'");
        if (refhour != -1) {
          std::set<std::string> reftimes_txt;
          for (const miutil::miTime& rt : reftimes)
            reftimes_txt.insert(rt.isoTime("T"));
          const std::string reftime_txt = getBestReferenceTime(reftimes_txt, refoffset, refhour);
          if (!reftime_txt.empty())
            reftime = miutil::miTime(reftime_txt);
        }
        if (reftime.undef() && !reftimes.empty())
          reftime = reftimes.back();
      }
      if (!model.empty() && !field.empty() && !reftime.undef())
        addField(PlotSpec(model, reftime, field), poptions, -1, false);
    }
  }

  fieldChangeDone();
}


bool QtManager::supportsTimeGraph()
{
  METLIBS_LOG_SCOPE();
  if (mCollector->getSelectedPlots().empty())
    return true;
  if (mCrossectionTimes.size() < 2)
    return false;
  // TODO need to check if there are any length-1 cross-sections
  return true;
}


void QtManager::switchTimeGraph(bool on)
{
  METLIBS_LOG_SCOPE(LOGVAL(on));
  if (on == mTimeGraphMode)
    return;

  if (on && !supportsTimeGraph())
    on = false;

  mTimeGraphMode = on;
  Q_EMIT timeGraphModeChanged(mTimeGraphMode);
  handleChangedCrossectionList("");

  // we cannot call handleChangedTimeList as mCrossectionTimes does
  // not actually change; what changes is only the return value of
  // getTimeCount(), which checks isTimeGraph()
  Q_EMIT timeListChanged();
  mPlotTime = -1;
  setTimeIndexToBestMatch();

  dataChange |= CHANGED_SEL | CHANGED_TIME;
}


void QtManager::setTimeGraph(const LonLat& position)
{
  METLIBS_LOG_SCOPE();
  if (!supportsTimeGraph())
    return;
  switchTimeGraph(true);

  const ModelReftime model1 = mCollector->getFirstModel();
  if (!model1.valid())
    return;

  Source_p src = mCollector->getSetup()->findSource(model1.model);
  if (!src)
    return;

  Inventory_cp inv = src->getInventory(model1.reftime);
  if (!inv)
    return;

  Crossection_cp cs = inv->findCrossectionPoint(position);
  if (!cs)
    return;

  int cs_index = findCrossectionIndex(QString::fromStdString(cs->label()));
  if (cs_index >= 0)
    setCrossectionIndex(cs_index);
}

// ------------------------------------------------------------------------

std::vector<std::string> QtManager::writeVcrossOptions()
{
  string_v options;
  for (const miutil::KeyValue_v& o : mOptions->writeOptions())
    options.push_back(miutil::mergeKeyValue(o));
  return options;
}


void QtManager::readVcrossOptions(const std::vector<std::string>& log,
    const std::string& thisVersion, const std::string& logVersion)
{
  std::vector<miutil::KeyValue_v> options;
  for (const std::string& line : log)
    options.push_back(miutil::splitKeyValue(line));
  mOptions->readOptions(options);

  updateOptions();
}


std::vector<std::string> QtManager::writePlotOptions()
{
  string_v loglines;

  // this line is for the history -- we have no history, but we have to keep this line
  loglines.push_back("================");

  // write used field options

  for (string_kv_m::const_iterator itO = userFieldOptions.begin(); itO != userFieldOptions.end(); ++itO)
    loglines.push_back(itO->first + " " + miutil::mergeKeyValue(itO->second));
  loglines.push_back("================");

  return loglines;
}


void QtManager::readPlotOptions(const string_v& loglines,
    const std::string& thisVersion, const std::string& logVersion)
{
  userFieldOptions.clear();

  string_v::const_iterator itL = loglines.begin();
  while (itL != loglines.end() and not diutil::startswith(*itL, "===="))
    ++itL;

  if (itL != loglines.end() and diutil::startswith(*itL, "===="))
    ++itL;

  for (; itL != loglines.end(); ++itL) {
    if (diutil::startswith(*itL, "===="))
      break;
    const int firstspace = itL->find_first_of(' ');
    if (firstspace <= 0 or firstspace >= int(itL->size())-1)
      continue;

    const std::string fieldname = itL->substr(0, firstspace);
    const miutil::KeyValue_v options = miutil::splitKeyValue(itL->substr(firstspace+1));

    userFieldOptions[fieldname] = options;
  }
}

} // namespace vcross
