/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2022 met.no

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

#include "diana_config.h"

#include "diObsPlot.h"

#include "diGlUtilities.h"
#include "diImageGallery.h"
#include "diLocalSetupParser.h"
#include "diObsPositions.h"
#include "diStaticPlot.h"
#include "diUtilities.h"
#include "miSetupParser.h"
#include "util/misc_util.h"
#include "util/qstring_util.h"
#include "util/string_util.h"
#include "util/time_util.h"

#include <mi_fieldcalc/MetConstants.h>
#include <mi_fieldcalc/math_util.h>

#include <puTools/miStringFunctions.h>

#include <QPolygonF>
#include <QString>
#include <QTextCodec>

#include <boost/range/adaptor/reversed.hpp>

#include <fstream>
#include <iomanip>
#include <sstream>

#define MILOGGER_CATEGORY "diana.ObsPlot"
#include <miLogger/miLogging.h>

using namespace miutil;
using boost::adaptors::reverse;

std::map<std::string, std::vector<std::string>> ObsPlot::visibleStations;
std::map<std::string, ObsPlot::metarww> ObsPlot::metarMap;
std::map<int, int> ObsPlot::lwwg2;

std::string ObsPlot::currentPriorityFile = "";
std::vector<std::string> ObsPlot::priorityList;
std::vector<short> ObsPlot::itabSynop;
std::vector<short> ObsPlot::iptabSynop;
std::vector<short> ObsPlot::itabMetar;
std::vector<short> ObsPlot::iptabMetar;

static const int undef = -32767; //should be defined elsewhere

namespace /*anonymous*/ {
const float DEG_TO_RAD = M_PI / 180;

class PushPopTranslateScale : public diutil::GlMatrixPushPop {
public:
  PushPopTranslateScale(DiGLPainter* gl, float scale)
    : GlMatrixPushPop(gl)
    { gl->Scalef(scale, scale, 1); }
  PushPopTranslateScale(DiGLPainter* gl, const QPointF& translate)
    : GlMatrixPushPop(gl)
    { gl->Translatef(translate.x(), translate.y(), 0); }
  PushPopTranslateScale(DiGLPainter* gl, const QPointF& translate, float scale)
    : GlMatrixPushPop(gl)
    { gl->Translatef(translate.x(), translate.y(), 0); gl->Scalef(scale, scale, 1); }
};

std::string get_popup_text(const ObsDataRef& dt, const std::string& key)
{
  const std::string* s = dt.get_string(key);
  if (s && (int)miutil::to_float(*s) != undef)
    return *s;

  const float* f = dt.get_float(key);
  if (f && (int)*f != undef)
    return miutil::from_number(*f);

  return "X";
}

bool isauto(const ObsDataRef& obs)
{
  const float* pauto = obs.get_float("auto");
  return pauto && (*pauto == 0);
}

bool isdata(const ObsDataRef& obs)
{
  const float* pisdata = obs.get_float("isdata");
  return !pisdata || (*pisdata != 0);
}

} // namespace /*anonymous*/

void ObsPlotCollider::clear()
{
  METLIBS_LOG_SCOPE(LOGVAL(xUsed.size()));

  xUsed.clear();
  yUsed.clear();
  usedBox.clear();
}

bool ObsPlotCollider::positionFree(float x, float y, float xdist, float ydist)
{
  METLIBS_LOG_SCOPE(LOGVAL(x) << LOGVAL(y) << LOGVAL(xdist) << LOGVAL(ydist));

  for (size_t i = 0; i < xUsed.size(); i++) {
    if (fabsf(x - xUsed[i]) < xdist && fabsf(y - yUsed[i]) < ydist)
      return false;
  }
  xUsed.push_back(x);
  yUsed.push_back(y);
  return true;
}

void ObsPlotCollider::positionPop()
{
  if (xUsed.size()) {
    xUsed.pop_back();
    yUsed.pop_back();
  }
}

bool ObsPlotCollider::areaFree(const Box* b1, const Box* b2)
{
  bool c = collision(*b1);
  if (!c && b2)
    c = collision(*b2);
  if (c)
    return false;
  usedBox.push_back(*b1);
  usedBox.back().index = 0;
  if (b2) {
    usedBox.push_back(*b2);
    usedBox.back().index = 1;
  }
  return true;
}

void ObsPlotCollider::areaPop()
{
  while (usedBox.size()) {
    bool last = usedBox.back().index == 0;
    usedBox.pop_back();
    if (last)
      break;
  }
}

bool ObsPlotCollider::collision(const Box& box) const
{
  int n = usedBox.size();
  int i = 0;
  for (; i < n; ++i) {
    const Box& ub = usedBox[i];
    if (!(box.x1 > ub.x2 || box.x2 < ub.x1 || box.y1 > ub.y2 || box.y2 < ub.y1))
      break;
  }
  return (i != n);
}

// ========================================================================

ObsPlot::ObsPlot(const std::string& dn, ObsPlotType plottype)
    : visible_positions_count_(0)
    , m_plottype(plottype)
    , dialogname(dn)
{
  METLIBS_LOG_SCOPE();

  x = NULL;
  y = NULL;
  markerSize = -1;
  textSize = 1;
  allObs = false;
  levelAsField = false;
  level = -10;
  priority = false;
  density = 1;
  numPar = 0;
  tempPrecision = false;
  unit_ms = false;
  show_VV_as_code_ = true;
  plotundef = false;
  vertical_orientation = true;
  left_alignment = true;
  showpos = false;
  devfield.reset(0);
  moretimes = false;
  next = false;
  previous = false;
  thisObs = false;
  wind_scale = -1;
  firstplot = true;
  recalculate_densities_ = false;
  itab = 0;
  iptab = 0;
  onlypos = false;
  showOnlyPrioritized = false;
  parameterName = false;
  popupText = false;
  qualityFlag = false;
  wmoFlag = false;
  annotations = true;
  has_deltatime = false;
}

ObsPlot::~ObsPlot()
{
  METLIBS_LOG_SCOPE();
  delete[] x;
  delete[] y;
}

void ObsPlot::getAnnotation(std::string& str, Colour& col) const
{
  METLIBS_LOG_SCOPE();
  str = annotation;
  col = origcolour;
}

std::string ObsPlot::getEnabledStateKey() const
{
  return getPlotName();
}

void ObsPlot::updatePlotNameAndAnnotation()
{
  bool first = true;
  std::ostringstream ost;
  for (const std::string& rn : readernames) {
    if (!first)
      ost << ' ';
    first = false;
    ost << rn;
  }

  if (!first && level != -10)
    ost << ' ' << level << levelsuffix;

  setPlotName(ost.str());

  if (getStatus() == P_WAITING) {
    ost << " ...";
  } else if (getStatus() == P_ERROR) {
    ost << " ERROR";
  } else {
    if (!obsTime.undef()) {
      ost << ' ' << obsTime.isoTime();
      if (getTimeDiff() > 0) {
        const miutil::miTime& mapTime_ = getStaticPlot()->getTime();
        const miutil::miTime minTime = miutil::addMin(mapTime_, -getTimeDiff());
        const miutil::miTime maxTime = miutil::addMin(mapTime_, +getTimeDiff());
        ost << " (" << minTime.format("%H:%M", "", true) << " - " << maxTime.format("%H:%M", "", true) << ')';
      }
    }
    ost << " (" << numVisiblePositions() << " / " << getObsCount() << ')';
  }

  annotation = ost.str();
}

void ObsPlot::getDataAnnotations(std::vector<std::string>& anno) const
{
  METLIBS_LOG_SCOPE();

  if (!isEnabled() || !annotations || getObsCount() == 0 || wind_scale < 0)
    return;

  const std::string vectorAnnotationSize = miutil::from_number(21 * getStaticPlot()->getPhysToMapScaleX());
  const std::string vectorAnnotationText = miutil::from_number(2.5f * wind_scale, 2) + "m/s";
  for (const std::string& a : anno) {
    if (miutil::contains(a, "arrow")) {
      if (miutil::contains(a, "arrow="))
        continue;

      std::string endString;
      size_t nn = a.find_first_of(",");
      if (nn != std::string::npos) {
        endString = a.substr(nn);
      }

      std::string str = "arrow=" + vectorAnnotationSize
          + ",feather=true,tcolour=" + colour.Name() + endString;
      anno.push_back(str);

      str = "text=\"" + vectorAnnotationText + "\"" /* no spaces, but maybe ',' from the float ? */
          + ",tcolour=" + colour.Name() + endString;
      anno.push_back(str);
    }
  }
}

const PlotCommand_cpv ObsPlot::getObsExtraAnnotations() const
{
  if (isEnabled() && annotations) {
    return extraAnnotations;
  }
  return PlotCommand_cpv();
}

void ObsPlot::setObsData(ObsDataContainer_cp obs)
{
  METLIBS_LOG_SCOPE();
  obsp = ObsDataRotated(obs);
}

int ObsPlot::getLevel()
{
  if (levelAsField) //from field
    level = getStaticPlot()->getVerticalLevel();

  // Default level
  if (level == -1) {
    level = 1000;
  }

  return level;
}

void ObsPlot::setObsTime(const miutil::miTime& t)
{
  obsTime = t;
}

size_t ObsPlot::numVisiblePositions() const
{
  return visible_positions_count_;
}

void ObsPlot::updateVisiblePositionCount()
{
  METLIBS_LOG_SCOPE();

  visible_positions_count_ = 0;
  if (x && y) {
    const size_t npos = getObsCount();
    for (size_t i = 0; i < npos; i++) {
      if (getStaticPlot()->getMapSize().isinside(x[i], y[i]))
        visible_positions_count_++;
    }
  }
}

size_t ObsPlot::getObsCount() const
{
  return obsp.size();
}

// static
std::string ObsPlot::extractDialogname(const miutil::KeyValue_v& kvs)
{
  std::string dialogname;
  const size_t i_plot = miutil::rfind(kvs, "plot");
  if (i_plot != size_t(-1)) {
    const miutil::KeyValue& kv_plot = kvs[i_plot];
    if (kv_plot.hasValue()) {
      const std::vector<std::string> vstr = miutil::split(kv_plot.value(), ":");
      if (!vstr.empty())
        dialogname = vstr[0];
    }
  }
  return dialogname;
}

// static
ObsPlotType ObsPlot::extractPlottype(const std::string& dialogname)
{
  const std::string valp = miutil::to_lower(dialogname);
  if (valp == "synop") {
    return OPT_SYNOP;
  } else if (valp == "metar") {
    return OPT_METAR;
  }
#ifdef ROADOBS
  // To avoid that roadobs will be set to ascii below
  else if (valp == "synop_wmo" || valp == "synop_ship") {
    return OPT_SYNOP;
  } else if (valp == "metar_icao") {
    return OPT_METAR;
  }
#endif // !ROADOBS
  else {
    return OPT_LIST;
  }
}

void ObsPlot::setPlotInfo(const miutil::KeyValue_v& pin)
{
  METLIBS_LOG_SCOPE();

  poptions.fontname = diutil::BITMAPFONT;
  poptions.fontface = diutil::F_NORMAL;

  PlotOptionsPlot::setPlotInfo(pin);

  std::vector<std::string> parameter;
  for (const miutil::KeyValue& kv : pin) {
    const std::string& key = kv.key();
    const std::string& value = kv.value();
    const std::string value_lower_case = miutil::to_lower(value);
    if (key == "plot") {
      if (value == "pressure")
        levelsuffix = "hPa";
      else if (value == "ocean")
        levelsuffix = "m";
    } else if (key == "data") {
      readernames = miutil::split(value, ",");
    } else if (key == "parameter") {
      parameter = miutil::split(value, 0, ",");
    } else if (key == "scale") {
      textSize = kv.toFloat();
      if (markerSize < 0)
        markerSize = kv.toFloat();
    } else if (key == "marker.size") {
      markerSize = kv.toFloat();
    } else if (key == "text.size") {
      textSize = kv.toFloat();
    } else if (key == "density") {
      if (value_lower_case == "allobs")
        allObs = true;
      else
        density = kv.toFloat();
    } else if (key == "priority") {
      priorityFile = value;
      priority = true;
    } else if (key == "colour") {
      origcolour = Colour(value);
    } else if (key == "devfield") {
      if (kv.toBool()) {
        devfield.reset(new ObsPositions);
      } else {
        devfield.reset(0);
      }
    } else if (key == "devcolour1") {
      mslpColour1 = Colour(value);
    } else if (key == "devcolour2") {
      mslpColour2 = Colour(value);
    } else if (key == "tempprecision") {
      tempPrecision = kv.toBool();
    } else if (key == "unit_ms") {
      unit_ms = kv.toBool();
    } else if (key == "show_VV_as_code") {
      show_VV_as_code_ = kv.toBool();
    } else if (key == "plotundef") {
      plotundef = kv.toBool();
    } else if (key == "parametername") {
      parameterName = kv.toBool();
    } else if (key == "popup") {
      popupText = kv.toBool();
    } else if (key == "qualityflag") {
      qualityFlag = kv.toBool();
    } else if (key == "wmoflag") {
      wmoFlag = kv.toBool();
    } else if (key == "moretimes") {
      moretimes = kv.toBool();
    } else if (key == "sort") {
      decodeSort(value);
    } else if (key == "timediff")
      if (value_lower_case == "alltimes")
        timeDiff = -1;
      else
        timeDiff = kv.toInt();
    else if (key == "level") {
      if (value_lower_case == "asfield") {
        levelAsField = true;
        level = -1;
      } else
        level = kv.toInt();
    } else if (key == "onlypos") {
      onlypos = true;
    } else if (key == "showonlyprioritized") {
      showOnlyPrioritized = true;
    } else if (key == "image") {
      image = value;
    } else if (key == "showpos") {
      showpos = true;
    } else if (key == "orientation") {
      if (value_lower_case == "horizontal")
        vertical_orientation = false;
    } else if (key == "alignment") {
      if (value_lower_case == "right")
        left_alignment = false;
    } else if (key == "criteria") {
      decodeCriteria(value);
    } else if (key == "arrowstyle") {
      if (value_lower_case == "wind")
        poptions.arrowstyle = arrow_wind;
      else if (value_lower_case == "wind_arrow")
        poptions.arrowstyle = arrow_wind_arrow;
    } else if (key == "wind_scale") {
      wind_scale = kv.toFloat();
    } else if (key == "annotations") {
      annotations = kv.toBool();
    } else if (key == "font") {
      poptions.fontname = value;
    } else if (key == "face") {
      poptions.fontface = diutil::fontFaceFromString(value);
    }
  }

  if (markerSize < 0)
    markerSize = textSize;
  parameterDecode("all", false);
  numPar = parameter.size();
  for (const std::string& p : parameter) {
    parameterDecode(p);
  }
  if (mslp())
    parameterDecode("PPPP_mslp");

  // static tables, read once

  std::string path = LocalSetupParser::basicValue("obsplotfilepath");

  if (isSynopList()) {
    if (itabSynop.empty() || iptabSynop.empty()) {
      if (!readTable(plottype(), path + "/itab_synop.txt", path + "/iptab_synop.txt", itabSynop, iptabSynop))
        METLIBS_LOG_ERROR("could not read itab_synop.txt");
    }
    itab = &itabSynop;
    iptab = &iptabSynop;

  } else if (plottype() == OPT_METAR) {
    if (itabMetar.empty() || iptabMetar.empty()) {
      if (!readTable(plottype(), path + "/itab_metar.txt", path + "/iptab_metar.txt", itabMetar, iptabMetar))
        METLIBS_LOG_ERROR("could not read itab_metar.txt");
    }
    itab = &itabMetar;
    iptab = &iptabMetar;
  }

  updatePlotNameAndAnnotation();
}

