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

#include "VcrossQtManager.h"

#include "diPlotOptions.h"
#include "diUtilities.h"
#include "VcrossComputer.h"
#include "VcrossOptions.h"
#include "VcrossQtPlot.h"

#include <diField/VcrossUtil.h>

#include <puTools/mi_boost_compatibility.hh>
#include <puTools/miSetupParser.h>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/join.hpp>

#define MILOGGER_CATEGORY "diana.VcrossManager"
#include <miLogger/miLogging.h>

namespace /* anonymous */ {

const std::string EMPTY_STRING;

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

miutil::miTime getBestReferenceTime(const std::vector<miutil::miTime>& reftimes,
    int refOffset, int refHour)
{
  // FIXME this is an almost verbatim copy of diField FieldManager::getBestReferenceTime

  if (reftimes.empty())
    return miutil::miTime();

  //if refime is not a valid time, return string
  miutil::miTime reftime = reftimes.back();
  if (reftime.undef()) {
    return reftime;
  }

  if (refHour > -1) {
    miutil::miDate date = reftime.date();
    miutil::miClock clock(refHour, 0, 0);
    reftime = miutil::miTime(date, clock);
  }

  reftime.addDay(refOffset);
  if (std::find(reftimes.begin(), reftimes.end(), reftime) != reftimes.end())
    return reftime;

  //referencetime not found. If refHour is given and no refoffset, try yesterday
  if (refHour > -1 && refOffset == 0) {
    reftime.addDay(-1);
    if (std::find(reftimes.begin(), reftimes.end(), reftime) != reftimes.end())
      return reftime;
  }
  return miutil::miTime();
}

} // namespace anonymous

namespace vcross {

QtManager::PlotSpec::PlotSpec(const std::string& model, const vctime_t& reftime, const std::string& field)
  : mModelReftime(model, util::from_miTime(reftime))
  , mField(field)
{
}

// ########################################################################

QtManager::QtManager()
  : mCollector(new Collector(miutil::make_shared<Setup>()))
  , mOptions(new VcrossOptions())
  , mPlot(new QtPlot(mOptions))
  , dataChange(0)
  , inFieldChangeGroup(0)
  , mCrossectionCurrent(-1)
  , mPlotTime(-1)
  , mTimeGraphMode(false)
  , mHasSupportForDynamicCs(false)
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

  mCrossectionCurrent = -1;
  mPlotTime = -1;
  mTimeGraphMode = false;
  mCrossectionZooms.clear();

  removeAllFields();

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
  mCollector->setUpdateRequiredNeeded(); // might (no longer) need to request inflight
  preparePlot();
}


int QtManager::getCrossectionIndex() const
{
  return mCrossectionCurrent;
}


void QtManager::setCrossectionIndex(int index)
{
  METLIBS_LOG_SCOPE(LOGVAL(index));
  if (index != mCrossectionCurrent && index >= 0 && index < getCrossectionCount()) {
    mCrossectionCurrent = index;
    handleChangedCrossection();
  }
}


int QtManager::findCrossectionIndex(const QString& label)
{
  METLIBS_LOG_SCOPE(LOGVAL(label.toStdString()));
  for (int i=0; i<getCrossectionCount(); ++i) {
    if (label == getCrossectionLabel(i))
      return i;
  }
  return -1;
}


QString QtManager::getCrossectionLabel(int index) const
{
  if (index >= 0 && index < getCrossectionCount())
    return QString::fromStdString(mCrossectionLabels.at(index));
  else
    return QString();
}


int QtManager::getCrossectionCount() const
{
  return mCrossectionLabels.size();
}


