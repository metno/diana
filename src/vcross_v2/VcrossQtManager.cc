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

#define MILOGGER_CATEGORY "diana.VcrossManager"
#include <miLogger/miLogging.h>

namespace /* anonymous */ {

const std::string EMPTY_STRING;
const char KEY_CROSSECTION_EQ[] = "CROSSECTION=";

} // namespace anonymous

namespace vcross {

QtManager::QtManager()
  : mCollector(new Collector(miutil::make_shared<Setup>()))
  , mOptions(new VcrossOptions())
  , mPlot(new QtPlot(mOptions))
  , dataChange(0)
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

void QtManager::parseSetup(const string_v& sources, const string_v&computations, const string_v&plots)
{
  METLIBS_LOG_SCOPE();
  cleanup();
  vc_configure(mCollector->getSetup(), sources, computations, plots);
  mCollector->setupChanged();
}

void QtManager::updateOptions()
{
  preparePlot();
}

void QtManager::cleanup()
{
  METLIBS_LOG_SCOPE();

  mCrossectionCurrent = -1;
  mPlotTime = -1;
  mTimeGraphPos = -1;
  mCrossectionZooms.clear();

  mCollector->clear();
  mPlot->clear();
}

//void QtManager::cleanupDynamicCrossSections()
//{
//  METLIBS_LOG_SCOPE();
//  BOOST_FOREACH(VcrossField* f, miutil::adaptors::values(vcfields)) {
//    if (f)
//      f->cleanup();
//  }
//}

void QtManager::setCrossection(const std::string& csLabel)
{
  METLIBS_LOG_SCOPE();
  if (csLabel == currentCSName())
    return;

  string_v::const_iterator it = std::find(mCrossectionLabels.begin(), mCrossectionLabels.end(), csLabel);
  if (it == mCrossectionLabels.end()) {
    METLIBS_LOG_WARN("crossection '" << csLabel << "' not found");
    return;
  }

  mCrossectionCurrent = (it - mCrossectionLabels.begin());
  dataChange |= CHANGED_CS;
}

void QtManager::setDynamicCrossection(const std::string& csLabel, const LonLat_v& points)
{
  METLIBS_LOG_SCOPE();

  typedef std::set<Source_p> Source_ps;
  Source_ps dynSources;

  const SelectedPlot_cpv& spv = mCollector->getSelectedPlots();
  for (SelectedPlot_cpv::const_iterator it = spv.begin(); it != spv.end(); ++it) {
    SelectedPlot_cp sp = *it;
    if (Source_p src = mCollector->getSetup()->findSource(sp->model)) {
      if (src->supportsDynamicCrossections())
        dynSources.insert(src);
    }
  }

  if (points.size() >= 2) {
    for (Source_ps::iterator it = dynSources.begin(); it != dynSources.end(); ++it)
      (*it)->addDynamicCrossection(csLabel, points);
  } else {
    for (Source_ps::iterator it = dynSources.begin(); it != dynSources.end(); ++it) {
      if (Inventory_cp inv = (*it)->getInventory()) {
        if (Crossection_cp cs = inv->findCrossectionByLabel(csLabel))
          (*it)->dropDynamicCrossection(cs);
      }
    }
  }

  setModels();
  dataChange |= CHANGED_SEL;
}

std::string QtManager::setCrossection(int step)
{
  METLIBS_LOG_SCOPE(LOGVAL(step));

  if (mCrossectionLabels.empty()) {
    METLIBS_LOG_DEBUG("no cs labels, cannot step cs");
    return "";
  }

  if (vcross::util::step_index(mCrossectionCurrent, step, mCrossectionLabels.size())) {
    dataChange |= CHANGED_CS;
  }

  return currentCSName();
}

const std::string& QtManager::currentCSName() const
{
  if (mCrossectionCurrent >= 0 and mCrossectionCurrent < (int)mCrossectionLabels.size())
    return mCrossectionLabels.at(mCrossectionCurrent);
  else
    return EMPTY_STRING;
}

void QtManager::updateCSPoints()
{
  METLIBS_LOG_SCOPE();

  // FIXME this is a bad function, it will not work if models have the same cross-section label but different points

  const std::string& csLabel = currentCSName();
  METLIBS_LOG_DEBUG(LOGVAL(csLabel));
  const SelectedPlot_cpv& spv = mCollector->getSelectedPlots();
  for (SelectedPlot_cpv::const_iterator it = spv.begin(); it != spv.end(); ++it) {
    if (Source_p src = mCollector->getSetup()->findSource((*it)->model)) {
      if (Inventory_cp inv = src->getInventory()) {
        if (Crossection_cp cs = inv->findCrossectionByLabel(csLabel)) {
          mCrossectionPoints = cs->points;
          METLIBS_LOG_DEBUG("found " << mCrossectionPoints.size() << " cs points");
          return;
        }
      }
    }
  }
  METLIBS_LOG_DEBUG("no cs points found");
  mCrossectionPoints.clear();
  mCrossectionCurrent = -1;
  mTimeGraphPos = -1;
  mCrossectionZooms.clear();
}

// ------------------------------------------------------------------------

void QtManager::getCrossections(LocationData& locationdata)
{
  METLIBS_LOG_SCOPE();
  locationdata = locationData;
}

// ------------------------------------------------------------------------

namespace {
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
} // namespace

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


void QtManager::setTime(const miutil::miTime& time)
{
  METLIBS_LOG_SCOPE();

  times_t::const_iterator it = std::find(mCrossectionTimes.begin(), mCrossectionTimes.end(), time);
  if (it == mCrossectionTimes.end()) {
    METLIBS_LOG_WARN("time " << time << " not found");
    return;
  }

  mPlotTime = (it - mCrossectionTimes.begin());
  dataChange |= CHANGED_TIME;
}


QtManager::vctime_t QtManager::setTime(int step)
{
  METLIBS_LOG_SCOPE(LOGVAL(step));

  if (mCrossectionTimes.empty())
    return miutil::miTime::nowTime();

  if (vcross::util::step_index(mPlotTime, step, mCrossectionTimes.size()))
    dataChange |= CHANGED_TIME;
  return currentTime();
}

// ------------------------------------------------------------------------

void QtManager::setTimeToBestMatch(const vctime_t& time)
{
  METLIBS_LOG_SCOPE(LOGVAL(time));

  if (mCrossectionTimes.empty()) {
    mPlotTime = -1;
    return;
  }

  int bestTime = 0;
  int bestDiff = std::abs(miutil::miTime::minDiff(mCrossectionTimes.front(), time));
  for (size_t i=1; i<mCrossectionTimes.size(); i++) {
    const int diff = std::abs(miutil::miTime::minDiff(mCrossectionTimes[i], time));
    if (diff < bestDiff) {
      bestDiff = diff;
      bestTime = i;
    }
  }
  mPlotTime = bestTime;
  METLIBS_LOG_DEBUG(LOGVAL(mPlotTime));
  dataChange |= CHANGED_TIME;
}

QtManager::vctime_t QtManager::currentTime() const
{
  METLIBS_LOG_SCOPE(LOGVAL(mPlotTime) << LOGVAL(mCrossectionTimes.size()));

  if (mPlotTime >= 0 and mPlotTime < (int)mCrossectionTimes.size())
    return mCrossectionTimes.at(mPlotTime);
  else if (not mCrossectionTimes.empty())
    return mCrossectionTimes.front();
  else
    return vctime_t::nowTime();
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
  mCrossectionZooms[currentCSName()] = mPlot->viewGetCurrentZoom();
}

void QtManager::restoreZoom()
{
  cs_zoom_t::const_iterator it = mCrossectionZooms.find(currentCSName());
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

  const std::string& cs = currentCSName();
  if (cs.empty()) {
    METLIBS_LOG_WARN("no crossection chosen");
    return;
  }

  updateCSPoints();

  if (mCollector->getSelectedPlots().empty()) {
    mPlot->clear();
    return;
  }

  mPlot->clear((dataChange == CHANGED_NO or dataChange == CHANGED_TIME), (dataChange != CHANGED_SEL));

  vcross::Z_AXIS_TYPE zType;
  if(mOptions->verticalCoordinate == "Pressure")
    zType = vcross::Z_TYPE_PRESSURE;
  else
    zType = vcross::Z_TYPE_ALTITUDE;
  mCollector->requireVertical(zType);

  model_values_m model_values;
  if (not isTimeGraph()) {
    METLIBS_LOG_DEBUG(LOGVAL(mCrossectionCurrent));
    if (mCrossectionCurrent >= 0) {
      const Time user_time = util::from_miTime(currentTime());
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

  // TODO decide if to plot surface altitude or pressure
  const std::string& model1 = mCollector->getSelectedPlots().front()->model;
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

// ------------------------------------------------------------------------

std::map<std::string,std::string> QtManager::getAllFieldOptions()
{
  METLIBS_LOG_SCOPE();
  return  mCollector->getSetup()->getAllPlotOptions();
}

// ------------------------------------------------------------------------

string_v QtManager::getFieldNames(const std::string& model)
{
  METLIBS_LOG_SCOPE(LOGVAL(model));
  const ResolvedPlot_cpv& available_plots = mCollector->getAllResolvedPlots(model);
  string_v plot_names;
  for (ResolvedPlot_cpv::const_iterator it = available_plots.begin(); it != available_plots.end(); ++it)
    plot_names.push_back((*it)->name());
  return plot_names;
}

// ------------------------------------------------------------------------

void QtManager::mainWindowTimeChanged(const miutil::miTime& mwTime)
{
  METLIBS_LOG_SCOPE();
  setTimeToBestMatch(mwTime);
}

// ------------------------------------------------------------------------

bool QtManager::setSelection(const string_v& vstr)
{
  METLIBS_LOG_SCOPE();

  // save plotStrings
  mPlotStrings = vstr;
  mPlot->clear();

  mCollector->clear();
  vc_select_plots(mCollector, vstr);

  if (not mCollector->getSelectedPlots().empty()) {
    const std::string& model1 = mCollector->getSelectedPlots().front()->model;
    vc_require_surface(mCollector, model1);

    if (mOptions->pInflight) {
      vc_require_unit(mCollector, model1, VC_INFLIGHT_PRESSURE, "hPa");
      vc_require_unit(mCollector, model1, VC_INFLIGHT_ALTITUDE, "m");
    }
  }

  dataChange |= CHANGED_SEL;
  return setModels();
}

// ------------------------------------------------------------------------

bool QtManager::setModels()
{
  METLIBS_LOG_SCOPE();

  mCsPredefined.clear();
  mHasSupportForDynamicCs = false;

  const vctime_t timeBefore = currentTime(); // remember current time, search for it later
  const std::string csBefore = currentCSName(); // remember current cs' name, search for it later

  typedef std::set<std::string> string_s;

  string_s models;
  const SelectedPlot_cpv& spv = mCollector->getSelectedPlots();
  for (SelectedPlot_cpv::const_iterator it = spv.begin(); it != spv.end(); ++it)
    models.insert((*it)->model);

  std::set<LocationElement, lt_LocationElement> le;

  string_s csLabels;
  std::set<miutil::miTime> times;
  for (string_s::const_iterator itM = models.begin(); itM != models.end(); ++itM) {
    const std::string& m = *itM;
    METLIBS_LOG_DEBUG(LOGVAL(m));

    if (Source_p src = mCollector->getSetup()->findSource(m)) {
      if (src->supportsDynamicCrossections()) {
        METLIBS_LOG_DEBUG("model '" << m << "' supports dynamic cs");
        mHasSupportForDynamicCs = true;
        const std::map<std::string,std::string>& options = mCollector->getSetup()->getModelOptions(m);
        METLIBS_LOG_DEBUG("model '" << m << "' has " << options.size() << " options");
        if (not options.empty()) {
          const std::map<std::string,std::string>::const_iterator it = options.find("predefined_cs");
          METLIBS_LOG_DEBUG("model '" << m << "' has " << it->second << "'");
          if (it != options.end() and not it->second.empty()) {
            mCsPredefined.insert(it->second);
            METLIBS_LOG_DEBUG("model '" << m << "' has predefined cs '" << it->second << "'");
          }
        }
      } else {
        METLIBS_LOG_DEBUG("model '" << m << "' does not support dynamic cs");
      }

      if (Inventory_cp inv = src->getInventory()) {
        METLIBS_LOG_DEBUG(LOGVAL(inv->crossections.size()));
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

          csLabels.insert(cs->label);
          METLIBS_LOG_DEBUG(LOGVAL(cs->label));
        }

        for (Times::timevalue_v::const_iterator itT = inv->times.values.begin(); itT != inv->times.values.end(); ++itT)
          times.insert(util::to_miTime(inv->times.unit, *itT));
      }
    }
  }
  bool modelChange = false;

  string_v newCrossectionLabels;
  times_t newCrossectionTimes;
  util::from_set(newCrossectionLabels, csLabels);
  util::from_set(newCrossectionTimes, times);
  if (newCrossectionTimes != mCrossectionTimes or newCrossectionLabels != mCrossectionLabels)
    modelChange = true;
  std::swap(mCrossectionLabels, newCrossectionLabels);
  std::swap(mCrossectionTimes,  newCrossectionTimes);

  locationData.elements.clear();
  locationData.elements.insert(locationData.elements.end(), le.begin(), le.end());

  fillLocationData(locationData);

  if (mCrossectionLabels.empty() or mCrossectionTimes.empty()) {
    METLIBS_LOG_DEBUG("no times or crossections");
    mCrossectionCurrent = -1;
    mPlotTime = -1;
    return true;
  }

  setTimeToBestMatch(timeBefore);

  if (mCrossectionCurrent < 0 // no previous cs
      or csLabels.find(csBefore) == csLabels.end()) // current cs' name no longer known
  {
    mCrossectionCurrent = 0;
    modelChange = true;
  }
  return modelChange;
}

// ------------------------------------------------------------------------

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
  dataChange = CHANGED_SEL;
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
  dataChange = CHANGED_SEL;
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
  dataChange = CHANGED_SEL;
}

// ------------------------------------------------------------------------

void QtManager::parseQuickMenuStrings(const std::vector<std::string>& qm_lines)
{
  string_v vcross_data, vcross_options;
  std::string crossection;

  for (string_v::const_iterator it = qm_lines.begin(); it != qm_lines.end(); ++it) { // copy because it has to be trimmed
    const std::string line = miutil::trimmed(*it);
    if (line.empty())
      continue;

    const std::string upline = miutil::to_upper(line);

    if (diutil::startswith(upline, KEY_CROSSECTION_EQ)) {
      // next line assumes that miutil::to_upper does not change character count
      crossection = line.substr(sizeof(KEY_CROSSECTION_EQ)-1);
      if (miutil::contains(crossection, "\""))
        miutil::remove(crossection, '\"');
    } else if (miutil::contains(upline, "VCROSS ")) {
      vcross_data.push_back(line);
    } else {
      // assume plot-options
      vcross_options.push_back(line);
    }
  }

  mOptions->readOptions(vcross_options);
  setSelection(vcross_data);
  setCrossection(crossection);
}

// ------------------------------------------------------------------------

std::vector<std::string> QtManager::getQuickMenuStrings()
{
  std::vector<std::string> qm_lines;
  qm_lines.push_back("VCROSS");

  const std::vector<std::string> vstrOpt = getOptions()->writeOptions();
  qm_lines.insert(qm_lines.end(), vstrOpt.begin(),  vstrOpt.end());

  qm_lines.insert(qm_lines.end(), mPlotStrings.begin(), mPlotStrings.end());

  qm_lines.push_back(KEY_CROSSECTION_EQ + getCrossection());
  return qm_lines;
}

// ------------------------------------------------------------------------

std::vector<std::string> QtManager::writeLog()
{
  return mOptions->writeOptions();
}

// ------------------------------------------------------------------------

void QtManager::readLog(const std::vector<std::string>& log,
    const std::string& thisVersion, const std::string& logVersion)
{
  mOptions->readOptions(log);
}

} // namespace vcross