static int normalize_angle(float dd)
{
  if (dd < 1)
    dd += 360;
  if (dd > 360)
    dd -= 360;
  return dd;
}

void ObsPlot::updateObsPositions()
{
  if (!mslp())
    return;

  const int numObs = getObsCount();
  devfield->resize(numObs);
  std::copy(x, x+numObs, devfield->xpos);
  std::copy(y, y+numObs, devfield->ypos);
  devfield->convertToGrid = true;
  devfield->obsArea = getStaticPlot()->getMapArea();
}

void ObsPlot::setParameters(const std::vector<ObsDialogInfo::Par>& vp)
{
  if (!vparam.empty())
    return;

  vparam = vp;
  for (ObsDialogInfo::Par& par : vparam) {
    if (par.name == "DeltaTime") {
      has_deltatime = true;
      break;
    }
  }
}

void ObsPlot::reprojectData()
{
  METLIBS_LOG_SCOPE();

  const int numObs = getObsCount();
  if (numObs == 0)
    return;

  delete[] x;
  delete[] y;

  x = new float[numObs];
  y = new float[numObs];

  for (int i = 0; i < numObs; i++)
    getObsLonLat(i, x[i], y[i]);

  // convert points to correct projection
  getStaticPlot()->GeoToMap(numObs, x, y);

  // find direction of north for each observation
  std::unique_ptr<float[]> u(new float[numObs]);
  std::unique_ptr<float[]> v(new float[numObs]);
  std::fill(u.get(), u.get() + numObs, 0);
  std::fill(v.get(), v.get() + numObs, 10);

  getStaticPlot()->GeoToMap(numObs, x, y, u.get(), v.get());

  for (int i = 0; i < numObs; i++) {
    const int angle = (int) (atan2f(u[i], v[i]) * 180 / M_PI);
    if (const float* pdd = obsp.get_unrotated_float(i, "dd")) {
      float dd = *pdd;
      if (dd > 0 and dd <= 360) {
        dd = normalize_angle(dd + angle);
        obsp.put_rotated_float(i, "dd_adjusted", dd);
      }
    }

    if (const float* pdw1dw1 = obsp.get_unrotated_float(i, "dw1dw1"))
      obsp.put_rotated_float(i, "dw1dw1", normalize_angle(*pdw1dw1 + angle));

    if (const float* pds = obsp.get_unrotated_float(i, "ds"))
      obsp.put_rotated_float(i, "ds", normalize_angle(*pds + angle));
  }

  updateObsPositions();
}

void ObsPlot::setData(bool success)
{
  METLIBS_LOG_SCOPE(LOGVAL(success));

  firstplot = true;
  nextplot.clear();
  notplot.clear();
  list_plotnr.clear();

  if (!success) {
    obsp = ObsDataRotated();
  }

  const bool empty = (getObsCount() == 0);
  if (!success) {
    METLIBS_LOG_INFO("failed");
    setStatus(P_ERROR);
  } else if (empty) {
    METLIBS_LOG_INFO("no data");
    setStatus(P_OK_EMPTY);
  } else {
    setStatus(P_OK_DATA);
  }

  reprojectData();
  updateVisiblePositionCount();
  updatePlotNameAndAnnotation();
  if (getStatus() != P_OK_DATA)
    return;

  updateDeltaTimes();

  //sort stations according to priority file
  priority_sort();
  //put stations plotted last time on top of list
  readStations();
  //sort stations according to time
  if (moretimes)
    time_sort();

  // sort according to parameter
  for (const auto& sc : sortcriteria)
    parameter_sort(sc.first, sc.second);

  if (plottype() == OPT_METAR && metarMap.empty())
    initMetarMap();
}

void ObsPlot::getObsLonLat(int obsidx, float& x, float& y)
{
  const auto& b = obsp.basic(obsidx);
  x = b.xpos;
  y = b.ypos;
}

void ObsPlot::logStations()
{
  METLIBS_LOG_SCOPE();
  if (!nextplot.empty()) {
    std::vector<std::string>& vs = visibleStations[dialogname];
    vs.clear();
    for (int n : nextplot) {
      vs.push_back(obsp.basic(n).id);
    }
  }
}

void ObsPlot::readStations()
{
  METLIBS_LOG_SCOPE();

  std::vector<std::string>& vs = visibleStations[dialogname];
  if (!vs.empty()) {
    std::vector<int> tmpList = all_from_file;
    all_stations.clear();

    // Fill the stations from stat into priority_vector,
    // and mark them in tmpList
    const int numObs = obsp.size();
    for (int k = 0; k < numObs; k++) {
      const int i = all_from_file[k];
      if (std::find(vs.begin(), vs.end(), obsp.basic(i).id) != vs.end()) {
        all_stations.push_back(i);
        tmpList[i] = -1;
      }
    }

    for (size_t i = 0; i < tmpList.size(); i++) {
      if (tmpList[i] != -1) {
        all_stations.push_back(i);
      }
    }

  } else {
    all_stations = all_from_file;
  }

  firstplot = true;
  nextplot.clear();
  notplot.clear();
  list_plotnr.clear();
  fromFile = false;
}

void ObsPlot::clear()
{
  METLIBS_LOG_SCOPE();
  //  logStations();
  firstplot = true;
  list_plotnr.clear();    // list of all stations, value = plotnr
  all_this_area.clear(); // all stations within area
  all_stations.clear(); // all stations, from last plot or from file
  all_from_file.clear(); // all stations, from file or priority list
  nextplot.clear();
  notplot.clear();
  obsp = ObsDataRotated();
  visible_positions_count_ = 0;
  extraAnnotations.clear();
}

void ObsPlot::priority_sort()
{
  METLIBS_LOG_SCOPE();

  //sort the observations according to priority list
  int numObs = getObsCount();

  int i;

  all_from_file.resize(numObs);

  std::vector<int> automat;
  int n = 0;
  for (i = 0; i < numObs; i++) {
    const auto obs = obsp[i];

    if (!isauto(obs))
      all_from_file[n++] = i;
    else
      automat.push_back(i);
  }
  int na = automat.size();
  for (i = 0; i < na; i++)
    all_from_file[n++] = automat[i];

  if (priority) {

    if (currentPriorityFile != priorityFile)
      readPriorityFile(priorityFile);

    if (priorityList.size() > 0) {

      std::vector<int> tmpList = all_from_file;
      all_from_file.clear();

      // Fill the stations from priority list into all_from_file,
      // and mark them in tmpList
      int j, n = priorityList.size();
      for (j = 0; j < n; j++) {
        i = 0;
        while (i < numObs && obsp.basic(i).id != priorityList[j])
          i++;
        if (i < numObs) {
          all_from_file.push_back(i);
          tmpList[i] = -1;
        }
      }

      if (!showOnlyPrioritized) {
        for (size_t i = 0; i < tmpList.size(); i++)
          if (tmpList[i] != -1)
            all_from_file.push_back(i);
      }
    }
  }
}

void ObsPlot::time_sort(void)
{
  //sort observations according to time
  //both all_from_file (sorted acc. to priority file) and
  // all_stations (stations from last plot on top) are sorted
  METLIBS_LOG_SCOPE();

  int index, numObs = 0;

  numObs = obsp.size();

  std::vector<int> diff(numObs);
  std::multimap<int, int> sortmap1;
  std::multimap<int, int> sortmap2;

  // Data from obs-files or database
  // find mindiff = abs(obsTime-plotTime) for all observations
  for (int i = 0; i < numObs; i++)
    diff[i] = abs(miTime::minDiff(obsp.basic(i).obsTime, obsTime));

  //Sorting ...
  for (int i = 0; i < numObs; i++) {
    index = all_stations[i];
    sortmap1.insert(std::make_pair(diff[index], index));
  }
  for (int i = 0; i < numObs; i++) {
    index = all_from_file[i];
    sortmap2.insert(std::make_pair(diff[index], index));
  }

  std::multimap<int, int>::iterator p = sortmap1.begin();
  for (int i = 0; i < numObs; i++, p++)
    all_stations[i] = p->second;

  std::multimap<int, int>::iterator q = sortmap2.begin();
  for (int i = 0; i < numObs; i++, q++)
    all_from_file[i] = q->second;
}

void ObsPlot::parameter_sort(const std::string& parameter, bool minValue)
{
  METLIBS_LOG_SCOPE();

  //sort the observations according to priority list
  if (obsp.empty())
    return;

  std::vector<int> tmpFileList = all_from_file;
  std::vector<int> tmpStnList = all_stations;

  std::multimap<std::string, int> stringFileSortmap;
  std::multimap<std::string, int> stringStnSortmap;
  std::multimap<double, int> numFileSortmap;
  std::multimap<double, int> numStnSortmap;

  // Fill the observation with parameter
  // and mark them in tmpList
  int index;
  for (size_t i = 0; i < tmpFileList.size(); i++) {
    index = all_from_file[i];
    if (const std::string* stringkey = obsp[index].get_string(parameter)) {
      if (miutil::is_number(*stringkey)) {
        double numberkey = miutil::to_double(*stringkey);
        numFileSortmap.insert(std::pair<double, int>(numberkey, index));
      } else {
        stringFileSortmap.insert(std::make_pair(*stringkey, index));
      }
      tmpFileList[i] = -1;

    }
  }

  for (size_t i = 0; i < tmpStnList.size(); i++) {
    index = all_stations[i];
    if (const std::string* stringkey = obsp[index].get_string(parameter)) {
      if (miutil::is_number(*stringkey)) {
        double numberkey = miutil::to_double(*stringkey);
        numStnSortmap.insert(std::pair<double, int>(numberkey, index));
      } else {
        stringStnSortmap.insert(std::make_pair(*stringkey, index));
      }
      tmpStnList[i] = -1;

    }
  }

  all_from_file.clear();
  all_stations.clear();
  if (!minValue) {
    for (std::multimap<double, int>::iterator p = numFileSortmap.begin(); p != numFileSortmap.end(); p++) {
      all_from_file.push_back(p->second);
    }
    for (std::multimap<std::string, int>::iterator p = stringFileSortmap.begin(); p != stringFileSortmap.end(); p++) {
      all_from_file.push_back(p->second);
    }

    for (std::multimap<double, int>::iterator p = numStnSortmap.begin(); p != numStnSortmap.end(); p++) {
      all_stations.push_back(p->second);
    }
    for (std::multimap<std::string, int>::iterator p = stringStnSortmap.begin(); p != stringStnSortmap.end(); p++) {
      all_stations.push_back(p->second);
    }
  } else {
    for (std::multimap<std::string, int>::reverse_iterator p = stringFileSortmap.rbegin(); p != stringFileSortmap.rend(); p++) {
      all_from_file.push_back(p->second);
    }
    for (std::multimap<double, int>::reverse_iterator p = numFileSortmap.rbegin(); p != numFileSortmap.rend(); p++) {
      all_from_file.push_back(p->second);
    }
    for (std::multimap<std::string, int>::reverse_iterator p = stringStnSortmap.rbegin(); p != stringStnSortmap.rend(); p++) {
      all_stations.push_back(p->second);
    }
    for (std::multimap<double, int>::reverse_iterator p = numStnSortmap.rbegin(); p != numStnSortmap.rend(); p++) {
      all_stations.push_back(p->second);
    }
  }

  if (!showOnlyPrioritized) {
    for (size_t i = 0; i < tmpFileList.size(); i++)
      if (tmpFileList[i] != -1) {
        all_from_file.push_back(tmpFileList[i]);
      }

    for (size_t i = 0; i < tmpStnList.size(); i++)
      if (tmpStnList[i] != -1) {
        all_stations.push_back(tmpStnList[i]);
      }
  }
}

void ObsPlot::readPriorityFile(const std::string& filename)
{
  METLIBS_LOG_SCOPE("filename: " << filename);

  priorityList.clear();

  // this will prevent opening a nonexisting file many times
  currentPriorityFile = filename;

  std::ifstream inFile;
  std::string line;

  inFile.open(filename.c_str(), std::ios::in);
  if (inFile.bad()) {
    METLIBS_LOG_WARN("ObsPlot: Can't open file: " << priorityFile);
    return;
  }

  while (getline(inFile, line)) {
    if (line.length() > 0) {
      miutil::trim(line);
      if (line.length() > 0 && line[0] != '#')
        priorityList.push_back(line);
    }
  }

  inFile.close();
}

//***********************************************************************

void ObsPlot::updateFromEditField()
{
  METLIBS_LOG_SCOPE();

  //PPPP-mslp
  if (mslp()) {
    // TODO this has to be done after ObsManager::updateObsPositions, ie after changeProjection and any field edit
    // startxy is set above, in getPositions; values is interpolated editfield from EditManager::obs_mslp
    int numObs = obsp.size();
    for (int i = 0; i < numObs; i++) {
      if (const float* pPPPP = obsp.get_float(i, "PPPP")) {
        const float ief = devfield->interpolatedEditField[i];
        if (ief < 0.9e+35)
          obsp.put_float(i, "PPPP_mslp", *pPPPP - ief);
      }
    }
  }
}

//***********************************************************************

int ObsPlot::findObs(int xx, int yy, const std::string& type)
{
  METLIBS_LOG_SCOPE();
  using miutil::square;

  float min_r = square(10 * getStaticPlot()->getPhysToMapScaleX());
  float r;
  int min_i = -1;

  const XY pos = getStaticPlot()->PhysToMap(XY(xx, yy));

  if ( type == "notplot" ) {
    //find closest station, closer than min_r, from list of stations not plotted
    for (size_t i = 0; i < notplot.size(); i++ ) {
      r = square(pos.x() - x[notplot[i]]) + square(pos.y() - y[notplot[i]]);
      if ( r < min_r ) {
        min_r = r;
        min_i = i;
      }
    }
    //if last station are closer, return -1
    if ( min_i > -1 && nextplot.size() ) {
      r = square(pos.x() - x[nextplot[0]]) + square(pos.y() - y[nextplot[0]]);
      if ( r < min_r )
        return -1;
    }
  } else if (type == "nextplot" ) {
    //find closest station, closer than min_r, from list of stations plotted
    for (size_t i = 0; i < nextplot.size(); i++ ) {
      r = square(pos.x() - x[nextplot[i]]) + square(pos.y() - y[nextplot[i]]);
      if (r < min_r) {
        min_r = r;
        min_i = i;
      }
    }
  } else {
    //find closest station, closer than min_r
    for (size_t i = 0; i < obsp.size(); i++ ) {
      r = square(pos.x() - x[i]) + square(pos.y() - y[i]);
      if (r < min_r) {
        min_r = r;
        min_i = i;
      }
    }
  }

  return min_i;
}

