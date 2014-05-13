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

#include "VcrossQtManager.h"

#include "diPlotOptions.h"
//#include "diLocationPlot.h"
#include "VcrossComputer.h"
#include "VcrossOptions.h"
#include "VcrossQtPlot.h"
//#include "diVcrossSetup.h"
#include <diField/VcrossUtil.h>

#include <puTools/mi_boost_compatibility.hh>
#include <puTools/miSetupParser.h>

#include <boost/foreach.hpp>
#include <boost/range/algorithm.hpp>

#define MILOGGER_CATEGORY "vcross.QtManager"
#include <miLogger/miLogging.h>

namespace /* anonymous */ {

const std::string EMPTY_STRING;

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
{
  METLIBS_LOG_SCOPE();
  string_v sources;
  miutil::SetupParser::getSection("VERTICAL_CROSSECTION_FILES", sources);
  string_v computations;
  miutil::SetupParser::getSection("VERTICAL_CROSSECTION_COMPUTATIONS", computations);
  string_v plots;
  miutil::SetupParser::getSection("VERTICAL_CROSSECTION_PLOTS", plots);
  parseSetup(sources,computations,plots);

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

void QtManager::readOptions(const string_v& vstr)
{
  return mOptions->readOptions(vstr);
}

void QtManager::cleanup()
{
  METLIBS_LOG_SCOPE();

  mCrossectionCurrent = -1;
  mPlotTime = -1;
  mTimeGraphPos = -1;

  mCollector->clear();
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

  string_v::const_iterator it = std::find(mCrossectionLabels.begin(), mCrossectionLabels.end(), csLabel);
  if (it == mCrossectionLabels.end()) {
    METLIBS_LOG_WARN("crossection '" << csLabel << "' not found");
    return;
  }

  mCrossectionCurrent = (it - mCrossectionLabels.begin());
  dataChange |= CHANGED_CS;
}

//bool QtManager::setCrossection(float lat, float lon)
//{
//  METLIBS_LOG_SCOPE(LOGVAL(lat) << LOGVAL(lon));
//
//  std::set<std::string> csnames(nameList.begin(), nameList.end());
//  BOOST_FOREACH(VcrossField* f, miutil::adaptors::values(vcfields)) {
//    const std::string n = f->setLatLon(lat, lon);
//    if (not n.empty())
//      csnames.insert(n);
//  }
//  VcrossUtil::from_set(nameList, csnames);
//  dataChange |= CHANGED_CS;
//  return true;
//}

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
  BOOST_FOREACH(SelectedPlot_cp sp, mCollector->getSelectedPlots()) {
    if (Source_p src = mCollector->getSetup()->findSource(sp->model)) {
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
}

void QtManager::decreasePart(int px1, int py1, int px2, int py2)
{
  mPlot->viewZoomIn(px1, py1, px2, py2);
}

void QtManager::increasePart()
{
  mPlot->viewZoomOut();
}

void QtManager::standardPart()
{
  mPlot->viewStandard();
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
    METLIBS_LOG_ERROR(e.what());
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

  mPlot->clear((dataChange != CHANGED_SEL), (dataChange != CHANGED_SEL));

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
  vcross::Z_AXIS_TYPE Z_TYPE;
  if(mOptions->verticalCoordinate == "Pressure")
    Z_TYPE = vcross::Z_TYPE_PRESSURE;
  else
    Z_TYPE = vcross::Z_TYPE_HEIGHT;

  const EvaluatedPlot_cpv evaluated_plots = vc_evaluate_plots(mCollector, model_values, Z_TYPE);
  BOOST_FOREACH(EvaluatedPlot_cp ep, evaluated_plots) {
    mPlot->addPlot(ep);
  }

  // TODO decide if to plot surface height or pressure
  const std::string& model1 = mCollector->getSelectedPlots().front()->model;
  vc_evaluate_surface(mCollector, model_values, model1);

  mPlot->setVerticalAxis();
  if (Z_TYPE == vcross::Z_TYPE_PRESSURE or Z_TYPE == vcross::Z_TYPE_HEIGHT) {
    const std::string surface = (Z_TYPE == vcross::Z_TYPE_PRESSURE) ?
        VC_SURFACE_PRESSURE : VC_SURFACE_HEIGHT;
    const std::string surface_unit = (Z_TYPE == vcross::Z_TYPE_PRESSURE) ?
        "hPa" : "m";
    InventoryBase_cp s1 = mCollector->getResolvedField(model1, surface);
    if (s1)
      mPlot->setSurface(util::unitConversion(model_values.at(model1).at(surface), s1->unit(), surface_unit));
  }

  //mPlot->setVerticalAxis(zQuantity);
  mPlot->prepare();
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
  BOOST_FOREACH(const ResolvedPlot_cp rp, available_plots) {
    plot_names.push_back(rp->name());
  }
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

  vc_select_plots(mCollector, vstr);

  if (not mCollector->getSelectedPlots().empty()) {
    const std::string& model1 = mCollector->getSelectedPlots().front()->model;
    vc_require_surface(mCollector, model1);

    BOOST_FOREACH(SelectedPlot_cp sp, mCollector->getSelectedPlots())
        vc_require_pressure_height(mCollector, sp->model);
  }

  dataChange |= CHANGED_SEL;
  return setModels();
}

// ------------------------------------------------------------------------

bool QtManager::setModels()
{
  METLIBS_LOG_SCOPE();

  const vctime_t timeBefore = currentTime(); // remember current time, search for it later
  const std::string csBefore = currentCSName(); // remember current cs' name, search for it later

  std::set<std::string> models;
  BOOST_FOREACH(SelectedPlot_cp sp, mCollector->getSelectedPlots()) {
    models.insert(sp->model);
  }
  std::set<LocationElement, lt_LocationElement> le;

  std::set<std::string> csLabels;
  std::set<miutil::miTime> times;
  BOOST_FOREACH(const std::string& m, models) {
    METLIBS_LOG_DEBUG(LOGVAL(m));

    if (Source_p src = mCollector->getSetup()->findSource(m)) {
      if (Inventory_cp inv = src->getInventory()) {
        BOOST_FOREACH(Crossection_cp cs, inv->crossections) {

          LonLat_v  crossectionPoints = cs->points;
          if ( crossectionPoints.size() < 2 )
            continue;
          LocationElement el;
          el.name = cs->label;
          BOOST_FOREACH(const LonLat& ll, crossectionPoints) {
            el.xpos.push_back(ll.lonDeg());
            el.ypos.push_back(ll.latDeg());
          }
          le.insert(el);

          csLabels.insert(cs->label);
        }

        BOOST_FOREACH(Time::timevalue_t time, inv->times.values) {
          times.insert(util::to_miTime(inv->times.unit, time));
        }
      }
    }
  }
  util::from_set(mCrossectionLabels, csLabels);
  util::from_set(mCrossectionTimes, times);

  locationData.elements.insert(locationData.elements.end(), le.begin(), le.end());
  fillLocationData(locationData);

  if (mCrossectionLabels.empty() or mCrossectionTimes.empty()) {
    METLIBS_LOG_DEBUG("no times or crossections");
    mCrossectionCurrent = -1;
    mPlotTime = -1;
    return true;
  }

  setTimeToBestMatch(timeBefore);

  bool modelChange = false;
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
  mTimeGraphPos = -1;
  dataChange = CHANGED_SEL;
}

// ------------------------------------------------------------------------

void QtManager::setTimeGraphPos(int plotx, int /*ploty*/)
{
  METLIBS_LOG_SCOPE();

  if (mCollector->getSelectedPlots().empty())
    return;

  mTimeGraphPos = mPlot->getNearestPos(plotx);
  dataChange = CHANGED_SEL;
}

// ------------------------------------------------------------------------

void QtManager::setTimeGraphPos(int incr)
{
  METLIBS_LOG_SCOPE();
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
  mTimeGraphPos = index;
  dataChange = CHANGED_SEL;
}

// ------------------------------------------------------------------------

void QtManager::parseQuickMenuStrings(const std::vector<std::string>& vstr)
{
  std::vector<std::string> vcross_data, vcross_options;
  std::string crossection;

  BOOST_FOREACH(std::string line, vstr) { // copy because it has to be trimmed
    miutil::trim(line);
    if (line.empty())
      continue;

    const std::string upline = miutil::to_upper(line);

    if (miutil::contains(upline, "CROSSECTION=")) {
      const std::vector<std::string> vs = miutil::split(line, "=");
      crossection = vs[1];
      if (miutil::contains(crossection, "\""))
        miutil::remove(crossection, '\"');
    } else if (miutil::contains(upline, "VCROSS ")) {
      vcross_data.push_back(line);
    } else {
      // assume plot-options
      vcross_options.push_back(line);
    }
  }

  readOptions(vcross_options);
  setSelection(vcross_data);
  setCrossection(crossection);
}

// ------------------------------------------------------------------------

std::vector<std::string> QtManager::getQuickMenuStrings()
{
  std::vector<std::string> vstr;

  const std::vector<std::string> vstrOpt = getOptions()->writeOptions();
  const std::vector<std::string> vstrPlot = mPlotStrings;
  const std::string crossection = "CROSSECTION=" + getCrossection();

  vstr.push_back("VCROSS");
  vstr.insert(vstr.end(), vstrOpt.begin(),  vstrOpt.end());
  vstr.insert(vstr.end(), vstrPlot.begin(), vstrPlot.end());
  vstr.push_back(crossection);

  return vstr;
}

// ------------------------------------------------------------------------

std::vector<std::string> QtManager::writeLog()
{
  return mOptions->writeOptions();
}

// ------------------------------------------------------------------------

void QtManager::readLog(const std::vector<std::string>& vstr,
    const std::string& thisVersion, const std::string& logVersion)
{
  mOptions->readOptions(vstr);
}

} // namespace vcross
