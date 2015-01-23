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

} // namespace anonymous

namespace vcross {

QtManager::QtManager()
  : mCollector(new Collector(miutil::make_shared<Setup>()))
  , mOptions(new VcrossOptions())
  , mPlot(new QtPlot(mOptions))
  , dataChange(0)
  , inFieldChangeGroup(false)
  , mCrossectionCurrent(-1)
  , mTimeGraphPos(-1)
  , mPlotTime(-1)
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
  mTimeGraphPos = -1;
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
  const Source_ps dynSources = listDynamicSources();
  for (Source_ps::iterator it = dynSources.begin(); it != dynSources.end(); ++it)
    (*it)->addDynamicCrossection(label.toStdString(), points);

  handleChangedCrossectionList(label);
}

void QtManager::removeDynamicCrossection(const QString& label)
{
  const QString before = getCrossectionLabel();
  const Source_ps dynSources = listDynamicSources();
  for (Source_ps::iterator it = dynSources.begin(); it != dynSources.end(); ++it) {
    if (Inventory_cp inv = (*it)->getInventory()) {
      if (Crossection_cp cs = inv->findCrossectionByLabel(label.toStdString()))
        (*it)->dropDynamicCrossection(cs);
    }
  }

  handleChangedCrossectionList(before);
}

void QtManager::handleChangedCrossectionList(const QString& oldLabel)
{
  METLIBS_LOG_SCOPE(LOGVAL(oldLabel.toStdString()));

  mCsPredefined.clear();
  const bool hadSupportForDynamicCs = mHasSupportForDynamicCs;
  mHasSupportForDynamicCs = false;

  std::set<LocationElement, lt_LocationElement> le;
  const std::string model1 = mCollector->getFirstModel();
  METLIBS_LOG_DEBUG(LOGVAL(model1));

  string_s labels;
  if (!model1.empty()) {
    if (Source_p src = mCollector->getSetup()->findSource(model1)) {
      if (src->supportsDynamicCrossections()) {
        mHasSupportForDynamicCs = true;
        const std::map<std::string,std::string>& options = mCollector->getSetup()->getModelOptions(model1);
        if (not options.empty()) {
          const std::map<std::string,std::string>::const_iterator it = options.find("predefined_cs");
          if (it != options.end() and not it->second.empty()) {
            mCsPredefined.insert(it->second);
          }
        }
      }
  
      if (Inventory_cp inv = src->getInventory()) {
        for (Crossection_cpv::const_iterator itCS = inv->crossections.begin(); itCS!= inv->crossections.end(); ++itCS) {
          Crossection_cp cs = *itCS;
          
          const LonLat_v& crossectionPoints = (*itCS)->points;
          if (crossectionPoints.size() < 2)
            continue;
          LocationElement el;
          el.name = cs->label;
          for (LonLat_v::const_iterator itL = crossectionPoints.begin(); itL != crossectionPoints.end(); ++itL) {
            el.xpos.push_back(itL->lonDeg());
            el.ypos.push_back(itL->latDeg());
          }
          le.insert(el);
          
          labels.insert(cs->label);
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
    mCrossectionCurrent = -1; //std::min(getCrossectionCount()-1, mCrossectionCurrent);

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
  if (mCrossectionCurrent >= 0 and mCrossectionCurrent < getCrossectionCount()) {
    const std::string& label = mCrossectionLabels.at(mCrossectionCurrent);
    if (Source_p src = mCollector->getSetup()->findSource(mCollector->getFirstModel()))
      if (Inventory_cp inv = src->getInventory())
        if (Crossection_cp cs = inv->findCrossectionByLabel(label))
          mCrossectionPoints = cs->points;
  } else {
    mCrossectionCurrent = -1;
    mTimeGraphPos = -1;
    mCrossectionZooms.clear();
  }

  dataChange |= CHANGED_CS;
  Q_EMIT crossectionIndexChanged(mCrossectionCurrent);
}


void QtManager::getCrossections(LocationData& locationdata)
{
  METLIBS_LOG_SCOPE();
  locationdata = locationData;
}


QtManager::Source_ps QtManager::listDynamicSources() const
{
  Source_ps dynSources;
  const SelectedPlot_pv& spv = mCollector->getSelectedPlots();
  for (SelectedPlot_pv::const_iterator it = spv.begin(); it != spv.end(); ++it) {
    SelectedPlot_cp sp = *it;
    if (Source_p src = mCollector->getSetup()->findSource(sp->model)) {
      if (src->supportsDynamicCrossections())
        dynSources.insert(src);
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
  if (index != mPlotTime && index >= 0 && index < getTimeCount()) {
    mPlotTime = index;
    handleChangedTime();
  }
}


void QtManager::setTimeToBestMatch(const QtManager::vctime_t& time)
{
  METLIBS_LOG_SCOPE(LOGVAL(time));

  int bestTime = -1;
  if (!mCrossectionTimes.empty()) {
    bestTime = 0;
    if (!time.undef()) {
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
  mPlotTime = bestTime;
  handleChangedTime();
}


int QtManager::getTimeIndex() const
{
  return mPlotTime;
}


int QtManager::getTimeCount() const
{
  return mCrossectionTimes.size();
}


QtManager::vctime_t QtManager::getTimeValue(int index) const
{
  if (index >= 0 && index < getTimeCount())
    return mCrossectionTimes.at(index);
  else
    return vctime_t();
}


void QtManager::handleChangedTimeList(const vctime_t& oldTime)
{
  METLIBS_LOG_SCOPE();
  const std::string model1 = mCollector->getFirstModel();

  std::set<miutil::miTime> times;
  if (!model1.empty()) {
    if (Source_p src = mCollector->getSetup()->findSource(model1)) {
      if (Inventory_cp inv = src->getInventory()) {
        for (Times::timevalue_v::const_iterator itT = inv->times.values.begin(); itT != inv->times.values.end(); ++itT)
          times.insert(util::to_miTime(inv->times.unit, *itT));
      }
    }
  }

  vctime_v newTimes;
  util::from_set(newTimes, times);
  if (mCrossectionTimes != newTimes) {
    std::swap(mCrossectionTimes, newTimes);
    mPlotTime = -1; //std::min(getTimeCount()-1, mPlotTime);

    dataChange |= CHANGED_TIME;
    Q_EMIT timeListChanged();
    setTimeToBestMatch(oldTime);
  }
}


void QtManager::handleChangedTime()
{
  dataChange |= CHANGED_TIME;
  Q_EMIT timeIndexChanged(mPlotTime);
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
  mCrossectionZooms[getCrossectionLabel().toStdString()] = mPlot->viewGetCurrentZoom();
}

void QtManager::restoreZoom()
{
  cs_zoom_t::const_iterator it = mCrossectionZooms.find(getCrossectionLabel().toStdString());
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
  METLIBS_LOG_SCOPE();

  const std::string model1 = mCollector->getFirstModel();
  if (getCrossectionIndex() < 0 || getCrossectionIndex() >= getCrossectionCount() || model1.empty()) {
    mPlot->clear();
    return;
  }

  const std::string cs = getCrossectionLabel().toStdString();
  const vcross::Z_AXIS_TYPE zType = mOptions->getVerticalType();

  mPlot->clear((dataChange == CHANGED_NO or dataChange == CHANGED_TIME), (dataChange != CHANGED_SEL));

  if (mCollector->updateRequired()) {
    mCollector->requireVertical(zType);

    if (!model1.empty()) {
      vc_require_surface(mCollector, model1);
      
      if (mOptions->pInflight) {
        vc_require_unit(mCollector, model1, VC_INFLIGHT_PRESSURE, "hPa");
        vc_require_unit(mCollector, model1, VC_INFLIGHT_ALTITUDE, "m");
      }
    }
  }

  model_values_m model_values;
  if (not isTimeGraph()) {
    METLIBS_LOG_DEBUG(LOGVAL(mCrossectionCurrent));
    if (mCrossectionCurrent >= 0) {
      const Time user_time = util::from_miTime(getTimeValue(getTimeIndex()));
      METLIBS_LOG_DEBUG(LOGVAL(user_time.unit) << LOGVAL(user_time.value));
      model_values = vc_fetch_crossection(mCollector, cs, user_time);
      mPlot->setHorizontalCross(cs, mCrossectionPoints);
    }
  } else {
    METLIBS_LOG_DEBUG(LOGVAL(mCrossectionCurrent) << LOGVAL(mTimeGraphPos));
    if (mCrossectionCurrent >= 0 and mTimeGraphPos >= 0) {
      const LonLat& ll = mCrossectionPoints.at(mTimeGraphPos);
      model_values = vc_fetch_timegraph(mCollector, ll);
      mPlot->setHorizontalTime(ll, mCrossectionTimes);
    }
  }

  mPlot->setVerticalAxis();

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


string_v QtManager::getFieldNames(const std::string& model, bool includeSelected)
{
  METLIBS_LOG_SCOPE(LOGVAL(model));
  const ResolvedPlot_cpv& available_plots = mCollector->getAllResolvedPlots(model);

  string_v plot_names;
  for (ResolvedPlot_cpv::const_iterator it = available_plots.begin(); it != available_plots.end(); ++it) {
    const std::string& field = (*it)->name();
    if (!includeSelected && findSelectedPlot(model, field))
      continue;
    plot_names.push_back(field);
  }

  return plot_names;
}

std::string QtManager::getPlotOptions(const std::string& model, const std::string& field, bool fromSetup) const
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


SelectedPlot_p QtManager::findSelectedPlot(const std::string& model, const std::string& field)
{
  return getSelectedPlot(findSelectedPlotIndex(model, field));
}


int QtManager::findSelectedPlotIndex(const std::string& model, const std::string& field)
{
  const SelectedPlot_pv& sps = mCollector->getSelectedPlots();
  for (size_t i=0; i<sps.size(); ++i) {
    SelectedPlot_p sp = sps[i];
    if (sp->model == model && sp->name() == field)
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
    return sp->model;
  else
    return EMPTY_STRING;
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
  inFieldChangeGroup = true;
  Q_EMIT fieldChangeBegin(script);
}


void QtManager::fieldChangeDone()
{
  updateCrossectionsTimes();
  Q_EMIT fieldChangeEnd();
  inFieldChangeGroup = false;
}


void QtManager::updateCrossectionsTimes()
{
  if (dataChange & CHANGED_SEL) {
    // FIXME getCrossectionLabel call is too late, we have to remember the crossection label before changing the crossection list
    handleChangedCrossectionList(getCrossectionLabel());
    handleChangedTimeList(getTimeValue());
  }
}


void QtManager::addField(const std::string& model, const std::string& field,
    const std::string& fieldOpts, int idx, bool updateUserFieldOptions)
{
  if (findSelectedPlot(model, field))
    return;

  idx = mCollector->insertPlot(model, field, miutil::split(fieldOpts), idx);
  if (idx >= 0) {
    if (updateUserFieldOptions)
      userFieldOptions[field] = fieldOpts;

    dataChange |= CHANGED_SEL;

    Q_EMIT fieldAdded(model, field, idx);
    if (!inFieldChangeGroup)
      updateCrossectionsTimes();
    // TODO trigger plot update
  }
}


void QtManager::updateField(const std::string& model, const std::string& field, const std::string& fieldOpts)
{
  METLIBS_LOG_SCOPE(LOGVAL(model) << LOGVAL(field));

  const int idx = findSelectedPlotIndex(model, field);
  METLIBS_LOG_DEBUG(LOGVAL(idx));
  if (idx < 0)
    return;

  SelectedPlot_p sp = getSelectedPlot(idx);
  const string_v nfo = miutil::split(fieldOpts);
  userFieldOptions[field] = fieldOpts;
  if (sp->options != nfo) {
    METLIBS_LOG_DEBUG(LOGVAL(fieldOpts));
    sp->options = nfo;

    dataChange |= CHANGED_SEL;
    
    Q_EMIT fieldOptionsChanged(model, field, idx);
    // TODO trigger plot update
  }
}


void QtManager::removeField(const std::string& model, const std::string& field)
{
  const int idx = findSelectedPlotIndex(model, field);
  if (idx <= 0)
    return;

  mCollector->removePlot(idx);

  dataChange |= CHANGED_SEL;

  Q_EMIT fieldRemoved(model, field, idx);
    if (!inFieldChangeGroup)
      updateCrossectionsTimes();
  // TODO trigger plot update
}


void QtManager::removeAllFields()
{
  METLIBS_LOG_SCOPE();
  while (getFieldCount() > 0) {
    mCollector->removePlot(0);
    dataChange |= CHANGED_SEL;
    Q_EMIT fieldRemoved(getModelAt(0), getFieldAt(0), 0);
    // TODO trigger plot update
  }
  if (!inFieldChangeGroup)
    updateCrossectionsTimes();
}


void QtManager::setFieldVisible(int index, bool visible)
{
  if (SelectedPlot_p sp = getSelectedPlot(index)) {
    if (visible != sp->visible) {
      sp->visible = visible;
      mCollector->setUpdateRequiredNeeded();

      dataChange |= CHANGED_SEL;

      Q_EMIT fieldVisibilityChanged(sp->model, sp->name(), index);
      if (!inFieldChangeGroup)
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
      } else {
        if (!poptions.empty())
          poptions += " ";
        poptions += token;
      }
    }
    
    if (!model.empty() && !field.empty())
      addField(model, field, poptions, -1, false);
  }

  fieldChangeDone();
}


bool QtManager::timeGraphOK()
{
  METLIBS_LOG_SCOPE();
  return (not mCollector->getSelectedPlots().empty());
}

// ------------------------------------------------------------------------

void QtManager::disableTimeGraph()
{
  METLIBS_LOG_SCOPE();
  if (mTimeGraphPos >= 0)
    mCrossectionZooms.clear();
  mTimeGraphPos = -1;
  dataChange |= CHANGED_SEL;
}

// ------------------------------------------------------------------------

void QtManager::setTimeGraphPos(int plotx, int /*ploty*/)
{
  METLIBS_LOG_SCOPE();

  if (mCollector->getSelectedPlots().empty())
    return;

  if (mTimeGraphPos < 0)
    mCrossectionZooms.clear();
  mTimeGraphPos = mPlot->getNearestPos(plotx);
  dataChange |= CHANGED_SEL;
}

// ------------------------------------------------------------------------

void QtManager::setTimeGraphPos(int incr)
{
  METLIBS_LOG_SCOPE();
  if (mTimeGraphPos < 0)
    mCrossectionZooms.clear();
  if (util::step_index(mTimeGraphPos, incr, mCrossectionTimes.size()))
    dataChange |= CHANGED_SEL; // TODO
}

// ------------------------------------------------------------------------

void QtManager::setTimeGraph(const LonLat& position)
{
  METLIBS_LOG_SCOPE();
#if 0
  if (mCollector->getSelectedPlots().empty())
    return;

  const std::string& model1 = mCollector->getSelectedPlots().front()->model;
  Source_p src = mCollector->getSetup()->findSource(model1);
  if (not src)
    return;
  Inventory_cp inv = src->getInventory();
  if (not inv)
    return;
  size_t index;
  Crossection_cp cs = inv->findCrossectionPoint(position, index);
  if (not cs)
    return;

  setCrossection(cs->label);
  if (mTimeGraphPos < 0)
    mCrossectionZooms.clear();
  mTimeGraphPos = index;
  dataChange |= CHANGED_SEL;
#endif
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