bool ObsPlot::showpos_findObs(int xx, int yy)
{
  METLIBS_LOG_SCOPE("xx: " << " yy: " << yy);

  if (!showpos)
    return false;

  int min_i = findObs(xx,yy,"notplot");

  if (min_i < 0) {
    return false;
  }

  //insert station found in list of stations to plot, and remove it
  //from list of stations not to plot
  std::vector<int>::iterator min_p = notplot.begin() + min_i;
  nextplot.insert(nextplot.begin(), notplot[min_i]);
  notplot.erase(min_p);
  thisObs = true;
  return true;
}

void ObsPlot::setPopupSpec(const std::vector<std::string>& txt)
{
  std::string datatype;
  std::vector<std::string> data;

  for (const std::string& t : txt) {
    if (miutil::contains(t, "datatype")) {
      if (!datatype.empty() && !data.empty()) {
        popupSpec[datatype] = data;
        data.clear();
      }
      std::vector<std::string> token = miutil::split(t, "=");
      if (token.size() == 2) {
        datatype = token[1];
      }
    } else {
      data.push_back(t);
    }
  }
  if (!datatype.empty()) {
    popupSpec[datatype] = data;
  }
}

//***********************************************************************
bool ObsPlot::getObsPopupText(int xx, int yy, std::string& setuptext)
{
  METLIBS_LOG_SCOPE("xx: " << " yy: " << yy);

  if (!popupText)
    return false;
  const int min_i = findObs(xx, yy);
  if (min_i < 0)
    return false;

  bool found = false;
  const ObsDataRef& dt = obsp[min_i];

  for (const std::string& datatype : readernames) {
    std::map<std::string, std::vector<std::string>>::const_iterator f_p = popupSpec.find(datatype);
    if (f_p == popupSpec.end())
      continue;

    found = true;
    const std::vector<std::string>& mdata = f_p->second;
    if (mdata.empty())
      continue;

    setuptext += "<table>";
    if (!dt.obsTime().undef()) {
      setuptext += "<tr>";
      setuptext += "<td>";
      setuptext += "<span style=\"background: red; color: red\">X</span>";
      setuptext += dt.obsTime().isoTime();
      setuptext += "</td>";
      setuptext += "</tr>";
    }
    for (const std::string& mdat : mdata) {
      setuptext += "<tr>";
      setuptext += "<td>";
      setuptext += "<span style=\"background: red; color: red\">X</span>";
      std::vector<std::string> token = miutil::split(mdat, ' ');
      for (size_t j = 0; j < token.size(); j++) {
        if (miutil::contains(token[j], "$")) {
          miutil::trim(token[j]);
          miutil::remove(token[j], '$');
          setuptext += get_popup_text(dt, token[j]);
          setuptext += " ";
        } else if (miutil::contains(token[j], ":")) {
          std::vector<std::string> values = miutil::split(token[j], ":");
          for (size_t i = 0; i < values.size(); i++) {
            if (miutil::contains(values[i], "=")) {
              std::vector<std::string> keys = miutil::split(values[i], "=");
              if (keys.size() == 2) {
                const std::string* s = dt.get_string(token[0]);
                if (s && *s == keys[0])
                  setuptext += keys[1];
                setuptext += " ";
              }
            }
          }
        } else {
          setuptext += token[j];
          setuptext += " ";
        }
      }
      setuptext += "</td>";
      setuptext += "</tr>";
    } //end of for mdata
    setuptext += "</table>";

  } // end of datatypes

  if (!found) {
    setuptext += "<table>";
    for (const ObsDialogInfo::Par& param : vparam) {
      if (pFlag.count(param.name)) {
        setuptext += "<tr>";
        setuptext += "<td>";
        setuptext += param.name;
        setuptext += "</td>";
        setuptext += "<td>";
        setuptext += "  ";
        setuptext += get_popup_text(dt, param.name);
        setuptext += " ";
        setuptext += "</td>";
        setuptext += "</tr>";
        setuptext += "</table>";
      }
    }
  }
  return !setuptext.empty();
}

void ObsPlot::nextObs(bool Next)
{
  METLIBS_LOG_SCOPE();

  thisObs = false;

  if (Next) {
    next = true;
    plotnr++;
  } else {
    previous = true;
    plotnr--;
  }
}

bool ObsPlot::isSynopList() const
{
  return plottype() == OPT_SYNOP || plottype() == OPT_LIST;
}

bool ObsPlot::isSynopMetar() const
{
  return plottype() == OPT_SYNOP || plottype() == OPT_METAR;
}

int ObsPlot::calcNum() const
{
  int num = numPar;
  if (pFlag.count("wind"))
    num--;
  // The parameter "pos" contains both lat and lon TODO: remove this
  if (pFlag.count("pos"))
    num++;
  return num;
}

static const float circle_radius = 7;

void ObsPlot::drawCircle(DiGLPainter* gl)
{
  if (plottype() == OPT_LIST) {
    const float d = circle_radius * 0.25;
    gl->drawRect(false, -d, -d, d, d);
  } else {
    gl->drawCircle(false, 0, 0, circle_radius);
  }
}

void ObsPlot::changeProjection(const Area& /*mapArea*/, const Rectangle& /*plotSize*/, const diutil::PointI& /*physSize*/)
{
  recalculate_densities_ = true;
  reprojectData();
  updateVisiblePositionCount();
  updatePlotNameAndAnnotation();
}

void ObsPlot::plot(DiGLPainter* gl, PlotOrder zorder)
{
  METLIBS_LOG_TIME();

  if (zorder != PO_LINES && zorder != PO_OVERLAY)
    return;

  if (!isEnabled()) {
    // make sure plot-densities etc are recalc. next time
    recalculate_densities_ = true;
    return;
  }

  if (getStatus() != P_OK_DATA)
    return;

  const size_t numObs = getObsCount();

  // Update observation delta time before checkPlotCriteria
  if (updateDeltaTimes())
    recalculate_densities_ = true;

  const Colour selectedColour = origcolour;
  origcolour = getStaticPlot()->notBackgroundColour(origcolour);
  gl->setColour(origcolour);

  if (textSize < 1.75)
    gl->LineWidth(1);
  else if (textSize < 2.55)
    gl->LineWidth(2);
  else
    gl->LineWidth(3);

  gl->setFont(poptions.fontname, poptions.fontface, 8 * textSize);
  scale = textSize * getStaticPlot()->getPhysToMapScaleX() * 0.7;

  if (poptions.antialiasing)
    gl->Enable(DiGLPainter::gl_MULTISAMPLE);
  else
    gl->Disable(DiGLPainter::gl_MULTISAMPLE);

  //Plot markers only
  if (onlypos) {
    if (image == "off")
      return;
    ImageGallery ig;
    ig.plotImages(gl, getStaticPlot()->plotArea(), numObs, image, x, y, true, markerSize);
    return;
  }

  int num = calcNum();
  float xdist = 0, ydist = 0;
  // I think we should plot roadobs like synop here
  // OBS!******************************************

  if (isSynopMetar()) {
    xdist = 100 * scale / density;
    ydist = 90 * scale / density;
  } else if (plottype() == OPT_LIST) {
    if (num > 0) {
      if (vertical_orientation) {
        xdist = 58 * scale / density;
        ydist = 18 * (num + 0.2) * scale / density;
      } else {
        xdist = 50 * num * scale / density;
        ydist = 10 * scale / density;
      }
    } else {
      xdist = 14 * scale / density;
      ydist = 14 * scale / density;
    }
  }

  //**********************************************************************
  //Which stations to plot

  bool testpos = true; // positionFree or areaFree must be tested
  std::vector<int> ptmp;
  std::vector<int>::iterator p, pbegin, pend;

  if (firstplot || recalculate_densities_) { // new area

    //init of areaFreeSetup
    // I think we should plot roadobs like synop here
    // OBS!******************************************
    if (plottype() == OPT_LIST) {
      float w, h;
      gl->getTextSize("0", w, h);
      float space = w * 0.5;
      areaFreeSetup(scale, space, num, xdist, ydist);
    }

    thisObs = false;

    // new area, find stations inside current area
    all_this_area.clear();
    for (int i : all_stations) {
      if (getStaticPlot()->getMapSize().isinside(x[i], y[i])) {
        all_this_area.push_back(i);
      }
    }

    //    METLIBS_LOG_DEBUG("all this area:"<<all_this_area.size());
    // plot the observations from last plot if possible,
    // then the rest if possible

    if (!firstplot) {
      std::vector<int> a, b;
      const size_t n = list_plotnr.size();
      if (n == numObs) {
        for (int i : all_this_area) {
          if (list_plotnr[i] == plotnr)
            a.push_back(i);
          else
            b.push_back(i);
        }
        if (!a.empty()) {
          all_this_area = a;
          diutil::insert_all(all_this_area, b);
        }
      }
    }

    //reset
    list_plotnr.clear();
    list_plotnr.insert(list_plotnr.begin(), numObs, -1);
    maxnr = plotnr = 0;

    pbegin = all_this_area.begin();
    pend = all_this_area.end();

  } else if (thisObs) {
    //    METLIBS_LOG_DEBUG("thisobs");
    // plot the station pointed at and those plotted last time if possible,
    // then the rest if possible.
    ptmp = nextplot;
    diutil::insert_all(ptmp, notplot);
    pbegin = ptmp.begin();
    pend = ptmp.end();

  } else if (plotnr > maxnr) {
    //    METLIBS_LOG_DEBUG("plotnr:"<<plotnr);
    // plot as many observations as possible which have not been plotted before
    maxnr++;
    plotnr = maxnr;

    for (int i : all_this_area) {
      if (list_plotnr[i] == -1)
        ptmp.push_back(i);
    }
    pbegin = ptmp.begin();
    pend = ptmp.end();

  } else if (previous && plotnr < 0) {
    //    METLIBS_LOG_DEBUG("plotnr:"<<plotnr);
    // should return to the initial priority as often as possible...

    if (!fromFile) { //if priority from last plot has been used so far
      all_this_area.clear();
      for (size_t j = 0; j < numObs; j++) {
        int i = all_from_file[j];
        if (getStaticPlot()->getMapSize().isinside(x[i], y[i]))
          all_this_area.push_back(i);
      }
      all_stations = all_from_file;
      fromFile = true;
    }
    //clear
    plotnr = maxnr = 0;
    list_plotnr.clear();
    list_plotnr.insert(list_plotnr.begin(), numObs, -1);

    pbegin = all_this_area.begin();
    pend = all_this_area.end();

  } else if (previous || next) {
    //    METLIBS_LOG_DEBUG("plotnr:"<<plotnr);
    //    plot observations from plotnr
    notplot.clear();
    nextplot.clear();
    for (int i : all_this_area) {
      if (list_plotnr[i] == plotnr) {
        nextplot.push_back(i);
      } else if (list_plotnr[i] > plotnr || list_plotnr[i] == -1) {
        notplot.push_back(i);
      }
    }
    testpos = false; //no need to test positionFree or areaFree

  } else {
    //nothing has changed
    testpos = false; //no need to test positionFree or areaFree
  }
  //######################################################
  //  int ubsize1= usedBox.size();
  //######################################################

  if (testpos) { //test of positionFree or areaFree
    notplot.clear();
    nextplot.clear();
    // I think we should plot roadobs like synop here
    // OBS!******************************************
    const bool listplot = (plottype() == OPT_LIST);
    for (p = pbegin; p != pend; p++) {
      int i = *p;
      if (allObs || (listplot && areaFree(i)) || (!listplot && collider_->positionFree(x[i], y[i], xdist, ydist))) {
        // Select parameter with correct accumulation/max value interval
        if (pFlag.count("911ff")) {
          checkGustTime(i);
        }
        if (pFlag.count("rrr")) {
          checkAccumulationTime(i);
        }
        if (pFlag.count("fxfx")) {
          checkMaxWindTime(i);
        }
        const auto& dta = obsp[i];
        if (checkPlotCriteria(dta)) {
          nextplot.push_back(i);
          list_plotnr[i] = plotnr;
        } else {
          list_plotnr[i] = -2;
          if (listplot)
            collider_->areaPop();
          else if (!allObs)
            collider_->positionPop();
        }
      } else {
        notplot.push_back(i);
      }
    }
    if (thisObs) {
      for (int n : notplot) {
        if (list_plotnr[n] == plotnr)
          list_plotnr[n] = -1;
      }
    }
  }

  //PLOTTING

  if (showpos) {
    Colour col("red");
    if (col == colour)
      col = Colour("blue");
    col = getStaticPlot()->notBackgroundColour(col);
    gl->setColour(col);
    float d = 4.5 * scale;
    for (int j : notplot)
      gl->drawCross(x[j], y[j], d, true);
  }
  gl->setColour(origcolour);

  for (size_t n : nextplot) {
    plotIndex(gl, n);
  }

  //reset

  next = false;
  previous = false;
  thisObs = false;
  if (nextplot.empty())
    plotnr = -1;
  origcolour = selectedColour; // reset in case a background contrast colour was used
  firstplot = false;
  recalculate_densities_ = false;
}

void ObsPlot::plotIndex(DiGLPainter* gl, int index)
{
  if (plottype() == OPT_SYNOP) {
    plotSynop(gl, index);
  } else if (plottype() == OPT_METAR) {
    plotMetar(gl, index);
  } else if (plottype() == OPT_LIST) {
    plotList(gl, index);
  }
}

void ObsPlot::areaFreeSetup(float scale, float space, int num, float xdist, float ydist)
{
  METLIBS_LOG_SCOPE(LOGVAL(scale) << LOGVAL(space) << LOGVAL(num) << LOGVAL(xdist) << LOGVAL(ydist));

  areaFreeSpace = space;

  if (pFlag.count("wind"))
    areaFreeWindSize = scale * 47.;
  else
    areaFreeWindSize = 0.0;

  if (num > 0) {
    float d = space * 0.5;
    areaFreeXsize = xdist + space;
    areaFreeYsize = ydist + space;
    areaFreeXmove[0] = -xdist - d;
    areaFreeXmove[1] = -d;
    areaFreeXmove[2] = -d;
    areaFreeXmove[3] = -xdist - d;
    areaFreeYmove[0] = -d;
    areaFreeYmove[1] = -d;
    areaFreeYmove[2] = -ydist - d;
    areaFreeYmove[3] = -ydist - d;
  } else {
    areaFreeXsize = 0.0;
    areaFreeYsize = 0.0;
  }
}