void QtManager::addDynamicCrossection(const QString& label, const LonLat_v& points)
{
  const Source_Reftime_ps dynSources = listDynamicSources();
  for (Source_Reftime_ps::iterator it = dynSources.begin(); it != dynSources.end(); ++it) {
    Source_p src = it->first;
    const Time& reftime = it->second;
    src->addDynamicCrossection(reftime, label.toStdString(), points);
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

  mCsPredefined.clear();
  const bool hadSupportForDynamicCs = mHasSupportForDynamicCs;
  mHasSupportForDynamicCs = false;

  std::set<LocationElement, lt_LocationElement> le;
  const ModelReftime model1 = mCollector->getFirstModel();
  METLIBS_LOG_DEBUG(LOGVAL(model1));

  string_s labels;
  if (!model1.model.empty()) {
    if (Source_p src = mCollector->getSetup()->findSource(model1.model)) {
      if (src->supportsDynamicCrossections(model1.reftime)) {
        mHasSupportForDynamicCs = true;
        const std::map<std::string,std::string>& options = mCollector->getSetup()->getModelOptions(model1.model);
        if (not options.empty()) {
          const std::map<std::string,std::string>::const_iterator it = options.find("predefined_cs");
          if (it != options.end() and not it->second.empty()) {
            mCsPredefined.insert(it->second);
          }
        }
      }

      if (Inventory_cp inv = src->getInventory(model1.reftime)) {
        for (Crossection_cpv::const_iterator itCS = inv->crossections.begin(); itCS!= inv->crossections.end(); ++itCS) {
          Crossection_cp cs = *itCS;

          if ((isTimeGraph() && cs->length() != 1)
              || (!isTimeGraph() && cs->length() < 2))
            continue;
          LocationElement el;
          el.name = cs->label();
          for (size_t i = 0; i<cs->length(); ++i) {
            el.xpos.push_back(cs->point(i).lonDeg());
            el.ypos.push_back(cs->point(i).latDeg());
          }
          le.insert(el);

          labels.insert(cs->label());
        }
      }
    }
  }

  string_v newLabels;
  util::from_set(newLabels, labels);
  if (newLabels != mCrossectionLabels
      || hadSupportForDynamicCs != mHasSupportForDynamicCs)
  {
    dataChange |= CHANGED_CS;
    std::swap(mCrossectionLabels, newLabels);
    mCrossectionCurrent = -1;

    locationData.elements.clear();
    locationData.elements.insert(locationData.elements.end(), le.begin(), le.end());
    fillLocationData(locationData);

    Q_EMIT crossectionListChanged();
    if (getCrossectionCount() > 0)
      setCrossectionIndex(std::max(0, findCrossectionIndex(oldLabel)));
  }
}

void QtManager::handleChangedCrossection()
{
  // FIXME this is a bad function, it will not work if models have the same cross-section label but different points

  mCrossectionPoints.clear();
  mCrossectionPointsRequested.clear();
  if (mCrossectionCurrent >= 0 and mCrossectionCurrent < getCrossectionCount()) {
    const std::string& label = mCrossectionLabels.at(mCrossectionCurrent);
    const ModelReftime& mr = mCollector->getFirstModel();
    if (Source_p src = mCollector->getSetup()->findSource(mr.model))
      if (Inventory_cp inv = src->getInventory(mr.reftime))
        if (Crossection_cp cs = inv->findCrossectionByLabel(label)) {
          mCrossectionPoints.reserve(cs->length());
          for (size_t i = 0; i<cs->length(); ++i)
            mCrossectionPoints.push_back(cs->point(i));
          mCrossectionPointsRequested.reserve(cs->lengthRequested());
          for (size_t i = 0; i<cs->lengthRequested(); ++i)
            mCrossectionPointsRequested.push_back(cs->pointRequested(i));
        }
  } else {
    mCrossectionCurrent = -1;
  }

  dataChange |= CHANGED_CS;
  Q_EMIT crossectionIndexChanged(mCrossectionCurrent);
}


void QtManager::getCrossections(LocationData& locationdata)
{
  locationdata = locationData;
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


void QtManager::fillLocationData(LocationData& ld)
{
  METLIBS_LOG_SCOPE();

  Projection pgeo;
  pgeo.setGeographic();
  const Rectangle rgeo(0, 0, 90, 360);
  const Area geoArea(pgeo, rgeo);

  std::ostringstream annot;
  annot << "Vertikalsnitt";
  ld.name =              "vcross";
  ld.locationType =      location_line;
  ld.area =              geoArea;
  ld.annotation =        annot.str();
  ld.colour =            mOptions->vcOnMapColour;
  ld.linetype =          mOptions->vcOnMapLinetype;
  ld.linewidth =         mOptions->vcOnMapLinewidth;
  ld.colourSelected =    mOptions->vcSelectedOnMapColour;
  ld.linetypeSelected =  mOptions->vcSelectedOnMapLinetype;
  ld.linewidthSelected = mOptions->vcSelectedOnMapLinewidth;
}


void QtManager::setTimeIndex(int index)
{
  if (index >= 0 && index < getTimeCount())
    handleChangedTime(index);
}


void QtManager::setTimeToBestMatch(const QtManager::vctime_t& time)
{
  METLIBS_LOG_SCOPE();

  int bestTime = -1;
  if (getTimeCount()>0) {
    bestTime = 0;
    if (!time.undef()) {
      METLIBS_LOG_DEBUG(LOGVAL(time));
      int bestDiff = std::abs(miutil::miTime::minDiff(getTimeValue(0), time));
      for (int i=1; i<getTimeCount(); i++) {
        const int diff = std::abs(miutil::miTime::minDiff(getTimeValue(i), time));
        if (diff < bestDiff) {
          bestDiff = diff;
          bestTime = i;
        }
      }
    }
  }
  handleChangedTime(bestTime);
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
  if (!isTimeGraph() && index >= 0 && index < getTimeCount())
    return mCrossectionTimes.at(index);
  else
    return vctime_t();
}


void QtManager::handleChangedTimeList(const vctime_t& oldTime)
{
  METLIBS_LOG_SCOPE();
  const ModelReftime model1 = mCollector->getFirstModel();

  std::set<miutil::miTime> times;
  if (!model1.model.empty()) {
    if (Source_p src = mCollector->getSetup()->findSource(model1.model)) {
      if (Inventory_cp inv = src->getInventory(model1.reftime)) {
        for (Times::timevalue_v::const_iterator itT = inv->times.values.begin(); itT != inv->times.values.end(); ++itT)
          times.insert(util::to_miTime(inv->times.unit, *itT));
      }
    }
  }
  vctime_v newTimes;
  util::from_set(newTimes, times);

  if (mCrossectionTimes != newTimes) {
    std::swap(mCrossectionTimes, newTimes);
    mPlotTime = -1;

    if (mTimeGraphMode && !supportsTimeGraph())
      switchTimeGraph(false);

    dataChange |= CHANGED_TIME;
    Q_EMIT timeListChanged();
    setTimeToBestMatch(oldTime);
  }
}


void QtManager::handleChangedTime(int plotTimeIndex)
{
  if (plotTimeIndex != mPlotTime) {
    mPlotTime = plotTimeIndex;
    dataChange |= CHANGED_TIME;
    Q_EMIT timeIndexChanged(mPlotTime);
  }
}


void QtManager::setPlotWindow(int w, int h)
{
  mPlot->viewSetWindow(w, h);
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
  if (getCrossectionIndex() < 0 || getCrossectionIndex() >= getCrossectionCount()
      || (isTimeGraph() && mCrossectionPoints.size() != 1)
      || model1.model.empty())
  {
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
    model_values = vc_fetch_crossection(mCollector, cs, user_time);
    mPlot->setHorizontalCross(cs, getTimeValue(), mCrossectionPoints,
        mCrossectionPointsRequested);
  } else {
    const LonLat& ll = mCrossectionPoints.at(0);
    model_values = vc_fetch_timegraph(mCollector, ll);
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
      surface_unit = "hPa";
    } else if (zType == vcross::Z_TYPE_ALTITUDE) {
      surface_id = VC_SURFACE_ALTITUDE;
      surface_unit = "m";
    }
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
      inflight_unit = "hPa";
    } else if (zType == vcross::Z_TYPE_ALTITUDE) {
      inflight_id = VC_INFLIGHT_ALTITUDE;
      inflight_unit = "m";
    }
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

std::string QtManager::getPlotOptions(const std::string& field, bool fromSetup) const
{
  if (!fromSetup) {
    const string_string_m::const_iterator itU = userFieldOptions.find(field);
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


std::string QtManager::getOptionsAt(int position) const
{
  if (vcross::SelectedPlot_p sp = getSelectedPlot(position))
    return sp->optionString();
  else
    return EMPTY_STRING;
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
  if (inFieldChangeGroup == 1) {
    updateCrossectionsTimes();
    Q_EMIT fieldChangeEnd();
  }
  if (inFieldChangeGroup > 0)
    inFieldChangeGroup -= 1;
  else
    METLIBS_LOG_ERROR("wrong call to fieldChangeDone, inFieldChangeGroup == " << inFieldChangeGroup);
}


void QtManager::updateCrossectionsTimes()
{
  if (dataChange & CHANGED_SEL) {
    // FIXME getCrossectionLabel call is too late, we have to remember the crossection label before changing the crossection list
    handleChangedCrossectionList(getCrossectionLabel());
    handleChangedTimeList(getTimeValue());
  }
}


void QtManager::addField(const PlotSpec& ps, const std::string& fieldOpts,
    int idx, bool updateUserFieldOptions)
{
  idx = insertField(ps.modelReftime(), ps.field(), miutil::split(fieldOpts), idx);
  if (idx >= 0 && updateUserFieldOptions)
    userFieldOptions[ps.field()] = fieldOpts;
}


int QtManager::insertField(const ModelReftime& model, const std::string& plot,
    const string_v& options, int idx)
{
  METLIBS_LOG_SCOPE(LOGVAL(idx));
  idx = mCollector->insertPlot(model, plot, options, idx);
  METLIBS_LOG_DEBUG(LOGVAL(idx));
  if (idx >= 0) {
    dataChange |= CHANGED_SEL;

    Q_EMIT fieldAdded(idx);
    if (inFieldChangeGroup == 0)
      updateCrossectionsTimes();
    // TODO trigger plot update
  }
  return idx;
}


void QtManager::updateField(int idx, const std::string& fieldOpts)
{
  METLIBS_LOG_SCOPE(LOGVAL(idx));
  if (idx < 0 || idx >= (int)mCollector->getSelectedPlots().size())
    return;

  SelectedPlot_p sp = getSelectedPlot(idx);
  if (not sp)
    return;

  const string_v nfo = miutil::split(fieldOpts);
  userFieldOptions[sp->name()] = fieldOpts;
  if (sp->options != nfo) {
    METLIBS_LOG_DEBUG(LOGVAL(fieldOpts));
    sp->options = nfo;

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


void QtManager::selectFields(const string_v& to_plot)
{
  METLIBS_LOG_SCOPE();

  // TODO this method should be removed from QtManager

  fieldChangeStart(true);
  removeAllFields();

  for (string_v::const_iterator itL = to_plot.begin(); itL != to_plot.end(); ++itL) {
    const std::string& line = *itL;
    if (util::isCommentLine(line))
      continue;
    METLIBS_LOG_DEBUG(LOGVAL(line));

    string_v m_p_o = miutil::split(line);
    std::string poptions;
    std::string model, field;
    vctime_t reftime;
    int refhour = -1, refoffset = 0;

    for (string_v::const_iterator itT = m_p_o.begin(); itT != m_p_o.end(); ++itT) {
      const std::string& token = *itT;
      string_v stoken = miutil::split(token,"=");
      if (stoken.size() != 2)
        continue;
      std::string key = boost::algorithm::to_lower_copy(stoken[0]);
      if (key == "model") {
        model = stoken[1];
      } else if (key == "field") {
        field = stoken[1];
      } else if (key == "reftime") {
        reftime = vctime_t(stoken[1]);
      } else if (key == "refhour") {
        refhour = miutil::to_int(stoken[1]);
      } else if (key == "refoffset") {
        refoffset = miutil::to_int(stoken[1]);
      } else {
        if (!poptions.empty())
          poptions += " ";
        poptions += token;
      }
    }
    if (reftime.undef() && !model.empty()) {
      const vctime_v reftimes = getModelReferenceTimes(model);
      if (refhour != -1)
        reftime = getBestReferenceTime(reftimes, refoffset, refhour);
      if (reftime.undef() && !reftimes.empty())
        reftime = reftimes.back();
    }
    if (!model.empty() && !field.empty() && !reftime.undef())
      addField(PlotSpec(model, reftime, field), poptions, -1, false);
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
  if (!mCrossectionTimes.empty())
    setTimeToBestMatch(mCrossectionTimes[0]);

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
  return mOptions->writeOptions();
}


void QtManager::readVcrossOptions(const std::vector<std::string>& log,
    const std::string& thisVersion, const std::string& logVersion)
{
  mOptions->readOptions(log);
  updateOptions();
}


std::vector<std::string> QtManager::writePlotOptions()
{
  string_v loglines;

  // this line is for the history -- we have no history, but we have to keep this line
  loglines.push_back("================");

  // write used field options

  for (string_string_m::const_iterator itO = userFieldOptions.begin(); itO != userFieldOptions.end(); ++itO)
    loglines.push_back(itO->first + " " + itO->second);
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
    const std::string options = itL->substr(firstspace+1);

    userFieldOptions[fieldname] = options;
  }
}

} // namespace vcross