bool ObsPlot::areaFree(int idx)
{
  METLIBS_LOG_SCOPE("idx: " << idx);

  float xc = x[idx];
  float yc = y[idx];

  ObsPlotCollider::Box ub[2];
  int idd = 0, nb = 0;

  int pos = 1;

  if (areaFreeWindSize > 0.0) {
    if (const float* pdd = obsp[idx].get_float("dd"))
      idd = (int)*pdd;

    if (idd > 0 && idd < 361) {
      float dd = float(idd) * M_PI / 180.;
      float xw = areaFreeWindSize * sin(dd);
      float yw = areaFreeWindSize * cos(dd);
      if (xw < 0.) {
        ub[0].x1 = xc + xw - areaFreeSpace;
        ub[0].x2 = xc + areaFreeSpace;
      } else {
        ub[0].x1 = xc - areaFreeSpace;
        ub[0].x2 = xc + xw + areaFreeSpace;
      }
      if (yw < 0.) {
        ub[0].y1 = yc + yw - areaFreeSpace;
        ub[0].y2 = yc + areaFreeSpace;
      } else {
        ub[0].y1 = yc - areaFreeSpace;
        ub[0].y2 = yc + yw + areaFreeSpace;
      }
      nb = 1;
      if (vertical_orientation) {
        pos = (idd - 1) / 90;
      } else {
        if (idd < 91) {
          ub[0].x1 += areaFreeXsize / 2;
          ub[0].x2 += areaFreeXsize * 1.5;
        }
      }
    }
  }

  if (areaFreeXsize > 0.) {
    xc += areaFreeXmove[pos];
    yc += areaFreeYmove[pos];
    ub[nb].x1 = xc;
    ub[nb].x2 = xc + areaFreeXsize;
    ub[nb].y1 = yc;
    ub[nb].y2 = yc + areaFreeYsize;
    nb++;
  }

  return collider_->areaFree(&ub[0], nb == 2 ? &ub[1] : 0);
}

void ObsPlot::advanceByDD(int dd, QPointF& xypos)
{
  if (!vertical_orientation && dd > 20 && dd < 92) {
    const float sindd = sin(dd * M_PI / 180);
    float dx;
    if (dd < 70) {
      dx = 48 * sindd / 2;
    } else if (dd < 85) {
      dx = 55 * sindd;
    } else {
      dx = 48 * sindd;
    }
    xypos.rx() += dx;
  }
}

void ObsPlot::printListParameter(DiGLPainter* gl, const ObsDataRef& dta, const ObsDialogInfo::Par& param, QPointF& xypos, float yStep, bool align_right,
                                 float xshift)
{
  METLIBS_LOG_SCOPE(LOGVAL(param.name));

  if (!pFlag.count(miutil::to_lower(param.name)))
    return;

  if (param.name == "pos") {
    printListPos(gl, dta, xypos, yStep, align_right);
    return;
  }

  const std::string* s_p = dta.get_string(param.name);
  const float* f_p = dta.get_float(param.name);
  if (!plotundef && !s_p && !f_p)
    return;

  xypos.ry() -= yStep;

  if (miutil::contains(param.name, "RRR")) {
    printListRRR(gl, dta,param.name, xypos, align_right);
    return;
  }
  if (param.symbol > -1) {
    printListSymbol(gl, dta,param,xypos,yStep,align_right,xshift);
  } else {
    METLIBS_LOG_DEBUG(LOGVAL(param.name));
    if (s_p) {
      checkColourCriteria(gl, param.name,undef);
      std::string str = *s_p;
      if (parameterName)
        str = param.name + " " + str;
      METLIBS_LOG_DEBUG(LOGVAL(param.name) << LOGVAL(str));

      printListString(gl, str, xypos, align_right);
    } else {
      if (f_p) {
        checkColourCriteria(gl, param.name, *f_p);
        if (param.name == "VV") {
          printVisibility(gl, *f_p, dta.ship_buoy(), xypos, align_right);
        } else if (param.type == ObsDialogInfo::pt_knot && !unit_ms) {
          printList(gl, miutil::ms2knots(*f_p), xypos, param.precision, align_right);
        } else if (param.type == ObsDialogInfo::pt_temp && tempPrecision) {
          printList(gl, *f_p, xypos, 0, align_right);
        } else {
          printList(gl, *f_p, xypos, param.precision, align_right);
        }
      } else {
        printUndef(gl, xypos, align_right);
      }
    }
  }
}

void ObsPlot::printListSymbol(DiGLPainter* gl, const ObsDataRef& dta, const ObsDialogInfo::Par& param, QPointF& xypos, float yStep, bool align_right,
                              const float& xshift)
{
  if (const float* f_p = dta.get_float(param.name)) {
    checkColourCriteria(gl, param.name, *f_p);
    QPointF spos = xypos * scale - QPointF(xshift, 0);
    if (param.symbol == 201 && *f_p > 0 && *f_p < 9) {
      symbol(gl, vtab(param.symbol + (int)*f_p), spos, 0.6 * scale, align_right);
    } else if (param.symbol > 0 && *f_p > 0 && *f_p < 10) {
      symbol(gl, vtab(param.symbol + (int)*f_p), spos, 0.6 * scale, align_right);
    } else if ((param.name == "W1" || param.name == "W2") && *f_p > 2) {
      pastWeather(gl, (int)*f_p, spos, 0.6 * scale, align_right);

    } else if (param.name == "ww") {

      if (const float* pTTT = dta.get_float("TTT")) {
        weather(gl, (short int)*f_p, *pTTT, dta.ship_buoy(), spos - QPointF(0, 0.2 * yStep * scale), scale * 0.6, align_right);
      } else {
        if (plotundef)
          printUndef(gl, xypos, align_right);
      }
    } else if (param.name == "ds") {
      arrow(gl, *f_p, spos, scale * 0.6);
    } else if (param.name == "dw1dw1") {
      zigzagArrow(gl, *f_p, spos, scale * 0.6);
    }
    if (!vertical_orientation)
      xypos.rx() += 20;

  } else {
    printUndef(gl, xypos, align_right);
  }
}

void ObsPlot::printListRRR(DiGLPainter* gl, const ObsDataRef& dta, const std::string& param, QPointF& xypos, bool align_right)
{
  if (const float* f_p = dta.get_float(param)) {
    checkColourCriteria(gl, param, *f_p);
    if (*f_p < 0.0) { // Precipitation, but less than 0.1 mm (0.0)
      printListString(gl, "0.0", xypos, align_right);
    } else if (*f_p < 0.1) { // No precipitation (0.)
      printListString(gl, "0.", xypos, align_right);
    } else {
      printList(gl, *f_p, xypos, 1, align_right);
    }
  } else {
    printUndef(gl, xypos, align_right);
  }
}

void ObsPlot::printListPos(DiGLPainter* gl, const ObsDataRef& dta, QPointF& xypos, float yStep, bool align_right)
{
  xypos.ry() -= yStep;
  std::string str1 = diutil::formatLatitude(dta.ypos(), 2, 6).toStdString();
  std::string str2 = diutil::formatLongitude(dta.xpos(), 2, 6).toStdString();
  if (yStep <= 0)
    str1.swap(str2); // change order of lon and lat

  checkColourCriteria(gl, "Pos", 0);

  printString(gl, str1, xypos, align_right);
  advanceByStringWidth(gl, str1, xypos);
  xypos.ry() -= yStep;
  printString(gl, str2, xypos, align_right);
  advanceByStringWidth(gl, str2, xypos);
}

void ObsPlot::printUndef(DiGLPainter* gl, QPointF& xypos, bool align_right)
{
  gl->setColour(colour);
  printListString(gl, "X", xypos, align_right);
}

void ObsPlot::printList(DiGLPainter* gl, float f, QPointF& xypos, int precision,
    bool align_right, bool fill_2)
{
  if (f != undef) {
    bool do_showplus = false, do_showpoint = false, do_fill = fill_2;

    QString cs;
    if (do_showplus)
      cs.append(f >= 0 ? '+' : '-');
    cs.append(QString::number(f, 'f', precision));
    if (do_showpoint && precision == 0)
      cs.append('.');
    if (do_fill)
      cs = cs.rightJustified(2, '0');

    printListString(gl, cs.toStdString(), xypos, align_right);
  } else {
    printListString(gl, "X", xypos, align_right);
  }
}

float ObsPlot::advanceByStringWidth(DiGLPainter* gl, const std::string& txt, QPointF& xypos)
{
  float w, h;
  gl->getTextSize(txt, w, h);
  if (!vertical_orientation)
    xypos.rx() += w / scale + 5;
  return w;
}

void ObsPlot::printListString(DiGLPainter* gl, const std::string& txt, QPointF& xypos, bool align_right)
{
  QPointF xy = xypos * scale;
  const float w = advanceByStringWidth(gl, txt, xypos);
  if (align_right)
    xy.rx() -= w;
  gl->drawText(txt, xy.x(), xy.y(), 0.0);
}

bool ObsPlot::checkQuality(const ObsDataRef& dta) const
{
  if (not qualityFlag)
    return true;
  if (const float* pq = dta.get_float("quality"))
    return (int(*pq) & QUALITY_GOOD);
  return true; // assume good data
}

bool ObsPlot::checkWMOnumber(const ObsDataRef& dta) const
{
  if (not wmoFlag)
    return true;
  return (dta.get_float("wmonumber") != nullptr);
}

void ObsPlot::plotList(DiGLPainter* gl, int index)
{
  METLIBS_LOG_SCOPE("index: " << index);

  const auto& dta = obsp[index];

  DiGLPainter::GLfloat radius = 3.0;
  int printPos = -1;
  if (!left_alignment)
    printPos = 0;
  QPointF xypos(0, 0);
  float xshift = 0;
  float width, height;
  gl->getTextSize("0", width, height);
  height *= 1.2; // FIXME
  float yStep = height / scale; //depend on character height
  bool align_right = false;
  const float *pdd, *pdd_rotated, *pff;
  bool windOK = pFlag.count("Wind") && (pdd = dta.get_float("dd")) && (pdd_rotated = dta.get_float("dd_adjusted")) && (pff = dta.get_float("ff"));

  if (not checkQuality(dta) or not checkWMOnumber(dta))
    return;

  if (!isdata(dta))
    return;

  //reset colour
  gl->setColour(origcolour);
  colour = origcolour;

  checkTotalColourCriteria(gl, dta);

  const std::string thisImage = checkMarkerCriteria(dta);
  float thisMarkerSize = checkMarkersizeCriteria(dta);

  ImageGallery ig;
  float xShift = 0;
  float yShift = 0;

  if (!windOK) {
    if (const std::string* it = dta.get_string("image")) {
      const std::string& thatImage = *it;
      xShift = ig.widthp(thatImage) / 2;
      yShift = ig.heightp(thatImage) / 2;
      ig.plotImage(gl, getStaticPlot()->plotArea(), thatImage, x[index], y[index], true, thisMarkerSize);
    } else if (image != "off") {
      xShift = ig.widthp(thisImage) / 2;
      yShift = ig.heightp(thisImage) / 2;
      ig.plotImage(gl, getStaticPlot()->plotArea(), thisImage, x[index], y[index], true, thisMarkerSize);
    }
    if (vertical_orientation)
      xypos.ry() += yShift;
    xypos.rx() += xShift;
  }

  PushPopTranslateScale pushpop1(gl, QPointF(x[index], y[index]));

  if (windOK) {
    int dd = int(*pdd);
    int dd_adjusted = int(*pdd_rotated);
    float ff = *pff;

    PushPopTranslateScale pushpop2(gl, scale);
    bool ddvar = false;
    if (dd == 990 || dd == 510) {
      ddvar = true;
      dd_adjusted = 270;
    }
    printPos = (dd_adjusted - 1) / 90;

    if (ff == 0) {
      dd = 0;
      printPos = 1;
      if (vertical_orientation)
        xypos.ry() += yShift;
      xypos.rx() += xShift;
    }

    checkColourCriteria(gl, "dd", dd);
    checkColourCriteria(gl, "ff", ff);
    plotWind(gl, dd_adjusted, ff, ddvar, radius, dta.ypos() > 0);

    advanceByDD(dd_adjusted, xypos);
  }

  if (!vertical_orientation) {
    yStep = 0;
  } else {
    align_right = (printPos == 0 || printPos == 3);
    if ( printPos < 2 ) {
      yStep *= -1;
      xypos.ry() += yStep;
    }
    if ( printPos == 0 || printPos == 3 )
      xshift = 2 * width;

  }
  if (plottype() == OPT_LIST) {
    if (yStep < 0)
      for (const auto& p : reverse(vparam)) {

        printListParameter(gl, dta, p, xypos, yStep, align_right, xshift);
      }
    else
      for (const auto& p : vparam)
        printListParameter(gl, dta, p, xypos, yStep, align_right, xshift);
  }
}


int ObsPlot::vtab(int idx) const
{
  if (idx >= 0 && idx < (int)itab->size())
    return (*itab)[idx];
  else {
    METLIBS_LOG_ERROR("itab idx=" << idx << " out of range 0.." << itab->size());
    return -1000;
  }
}

QPointF ObsPlot::xytab(int idxy) const
{
  return xytab(idxy, idxy+1);
}

QPointF ObsPlot::xytab(int idx, int idy) const
{
  if (idx >= 0 && idx < (int)iptab->size() && idy >= 0 && idy < (int)iptab->size())
    return QPointF((*iptab)[idx], (*iptab)[idy]);
  else {
    METLIBS_LOG_ERROR("iptab idx=" << idx << " / " << idy << " out of range 0.." << iptab->size());
    return QPointF(-100, 100);
  }
}

void ObsPlot::plotSynop(DiGLPainter* gl, int index)
{
  METLIBS_LOG_SCOPE("index: " << index);

  const auto dta = obsp[index];

  if (not checkQuality(dta) or not checkWMOnumber(dta))
    return;

  DiGLPainter::GLfloat radius = 7.0;
  int lpos;
  const float* ttt_p = dta.get_float("TTT");

  if (!isdata(dta))
    return;

  //Some positions depend on wheather the following parameters are plotted or not
  bool ClFlag = ((pFlag.count("cl") && dta.get_float("Cl")) || ((pFlag.count("st.type") && (not dta.dataType().empty()))));
  bool TxTnFlag = (pFlag.count("txtn") && dta.get_float("TxTn"));
  bool timeFlag = (pFlag.count("time") && dta.ship_buoy());

  //reset colour
  gl->setColour(origcolour);
  colour = origcolour;

  checkTotalColourCriteria(gl, dta);

  PushPopTranslateScale pushpop1(gl, QPointF(x[index], y[index]));
  PushPopTranslateScale pushpop2(gl, scale);

  //wind - dd,ff
  const float *pdd, *pff, *pdd_rotated;
  if (pFlag.count("wind")
      && (pdd = obsp.get_unrotated_float(index, "dd"))
      && (pff = dta.get_float("ff"))
      && (pdd_rotated = dta.get_float("dd_adjusted"))
      &&(*pdd != undef))
  {
    bool ddvar = false;
    int dd = (int)*pdd;
    int dd_adjusted = (int)*pdd_rotated;
    if (dd == 990 || dd == 510) {
      ddvar = true;
      dd_adjusted = 270;
    }
    if (miutil::ms2knots(*pff) < 1.)
      dd = 0;
    lpos = vtab((dd / 10 + 3) / 2) + 10;
    checkColourCriteria(gl, "dd", dd);
    checkColourCriteria(gl, "ff", *pff);
    plotWind(gl, dd_adjusted, *pff, ddvar, radius, dta.ypos() > 0);
  } else {
    lpos = vtab(1) + 10;
  }

  { // Total cloud cover - N
    float fN = undef;
    if (const float* f_p = dta.get_float("N")) {
      fN = *f_p;
      checkColourCriteria(gl, "N", fN);
    } else {
      gl->setColour(colour);
    }
    if (isauto(dta))
      cloudCoverAuto(gl, fN, radius);
    else
      cloudCover(gl, fN, radius);
  }

  const float *f_p, *h_p;

  //Weather - WW
  float VVxpos = xytab(lpos + 14).x() + 22;
  if (pFlag.count("ww") && (f_p = dta.get_float("ww")) && ttt_p) {
    checkColourCriteria(gl, "ww", *f_p);
    const QPointF wwxy = xytab(lpos + 12);
    VVxpos = wwxy.x() - 20;
    weather(gl, (short int)*f_p, *ttt_p, dta.ship_buoy(), wwxy);
  }

  //characteristics of pressure tendency - a
  const float* ppp_p = dta.get_float("ppp");
  if (pFlag.count("a") && (f_p = dta.get_float("a")) && *f_p >= 0 && *f_p < 9) {
    checkColourCriteria(gl, "a", *f_p);
    if (ppp_p && *ppp_p > 9)
      symbol(gl, vtab(201 + (int)*f_p), xytab(lpos + 42) + QPointF(12, 0), 0.8);
    else
      symbol(gl, vtab(201 + (int)*f_p), xytab(lpos + 42), 0.8);
  }

  // High cloud type - Ch
  if (pFlag.count("ch") && (f_p = dta.get_float("Ch"))) {
    checkColourCriteria(gl, "Ch", *f_p);
    // METLIBS_LOG_DEBUG("Ch: " << *f_p);
    symbol(gl, vtab(190 + (int)*f_p), xytab(lpos + 4), 0.8);
  }

  // Middle cloud type - Cm
  if (pFlag.count("cm") && (f_p = dta.get_float("Cm"))) {
    checkColourCriteria(gl, "Cm", *f_p);
    // METLIBS_LOG_DEBUG("Cm: " << *f_p);
    symbol(gl, vtab(180 + (int)*f_p), xytab(lpos + 2), 0.8);
  }

  // Low cloud type - Cl
  if (pFlag.count("cl") && (f_p = dta.get_float("Cl"))) {
    checkColourCriteria(gl, "Cl", *f_p);
    // METLIBS_LOG_DEBUG("Cl: " << *f_p);
    symbol(gl, vtab(170 + (int)*f_p), xytab(lpos + 22), 0.8);
  }

  // Past weather - W1
  if (pFlag.count("w1") && (f_p = dta.get_float("W1"))) {
    checkColourCriteria(gl, "W1", *f_p);
    pastWeather(gl, int(*f_p), xytab(lpos + 34), 0.8);
  }

  // Past weather - W2
  if (pFlag.count("w2") && (f_p = dta.get_float("W2"))) {
    checkColourCriteria(gl, "W2", *f_p);
    pastWeather(gl, (int)*f_p, xytab(lpos + 36), 0.8);
  }

  // Direction of ship movement - ds
  if (pFlag.count("ds") && dta.get_float("vs") && (f_p = dta.get_float("ds"))) {
    checkColourCriteria(gl, "ds", *f_p);
    arrow(gl, *f_p, xytab(lpos + 32));
  }

  // Direction of swell waves - dw1dw1
  if (pFlag.count("dw1dw1") && (f_p = dta.get_float("dw1dw1"))) {
    checkColourCriteria(gl, "dw1dw1", *f_p);
    zigzagArrow(gl, *f_p, xytab(lpos + 30));
  }

  pushpop2.PopMatrix();

  // Pressure - PPPP
  if (mslp()) {
    if ((f_p = dta.get_float("PPPP_mslp"))) {
      checkColourCriteria(gl, "PPPP_mslp", *f_p);
      printNumber(gl, *f_p, xytab(lpos + 44), "PPPP_mslp");
    }
  } else if (pFlag.count("pppp") && (f_p = dta.get_float("PPPP"))) {
    checkColourCriteria(gl, "PPPP", *f_p);
    printNumber(gl, *f_p, xytab(lpos + 44), "PPPP");
  }

  // Pressure tendency over 3 hours - ppp
  if (pFlag.count("ppp") && ppp_p) {
    checkColourCriteria(gl, "ppp", *ppp_p);
    printNumber(gl, *ppp_p, xytab(lpos + 40), "ppp");
  }
  // Clouds
  if (pFlag.count("nh") || pFlag.count("h")) {
    f_p = dta.get_float("Nh");
    h_p = dta.get_float("h");
    if (f_p || h_p) {
      float Nh, h;
      if (!f_p)
        Nh = undef;
      else
        Nh = *f_p;
      if (!h_p)
        h = undef;
      else
        h = *h_p;
      if (Nh != undef)
        checkColourCriteria(gl, "Nh", Nh);
      if (h != undef)
        checkColourCriteria(gl, "h", h);
      if (ClFlag) {
        amountOfClouds(gl, (short int) Nh, (short int) h, xytab(lpos + 24));
      } else {
        amountOfClouds(gl, (short int) Nh, (short int) h, xytab(lpos + 24) + QPointF(0, 10));
      }
    }
  }

  // Clouds, detailed
  if (pFlag.count("clouds")) {
    checkColourCriteria(gl, "Clouds", 0);
    int ncl = dta.cloud().size();
    for (int i = 0; i < ncl; i++)
      printString(gl, dta.cloud()[i], xytab(lpos + 18) + QPointF(0, -i * 12));
  }

  //Precipitation - RRR
  if (pFlag.count("rrr") && !(dta.ship_buoy() && dta.get_float("ds") && dta.get_float("vs"))) {
    if ((f_p = dta.get_float("RRR"))) {
      checkColourCriteria(gl, "RRR", *f_p);
      if (*f_p < 0.0) // Precipitation, but less than 0.1 mm (0.0)
        printString(gl, "0.0", xytab(lpos + 32) + QPointF(2, 0));
      else if (*f_p < 0.1) // No precipitation (0.)
        printString(gl, "0.", xytab(lpos + 32) + QPointF(2, 0));
      else
        printNumber(gl, *f_p, xytab(lpos + 32) + QPointF(2, 0), "RRR");
    }
  }
  // Horizontal visibility - VV
  if (pFlag.count("vv") && (f_p = dta.get_float("VV"))) {
    checkColourCriteria(gl, "VV", *f_p);
    QPointF vvxy(VVxpos, xytab(lpos + 14).y());
    printVisibility(gl, *f_p, dta.ship_buoy(), vvxy);
  }
  // Temperature - TTT
  if (pFlag.count("ttt") && ttt_p) {
    checkColourCriteria(gl, "TTT", *ttt_p);
    printNumber(gl, *ttt_p, xytab(lpos + 10), "temp");
  }

  // Dewpoint temperature - TdTdTd
  if (pFlag.count("tdtdtd") && (f_p = dta.get_float("TdTdTd"))) {
    checkColourCriteria(gl, "TdTdTd", *f_p);
    printNumber(gl, *f_p, xytab(lpos + 16), "temp");
  }

  // Max/min temperature - TxTxTx/TnTnTn
  if (TxTnFlag) {
    if ((f_p = dta.get_float("TxTn"))) {
      checkColourCriteria(gl, "TxTn", *f_p);
      printNumber(gl, *f_p, xytab(lpos + 8), "temp");
    }
  }

  // Snow depth - sss
  if (pFlag.count("sss") && (f_p = dta.get_float("sss")) && !dta.ship_buoy()) {
    checkColourCriteria(gl, "sss", *f_p);
    printNumber(gl, *f_p, xytab(lpos + 46));
  }

  // Maximum wind speed (gusts) - 911ff
  if (pFlag.count("911ff")) {
    if ((f_p = dta.get_float("911ff"))) {
      checkColourCriteria(gl, "911ff", *f_p);
      float ff = unit_ms ? *f_p : miutil::ms2knots(*f_p);
      printNumber(gl, ff, xytab(lpos + 38), "fill_2", true);
    }
  }

  // State of the sea - s
  if (pFlag.count("s") && (f_p = dta.get_float("s"))) {
    checkColourCriteria(gl, "s", *f_p);
    if (TxTnFlag)
      printNumber(gl, *f_p, xytab(lpos + 6));
    else
      printNumber(gl, *f_p, xytab(lpos + 6) + QPointF(0, -14));
  }

  // Maximum wind speed
  if (pFlag.count("fxfx")) {
    if ((f_p = dta.get_float("fxfx")) && !dta.ship_buoy()) {
      checkColourCriteria(gl, "fxfx", *f_p);
      float ff = unit_ms ? *f_p : miutil::ms2knots(*f_p);
      if (TxTnFlag)
        printNumber(gl, ff, xytab(lpos + 6) + QPointF(10, 0), "fill_2", true);
      else
        printNumber(gl, ff, xytab(lpos + 6) + QPointF(10, -14), "fill_2", true);
    }
  }

  //Maritime

  // Ship's average speed - vs
  if (pFlag.count("vs") && dta.get_float("ds") && (f_p = dta.get_float("vs"))) {
    checkColourCriteria(gl, "vs", *f_p);
    printNumber(gl, *f_p, xytab(lpos + 32) + QPointF(18, 0));
  }

  //Time
  if (timeFlag && !dta.obsTime().undef()) {
    checkColourCriteria(gl, "Time", 0);
    printTime(gl, dta.obsTime(), xytab(lpos + 46), "left", "h.m");
  }

  // Ship or buoy identifier
  if (pFlag.count("id") && dta.ship_buoy()) {
    checkColourCriteria(gl, "Id", 0);
    if (timeFlag)
      printString(gl, dta.id(), xytab(lpos + 46) + QPointF(0, 15));
    else
      printString(gl, dta.id(), xytab(lpos + 46));
  }

  //Wmo block + station number - land stations
  if (pFlag.count("st.no") && !dta.ship_buoy()) {
    checkColourCriteria(gl, "st.no", 0);
    if ((pFlag.count("sss") && dta.get_float("sss"))) // if snow
      printString(gl, dta.id(), xytab(lpos + 46) + QPointF(0, 15));
    else
      printString(gl, dta.id(), xytab(lpos + 46));
  }
  // WMO station id or callsign
  if (pFlag.count("name") && !dta.id().empty()) {
    checkColourCriteria(gl, "name", 0);
    QPointF offset;
    if (pFlag.count("sss") && dta.get_float("sss")) // if snow
      offset = QPointF(0, 15);
    printString(gl, dta.id(), xytab(lpos + 46) + offset);
  }

  //Sea temperature
  if (pFlag.count("twtwtw") && (f_p = dta.get_float("TwTwTw"))) {
    checkColourCriteria(gl, "TwTwTw", *f_p);
    printNumber(gl, *f_p, xytab(lpos + 18), "temp", true);
  }

  //Wave

  if (pFlag.count("pwahwa")) {
    f_p = dta.get_float("PwaPwa");
    h_p = dta.get_float("HwaHwa");
    if (f_p || h_p) {
      float pwa = f_p ? *f_p : undef;
      float hwa = h_p ? *h_p : undef;
      checkColourCriteria(gl, "PwaHwa", 0);
      wave(gl, pwa, hwa, xytab(lpos + 20));
    }
  }

  if (pFlag.count("pw1hw1")) {
    f_p = dta.get_float("Pw1Pw1");
    h_p = dta.get_float("Hw1Hw1");
    if (f_p || h_p) {
      float pw1 = f_p ? *f_p : undef;
      float hw1 = h_p ? *h_p : undef;
      checkColourCriteria(gl, "Pw1Hw1", 0);
      wave(gl, pw1, hw1, xytab(lpos + 20));
    }
  }

}

void ObsPlot::plotMetar(DiGLPainter* gl, int index)
{
  METLIBS_LOG_SCOPE("index: " << index);

  const auto dta = obsp[index];

  DiGLPainter::GLfloat radius = 7.0;
  int lpos = vtab(1) + 10;

  if (!isdata(dta))
    return;

  const float *f_p, *h_p;

  //reset colour
  gl->setColour(origcolour);
  colour = origcolour;

  checkTotalColourCriteria(gl, dta);

  PushPopTranslateScale pushpop1(gl, QPointF(x[index], y[index]));

  //Circle
  PushPopTranslateScale pushpop2(gl, scale);
  drawCircle(gl);
  pushpop2.PopMatrix();

  //wind
  const float *pdd, *pdd_rotated, *pff;
  if (pFlag.count("wind") && (pdd = obsp.get_unrotated_float(index, "dd"))
      && (pdd_rotated = dta.get_float("dd_adjusted"))
      && (pff = dta.get_float("ff")))
  {
    checkColourCriteria(gl, "dd", *pdd);
    checkColourCriteria(gl, "ff", *pff);
    metarWind(gl, (int)*pdd_rotated, miutil::ms2knots(*pff), radius, lpos);
  }

  //limit of variable wind direction
  int dndx = 16;
  if (pFlag.count("dndx") && (f_p = dta.get_float("dndndn")) && (h_p = dta.get_float("dxdxdx"))) {
    QString cs = QString("%1V%2").arg(*f_p / 10).arg(*h_p / 10);
    printString(gl, cs.toStdString(), xytab(lpos + 2) + QPointF(2, 2));
    dndx = 2;
  }
  //Wind gust
  QPointF xyid = xytab(lpos + 4);
  if (pFlag.count("fmfm") && (f_p = dta.get_float("fmfm"))) {
    const float ff = unit_ms ? *f_p : miutil::ms2knots(*f_p);
    checkColourCriteria(gl, "fmfm", *f_p);
    printNumber(gl, ff, xyid + QPointF(2, 2 - dndx), "fill_2", true);
    //understrekes
    xyid += QPointF(20 + 15, -dndx + 8);
  } else {
    xyid += QPointF(2 + 15, 2 - dndx + 8);
  }

  //Temperature
  if (pFlag.count("ttt") && (f_p = dta.get_float("TTT"))) {
    checkColourCriteria(gl, "TTT", *f_p);
    //    if( dta.TT>-99.5 && dta.TT<99.5 ) //right align_righted
    printNumber(gl, *f_p, xytab(lpos + 12) + QPointF(23, 16), "temp");
  }

  //Dewpoint temperature
  if (pFlag.count("tdtdtd") && (f_p = dta.get_float("TdTdTd"))) {
    checkColourCriteria(gl, "TdTdTd", *f_p);
    //    if( dta.TdTd>-99.5 && dta.TdTd<99.5 )  //right align_righted and underlined
    printNumber(gl, *f_p, xytab(lpos + 14) + QPointF(23, -16), "temp", true);
  }

  PushPopTranslateScale pushpop3(gl, scale*0.8);

  //Significant weather
  int wwshift = 0; //idxm
  if (pFlag.count("ww")) {
    checkColourCriteria(gl, "ww", 0);
    if (dta.ww().size() > 0 && not dta.ww()[0].empty()) {
      metarSymbol(gl, dta.ww()[0], xytab(lpos + 8), wwshift);
    }
    if (dta.ww().size() > 1 && not dta.ww()[1].empty()) {
      metarSymbol(gl, dta.ww()[1], xytab(lpos + 10), wwshift);
    }
  }

  //Recent weather
  if (pFlag.count("reww")) {
    checkColourCriteria(gl, "REww", 0);
    if (dta.REww().size() > 0 && not dta.REww()[0].empty()) {
      int intREww[5];
      metarString2int(dta.REww()[0], intREww);
      if (intREww[0] >= 0 && intREww[0] < 100) {
        symbol(gl, vtab(40 + intREww[0]), xytab(lpos + 30) + QPointF(0, 2));
      }
    }
    if (dta.REww().size() > 1 && not dta.REww()[1].empty()) {
      int intREww[5];
      metarString2int(dta.REww()[1], intREww);
      if (intREww[0] >= 0 && intREww[0] < 100) {
        symbol(gl, vtab(40 + intREww[0]), xytab(lpos + 30) + QPointF(15, 2));
      }
    }
  }
  pushpop3.PopMatrix();

  //Visibility (worst)
  if (pFlag.count("vvvv/dv")) {
    if ((f_p = dta.get_float("VVVV"))) {
      checkColourCriteria(gl, "VVVV/Dv", 0);
      const QPointF xy = xytab(lpos + 12) + QPointF(22 + wwshift, 2);
      if ((h_p = dta.get_float("Dv"))) {
        printNumber(gl, float(int(*f_p) / 100), xy);
        printNumber(gl, vis_direction(*h_p), xy);
      } else {
        printNumber(gl, float(int(*f_p) / 100), xy);
      }
    }
  }

  //Visibility (best)
  if (pFlag.count("vxvxvxvx/dvx")) {
    if ((f_p = dta.get_float("VxVxVxVx"))) {
      checkColourCriteria(gl, "VVVV/Dv", 0);
      const QPointF dxy(22 + wwshift, 0);
      if ((h_p = dta.get_float("Dvx"))) {
        printNumber(gl, float(int(*f_p) / 100), xytab(lpos + 12, lpos + 15) + dxy);
        printNumber(gl, *h_p, xytab(lpos + 12) + dxy);
      } else {
        printNumber(gl, float(int(*f_p) / 100), xytab(lpos + 14) + dxy);
      }
    }
  }

  QPointF VVxpos = xytab(lpos + 14) + QPointF(22, 0);
  const std::string* s_p;
  if (pFlag.count("gwi") && (s_p = dta.get_string("GWI"))) {
    checkColourCriteria(gl, "GWI", 0);
    printString(gl, *s_p, xytab(lpos + 12, lpos + 13) + QPointF(-8, 0));
    VVxpos = xytab(lpos + 12) - QPointF(-28, 0);
  }

  // horizontal prevailing visibility
  if (pFlag.count("vv") && (f_p = dta.get_float("VV"))) {
    checkColourCriteria(gl, "VV", *f_p);
    QPointF vvxy(VVxpos.x(), xytab(lpos + 14).y());
    printVisibility(gl, *f_p, dta.ship_buoy(), vvxy);
  }

  //CAVOK
  if (pFlag.count("clouds")) {
    checkColourCriteria(gl, "Clouds", 0);

    if (dta.CAVOK()) {
      printString(gl, "CAVOK", xytab(lpos + 18) + QPointF(2, 2));
    } else { // Clouds
      int ncl = dta.cloud().size();
      for (int i = 0; i < ncl; i++)
        printString(gl, dta.cloud()[i], xytab(lpos + 18 + i * 4) + QPointF(2, 2));
    }
  }

  //QNH ??
  if (pFlag.count("phphphph")) {
    if ((f_p = dta.get_float("PHPHPHPH"))) {
      checkColourCriteria(gl, "PHPHPHPH", *f_p);
      int pp = (int)*f_p;
      pp -= (pp / 100) * 100;
      printNumber(gl, pp, xytab(lpos + 32) + QPointF(2, 2), "fill_2");
    }
  }

  //Id
  if (pFlag.count("id")) {
    checkColourCriteria(gl, "Id", 0);
    printString(gl, dta.metarId(), xyid);
  }

  // Name
  if (pFlag.count("name")) {
    checkColourCriteria(gl, "Name", 0);
    printString(gl, dta.id(), xyid);
  }
}

void ObsPlot::metarSymbol(DiGLPainter* gl, const std::string& ww, QPointF xypos, int &idxm)
{
  int intww[5];

  metarString2int(ww, intww);

  if (intww[0] < 0 || intww[0] > 99)
    return;

  int lww1, lww2, lww3, lww4;
  int sign;
  int idx = 0;
  float dx = xypos.x(), dy = xypos.y();

  lww1 = intww[0];
  lww2 = intww[1];
  lww3 = intww[2];
  lww4 = intww[3];
  sign = intww[4];

  if (lww1 == 24) {
    dx += 10;
    symbol(gl, vtab(40 + lww1), QPointF(dx, dy));
    lww3 = undef;
    if (lww2 > 500 && lww2 < 9000) {
      lww3 = lww2 - (lww2 / 100) * 100;
      lww2 = lww2 / 100;
      dx -= 5;
    }
    if (lww2 == 45 || (lww2 > 49 && lww2 < 71) || lww2 == 87 || lww2 == 89) {
      dy -= 6;
      symbol(gl, vtab(40 + lww2), QPointF(dx, dy));
      if (lww3 > 49) {
        dx += 10;
        symbol(gl, vtab(40 + lww3), QPointF(dx, dy));
      }
      dy += 6;
    }
    lww1 = undef;
  } else {
    if (lww2 > 0)
      idx -= 20;
    if (lww3 > 0)
      idx -= 20;
    if (lww4 > 0)
      idx -= 20;
    if (idx < idxm)
      idxm = idx;
  }
  dx = xypos.x() + idx;
  if (sign > 0) {
    symbol(gl, vtab(40 + sign), QPointF(dx+2, dy));
  }
  dx += 10;

  int lwwx[4];
  lwwx[0] = lww1;
  lwwx[1] = lww2;
  lwwx[2] = lww3;
  lwwx[3] = lww4;
  for (int i = 0; i < 4 && lwwx[i] > -1; i++) {
    lww1 = lwwx[i];
    dy = xypos.y();
    if (lww1 > 5000 && lww1 < 9000) {
      dy += 5;
      symbol(gl, vtab(40 + lww1 / 100), QPointF(dx, dy));
      dy -= 10;
      lww1 -= (lww1 / 100) * 100;
    }
    symbol(gl, vtab(40 + lww1), QPointF(dx, dy));
    dx += 20;
  }
}

void ObsPlot::metarString2int(std::string ww, int intww[])
{
  int size = ww.size();

  if (size == 0) {
    for (int i = 0; i < 4; i++)
      intww[i] = undef;
    intww[4] = 0;
    return;
  }

  int sign = 0;
  if (ww[0] == '-' || ww[0] == '+') {
    sign = (ww[0] == '-') ? 1 : 2;
    ww.erase(ww.begin());
  } else if (size > 1 && ww.substr(0, 2) == "VC") {
    sign = 3;
    ww.erase(ww.begin(), ww.begin() + 2);
  }

  std::vector<metarww> ia;
  for (int i = 0; i < size; i += 2) {
    std::string sub = ww.substr(i, 2);
    if (metarMap.find(sub) != metarMap.end())
      ia.push_back(metarMap[sub]);
  }

  int iasize = ia.size();
  for (int i = 1; i < iasize; i++) {
    if (ia[i].lwwg < ia[i - 1].lwwg) {
      metarww tmp = ia[i];
      ia[i] = ia[i - 1];
      ia[i - 1] = tmp;
      i = 0;
    }
  }

  if (iasize > 1) {
    bool k = false;
    if ((ia[0].lww == 11 || ia[0].lww == 41) && (ia[1].lww == 45)) //MIFG, BCFG
      k = true;
    if ((ia[0].lww == 38 || ia[0].lww == 36) && (ia[1].lww == 70)) //BLSN, DRSN
      k = true;
    //BLDU, BLSA, DRDU, DRSA
    if ((ia[0].lww == 38 || ia[0].lww == 36)
        && (ia[1].lww == 6 || ia[1].lww == 7)) {
      ia[0].lww = 31;
      k = true;
    }
    if (k)
      ia.erase(ia.begin() + 1);
  }

  iasize = ia.size();
  int j1 = 0;
  if (iasize > 1)
    for (int i = 0; i < iasize; i++)
      if (ia[i].lwwg == 2) {
        j1++;
        if (i != 0 && ia[0].lww == 80 && ia[i].lww == 80) {
          ia[i].lww = 70;
          ia[i].lwwg = 3;
          j1--;
        }
      }

  //testing group 2, only one should be left
  if (j1 > 1) {
    if (ia[0].lwwg == 2) {
      int j2 = lwwg2[ia[0].lww];
      for (int i = 1; i < iasize; i++)
        if (ia[i].lwwg == 2 && lwwg2[ia[i].lww] < j2) {
          ia[0].lww = ia[i].lww;
          ia.erase(ia.begin() + i);
        }
    }
  }

  if (ia.size() > 1)
    if (ia[0].lwwg == 3 && ia[0].lww != 79 && ia[1].lwwg == 3
        && ia[1].lww != 79) {
      ia[0].lww += 100 * ia[1].lww;
      ia.erase(ia.begin() + 1);
    }

  if (ia.size() > 2)
    if (ia[1].lwwg == 3 && ia[1].lww != 79 && ia[2].lwwg == 3
        && ia[2].lww != 79) {
      ia[1].lww += 100 * ia[2].lww;
      ia.erase(ia.begin() + 2);
    }

  if (ia.size() > 3)
    if (ia[2].lwwg == 3 && ia[2].lww != 79 && ia[3].lwwg == 3
        && ia[3].lww != 79) {
      ia[2].lww += 100 * ia[3].lww;
      ia.erase(ia.begin() + 3);
    }

  iasize = ia.size();
  int i;
  for (i = 0; i < iasize; i++) {
    intww[i] = ia[i].lww;
  }
  for (; i < 4; i++)
    intww[i] = undef;
  intww[4] = sign;
}

void ObsPlot::metarWind(DiGLPainter* gl, int dd, int ff, float & radius, int &lpos)
{
  METLIBS_LOG_SCOPE();
  DiGLPainter::GLfloat y1;
  lpos = vtab(1) + 10;
  //dd=999;
  bool box = false;
  bool kryss = true;

  //dd=999 - variable wind direction, ff will be written in the middle of the
  //         circle. (This never happends, because they write dd=990)
  if (dd == 999 && ff >= 0 && ff < 100) {
    printNumber(gl, ff, QPointF(-8, -8), "right");
    y1 = -12;
    box = true;
    kryss = false;
  } else if (dd < 0 || dd > 360) {
    lpos = vtab(20) + 10;
    if (ff >= 0 && ff < 100) //dd undefined - ff written above the circle
      printNumber(gl, ff, QPointF(0, 14), "center");
    else
      //dd and ff undefined - XX written above the circle
      printString(gl, "XX", QPointF(-8, 14));
    y1 = 10;
    box = true;
  }

  PushPopTranslateScale pushpop1(gl, scale);
  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_LINE);

  // Kryss over sirkelen hvis ikke variabel vindretn (dd=999)
  if (kryss)
    cloudCover(gl, undef, radius); //cloud cover not observed

  if (box) {
    gl->drawRect(false, -12, y1, 10, y1 + 22);
    return;
  }

  // vindretn. observert, men ikke vindhast. - skjer ikke for metar (kun synop)

  // vindstille
  if (dd == 0) {
    PushPopTranslateScale pushpop2(gl, 1.5);
    drawCircle(gl);
    cloudCover(gl, 9, radius); // slightly larger cross
    return;
  }

  //#### if( ff > 0 && ff < 100 ){
  if (dd>0 && ff > 0 && ff < 200) {
    lpos = vtab(((dd / 10) + 3) / 2) + 10;
    const float angle = (270-dd)*DEG_TO_RAD, ac = std::cos(angle), as = std::sin(angle);
    const float u = ff*ac, v = ff*as;
    // TODO turn barbs on southern hemisphere
    // FIXME properly draw direction line only until "radius" from (0,0)
    gl->drawWindArrow(u, v, -radius*ac, -radius*as, 47-radius, false, 1);
  }
}

void ObsPlot::initMetarMap()
{
  metarMap["BC"].lww = 41;
  metarMap["BC"].lwwg = 2;
  metarMap["BL"].lww = 38;
  metarMap["BL"].lwwg = 2;
  metarMap["BR"].lww = 10;
  metarMap["BR"].lwwg = 4;
  metarMap["DR"].lww = 36;
  metarMap["DR"].lwwg = 2;
  metarMap["DS"].lww = 34;
  metarMap["DS"].lwwg = 5;
  metarMap["DU"].lww = 6;
  metarMap["DU"].lwwg = 4;
  metarMap["DZ"].lww = 50;
  metarMap["DZ"].lwwg = 3;
  metarMap["FC"].lww = 19;
  metarMap["FC"].lwwg = 5;
  metarMap["FG"].lww = 45;
  metarMap["FG"].lwwg = 4;
  metarMap["FU"].lww = 4;
  metarMap["FU"].lwwg = 4;
  metarMap["FZ"].lww = 24;
  metarMap["GZ"].lwwg = 2;
  metarMap["GR"].lww = 89;
  metarMap["GR"].lwwg = 3;
  metarMap["GS"].lww = 87;
  metarMap["GS"].lwwg = 3;
  metarMap["HZ"].lww = 05;
  metarMap["HZ"].lwwg = 4;
  metarMap["IC"].lww = 76;
  metarMap["IC"].lwwg = 3;
  metarMap["MI"].lww = 11;
  metarMap["MI"].lwwg = 2;
  metarMap["PE"].lww = 79;
  metarMap["PE"].lwwg = 3;
  metarMap["PO"].lww = 8;
  metarMap["PO"].lwwg = 5;
  metarMap["RA"].lww = 60;
  metarMap["RA"].lwwg = 3;
  metarMap["RE"].lww = 60;
  metarMap["RE"].lwwg = 3;
  metarMap["SA"].lww = 7;
  metarMap["SA"].lwwg = 4;
  metarMap["SG"].lww = 77;
  metarMap["SG"].lwwg = 3;
  metarMap["SH"].lww = 80;
  metarMap["SH"].lwwg = 2;
  metarMap["SN"].lww = 70;
  metarMap["SN"].lwwg = 3;
  metarMap["SQ"].lww = 18;
  metarMap["SQ"].lwwg = 5;
  metarMap["SS"].lww = 34;
  metarMap["SS"].lwwg = 5;
  metarMap["TS"].lww = 17;
  metarMap["TS"].lwwg = 2;
  metarMap["VA"].lww = 4;
  metarMap["VA"].lwwg = 4;

  lwwg2[17] = 0;
  lwwg2[24] = 1;
  lwwg2[80] = 2;
  lwwg2[41] = 3;
  lwwg2[11] = 4;
  lwwg2[38] = 5;
  lwwg2[36] = 6;
  lwwg2[31] = 7;
}

void ObsPlot::printNumber(DiGLPainter* gl, float f, QPointF xy, const std::string& align,
    bool line, bool mark)
{
  float x = xy.x() * scale;
  float y = xy.y() * scale;

  std::ostringstream cs;

  if (align == "temp") {
    cs.setf(std::ios::fixed);
    if (tempPrecision) {
      f = diutil::float2int(f);
      cs.precision(0);
    } else {
      cs.precision(1);
    }
    cs << f;
    float w, h;
    gl->getTextSize(cs.str(), w, h);
    x -= w - 30 * scale;
  }

  else if (align == "center") {
    float w, h;
    cs << f;
    gl->getTextSize(cs.str(), w, h);
    x -= w / 2;
  }

  else if (align == "PPPP") {
    cs.width(3);
    cs.fill('0');
    f = (int(f * 10 + 0.5)) % 1000;
    cs << f;
  } else if (align == "float_1") {
    cs.setf(std::ios::fixed);
    cs.precision(1);
    cs << f;
    float w, h;
    gl->getTextSize(cs.str(), w, h);
    x -= w - 30 * scale;
  } else if (align == "fill_2") {
    int i = diutil::float2int(f);
    cs.width(2);
    cs.fill('0');
    cs << i;
  } else if (align == "fill_1") {
    int i = diutil::float2int(f);
    cs.width(1);
    cs.fill('0');
    cs << i;
  } else if (align == "ppp") {
    if (f > 0)
      cs << '+';
    else
      cs << '-';
    if (fabsf(f) < 1)
      cs << '0';
    cs << fabsf(f) * 10;
  } else if (align == "RRR") {
    cs.setf(std::ios::fixed);
    cs.setf(std::ios::showpoint);
    if (f < 1) {
      cs.precision(1);
    } else {
      cs.precision(0);
    }
    cs << f;
  } else if (align == "PPPP_mslp") {
    cs.setf(std::ios::fixed);
    cs.precision(1);
    cs.setf(std::ios::showpos);
    cs << f;
  } else
    cs << f;

  const std::string str = cs.str();
  float cw, ch;
  if (mark || line)
    gl->getTextSize(str, cw, ch);

  if (mark) {
    if (!(colour == Colour::WHITE))
      gl->setColour(Colour::WHITE);
    else
      gl->setColour(Colour::BLACK);
    gl->drawRect(true, x, y - 0.2 * ch, x + cw, y + 0.8 * ch);

    gl->setColour(Colour::BLACK);
    gl->drawRect(false, x, y - 0.2 * ch, x + cw, y + 0.8 * ch);
    gl->setColour(colour);
  }

  gl->drawText(str, x, y, 0.0);

  if (line)
    gl->drawLine(x, (y - ch / 6), (x + cw), (y - ch / 6));
}

void ObsPlot::printString(DiGLPainter* gl, const std::string& c, QPointF xy, bool align_right)
{
  float x = xy.x() * scale;
  float y = xy.y() * scale;

  float w, h;
  if (align_right)
    gl->getTextSize(c, w, h);
  if (align_right)
    x -= w;

  gl->drawText(c, x, y, 0.0);
}

void ObsPlot::printTime(DiGLPainter* gl, const miTime& time, QPointF xy, bool align_right, const std::string& format)
{
  if (time.undef())
    return;

  float x = xy.x() * scale;
  float y = xy.y() * scale;

  std::string s;
  if (format == "h.m") {
    s = time.format("%H.%M", "", true);
  } else if (format == "dato") {
    s = time.format("%m-%d", "", true);
  } else {
    s = time.isoTime();
  }

  if (align_right) {
    float w, h;
    gl->getTextSize(s, w, h);
    x -= w;
  }

  gl->drawText(s, x, y, 0.0);
}

void ObsPlot::printVisibility(DiGLPainter* gl, const float& VV, bool ship, QPointF& vvxy, bool align_right)
{

  std::ostringstream cs;

  if (show_VV_as_code_) {
    cs.width(2);
    cs.fill('0');
    cs << visibilityCode(VV, ship);
  } else {
    float VV_value = VV / 1000;
    if (VV_value < 5.0) {
      cs.setf(std::ios::fixed);
      cs.precision(1);
      cs << VV_value;
    } else {
      cs << diutil::float2int(VV_value);
    }
  }
  printListString(gl, cs.str(), vvxy, align_right);
}

int ObsPlot::visibilityCode(float VV, bool ship)
{
  //Code table 4377
  //12.2.1.3.2   In reporting visibility at sea,
  //             the decile 90 - 99 shall be used for VV

  int vv = int(VV);
  //SHIP
  if (ship) {
    if (vv < 50)
      return 90;
    if (vv < 200)
      return 91;
    if (vv < 500)
      return 92;
    if (vv < 1000)
      return 93;
    if (vv < 2000)
      return 94;
    if (vv < 4000)
      return 95;
    if (vv < 10000)
      return 96;
    if (vv < 20000)
      return 97;
    if (vv < 50000)
      return 98;
    return 99;
  }

  //LAND
  if (vv < 5100)
    return (vv /= 100);
  if (vv < 31000)
    return (vv / 1000 + 50);
  if (vv < 71000)
    return ((vv - 30000) / 5000 + 80);
  return 89;

}

int ObsPlot::vis_direction(float dv)
{
  if (dv < 22)
    return 8;
  return int(dv) / 45;
}

void ObsPlot::amountOfClouds(DiGLPainter* gl, short int Nh, short int h, QPointF xy)
{
  float x = xy.x(), y = xy.y();

  QString ost;
  if (Nh > -1 && Nh < 10)
    ost.setNum(Nh);
  else
    ost = "x";

  gl->drawText(ost, x * scale, y * scale, 0.0);

  x += 8;
  y -= 3;
  gl->drawText("/", x * scale, y * scale, 0.0);

  x += 6; // += 8;
  y -= 3;
  if (h > -1 && h < 10)
    ost.setNum(h);
  else
    ost = "x";

  gl->drawText(ost, x * scale, y * scale, 0.0);
}

void ObsPlot::checkAccumulationTime(size_t index)
{

  // todo: include this if all data sources reports time info
  //  if (dta.get_float("RRR")!= dta.fdata.end()){
  //    dta.fdata.erase(dta.get_float("RRR"));
  //  }

  int hour = obsTime.hour();
  const float* pRRR;
  if ((hour == 6 || hour == 18) && (pRRR = obsp.get_float(index, "RRR_12"))) {

    obsp.put_float(index, "RRR", *pRRR);

  } else if ((hour == 0 || hour == 12) && (pRRR = obsp.get_float(index, "RRR_6"))) {
    obsp.put_float(index, "RRR", *pRRR);

  } else if ((pRRR = obsp.get_float(index, "RRR_1"))) {
    obsp.put_float(index, "RRR", *pRRR);
  }
}

void ObsPlot::checkGustTime(size_t index)
{
  // todo: include this if all data sources reports time info
  //  if (dta.get_float("911ff")!= dta.fdata.end()){
  //    dta.fdata.erase(dta.get_float("911ff"));
  //  }
  int hour = obsTime.hour();
  const float* p911ff;
  if ((hour == 0 || hour == 6 || hour == 12 || hour == 18) && (p911ff = obsp.get_float(index, "911ff_360"))) {
    obsp.put_float(index, "911ff", *p911ff);
  } else if ((hour == 3 || hour == 9 || hour == 15 || hour == 21) && (p911ff = obsp.get_float(index, "911ff_180"))) {
    obsp.put_float(index, "911ff", *p911ff);
  } else if ((p911ff = obsp.get_float(index, "911ff_60"))) {
    obsp.put_float(index, "911ff", *p911ff);
  }
}

bool ObsPlot::updateDeltaTimes()
{
  bool updated = false;
  if (has_deltatime) {
    const miutil::miTime nowTime = miutil::miTime::nowTime();
    for (size_t i = 0; i < obsp.size(); ++i) {
      if (updateDeltaTime(i, nowTime))
        updated = true;
    }
  }
  return updated;
}

bool ObsPlot::updateDeltaTime(size_t index, const miutil::miTime& nowTime)
{
  if (obsp.basic(index).obsTime.undef())
    return false;

  obsp.put_string(index, "DeltaTime", miutil::from_number(miutil::miTime::secDiff(nowTime, obsp[index].obsTime())));
  return true;
}

void ObsPlot::checkMaxWindTime(size_t index)
{
  // todo: include this if all data sources reports time info
  //  if (dta.get_float("fxfx")!= dta.fdata.end()){
  //    dta.fdata.erase(dta.get_float("fxfx"));
  //  }

  int hour = obsTime.hour();
  const float* pfxfx;
  if ((hour == 0 || hour == 6 || hour == 12 || hour == 18) && (pfxfx = obsp.get_float(index, "fxfx_360"))) {

    obsp.put_float(index, "fxfx", *pfxfx);

  } else if ((hour == 3 || hour == 9 || hour == 15 || hour == 21) && (pfxfx = obsp.get_float(index, "fxfx_180"))) {
    obsp.put_float(index, "fxfx", *pfxfx);
  } else if ((pfxfx = obsp.get_float(index, "fxfx_60"))) {
    obsp.put_float(index, "fxfx", *pfxfx);
  }
}

void ObsPlot::arrow(DiGLPainter* gl, float angle, QPointF xypos, float scale)
{
  PushPopTranslateScale pushpop1(gl, xypos, scale);
  gl->Translatef(8, 8, 0.0);
  gl->Rotatef(360 - angle, 0.0, 0.0, 1.0);

  gl->drawLine(0, -6, 0, 6);

  gl->drawTriangle(true, QPointF(-2, 2), QPointF(0, 6), QPointF(2, 2));
}

void ObsPlot::zigzagArrow(DiGLPainter* gl, float angle, QPointF xypos, float scale)
{
  PushPopTranslateScale pushpop1(gl, xypos, scale);
  gl->Translatef(9, 9, 0.0);
  gl->Rotatef(359 - angle, 0.0, 0.0, 1.0);

  QPolygonF line;
  line << QPointF(0, 0)
       << QPointF(2, 1)
       << QPointF(-2, 3)
       << QPointF(2, 5)
       << QPointF(-2, 7)
       << QPointF(2, 9)
       << QPointF(0, 10);
  gl->drawPolyline(line);

  gl->drawLine(0,   0,  0, -10);
  gl->drawLine(0, -10,  4,  -6);
  gl->drawLine(0, -10, -4,  -6);
}

void ObsPlot::symbol(DiGLPainter* gl, int n, QPointF xypos, float scale, bool align_right)
{
  METLIBS_LOG_SCOPE("n: " << n << " xpos: " << xypos.x()
      << " ypos: " << xypos.y() << " scale: " << scale << " align_right: " << align_right);

  PushPopTranslateScale pushpop1(gl, xypos, scale);

  const int npos = iptab->at(n + 3);
  const int nstep = iptab->at(n + 9);
  const int k1 = n + 10;
  const int k2 = k1 + nstep * npos;

  QPolygonF line;
  QPointF xy = xytab(n + 4);
  line << xy;

  for (int i = k1; i < k2; i += nstep) {
    if (i < 0)
      break;
    QPointF dxy = xytab(i);
    if (std::abs(dxy.x()) < 100) {
      xy += dxy;
    } else {
      gl->drawPolyline(line);
      line.clear();

      dxy.rx() = std::fmod(dxy.x(), 100);
      xy += dxy;
    }
    line << xy;
  }

  if (line.count() >= 2)
    gl->drawPolyline(line);
}

void ObsPlot::cloudCover(DiGLPainter* gl, const float& fN, const float &radius)
{
  int N = diutil::float2int(fN);

  int i;
  float x, y;

  // Total cloud cover N
  drawCircle(gl);

  if (N < 0 || N > 9) { //cloud cover not observed
    x = radius * 1.1 / sqrt((float) 2);
    gl->drawCross(0, 0, x, true);
  } else if (N == 9) {
    x = radius / sqrt((float) 2);
    gl->drawCross(0, 0, x, true);
  } else if (N == 7) {
    // qpainter.drawChord(radius, -72*16, 144*16);
    gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
    gl->Begin(DiGLPainter::gl_POLYGON);
    for (i = 5; i < 46; i++) {
      x = radius * cos(i * 2 * M_PI / 100.0);
      y = radius * sin(i * 2 * M_PI / 100.0);
      gl->Vertex2f(y, x);
    }
    i = 5;
    x = radius * cos(i * 2 * M_PI / 100.0);
    y = radius * sin(i * 2 * M_PI / 100.0);
    gl->Vertex2f(y, x);
    gl->End();
    // qpainter.drawChord(radius, 108*16, 144*16);
    gl->Begin(DiGLPainter::gl_POLYGON);
    for (i = 55; i < 96; i++) {
      x = radius * cos(i * 2 * M_PI / 100.0);
      y = radius * sin(i * 2 * M_PI / 100.0);
      gl->Vertex2f(y, x);
    }
    i = 55;
    x = radius * cos(i * 2 * M_PI / 100.0);
    y = radius * sin(i * 2 * M_PI / 100.0);
    gl->Vertex2f(y, x);
    gl->End();
  } else {
    if (N == 1 || N == 3) {
      gl->drawLine(0, radius, 0, -radius);
    } else if (N == 5) {
      gl->drawLine(8, 0, -8, 0);
    }
    // qpainter.drawPie(radius, -90*16, N*45*16);
    gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
    gl->Begin(DiGLPainter::gl_POLYGON);
    gl->Vertex2f(0, 0);
    for (i = 0; i < 101 * (N / 2) / 4.0; i++) {
      x = radius * cos(i * 2 * M_PI / 100.0);
      y = radius * sin(i * 2 * M_PI / 100.0);
      gl->Vertex2f(y, x);
    }
    gl->Vertex2f(0, 0);
    gl->End();
  }
}

void ObsPlot::cloudCoverAuto(DiGLPainter* gl, const float& fN, const float &radius)
{
  const float tmp_radius = 0.6 * radius;
  const float y1 = -1.1 * tmp_radius;
  const float y2 = y1;
  const float x1 = y1 * sqrt(3);
  const float x2 = -1 * x1;
  const float x3 = 0;
  const float y3 = tmp_radius * 2.2;
  gl->drawTriangle(false, QPointF(x1, y1), QPointF(x2, y2), QPointF(x3, y3));

  const int N = diutil::float2int(fN);
  if (N < 0 || N > 9) { // cloud cover not observed
    float x = tmp_radius * 1.1 / sqrt(2);
    gl->drawCross(0, 0, x, true);
  } else if (N == 9) {
    // some special code.., fog perhaps
    float x = tmp_radius / sqrt(2);
    gl->drawCross(0, 0, x, true);
  } else if (N >= 6 && N <= 8) {
    gl->drawTriangle(true, QPointF(x1, y1), QPointF(x2, y2), QPointF(x3, y3));
  } else if (N >= 3 && N <= 5) {
    gl->drawTriangle(true, QPointF(0, y1), QPointF(x2, y2), QPointF(x3, y3));
  }
}

void ObsPlot::plotWind(DiGLPainter* gl, int dd, float ff_ms, bool ddvar, float radius, bool northernHemisphere)
{
  METLIBS_LOG_SCOPE();

  float ff; // wind in knots (if scaled by wind_scale: m/s)
  if (wind_scale > 0) {
    // wind_scale is used when the parameter wind is actually current.
    // Needed to scale the wind arrow
    ff = int(ff_ms * 10.0 / wind_scale);
  } else {
    // wind
    ff = miutil::ms2knots(ff_ms);
  }

  // just a guess of the max possible in plotting below
  if (ff > 200)
    ff = 200;

  diutil::GlMatrixPushPop pushpop1(gl);

  const bool no_cirle = (plottype() == OPT_LIST || wind_scale > 0);
  const float r = (no_cirle ? 0 : radius);

  // calm
  if (ff >= 0 && ff < 1) {
    gl->drawCircle(false, 0, 0, radius*1.5);
  } else if (ff < 0 || ddvar) {
    // line with cross
    gl->Rotatef(360 - dd, 0.0, 0.0, 1.0);

    float yy = 47;
    gl->drawLine(0, r, 0, yy);

    if (ddvar) {
      // cross it middle; otherwise at end
      yy /= 2;
    }
    gl->drawCross(0, yy, 3);
  } else {
    // normal wind arrow

    const bool arrow = poptions.arrowstyle == arrow_wind_arrow && (plottype() == OPT_LIST);

    const float angle = (270-dd)*DEG_TO_RAD, ac = std::cos(angle), as = std::sin(angle);
    const float u = ff*ac, v = ff*as;
    gl->drawWindArrow(u, v, -r * ac, -r * as, 47 - r, arrow, northernHemisphere ? 1 : -1);
  }
}

void ObsPlot::weather(DiGLPainter* gl, short int ww, float TTT, bool ship_buoy, QPointF xypos, float scale, bool align_right)
{
  const int auto2man[100] = { 0, 1, 2, 3, 4, 5, 0, 0, 0, 0, 10, 76, 13, 0, 0, 0,
      0, 0, 18, 0, 28, 21, 20, 21, 22, 24, 29, 38, 38, 37, 41, 41, 43, 45, 47,
      49, 0, 0, 0, 0, 63, 63, 65, 63, 65, 74, 75, 66, 67, 0, 53, 51, 53, 55, 56,
      57, 57, 58, 59, 0, 63, 61, 63, 65, 66, 67, 67, 68, 69, 0, 73, 71, 73, 75,
      79, 79, 79, 0, 0, 0, 81, 80, 81, 82, 85, 86, 86, 0, 0, 0, 17, 17, 95, 96,
      17, 97, 99, 0, 0, 0 };
  const int n_auto2man = boost::size(auto2man);
  const int l_auto2man = 100;

  if (ww >= l_auto2man) {
    const int wwa = ww - l_auto2man;
    if (wwa >= n_auto2man)
      return;
    ww = auto2man[wwa];
  }

  //do not plot ww<3
  if (ww < 3) {
    return;
  }

  int index = iptab->at(1247 + ww);
  if (ww == 7 && ship_buoy)
    index = iptab->at(1247 + 127);
  if (TTT < 0 && (ww > 92 && ww < 98))
    index = iptab->at(1247 + ww + 10);
  if ((TTT >= 0 && TTT < 3) && (ww == 95 || ww == 97))
    index = iptab->at(1247 + ww + 20);

  if (index > 3) {
    float idx = iptab->at(1211 + index);
    xypos += QPointF(idx + 0.2*(22 - idx), -4) * scale;
  }
  //do not plot ww<4

  int n = vtab(40 + ww);
  if (ww == 7 && ship_buoy)
    n = vtab(140);
  if (ww > 92 && TTT > -1000 && TTT < 1000) {
    if (TTT < 0 && (ww > 92 && ww < 96))
      n = vtab(48 + ww);
    if (TTT < 0 && (ww == 97))
      n = vtab(144);
    if ((TTT >= 0 && TTT < 3) && (ww == 95))
      n = vtab(145);
    if ((TTT >= 0 && TTT < 3) && (ww == 97))
      n = vtab(146);
  }

  symbol(gl, n, xypos, 0.8 * scale, align_right);
}

void ObsPlot::pastWeather(DiGLPainter* gl, int w, QPointF xypos, float scale, bool /*align_right*/)
{
  // manual codes - code table 4561, automatic codes - code table 4531
  const int auto2man[10] = { 0, 4, 3, 4, 6, 5, 6, 7, 8, 9 };
  const int n_auto2man = boost::size(auto2man);
  const int l_auto2man = 10;

  if (w >= l_auto2man) {
    const int wa = w - l_auto2man;
    if (wa >= n_auto2man)
      return;

    w = auto2man[wa];
  }

  // code figures 0, 1 and 2 of the W  code table shall be considered to represent phenomena without significance.
  if (w < 3)
    return;

  symbol(gl, vtab(158 + w), xypos, scale);
}

void ObsPlot::wave(DiGLPainter* gl, const float& PwPw, const float& HwHw, QPointF xypos, bool align_right)
{
  QString cs;

  int pwpw = diutil::float2int(PwPw);
  int hwhw = diutil::float2int(HwHw * 2); // meters -> half meters
  if (pwpw >= 0 && pwpw < 100) {
    cs = QString::number(pwpw).rightJustified(2, '0');
  } else
    cs = "xx";
  cs += "/";
  if (hwhw >= 0 && hwhw < 100) {
    cs += QString::number(hwhw).rightJustified(2, '0');
  } else
    cs += "xx";

  printListString(gl, cs.toStdString(), xypos, align_right);
}

namespace {

bool readTableFile(const std::string& filename, std::vector<short>& table, int count)
{
  table.clear();
  table.reserve(count + 1);
  table.push_back(0); // dummy to get fortran indices; also prevents re-reading

  std::ifstream ifs(filename.c_str());
  if (!ifs.is_open()) {
    METLIBS_LOG_WARN("unable to open table file '" << filename << "'");
    return false;
  }
  std::string line;
  int i = 0;
  while (ifs.good()) {
    getline(ifs,line);
    if (!miutil::contains(line, "#")) {
      i++;
      if (i >= count)
        break;
      table.push_back(miutil::to_int(line));
    }
  }
  if (i < count) {
    METLIBS_LOG_WARN("table file '" << filename << "' has fewer (" << i <<
        ") data lines than expected (" << count << ")");
  }
  return true;
}

} // namespace

// static
bool ObsPlot::readTable(const ObsPlotType type, const std::string& itab_filename, const std::string& iptab_filename,
    std::vector<short>& ritab, std::vector<short>& riptab)
{
  //   Initialize ritab and riptab from file.
  METLIBS_LOG_SCOPE("type: " << type << " filename: " << itab_filename << " filename: " << iptab_filename);

  size_t psize;
  ritab.clear();
  riptab.clear();

  if (type == OPT_SYNOP || type == OPT_LIST)
    psize= 11320;
  else if (type == OPT_METAR)
    psize = 3072;
  else {
    METLIBS_LOG_WARN("request to read itab/iptab for unknown plot type " << type);
    return false; // table for unknown plot type
  }

  // indexing as in fortran code (start from element 1)

  const int ITAB = 380;
  if (!readTableFile(itab_filename, ritab, ITAB))
    return false;

  if (!readTableFile(iptab_filename, riptab, psize))
    return false;

  return true;
}

void ObsPlot::decodeSort(const std::string& sortStr)
{
  std::vector<std::string> vstr = miutil::split(sortStr, ";");
  int nvstr = vstr.size();
  sortcriteria.clear();

  for (int i = 0; i < nvstr; i++) {
    std::vector<std::string> vcrit = miutil::split(vstr[i], ",");
    if (vcrit.size() > 1 && miutil::to_lower(vcrit[1]) == "desc") {
      sortcriteria[vcrit[0]] = false;
    } else {
      sortcriteria[vcrit[0]] = true;
    }
  }
}

void ObsPlot::decodeCriteria(const std::string& critStr)
{
  std::vector<std::string> vstr = miutil::split(critStr, ";");
  int nvstr = vstr.size();
  for (int i = 0; i < nvstr; i++) {
    std::vector<std::string> vcrit = miutil::split(vstr[i], ",");
    if (vcrit.size() < 2)
      continue;
    std::string sep;
    Sign sign;
    std::string parameter;
    float limit = 0.0;
    if (miutil::contains(vcrit[0], ">=")) {
      sep = ">=";
      sign = more_than_or_equal_to;
    } else if (miutil::contains(vcrit[0], ">")) {
      sep = ">";
      sign = more_than;
    } else if (miutil::contains(vcrit[0], "<=")) {
      sep = "<=";
      sign = less_than_or_equal_to;
    } else if (miutil::contains(vcrit[0], "<")) {
      sep = "<";
      sign = less_than;
    } else if (miutil::contains(vcrit[0], "==")) {
      sep = "==";
      sign = equal_to_exact;
    } else if (miutil::contains(vcrit[0], "=")) {
      sep = "=";
      sign = equal_to;
    } else {
      sign = no_sign;
    }
    if (not sep.empty()) {
      std::vector<std::string> sstr = miutil::split(vcrit[0], sep);
      if (sstr.size() != 2)
        continue;
      parameter = sstr[0];
      limit = atof(sstr[1].c_str());
      if (!unit_ms && ObsDialogInfo::findPar(parameter).type == ObsDialogInfo::pt_knot) {
        limit = miutil::knots2ms(limit);
      }
    } else {
      parameter = vcrit[0];
    }

    if (miutil::to_lower(vcrit[1]) == "plot") {
      plotCriteria pc;
      pc.limit = limit;
      pc.sign = sign;
      pc.plot = true;
      plotcriteria[parameter].push_back(pc);
    } else if (vcrit.size() > 2 && miutil::to_lower(vcrit[2]) == "marker") {
      markerCriteria mc;
      mc.limit = limit;
      mc.sign = sign;
      mc.marker = vcrit[1];
      markercriteria[parameter].push_back(mc);
    } else if (vcrit.size() > 2 && miutil::to_lower(vcrit[2]) == "markersize") {
      markersizeCriteria mc;
      mc.limit = limit;
      mc.sign = sign;
      mc.size = atof(vcrit[1].c_str());
      markersizecriteria[parameter].push_back(mc);
    } else {
      Colour c(vcrit[1]);
      colourCriteria cc;
      cc.limit = limit;
      cc.sign = sign;
      cc.colour = c;
      if (vcrit.size() == 3 && miutil::to_lower(vcrit[2]) == "total") {
        totalcolourcriteria[parameter].push_back(cc);
      } else {
        colourcriteria[parameter].push_back(cc);
      }
    }
  }
}

void ObsPlot::checkColourCriteria(DiGLPainter* gl, const std::string& param, float value)
{
  //Special case : plotting the difference between the mslp-field and the observed PPPP
  if (mslp() && param == "PPPP_mslp") {
    if (value > 0) {
      gl->setColour(mslpColour2);
    } else if (value < 0) {
      gl->setColour(mslpColour1);
    }
    return;
  }

  Colour col = colour;

  std::map<std::string, std::vector<colourCriteria>>::iterator p = colourcriteria.find(param);
  if (p != colourcriteria.end()) {
    if (p->first == "RRR")
      adjustRRR(value);

    for (size_t i = 0; i < colourcriteria[param].size(); i++) {
      if (value == undef || p->second[i].match(value))
        col = p->second[i].colour;
    }
  }

  gl->setColour(col);
}

bool ObsPlot::getValueForCriteria(const ObsDataRef& dta, const std::string& param, float& value)
{
  value = 0;

  if (const float* itf = dta.get_float(param)) {
    value = *itf;
  } else if (const std::string* its = dta.get_string(param)) {
    value = miutil::to_float(*its);
  } else if (miutil::to_lower(param) != dta.dataType()) {
    return false;
  }
  return true;
}

void ObsPlot::adjustRRR(float& value)
{
  //RRR=-0.1 - Precipitation, but less than 0.1 mm (0.0)
  //RRR=0   - No precipitation (0.)
  //Change value to make criteria more sensible, > 0 is true if RRR=-0.1, but not if RRR=0.0
  if (value < 0.0) {
    value = 0.0;
  } else if (value < 0.1) {
    value = -1.0;
  }
}

bool ObsPlot::baseCriteria::match(float value) const
{
  const float delta = fabsf(value) * 0.01;
  return (sign == less_than && value < limit)
      || (sign == less_than_or_equal_to && value <= limit + delta)
      || (sign == more_than && value > limit)
      || (sign == more_than_or_equal_to && value >= limit - delta)
      || (sign == equal_to && std::abs(value - limit) < delta)
      || (sign == equal_to_exact && std::abs(value - limit) == 0)
      || (sign == no_sign);
}

bool ObsPlot::checkPlotCriteria(const ObsDataRef& dta)
{
  if (plotcriteria.empty())
    return true;

  bool doPlot = false;

  for (const auto& p : plotcriteria) {
    float value = 0;
    if (!getValueForCriteria(dta, p.first, value))
      continue;
    if (p.first == "RRR")
      adjustRRR(value);

    bool bplot = true;
    for (const plotCriteria& c : p.second)
      bplot &= c.match(value);

    doPlot |= bplot;
  }

  return doPlot;
}

void ObsPlot::checkTotalColourCriteria(DiGLPainter* gl, const ObsDataRef& dta)
{
  if (totalcolourcriteria.empty())
    return;

  for (const auto& p : totalcolourcriteria) {
    float value = 0;
    if (!getValueForCriteria(dta, p.first, value))
      continue;
    if (p.first == "RRR")
      adjustRRR(value);

    for (const colourCriteria& c : p.second) {
      if (c.match(value))
        colour = c.colour;
    }
  }

  gl->setColour(colour);
}

std::string ObsPlot::checkMarkerCriteria(const ObsDataRef& dta)
{
  std::string marker = image;

  for (const auto& p : markercriteria) {
    float value = 0;
    if (!getValueForCriteria(dta, p.first, value))
      continue;
    if (p.first == "RRR")
      adjustRRR(value);

    for (const markerCriteria& c : p.second) {
      if (c.match(value))
        marker = c.marker;
    }
  }

  return marker;
}

float ObsPlot::checkMarkersizeCriteria(const ObsDataRef& dta)
{
  float relSize = 1;

  for (const auto& p : markersizecriteria) {
    float value = 0;
    if (!getValueForCriteria(dta, p.first, value))
      continue;
    if (p.first == "RRR")
      adjustRRR(value);

    for (const markersizeCriteria& c : p.second) {
      if (c.match(value))
        relSize = c.size;
    }
  }

  return relSize * markerSize;
}

void ObsPlot::parameterDecode(const std::string& parameter, bool add)
{
  std::string par;
  if (parameter == "txtxtx" || parameter == "tntntn")
    par = "txtn";
  else if (parameter == "dd_ff" || parameter == "vind")
    par = "vind";
  else if (parameter == "kjtegn")
    par = "id";
  else if (parameter == "tid")
    par = "time";
  else if (parameter == "dato")
    par = "date";
  if (!par.empty()) {
    pFlag[par] = add; // these are already lower case
  } else {
    pFlag[parameter] = add;
    pFlag[miutil::to_lower(parameter)] = add;
  }
}
