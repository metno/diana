/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2016 met.no

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diObsPlot.h"
#include "diRoadObsPlot.h"

#include "diObsPositions.h"
#include "diImageGallery.h"
#include "diGlUtilities.h"
#include "diLocalSetupParser.h"
#include "diKVListPlotCommand.h"
#include "diUtilities.h"
#include "miSetupParser.h"
#include "util/math_util.h"
#include "util/qstring_util.h"

#include <puCtools/stat.h>
#include <puTools/miStringFunctions.h>

#include <QPolygonF>
#include <QString>
#include <QTextCodec>

#include <fstream>
#include <iomanip>
#include <sstream>

#define MILOGGER_CATEGORY "diana.ObsPlot"
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;

map<std::string, vector<std::string> > ObsPlot::visibleStations;
map<std::string, ObsPlot::metarww> ObsPlot::metarMap;
map<int, int> ObsPlot::lwwg2;

std::string ObsPlot::currentPriorityFile = "";
vector<std::string> ObsPlot::priorityList;
vector<short> ObsPlot::itabSynop;
vector<short> ObsPlot::iptabSynop;
vector<short> ObsPlot::itabMetar;
vector<short> ObsPlot::iptabMetar;

static const int undef = -32767; //should be defined elsewhere

namespace /*anonymous*/ {
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

ObsPlot::ObsPlot(const miutil::KeyValue_v& pin, ObsPlotType plottype)
  : m_plottype(plottype)
  , mTextCodec(0)
{
  setPlotInfo(pin);

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
  vertical_orientation = true;
  left_alignment = true;
  showpos = false;
  devfield.reset(0);
  moretimes = false;
  next = false;
  previous = false;
  thisObs = false;
  current = -1;
  firstplot = true;
  beendisabled = false;
  itab = 0;
  iptab = 0;
  onlypos = false;
  showOnlyPrioritized = false;
  flaginfo = false;
  parameterName = false;
  popupText = false;
  qualityFlag = false;
  wmoFlag = false;
  annotations = true;

  knotParameters.insert("ff");
  knotParameters.insert("ffk");
  knotParameters.insert("911ff");
  knotParameters.insert("fxfx");
  knotParameters.insert("fmfm");

  Parameter p;
  p.name = "pos";
  vparam.push_back(p);

  p.name = "dd";
  p.symbol = -1;
  p.precision =0;
  vparam.push_back(p);

  p.name = "ff";
  p.knotParam = true;
  p.precision =0;
  vparam.push_back(p);

  p.name = "TTT";
  p.knotParam = false;
  p.tempParam = true;
  p.symbol = -1;
  p.precision =1;
  vparam.push_back(p);

  p.name = "TdTdTd";
  p.symbol = -1;
  p.precision =1;
  vparam.push_back(p);

  p.name = "PPPP";
  p.tempParam = false;
  p.precision =1;
  vparam.push_back(p);

  p.name = "PPP_mslp";
  p.precision =1;
  vparam.push_back(p);

  p.name = "ppp";
  p.precision =1;
  vparam.push_back(p);

  p.name = "a";
  p.symbol = 201;
  vparam.push_back(p);

  p.name = "h";
  p.symbol = -1;
  p.precision =0;
  vparam.push_back(p);

  p.name = "VV";
  p.precision =0;
  vparam.push_back(p);

  p.name = "N";
  p.precision =0;
  vparam.push_back(p);

  p.name = "RRR";
  p.precision =1;
  vparam.push_back(p);

  p.name = "ww";
  p.symbol = 0;
  vparam.push_back(p);

  p.name = "W1";
  p.symbol = 0;
  vparam.push_back(p);

  p.name = "W2";
  p.symbol = 0;
  vparam.push_back(p);

  p.name = "Nh";
  p.symbol = -1;
  p.precision = 0;
  vparam.push_back(p);

  p.name = "Ch";
  p.symbol = 190;
  vparam.push_back(p);

  p.name = "Cm";
  p.symbol = 180;
  vparam.push_back(p);

  p.name = "Cl";
  p.symbol = 170;
  vparam.push_back(p);

  p.name = "vs";
  p.symbol = 170;
  vparam.push_back(p);

  p.name = "ds";
  p.symbol = 170;
  vparam.push_back(p);

  p.name = "TwTwTw";
  p.tempParam = true;
  p.precision = 1;
  p.symbol = -1;
  vparam.push_back(p);

  p.name = "PwaHwa";
  p.tempParam = false;
  p.precision = 0;
  p.symbol = 0;
  vparam.push_back(p);

  p.name = "dw1dw1";
  p.symbol = 0;
  vparam.push_back(p);

  p.name = "Pw1Hw1";
  p.symbol = 0;
  vparam.push_back(p);

  p.name = "TxTn";
  p.tempParam = true;
  p.precision = 1;
  p.symbol = -1;
  vparam.push_back(p);

  p.name = "sss";
  p.tempParam = false;
  p.precision = 0;
  vparam.push_back(p);

  p.name = "911ff";
  p.knotParam = true;
  vparam.push_back(p);

  p.name = "s";
  p.knotParam = false;
  vparam.push_back(p);

  p.name = "fxfx";
  p.knotParam = true;
  vparam.push_back(p);

  p.name = "Id";
  p.tempParam = false;
  p.knotParam = false;
  vparam.push_back(p);

  p.name = "T_red";
  p.tempParam = true;
  vparam.push_back(p);

  p.name = "Date";
  vparam.push_back(p);

  p.name = "Time";
  vparam.push_back(p);

  p.name = "HHH";
  vparam.push_back(p);

  p.name = "Height";
  vparam.push_back(p);

  p.name = "Zone";
  vparam.push_back(p);

  p.name = "Name";
  vparam.push_back(p);

  p.name = "RRR_1";
  vparam.push_back(p);

  p.name = "RRR_6";
  vparam.push_back(p);

  p.name = "RRR_12";
  vparam.push_back(p);

  p.name = "RRR_24";
  vparam.push_back(p);

  p.name = "depth";
  p.precision =0;
  vparam.push_back(p);

  p.name = "TTTT";
  p.precision =2;
  vparam.push_back(p);

  p.name = "SSSS";
  vparam.push_back(p);

  p.name = "TE";
  vparam.push_back(p);

  p.name = "QI";
  vparam.push_back(p);

  p.name = "QI_NM";
  vparam.push_back(p);

  p.name = "QI_RFF";
  vparam.push_back(p);

  p.name = "quality";
  p.precision =0;
  vparam.push_back(p);
}

ObsPlot::~ObsPlot()
{
  METLIBS_LOG_SCOPE();
  delete[] x;
  delete[] y;
}

QString ObsPlot::decodeText(const std::string& text) const
{
  if (mTextCodec != 0)
    return mTextCodec->toUnicode(text.c_str());
  else
    return QString::fromLatin1(text.c_str());
}

void ObsPlot::setTextCodec(QTextCodec* codec)
{
  mTextCodec = codec;
}

void ObsPlot::setTextCodec(const char* codecName)
{
  setTextCodec(QTextCodec::codecForName(codecName));
}

void ObsPlot::getAnnotation(string &str, Colour &col) const
{
  //Append to number of plots to the annotation string
  if (not annotation.empty()) {
    string anno_str = (" ( " + miutil::from_number(numVisiblePositions())
    + " / " + miutil::from_number(numPositions()) + " )");
    str = annotation + anno_str;
  } else
    str = annotation;
  col = origcolour;
}

bool ObsPlot::getDataAnnotations(vector<string>& anno)
{
  METLIBS_LOG_SCOPE();

  if (!isEnabled() || !annotations || numPositions() == 0 || current < 0)
    return false;

  const std::string vectorAnnotationSize = miutil::from_number(21 * getStaticPlot()->getPhysToMapScaleX());
  const std::string vectorAnnotationText = miutil::from_number(2.5f * current, 2) + "m/s";
  for (const string& a : anno) {
    if (miutil::contains(a, "arrow")) {
      if (miutil::contains(a, "arrow="))
        continue;

      std::string endString;
      std::string startString;
      if (miutil::contains(a, ",")) {
        size_t nn = a.find_first_of(",");
        endString = a.substr(nn);
        startString = a.substr(0, nn);
      } else {
        startString = a;
      }

      std::string str = "arrow=" + vectorAnnotationSize
          + ",feather=true,tcolour=" + colour.Name() + endString;
      anno.push_back(str);

      str = "text=\"" + vectorAnnotationText + "\"" /* no spaces, but maybe ',' from the float ? */
          + ",tcolour=" + colour.Name() + endString;
      anno.push_back(str);
    }
  }
  return true;
}

const PlotCommand_cpv ObsPlot::getObsExtraAnnotations() const
{
  if ( isEnabled() && annotations ) {
    return labels;
  }
  return PlotCommand_cpv();
}

ObsData& ObsPlot::getNextObs()
{
  // METLIBS_LOG_SCOPE();
  ObsData d;
  d.dataType = currentDatatype;
  obsp.push_back(d);
  return obsp.back();
}

void ObsPlot::mergeMetaData(const std::map<std::string, ObsData>& metaData)
{
  //METLIBS_LOG_DEBUG(__FUNCTION__<<" : "<<obsp.size()<<" : "<<metaData.size());
  for (size_t i = 0; i < obsp.size(); ++i) {
    const std::map<std::string, ObsData>::const_iterator itM = metaData.find(obsp[i].id);
    if (itM != metaData.end()) {
      obsp[i].xpos = itM->second.xpos;
      obsp[i].ypos = itM->second.ypos;
    }
  }
}

void ObsPlot::addObsData(const std::vector<ObsData>& obs)
{
  obsp.insert(obsp.end(), obs.begin(), obs.end());
}

void ObsPlot::replaceObsData(std::vector<ObsData>& obs)
{
  // best performance ?
  int new_size = obsp.size() - obs.size();
  if (new_size < 0)
    new_size = 0;
  resetObs(new_size);
  addObsData(obs);
}

void ObsPlot::updateLevel(const std::string& dataType)
{
  METLIBS_LOG_SCOPE("dataType: " << dataType);
  if (level < -1) {
    return; //no levels
  }

  if (levelAsField) //from field
    level = getStaticPlot()->getVerticalLevel();
  if (level == -1) { //set default
    if (dataType == "aireps")
      level = 500;
    else if (dataType == "ocea")
      level = 0;
    else
      level = 1000;
  }
}

int ObsPlot::numVisiblePositions() const
{
  METLIBS_LOG_SCOPE();

  int npos = numPositions();
  int count = 0;
  for (int i = 0; i < npos; i++) {
    if (getStaticPlot()->getMapSize().isinside(x[i], y[i]))
      count++;
  }
  return count;
}

int ObsPlot::getObsCount() const
{
  return obsp.size();
}

long ObsPlot::findModificationTime(const std::string& fname)
{
  pu_struct_stat buf;
  if (pu_stat(fname.c_str(), &buf) == 0) {
    return buf.st_ctime; // FIXME using ctime here and mtime below
  } else {
    return 0;
  }
}

void ObsPlot::setModificationTime(const std::string& fname)
{
  METLIBS_LOG_SCOPE("fname: " << fname);
  fileNames.push_back(fname);
  modificationTime.push_back(findModificationTime(fname));
}

bool ObsPlot::isFileUpdated(const std::string& fname, long now, long mod_time)
{
  pu_struct_stat buf;
  if (pu_stat(fname.c_str(), &buf) != 0)
    return true;
  if (mod_time != (long) buf.st_mtime) // FIXME using mtime here and ctime above
    return true;
  return false;
}

bool ObsPlot::updateObs()
{
  METLIBS_LOG_SCOPE();

  const long now = time(0);
  for (size_t i = 0; i < fileNames.size(); i++) {
    if (isFileUpdated(fileNames[i], now, modificationTime[i]))
      return true;
  }

  return false; // no update needed
}

// static
ObsPlot* ObsPlot::createObsPlot(const PlotCommand_cp& pc)
{
  METLIBS_LOG_SCOPE();

  KVListPlotCommand_cp cmd = std::dynamic_pointer_cast<const KVListPlotCommand>(pc);
  if (!cmd)
    return 0;

  ObsPlotType plottype = OPT_SYNOP;
  std::string dialogname;
  bool flaginfo = false;
  const size_t i_plot = cmd->rfind("plot");
  if (i_plot != KVListPlotCommand::npos && !cmd->value(i_plot).empty()) {
    const std::vector<std::string> vstr = miutil::split(cmd->value(i_plot), ":");
    if (!vstr.empty())
      dialogname = vstr[0];
    if (dialogname.empty())
      METLIBS_LOG_WARN("probably malformed observation plot specification '" << cmd->value(i_plot) << "'");
    const std::string valp = miutil::to_lower(dialogname);
    if (valp == "pressure"|| valp == "list"
        || valp == "tide" || valp == "ocean")
    {
      plottype = OPT_LIST;
    } else if (valp == "synop") {
      plottype = OPT_SYNOP;
    } else if (valp == "metar") {
      plottype = OPT_METAR;
    } else if (valp == "hqc_synop") {
      plottype = OPT_SYNOP;
      flaginfo = true;
    } else if (valp == "hqc_list") {
      plottype = OPT_ASCII;
      flaginfo = true;
    }
#ifdef ROADOBS
    // To avoid that roadobs will be set to ascii below
    else if (valp == "synop_wmo" || valp == "synop_ship" || valp == "metar_icao" || valp == "roadobs") {
      plottype = OPT_ROADOBS;
    }
#endif // !ROADOBS
    else {
      plottype = OPT_ASCII;
    }
  }

#ifdef ROADOBS
  std::unique_ptr<ObsPlot> op(new RoadObsPlot(cmd->all(), plottype));
#else  // !ROADOBS
  std::unique_ptr<ObsPlot> op(new ObsPlot(cmd->all(), plottype));
#endif // !ROADOBS

  op->dialogname = dialogname;
  op->flaginfo = flaginfo;

  op->poptions.fontname = "BITMAPFONT";
  op->poptions.fontface = "normal";

  vector<std::string> parameter;
  for (const miutil::KeyValue& kv : cmd->all()) {
    const std::string& key = kv.key();
    const std::string& orig_value = kv.value();
    const std::string value = miutil::to_lower(orig_value);

    if (key == "plot") {
      continue;
    } else if (key == "data") {
      op->datatypes = miutil::split(value, ",");
    } else if (key == "parameter") {
      parameter = miutil::split(orig_value, 0, ",");
    } else if (key == "scale") {
      op->textSize = kv.toFloat();
    if (op->markerSize < 0)
        op->markerSize = kv.toFloat();
    } else if (key == "marker.size") {
      op->markerSize = kv.toFloat();
    } else if (key == "text.size") {
      op->textSize = kv.toFloat();
    } else if (key == "density") {
      if (value == "allobs")
        op->allObs = true;
      else
        op->density = kv.toFloat();
    } else if (key == "priority") {
      op->priorityFile = orig_value;
      op->priority = true;
    } else if (key == "colour") {
      op->origcolour = Colour(orig_value);
    } else if (key == "devfield") {
      if (kv.toBool()) {
        op->devfield.reset(new ObsPositions);
      } else {
        op->devfield.reset(0);
      }
    } else if (key == "devcolour1") {
      op->mslpColour1 = Colour(orig_value);
    } else if (key == "devcolour2") {
      op->mslpColour2 = Colour(orig_value);
    } else if (key == "tempprecision") {
      op->tempPrecision = kv.toBool();
    } else if (key == "unit_ms") {
      op->unit_ms = kv.toBool();
    } else if (key == "parametername") {
      op->parameterName = kv.toBool();
    } else if (key == "popup") {
      op->popupText = kv.toBool();
    } else if (key == "qualityflag") {
      op->qualityFlag = kv.toBool();
    } else if (key == "wmoflag") {
      op->wmoFlag = kv.toBool();
    } else if (key == "moretimes") {
      op->moretimes = kv.toBool();
    } else if (key == "sort") {
      op->decodeSort(orig_value);
      //     } else if (key == "allairepslevels") {
      //        allAirepsLevels = kv.toBool();
    } else if (key == "timediff")
      if (value == "alltimes")
        op->timeDiff = -1;
      else
        op->timeDiff = kv.toInt();
    else if (key == "level") {
      if (value == "asfield") {
        op->levelAsField = true;
        op->level = -1;
      } else
        op->level = kv.toInt();
      //      } else if (key == "leveldiff") {
      //        leveldiff = kv.toInt();
    } else if (key == "onlypos") {
      op->onlypos = true;
    } else if (key == "showonlyprioritized") {
      op->showOnlyPrioritized = true;
    } else if (key == "image") {
      op->image = orig_value;
    } else if (key == "showpos") {
     op->showpos = true;
    } else if (key == "orientation") {
      if (value == "horizontal")
        op->vertical_orientation = false;
    } else if (key == "alignment") {
      if (value == "right")
        op->left_alignment = false;
    } else if (key == "criteria") {
      op->decodeCriteria(orig_value);
    } else if (key == "arrowstyle") {
      if (value == "wind")
        op->poptions.arrowstyle = arrow_wind;
      else if (value == "wind_arrow")
        op->poptions.arrowstyle = arrow_wind_arrow;
    } else if (key == "annotations") {
      op->annotations = kv.toBool();
    } else if (key == "font") {
      op->poptions.fontname = orig_value;
    } else if (key == "face") {
      op->poptions.fontface = orig_value;
    }
  }

  if (op->markerSize < 0)
    op->markerSize = op->textSize;
  op->parameterDecode("all", false);
  op->numPar = parameter.size();
  for (int i = 0; i < op->numPar; i++) {
    op->parameterDecode(parameter[i]);
  }
  if (op->mslp())
    op->pFlag["pppp_mslp"] = true;

  // static tables, read once

  std::string path = LocalSetupParser::basicValue("obsplotfilepath");

  if (op->isSynopListRoad()) {
    if (itabSynop.empty() || iptabSynop.empty()) {
      if (!readTable(op->plottype(), path + "/itab_synop.txt", path + "/iptab_synop.txt", itabSynop, iptabSynop))
        return 0;
    }
    op->itab = &itabSynop;
    op->iptab = &iptabSynop;

  } else if (op->plottype() == OPT_METAR) {
    if (itabMetar.empty() || iptabMetar.empty()) {
      if (!readTable(op->plottype(), path + "/itab_metar.txt", path + "/iptab_metar.txt", itabMetar, iptabMetar))
        return 0;
    }
    op->itab = &itabMetar;
    op->iptab = &iptabMetar;
  }

  return op.release();
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

  const int numObs = numPositions();
  devfield->resize(numObs);
  for (int i = 0; i < numObs; i++) {
    devfield->xpos[i] = x[i];
    devfield->ypos[i] = y[i];
  }
  devfield->convertToGrid = true;
  devfield->obsArea = getStaticPlot()->getMapArea();
}

bool ObsPlot::setData()
{
  METLIBS_LOG_SCOPE();

  firstplot = true;
  nextplot.clear();
  notplot.clear();
  list_plotnr.clear();

  delete[] x;
  delete[] y;
  x = 0;
  y = 0;

  int numObs = numPositions();

  if (numObs < 1) {
    METLIBS_LOG_INFO("no data");
    return false;
  }

  x = new float[numObs];
  y = new float[numObs];

  for (int i = 0; i < numObs; i++)
    getObsLonLat(i, x[i], y[i]);

  // convert points to correct projection
  getStaticPlot()->GeoToMap(numObs, x, y);

  updateObsPositions();

  // find direction of north for each observation
  float *u = new float[numObs];
  float *v = new float[numObs];

  for (int i = 0; i < numObs; i++) {
    u[i] = 0;
    v[i] = 10;
  }

  getStaticPlot()->GeoToMap(numObs, x, y, u, v);

  for (int i = 0; i < numObs; i++) {
    const int angle = (int) (atan2f(u[i], v[i]) * 180 / M_PI);
    ObsData::fdata_t::const_iterator itc;
    if ((itc = obsp[i].fdata.find("dd")) != obsp[i].fdata.end()) {
      float dd = itc->second;
      if (dd > 0 and dd <= 360) {
        dd = normalize_angle(dd + angle);
        obsp[i].fdata["dd_adjusted"] = dd;
      }
    }
    ObsData::fdata_t::iterator it;
    if ((it = obsp[i].fdata.find("dw1dw1")) != obsp[i].fdata.end()) {
      float& dd = it->second;
      dd = normalize_angle(dd + angle);
    }
    // FIXME: stringdata ?
    if ((it = obsp[i].fdata.find("ds")) != obsp[i].fdata.end()) {
      float& dd = it->second;
      dd = normalize_angle(dd + angle);
    }
    ObsData::stringdata_t::iterator its;
    if ((its = obsp[i].stringdata.find("ds")) != obsp[i].stringdata.end()) {
      float tmp_dd = miutil::to_float(its->second);
      float& dd = tmp_dd;
      dd = normalize_angle(dd + angle);
    }
  }

  delete[] u;
  delete[] v;

  updateDeltaTimes();

  //sort stations according to priority file
  priority_sort();
  //put stations plotted last time on top of list
  readStations();
  //sort stations according to time
  if (moretimes)
    time_sort();

  // sort according to parameter
  for (std::map<string, bool>::iterator iter = sortcriteria.begin();
      iter != sortcriteria.end(); ++iter)
    parameter_sort(iter->first, iter->second);

  if (plottype() == OPT_METAR && metarMap.size() == 0)
    initMetarMap();

  return true;
}

void ObsPlot::getObsLonLat(int obsidx, float& x, float& y)
{
  x = obsp[obsidx].xpos;
  y = obsp[obsidx].ypos;
}

bool ObsPlot::timeOK(const miTime& t) const
{
  // called very often METLIBS_LOG_SCOPE("time: " << t.isoTime());

  return (timeDiff < 0 || abs(miTime::minDiff(t, Time)) < timeDiff + 1);
}

std::vector<int> & ObsPlot::getStationsToPlot()
{
  // Returns the visible stations
  return stations_to_plot;
}

// clear VisibleStations map from current dialogname
void ObsPlot::clearVisibleStations()
{
  visibleStations[dialogname].clear();
}

void ObsPlot::logStations()
{
  METLIBS_LOG_SCOPE();
  int n = nextplot.size();
  if (n) {
    visibleStations[dialogname].clear();
    for (int i = 0; i < n; i++) {
      visibleStations[dialogname].push_back(obsp[nextplot[i]].id);
    }
  }
}

void ObsPlot::readStations()
{
  METLIBS_LOG_SCOPE();

  int n = visibleStations[dialogname].size();
  if (n > 0) {

    vector<int> tmpList = all_from_file;
    all_stations.clear();

    // Fill the stations from stat into priority_vector,
    // and mark them in tmpList
    int i, j;
    int numObs = obsp.size();
    for (int k = 0; k < numObs; k++) {
      i = all_from_file[k];
      j = 0;
      while (j < n && visibleStations[dialogname][j] != obsp[i].id)
        j++;
      if (j < n) {
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
  obsp.clear();
  annotation.clear();
  setPlotName("");
  labels.clear();

  fileNames.clear();
  modificationTime.clear();
}

void ObsPlot::priority_sort()
{
  METLIBS_LOG_SCOPE();

  //sort the observations according to priority list
  int numObs = numPositions();

  //  METLIBS_LOG_DEBUG("Priority_sort:"<<numObs);
  int i;

  all_from_file.resize(numObs);

  // AF: synop: put automatic stations after other types (fixed,ship)
  //     (how to detect other obs. types, temp,aireps,... ???????)
  // FIXME: ix from database ?
  vector<int> automat;
  int n = 0;
  for (i = 0; i < numObs; i++) {
    if ((obsp[i].fdata.count("ix") && obsp[i].fdata["ix"] < 4)
        || ( obsp[i].fdata.count("auto") && obsp[i].fdata["auto"] != 0))
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

      vector<int> tmpList = all_from_file;
      all_from_file.clear();

      // Fill the stations from priority list into all_from_file,
      // and mark them in tmpList
      int j, n = priorityList.size();
      for (j = 0; j < n; j++) {
        i = 0;
        while (i < numObs && obsp[i].id != priorityList[j])
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

  vector<int> diff(numObs);
  multimap<int, int> sortmap1;
  multimap<int, int> sortmap2;

  // Data from obs-files or database
  // find mindiff = abs(obsTime-plotTime) for all observations
  for (int i = 0; i < numObs; i++)
    diff[i] = abs(miTime::minDiff(obsp[i].obsTime, Time));

  //Sorting ...
  for (int i = 0; i < numObs; i++) {
    index = all_stations[i];
    sortmap1.insert(pair<int, int>(diff[index], index));
  }
  for (int i = 0; i < numObs; i++) {
    index = all_from_file[i];
    sortmap2.insert(pair<int, int>(diff[index], index));
  }

  multimap<int, int>::iterator p = sortmap1.begin();
  for (int i = 0; i < numObs; i++, p++)
    all_stations[i] = p->second;

  multimap<int, int>::iterator q = sortmap2.begin();
  for (int i = 0; i < numObs; i++, q++)
    all_from_file[i] = q->second;
}

void ObsPlot::parameter_sort(const std::string& parameter, bool minValue)
{
  METLIBS_LOG_SCOPE();

  //sort the observations according to priority list
  if (obsp.empty())
    return;

  vector<int> tmpFileList = all_from_file;
  vector<int> tmpStnList = all_stations;

  multimap<string, int> stringFileSortmap;
  multimap<string, int> stringStnSortmap;
  multimap<double, int> numFileSortmap;
  multimap<double, int> numStnSortmap;

  // Fill the observation with parameter
  // and mark them in tmpList
  int index;
  for (size_t i = 0; i < tmpFileList.size(); i++) {
    index = all_from_file[i];
    if (obsp[index].stringdata.count(parameter)) {
      string stringkey = obsp[index].stringdata[parameter];
      if (miutil::is_number(stringkey)) {
        double numberkey = miutil::to_double(stringkey);
        numFileSortmap.insert(pair<double, int>(numberkey, index));
      } else {
        stringFileSortmap.insert(pair<string, int>(stringkey, index));
      }
      tmpFileList[i] = -1;

    }
  }

  for (size_t i = 0; i < tmpStnList.size(); i++) {
    index = all_stations[i];
    if (obsp[index].stringdata.count(parameter)) {
      string stringkey = obsp[index].stringdata[parameter];
      if (miutil::is_number(stringkey)) {
        double numberkey = miutil::to_double(stringkey);
        numStnSortmap.insert(pair<double, int>(numberkey, index));
      } else {
        stringStnSortmap.insert(pair<string, int>(stringkey, index));
      }
      tmpStnList[i] = -1;

    }
  }

  all_from_file.clear();
  all_stations.clear();
  if (!minValue) {
    for (multimap<double, int>::iterator p = numFileSortmap.begin();
        p != numFileSortmap.end(); p++) {
      all_from_file.push_back(p->second);
    }
    for (multimap<string, int>::iterator p = stringFileSortmap.begin();
        p != stringFileSortmap.end(); p++) {
      all_from_file.push_back(p->second);
    }

    for (multimap<double, int>::iterator p = numStnSortmap.begin();
        p != numStnSortmap.end(); p++) {
      all_stations.push_back(p->second);
    }
    for (multimap<string, int>::iterator p = stringStnSortmap.begin();
        p != stringStnSortmap.end(); p++) {
      all_stations.push_back(p->second);
    }
  } else {
    for (multimap<string, int>::reverse_iterator p = stringFileSortmap.rbegin();
        p != stringFileSortmap.rend(); p++) {
      all_from_file.push_back(p->second);
    }
    for (multimap<double, int>::reverse_iterator p = numFileSortmap.rbegin();
        p != numFileSortmap.rend(); p++) {
      all_from_file.push_back(p->second);
    }
    for (multimap<string, int>::reverse_iterator p = stringStnSortmap.rbegin();
        p != stringStnSortmap.rend(); p++) {
      all_stations.push_back(p->second);
    }
    for (multimap<double, int>::reverse_iterator p = numStnSortmap.rbegin();
        p != numStnSortmap.rend(); p++) {
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

  ifstream inFile;
  std::string line;

  inFile.open(filename.c_str(), ios::in);
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
      ObsData::fdata_t& fdatai = obsp[i].fdata;
      ObsData::fdata_t::const_iterator itPPPP = fdatai.find("PPPP");
      if (itPPPP != fdatai.end()) {
        const float ief = devfield->interpolatedEditField[i];
        if (ief < 0.9e+35)
          fdatai["PPPP_mslp"] = itPPPP->second - ief;
      }
    }
  }
}

//***********************************************************************

int ObsPlot::findObs(int xx, int yy, const std::string& type)
{
  METLIBS_LOG_SCOPE();
  using diutil::square;

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
  vector<int>::iterator min_p = notplot.begin()+min_i;
  nextplot.insert(nextplot.begin(), notplot[min_i]);
  notplot.erase(min_p);
  thisObs = true;
  return true;
}

void ObsPlot::setPopupSpec(std::vector<std::string>& txt)
{
  std::string datatype="";
  vector< std::string> data;

  for (size_t j = 0; j < txt.size(); j++) {
    if (miutil::contains(txt[j], "datatype")) {
      if (!datatype.empty() && data.size() > 0) {
        popupSpec[datatype] = data;
        data.clear();
      }
      vector<std::string> token = miutil::split(txt[j], "=");
      if (token.size() == 2 ){
        datatype = miutil::to_lower(token[1]);
      }
    } else {
      data.push_back(txt[j]);
      if (j==txt.size()-1) {
        if (!datatype.empty()) {
          popupSpec[datatype]=data;
          data.clear();
        }
      }
    }
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
  const ObsData& dt = obsp[min_i];
  for (const std::string& datatype : datatypes) {
    std::map<string, vector<std::string> >::const_iterator f_p = popupSpec.find(datatype);
    if (f_p == popupSpec.end())
      continue;

    found = true;
    const std::vector<std::string>& mdata = f_p->second;
    if (mdata.empty())
      continue;

    setuptext += "<table>";
    if (!dt.obsTime.undef()) {
      setuptext += "<tr>";
      setuptext += "<td>";
      setuptext += "<span style=\"background: red; color: red\">X</span>";
      setuptext += dt.obsTime.isoTime();
      setuptext += " ";
    }
    for (const std::string& mdat : mdata) {
      setuptext += "<tr>";
      setuptext += "<td>";
      setuptext += "<span style=\"background: red; color: red\">X</span>";
      vector<std::string> token = miutil::split(mdat, ' ');
      for (size_t j = 0; j < token.size(); j++) {
        if (miutil::contains(token[j], "$")) {
          miutil::trim(token[j]);
          miutil::remove(token[j], '$');
          if (miutil::to_int(dt.get_string(token[j])) != undef)
            setuptext += dt.get_string(token[j]);
          else
            setuptext += "X";
          setuptext += " ";
        } else if (miutil::contains(token[j], ":")) {
          vector<std::string> values = miutil::split(token[j], ":");
          for (size_t i = 0; i < values.size(); i++) {
            if (miutil::contains(values[i], "=")) {
              vector<std::string> keys = miutil::split(values[i], "=");
              if (keys.size() == 2) {
                if (dt.get_string(token[0]) == keys[0])
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
    } //end of for mdata
    setuptext += "</table>";
  } // end of datatypes

  if (!found) {
    setuptext += "<table>";
    int size = columnName.size();
    for (int i = 0; i < size; i++) {
      const std::string& param = columnName[i];
      if ( pFlag.count( param ) ) {
        setuptext += "<tr>";
        setuptext += "<td>";
        setuptext += param;
        setuptext += "</td>";
        setuptext += "<td>";
        setuptext += "  ";
        if (miutil::to_int(dt.get_string(param)) != undef )
          setuptext += dt.get_string(param);
        else
          setuptext += "X";
        setuptext += " ";
        setuptext += "</td>";
        setuptext += "</tr>";
        setuptext += "</table>";
      }
    }
  }
  return !setuptext.empty();
}

//***********************************************************************
bool ObsPlot::getObsName(int xx, int yy, string& name)
{
  METLIBS_LOG_SCOPE("xx: " << " yy: " << yy);

  int min_i = -1;

  if (onlypos) {
    min_i =  findObs(xx,yy);
    if (min_i < 0)
      return false;
  } else {
    min_i =  findObs(xx,yy,"nextplot");
    if (min_i < 0)
      return false;
    min_i = nextplot[min_i];
  }
  name = obsp[min_i].id;

  static std::string lastName;
  if (name == lastName)
    return false;

  lastName = name;

  selectedStation = name;
  return true;
}

//***********************************************************************

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

bool ObsPlot::isSynopListRoad() const
{
  return plottype() == OPT_SYNOP || plottype() == OPT_LIST
      || plottype() == OPT_ROADOBS;
}

bool ObsPlot::isSynopMetarRoad() const
{
  return plottype() == OPT_SYNOP || plottype() == OPT_METAR
      || plottype() == OPT_ROADOBS;
}

int ObsPlot::calcNum() const
{
  int num = numPar;
  if (pFlag.count("wind"))
    num--;
  // I think we should check for roadobsWind here also
  // OBS!******************************************
  if (plottype() != OPT_ASCII && plottype() != OPT_ROADOBS) {
    if (pFlag.count("pos"))
      num++;
  }
  return num;
}

static float circle_radius = 7;

void ObsPlot::drawCircle(DiGLPainter* gl)
{
  if (plottype() == OPT_LIST || plottype() == OPT_ASCII) {
    const float d = circle_radius * 0.25;
    gl->drawRect(false, -d, -d, d, d);
  } else {
    gl->drawCircle(false, 0, 0, circle_radius);
  }
}

void ObsPlot::plot(DiGLPainter* gl, PlotOrder zorder)
{
  METLIBS_LOG_SCOPE();

  if (zorder != LINES && zorder != OVERLAY)
    return;

  if (!isEnabled()) {
    // make sure plot-densities etc are recalc. next time
    if (getStaticPlot()->getDirty())
      beendisabled = true;
    return;
  }

  int numObs = numPositions();
  if (numObs == 0)
    return;

  // Update observation delta time before checkPlotCriteria
  if (updateDeltaTimes())
    getStaticPlot()->setDirty(true);

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
    ImageGallery ig;
    ig.plotImages(gl, getStaticPlot(), numObs, image, x, y, true, markerSize);
    return;
  }

  int num = calcNum();
  float xdist = 0, ydist = 0;
  // I think we should plot roadobs like synop here
  // OBS!******************************************

  if (isSynopMetarRoad()) {
    xdist = 100 * scale / density;
    ydist = 90 * scale / density;
  } else if (plottype() == OPT_LIST || plottype() == OPT_ASCII) {
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
  vector<int> ptmp;
  vector<int>::iterator p, pbegin, pend;

  if (getStaticPlot()->getDirty() || firstplot || beendisabled) { //new area

    //init of areaFreeSetup
    // I think we should plot roadobs like synop here
    // OBS!******************************************
    if (plottype() == OPT_LIST || plottype() == OPT_ASCII) {
      float w, h;
      gl->getTextSize("0", w, h);
      float space = w * 0.5;
      areaFreeSetup(scale, space, num, xdist, ydist);
    }

    thisObs = false;

    // new area, find stations inside current area
    all_this_area.clear();
    int nn = all_stations.size();
    for (int j = 0; j < nn; j++) {
      int i = all_stations[j];
      if (getStaticPlot()->getMapSize().isinside(x[i], y[i])) {
        all_this_area.push_back(i);
      }
    }

    //    METLIBS_LOG_DEBUG("all this area:"<<all_this_area.size());
    // plot the observations from last plot if possible,
    // then the rest if possible

    if (!firstplot) {
      vector<int> a, b;
      int n = list_plotnr.size();
      if (n == numObs) {
        int psize = all_this_area.size();
        for (int j = 0; j < psize; j++) {
          int i = all_this_area[j];
          if (list_plotnr[i] == plotnr)
            a.push_back(i);
          else
            b.push_back(i);
        }
        if (a.size() > 0) {
          all_this_area.clear();
          all_this_area.insert(all_this_area.end(), a.begin(), a.end());
          all_this_area.insert(all_this_area.end(), b.begin(), b.end());
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
    ptmp.insert(ptmp.end(), notplot.begin(), notplot.end());
    pbegin = ptmp.begin();
    pend = ptmp.end();

  } else if (plotnr > maxnr) {
    //    METLIBS_LOG_DEBUG("plotnr:"<<plotnr);
    // plot as many observations as possible which have not been plotted before
    maxnr++;
    plotnr = maxnr;

    int psize = all_this_area.size();
    for (int j = 0; j < psize; j++) {
      int i = all_this_area[j];
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
      for (int j = 0; j < numObs; j++) {
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
    int psize = all_this_area.size();
    notplot.clear();
    nextplot.clear();
    for (int j = 0; j < psize; j++) {
      int i = all_this_area[j];
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
    if (plottype() == OPT_LIST || plottype() == OPT_ASCII) {
      for (p = pbegin; p != pend; p++) {
        int i = *p;
        if (allObs || areaFree(i)) {
          //Select parameter with correct accumulation/max value interval
          if (pFlag.count("911ff")) {
            checkGustTime(obsp[i]);
          }
          if (pFlag.count("rrr")) {
            checkAccumulationTime(obsp[i]);
          }
          if (pFlag.count("fxfx")) {
            checkMaxWindTime(obsp[i]);
          }
          if (checkPlotCriteria(i)) {
            nextplot.push_back(i);
            list_plotnr[i] = plotnr;
          } else {
            list_plotnr[i] = -2;
            collider_->areaPop();
          }
        } else {
          notplot.push_back(i);
        }
      }
    } else {
      for (p = pbegin; p != pend; p++) {
        int i = *p;
        if (allObs || collider_->positionFree(x[i], y[i], xdist, ydist)) {
          //Select parameter with correct accumulation/max value interval
          if (plottype() != OPT_ROADOBS) {
            if (pFlag.count("911ff")) {
              checkGustTime(obsp[i]);
            }
            if (pFlag.count("rrr")) {
              checkAccumulationTime(obsp[i]);
            }
            if (pFlag.count("fxfx")) {
              checkMaxWindTime(obsp[i]);
            }
          }
          if (checkPlotCriteria(i)) {
            nextplot.push_back(i);
            list_plotnr[i] = plotnr;
          } else {
            list_plotnr[i] = -2;
            if (!allObs)
              collider_->positionPop();
          }
        } else {
          notplot.push_back(i);
        }
      }
    }
    if (thisObs) {
      int n = notplot.size();
      for (int i = 0; i < n; i++)
        if (list_plotnr[notplot[i]] == plotnr)
          list_plotnr[notplot[i]] = -1;
    }
  }

  //PLOTTING

  if (showpos) {
    Colour col("red");
    if (col == colour)
      col = Colour("blue");
    col = getStaticPlot()->notBackgroundColour(col);
    gl->setColour(col);
    int m = notplot.size();
    float d = 4.5 * scale;
    for (int i = 0; i < m; i++) {
      int j = notplot[i];
      gl->drawCross(x[j], y[j], d, true);
    }
    gl->setColour(origcolour);
  }

  const size_t n = nextplot.size();
  for (size_t i = 0; i < n; i++) {
    plotIndex(gl, nextplot[i]);
  }

  //reset

  next = false;
  previous = false;
  thisObs = false;
  if (nextplot.empty())
    plotnr = -1;
  origcolour = selectedColour; // reset in case a background contrast colour was used
  firstplot = false;
  beendisabled = false;
}

void ObsPlot::plotIndex(DiGLPainter* gl, int index)
{
  if (plottype() == OPT_SYNOP) {
    plotSynop(gl, index);
  } else if (plottype() == OPT_METAR) {
    plotMetar(gl, index);
  } else if (plottype() == OPT_LIST) {
    plotList(gl, index);
  } else if (plottype() == OPT_ASCII) {
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
    if (obsp[idx].fdata.count("dd")) {
      idd = (int) obsp[idx].fdata["dd"];
    }
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

void ObsPlot::printListParameter(DiGLPainter* gl, const ObsData& dta, const Parameter& param,
    QPointF& xypos, float yStep, bool align_right, float xshift)
{
  if (not pFlag.count(miutil::to_lower(param.name)) )
    return;

  if( param.name=="pos") {
    printListPos(gl, dta, xypos, yStep, align_right);
    return;
  }

  xypos.ry() -= yStep;

  if( miutil::contains(param.name,"RRR")) {
    printListRRR(gl, dta,param.name, xypos, align_right);
    return;
  }
  if ( param.symbol > -1) {
    printListSymbol(gl, dta,param,xypos,yStep,align_right,xshift);
  } else {
    const std::map<string, string>::const_iterator s_p = dta.stringdata.find(param.name);
    if (s_p != dta.stringdata.end()) {
      checkColourCriteria(gl, param.name,undef);
      printListString(gl, decodeText(s_p->second), xypos, align_right);
    } else {
      const std::map<string, float>::const_iterator f_p = dta.fdata.find(param.name);
      if (f_p != dta.fdata.end()) {
        checkColourCriteria(gl, param.name, f_p->second);
        if (param.name == "VV") {
          printList(gl, visibility(f_p->second, dta.show_time_id), xypos, 0, align_right, true);
        } else if ( param.knotParam && !unit_ms) {
          printList(gl, diutil::ms2knots(f_p->second), xypos, param.precision, align_right);
        } else if ( param.tempParam && tempPrecision) {
          printList(gl, f_p->second, xypos, 0, align_right);
        } else {
          printList(gl, f_p->second, xypos, param.precision, align_right);
        }
      } else {
        printUndef(gl, xypos, align_right);
      }
    }
  }
}

void ObsPlot::printListSymbol(DiGLPainter* gl, const ObsData& dta, const Parameter& param,
    QPointF& xypos, float yStep, bool align_right, const float& xshift)
{
  if (param.name == "PwaHwa" || param.name == "Pw1Hw1") {
    ObsData::fdata_t::const_iterator p, q;
    if (param.name == "PwaHwa") {
      p = dta.fdata.find("HwaHwa");
      q = dta.fdata.find("PwaPwa");
    } else if (param.name == "Pw1Hw1") {
      p = dta.fdata.find("Hw1Hw1");
      q = dta.fdata.find("Pw1Pw1");
    }
    if (p != dta.fdata.end() && q != dta.fdata.end()) {
      checkColourCriteria(gl, param.name, p->second);
      wave(gl, q->second, p->second, xypos, align_right);
      const QString str = "00/00";
      advanceByStringWidth(gl, str, xypos);
    } else {
      printUndef(gl, xypos, align_right);
    }

  } else {
    const std::map<string, float>::const_iterator f_p = dta.fdata.find(param.name);
    if (f_p != dta.fdata.end() ) {
      checkColourCriteria(gl, param.name, f_p->second);
      QPointF spos = xypos*scale - QPointF(xshift, 0);
      if ( param.symbol > 0 && f_p->second > 0 && f_p->second < 10) {
        symbol(gl, vtab(param.symbol + (int) f_p->second), spos, 0.6 * scale, align_right);
      } else if((param.name == "W1" || param.name == "W2") &&f_p->second > 2) {
        pastWeather(gl, (int) f_p->second, spos, 0.6 * scale, align_right);

      } else if (param.name == "ww") {

        const std::map<string, float>::const_iterator ttt_p = dta.fdata.find("TTT");
        if (ttt_p != dta.fdata.end()) {
          weather(gl, (short int) f_p->second, ttt_p->second, dta.show_time_id,
              spos - QPointF(0, 0.2*yStep*scale), scale * 0.6, align_right);
        } else {
          printUndef(gl, xypos, align_right);
        }
      } else if ( param.name == "ds" ) {
        arrow(gl, f_p->second, spos, scale * 0.6);
      } else if ( param.name == "dw1dw1" ) {
        zigzagArrow(gl, f_p->second, spos, scale * 0.6);
      }
      if (!vertical_orientation)
        xypos.rx() += 20;

    } else {
      printUndef(gl, xypos, align_right);
    }
  }
}

void ObsPlot::printListRRR(DiGLPainter* gl, const ObsData& dta, const std::string& param,
    QPointF& xypos, bool align_right)
{
  const std::map<string, float>::const_iterator f_p = dta.fdata.find(param);
  if (f_p != dta.fdata.end()) {
    checkColourCriteria(gl, param, f_p->second);
    if (f_p->second < 0.0) { //Precipitation, but less than 0.1 mm (0.0)
      printListString(gl, "0.0", xypos, align_right);
    } else if (f_p->second < 0.1) { //No precipitation (0.)
      printListString(gl, "0.", xypos, align_right);
    } else {
      printList(gl, f_p->second, xypos, 1, align_right);
    }
  } else {
    printUndef(gl, xypos, align_right);
  }
}

void ObsPlot::printListPos(DiGLPainter* gl, const ObsData& dta,
    QPointF& xypos, float yStep, bool align_right)
{
  xypos.ry() -= yStep;
  QString str1 = diutil::formatLatitude (dta.ypos, 2, 6);
  QString str2 = diutil::formatLongitude(dta.xpos, 2, 6);;
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

    printListString(gl, cs, xypos, align_right);
  } else {
    printListString(gl, "X", xypos, align_right);
  }
}

float ObsPlot::advanceByStringWidth(DiGLPainter* gl, const QString& txt, QPointF& xypos)
{
  float w, h;
  gl->getTextSize(txt, w, h);
  if (!vertical_orientation)
    xypos.rx() += w / scale + 5;
  return w;
}

void ObsPlot::printListString(DiGLPainter* gl, const QString& txt, QPointF& xypos, bool align_right)
{
  QPointF xy = xypos * scale;
  const float w = advanceByStringWidth(gl, txt, xypos);
  if (align_right)
    xy.rx() -= w;
  gl->drawText(txt, xy.x(), xy.y(), 0.0);
}

bool ObsPlot::checkQuality(const ObsData& dta) const
{
  if (not qualityFlag)
    return true;
  const std::map<string, float>::const_iterator itQ = dta.fdata.find("quality");
  if (itQ == dta.fdata.end())
    return true; // assume good data
  return (int(itQ->second) & QUALITY_GOOD);
}

bool ObsPlot::checkWMOnumber(const ObsData& dta) const
{
  if (not wmoFlag)
    return true;
  return dta.fdata.find("wmonumber") != dta.fdata.end();
}

void ObsPlot::plotList(DiGLPainter* gl, int index)
{
  METLIBS_LOG_SCOPE("index: " << index);

  const ObsData &dta = obsp[index];

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
  bool windOK = pFlag.count("Wind") && dta.fdata.count("dd")
        && dta.fdata.count("dd_adjusted") && dta.fdata.count("ff");


  if (not checkQuality(dta) or not checkWMOnumber(dta))
    return;

  //reset colour
  gl->setColour(origcolour);
  colour = origcolour;

  checkTotalColourCriteria(gl, index);

  const std::string thisImage = checkMarkerCriteria(index);
  float thisMarkerSize = checkMarkersizeCriteria(index);

  ImageGallery ig;
  float xShift = ig.widthp(image) / 2;
  float yShift = ig.heightp(image) / 2;

  if ( !pFlag.count("Wind") ) {
    ObsData::stringdata_t::const_iterator it = dta.stringdata.find("image");
    if (it != dta.stringdata.end()) {
      const std::string& thatImage = it->second;
      xShift = ig.widthp(thatImage) / 2;
      yShift = ig.heightp(thatImage) / 2;
      ig.plotImage(gl, getStaticPlot(), thatImage, x[index], y[index], true, thisMarkerSize);
    } else {
      ig.plotImage(gl, getStaticPlot(), thisImage, x[index], y[index], true, thisMarkerSize);
    }
  }

  PushPopTranslateScale pushpop1(gl, QPointF(x[index], y[index]));

  if ((not dta.id.empty()) && dta.id == selectedStation) {
     Colour c("red");
     gl->setColour(c);
   }

  if (windOK) {
    int dd = int(dta.fdata.find("dd")->second);
    int dd_adjusted = int(dta.fdata.find("dd_adjusted")->second);
    float ff = dta.fdata.find("ff")->second;

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
    plotWind(gl, dd_adjusted, ff, ddvar, radius, current);

    advanceByDD(dd_adjusted, xypos);
  } else  {
    if (vertical_orientation)
      xypos.ry() += yShift;
    xypos.rx() += xShift;
    if (pFlag.count("wind")) {
      QPointF center(0, 0);
      printUndef(gl, center, "left"); //undef wind, station position
    }
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

    xypos.ry() += -0.2 * yStep;
  }

  if ( plottype() == OPT_LIST ) {
    if (yStep < 0)
      for ( int i = vparam.size()-1; i>-1; --i )
        printListParameter(gl, dta,vparam[i],xypos,yStep,align_right,xshift);
    else
      for ( size_t i = 0; i<vparam.size(); ++i )
        printListParameter(gl, dta,vparam[i],xypos,yStep,align_right,xshift);

  } else if ( plottype() == OPT_ASCII ) {

    int n = columnName.size();
    if (yStep < 0)
      for (int i = n-1; i > -1; --i)
        plotAscii(gl, dta, columnName[i],xypos,yStep,align_right);
    else
      for (int i = 0; i < n; ++i)
        plotAscii(gl, dta, columnName[i],xypos,yStep,align_right);
  }
}

void ObsPlot::plotAscii(DiGLPainter* gl, const ObsData& dta, const std::string& param,
    QPointF& xypos, const float& yStep, bool align_right)
{
  if (pFlag.count(param) ) {
    xypos.ry() -= yStep;
    ObsData::stringdata_t::const_iterator its = dta.stringdata.find(param);
    if (its != dta.stringdata.end()) {
      std::string str = its->second;
      miutil::remove(str, '"');
      float value = atof(str.c_str());
      if (parameterName)
        str = param + " " + str;
      checkColourCriteria(gl, param, value);
      printListString(gl, decodeText(str), xypos, align_right);
    } else {
      printUndef(gl, xypos, align_right);
    }
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

  ObsData &dta = obsp[index];

  if (not checkQuality(dta) or not checkWMOnumber(dta))
    return;

  DiGLPainter::GLfloat radius = 7.0, x1, x2, x3, y1, y2, y3;
  int lpos;
  const map<string, float>::iterator fend = dta.fdata.end();
  map<string, float>::iterator f_p;
  map<string, float>::iterator h_p;
  map<string, float>::iterator ttt_p = dta.fdata.find("TTT");

  //Some positions depend on wheather the following parameters are plotted or not
  bool ClFlag = ((pFlag.count("cl") && dta.fdata.count("Cl"))
      || ((pFlag.count("st.type") && (not dta.dataType.empty()))));
  bool TxTnFlag = (pFlag.count("txtn") && dta.fdata.find("TxTn") != fend);
  bool timeFlag = (pFlag.count("time") && dta.show_time_id);
  bool precip = (dta.fdata.count("ix") && dta.fdata["ix"] == -1);

  //reset colour
  gl->setColour(origcolour);
  colour = origcolour;

  checkTotalColourCriteria(gl, index);

  PushPopTranslateScale pushpop1(gl, QPointF(x[index], y[index]));
  PushPopTranslateScale pushpop2(gl, scale);

  drawCircle(gl);

  // manned / automated station - ix
  if ((dta.fdata.count("ix") && dta.fdata["ix"] > 3)
      || (dta.fdata.count("auto") && dta.fdata["auto"] == 0)) {
    y1 = y2 = -1.1 * radius;
    x1 = y1 * sqrtf(3.0);
    x2 = -1 * x1;
    x3 = 0;
    y3 = radius * 2.2;
    gl->drawTriangle(false, QPointF(x1, y1), QPointF(x2, y2), QPointF(x3, y3));
  }

  //wind - dd,ff
  if (pFlag.count("wind") && dta.fdata.count("dd") && dta.fdata.count("ff")
      && dta.fdata.count("dd_adjusted") && dta.fdata["dd"] != undef) {
    bool ddvar = false;
    int dd = (int) dta.fdata["dd"];
    int dd_adjusted = (int) dta.fdata["dd_adjusted"];
    if (dd == 990 || dd == 510) {
      ddvar = true;
      dd_adjusted = 270;
    }
    if (diutil::ms2knots(dta.fdata["ff"]) < 1.)
      dd = 0;
    lpos = vtab((dd / 10 + 3) / 2) + 10;
    checkColourCriteria(gl, "dd", dd);
    checkColourCriteria(gl, "ff", dta.fdata["ff"]);
    plotWind(gl, dd_adjusted, dta.fdata["ff"], ddvar, radius);
  } else
    lpos = vtab(1) + 10;

  //Total cloud cover - N
  if ((f_p = dta.fdata.find("N")) != fend) {
    checkColourCriteria(gl, "N", f_p->second);
    cloudCover(gl, f_p->second, radius);
  } else if (!precip) {
    gl->setColour(colour);
    cloudCover(gl, undef, radius);
  }

  //Weather - WW
  float VVxpos = xytab(lpos + 14).x() + 22;
  if (pFlag.count("ww") && (f_p = dta.fdata.find("ww")) != fend) {
    checkColourCriteria(gl, "ww", f_p->second);
    const QPointF wwxy = xytab(lpos + 12);
    VVxpos = wwxy.x() - 20;
    weather(gl, (short int) f_p->second, ttt_p->second, dta.show_time_id, wwxy);
  }

  //characteristics of pressure tendency - a
  map<string, float>::iterator ppp_p = dta.fdata.find("ppp");
  if (pFlag.count("a") && (f_p = dta.fdata.find("a")) != fend
      && f_p->second >= 0 && f_p->second < 9) {
    checkColourCriteria(gl, "a", f_p->second);
    if (ppp_p != fend && ppp_p->second > 9)
      symbol(gl, vtab(201 + (int) f_p->second), xytab(lpos + 42) + QPointF(12, 0), 0.8);
    else
      symbol(gl, vtab(201 + (int) f_p->second), xytab(lpos + 42), 0.8);
  }

  // High cloud type - Ch
  if (pFlag.count("ch") && (f_p = dta.fdata.find("Ch")) != fend) {
    checkColourCriteria(gl, "Ch", f_p->second);
    //METLIBS_LOG_DEBUG("Ch: " << f_p->second);
    symbol(gl, vtab(190 + (int) f_p->second), xytab(lpos + 4), 0.8);
  }

  // Middle cloud type - Cm
  if (pFlag.count("cm") && (f_p = dta.fdata.find("Cm")) != fend) {
    checkColourCriteria(gl, "Cm", f_p->second);
    //METLIBS_LOG_DEBUG("Cm: " << f_p->second);
    symbol(gl, vtab(180 + (int) f_p->second), xytab(lpos + 2), 0.8);
  }

  // Low cloud type - Cl
  if (pFlag.count("cl") && (f_p = dta.fdata.find("Cl")) != fend) {
    checkColourCriteria(gl, "Cl", f_p->second);
    //METLIBS_LOG_DEBUG("Cl: " << f_p->second);
    symbol(gl, vtab(170 + (int) f_p->second), xytab(lpos + 22), 0.8);
  }

  // Past weather - W1
  if (pFlag.count("w1") && (f_p = dta.fdata.find("W1")) != fend) {
    checkColourCriteria(gl, "W1", f_p->second);
    pastWeather(gl, int(f_p->second), xytab(lpos + 34), 0.8);
  }

  // Past weather - W2
  if (pFlag.count("w2") && (f_p = dta.fdata.find("W2")) != fend) {
    checkColourCriteria(gl, "W2", f_p->second);
    pastWeather(gl, (int) f_p->second, xytab(lpos + 36), 0.8);
  }

  // Direction of ship movement - ds
  if (pFlag.count("ds") && dta.fdata.find("vs") != fend && (f_p = dta.fdata.find("ds")) != fend) {
    checkColourCriteria(gl, "ds", f_p->second);
    arrow(gl, f_p->second, xytab(lpos + 32));
  }

  // Direction of swell waves - dw1dw1
  if (pFlag.count("dw1dw1") && (f_p = dta.fdata.find("dw1dw1")) != fend) {
    checkColourCriteria(gl, "dw1dw1", f_p->second);
    zigzagArrow(gl, f_p->second, xytab(lpos + 30));
  }

  pushpop2.PopMatrix();

  // Pressure - PPPP
  if (mslp()) {
    if ((f_p = dta.fdata.find("PPPP_mslp")) != fend) {
      checkColourCriteria(gl, "PPPP_mslp", f_p->second);
      printNumber(gl, f_p->second, xytab(lpos + 44), "PPPP_mslp");
    }
  } else if (pFlag.count("pppp") && (f_p = dta.fdata.find("PPPP")) != fend) {
    checkColourCriteria(gl, "PPPP", f_p->second);
    printNumber(gl, f_p->second, xytab(lpos + 44), "PPPP");
  }

  // Pressure tendency over 3 hours - ppp
  if (pFlag.count("ppp") && ppp_p != fend) {
    checkColourCriteria(gl, "ppp", ppp_p->second);
    printNumber(gl, ppp_p->second, xytab(lpos + 40), "ppp");
  }
  // Clouds
  if (pFlag.count("nh") || pFlag.count("h")) {
    f_p = dta.fdata.find("Nh");
    h_p = dta.fdata.find("h");
    if (f_p != fend || h_p != fend) {
      float Nh, h;
      if (f_p == fend)
        Nh = undef;
      else
        Nh = f_p->second;
      if (h_p == fend)
        h = undef;
      else
        h = h_p->second;
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

  //Precipitation - RRR
  if (pFlag.count("rrr")
      && !(dta.show_time_id && dta.fdata.count("ds") && dta.fdata.count("vs"))) {
    if ((f_p = dta.fdata.find("RRR")) != fend) {
      checkColourCriteria(gl, "RRR", f_p->second);
      if (f_p->second < 0.0) //Precipitation, but less than 0.1 mm (0.0)
        printString(gl, "0.0", xytab(lpos + 32) + QPointF(2, 0));
      else if (f_p->second < 0.1) //No precipitation (0.)
        printString(gl, "0.", xytab(lpos + 32) + QPointF(2, 0));
      else
        printNumber(gl, f_p->second, xytab(lpos + 32) + QPointF(2, 0), "RRR");
    }
  }
  // Horizontal visibility - VV
  if (pFlag.count("vv") && (f_p = dta.fdata.find("VV")) != fend) {
    checkColourCriteria(gl, "VV", f_p->second);
    const QPointF vvxy(VVxpos, xytab(lpos + 14).y());
    printNumber(gl, visibility(f_p->second, dta.show_time_id), vvxy, "fill_2");
  }
  // Temperature - TTT
  if (pFlag.count("ttt") && ttt_p != fend) {
    checkColourCriteria(gl, "TTT", ttt_p->second);
    printNumber(gl, ttt_p->second, xytab(lpos + 10), "temp");
  }

  // Dewpoint temperature - TdTdTd
  if (pFlag.count("tdtdtd") && (f_p = dta.fdata.find("TdTdTd")) != fend) {
    checkColourCriteria(gl, "TdTdTd", f_p->second);
    printNumber(gl, f_p->second, xytab(lpos + 16), "temp");
  }

  // Max/min temperature - TxTxTx/TnTnTn
  if (TxTnFlag) {
    if ((f_p = dta.fdata.find("TxTn")) != fend) {
      checkColourCriteria(gl, "TxTn", f_p->second);
      printNumber(gl, f_p->second, xytab(lpos + 8), "temp");
    }
  }

  // Snow depth - sss
  if (pFlag.count("sss") && (f_p = dta.fdata.find("sss")) != fend
      && !dta.show_time_id) {
    checkColourCriteria(gl, "sss", f_p->second);
    printNumber(gl, f_p->second, xytab(lpos + 46));
  }

  // Maximum wind speed (gusts) - 911ff
  if (pFlag.count("911ff")) {
    if ((f_p = dta.fdata.find("911ff")) != fend) {
      checkColourCriteria(gl, "911ff", f_p->second);
      float ff = unit_ms ? f_p->second : diutil::ms2knots(f_p->second);
      printNumber(gl, ff, xytab(lpos + 38), "fill_2", true);
    }
  }

  // State of the sea - s
  if (pFlag.count("s") && (f_p = dta.fdata.find("s")) != fend) {
    checkColourCriteria(gl, "s", f_p->second);
    if (TxTnFlag)
      printNumber(gl, f_p->second, xytab(lpos + 6));
    else
      printNumber(gl, f_p->second, xytab(lpos + 6) + QPointF(0, -14));
  }

  // Maximum wind speed
  if (pFlag.count("fxfx")) {
    if ((f_p = dta.fdata.find("fxfx")) != fend
        && !dta.show_time_id) {
      checkColourCriteria(gl, "fxfx", f_p->second);
      float ff = unit_ms ? f_p->second : diutil::ms2knots(f_p->second);
      if (TxTnFlag)
        printNumber(gl, ff, xytab(lpos + 6) + QPointF(10, 0), "fill_2", true);
      else
        printNumber(gl, ff, xytab(lpos + 6) + QPointF(10, -14), "fill_2", true);
    }
  }

  //Maritime

  // Ship's average speed - vs
  if (pFlag.count("vs") && dta.fdata.find("ds") != fend && (f_p =
      dta.fdata.find("vs")) != fend) {
    checkColourCriteria(gl, "vs", f_p->second);
    printNumber(gl, f_p->second, xytab(lpos + 32) + QPointF(18, 0));
  }

  //Time
  if (timeFlag && !dta.obsTime.undef()) {
    checkColourCriteria(gl, "Time", 0);
    printTime(gl, dta.obsTime, xytab(lpos + 46), "left", "h.m");
  }

  // Ship or buoy identifier
  if (pFlag.count("id") && dta.show_time_id) {
    checkColourCriteria(gl, "Id", 0);
    QString kjTegn = decodeText(dta.id);
    if (timeFlag)
      printString(gl, kjTegn, xytab(lpos + 46) + QPointF(0, 15));
    else
      printString(gl, kjTegn, xytab(lpos + 46));
  }

  //Wmo block + station number - land stations
  if ((pFlag.count("st.no(5)") || pFlag.count("st.no(3)")) && !dta.show_time_id ) {
    checkColourCriteria(gl, "St.no(5)", 0);
    QString kjTegn = decodeText(dta.id);
    if (!pFlag.count("st.no(5)") && kjTegn.size() > 4) {
      kjTegn = kjTegn.mid(2, 3);
      checkColourCriteria(gl, "St.no(3)", 0);
    }
    if ((pFlag.count("sss") && dta.fdata.count("sss"))) //if snow
      printString(gl, kjTegn, xytab(lpos + 46) + QPointF(0, 15));
    else
      printString(gl, kjTegn, xytab(lpos + 46));
  }

  //Sea temperature
  if (pFlag.count("twtwtw") && (f_p = dta.fdata.find("TwTwTw")) != fend) {
    checkColourCriteria(gl, "TwTwTw", f_p->second);
    printNumber(gl, f_p->second, xytab(lpos + 18), "temp", true);
  }

  //Wave
  if (pFlag.count("pwahwa") && (f_p = dta.fdata.find("PwaPwa")) != fend
      && (h_p = dta.fdata.find("HwaHwa")) != fend) {
    checkColourCriteria(gl, "PwaHwa", 0);
    wave(gl, f_p->second, h_p->second, xytab(lpos + 20));
  }
  if (pFlag.count("pw1hw1")
      && ((f_p = dta.fdata.find("Pw1Pw1")) != fend
          && (h_p = dta.fdata.find("Hw1Hw1")) != fend)) {
    checkColourCriteria(gl, "Pw1Hw1", 0);
    wave(gl, f_p->second, h_p->second, xytab(lpos + 28));
  }

  if (!flaginfo) {
    return;
  }

  //----------------- HQC only ----------------------------------------
  //Flag colour

  if (pFlag.count("id")) {
    gl->setColour(colour);
    int dy = 0;
    if (timeFlag)
      dy += 13;
    if ((pFlag.count("sss") && dta.fdata.count("sss")))
      dy += 13;
    printString(gl, decodeText(dta.id), xytab(lpos + 46) + QPointF(0, dy));
  }

  //Flag + red/yellow/green
  if (pFlag.count("flag") && not hqcFlag.empty()
      && dta.flagColour.count(hqcFlag)) {
    PushPopTranslateScale pushpop2(gl, scale);
    gl->setColour(dta.flagColour[hqcFlag]);
    drawCircle(gl);
    cloudCover(gl, 8, radius);
    gl->setColour(colour);
  }

  //Type of station (replace Cl)
  bool typeFlag = (pFlag.count("st.type") && (not dta.dataType.empty()));
  if (typeFlag)
    printString(gl, decodeText(dta.dataType), xytab(lpos + 22));

  // Man. precip, marked by dot
  if (precip) {
    PushPopTranslateScale pushpop2(gl, scale * 0.3);
    drawCircle(gl);
    cloudCover(gl, 8, radius);
  }

  //id
  if (pFlag.count("flag") && not hqcFlag.empty() && dta.flag.count(hqcFlag)) {
    gl->setColour(paramColour["flag"]);
    int dy = 0;
    if (pFlag.count("id") || typeFlag)
      dy += 15;
    if (timeFlag)
      dy += 15;
    if (pFlag.count("sss") && dta.fdata.count("sss"))
      dy += 15; //if snow
    printString(gl, decodeText(dta.flag[hqcFlag]), xytab(lpos + 46) + QPointF(0, dy));
  }

  //red circle
  if (pFlag.count("id") && dta.id == selectedStation) {
    PushPopTranslateScale pushpop2(gl, scale * 1.3);
    DiGLPainter::GLfloat lwidth;
    gl->GetFloatv(DiGLPainter::gl_LINE_WIDTH, &lwidth);
    gl->LineWidth(2 * lwidth);
    Colour c("red");
    gl->setColour(c);
    drawCircle(gl);
    gl->LineWidth(lwidth);
  }
  //----------------- end HQC only ----------------------------------------
}

void ObsPlot::plotMetar(DiGLPainter* gl, int index)
{
  METLIBS_LOG_SCOPE("index: " << index);

  ObsData &dta = obsp[index];

  DiGLPainter::GLfloat radius = 7.0;
  int lpos = vtab(1) + 10;
  const map<string, float>::iterator fend = dta.fdata.end();
  map<string, float>::iterator f2_p;
  map<string, float>::iterator f_p;

  //reset colour
  gl->setColour(origcolour);
  colour = origcolour;

  checkTotalColourCriteria(gl, index);

  PushPopTranslateScale pushpop1(gl, QPointF(x[index], y[index]));

  //Circle
  PushPopTranslateScale pushpop2(gl, scale);
  drawCircle(gl);
  pushpop2.PopMatrix();

  //wind
  if (pFlag.count("wind") && dta.fdata.count("dd") && dta.fdata.count("ff")) {
    checkColourCriteria(gl, "dd", dta.fdata["dd"]);
    checkColourCriteria(gl, "ff", dta.fdata["ff"]);
    metarWind(gl, (int) dta.fdata["dd_adjusted"], diutil::ms2knots(dta.fdata["ff"]),
        radius, lpos);
  }

  //limit of variable wind direction
  int dndx = 16;
  if (pFlag.count("dndx") && (f_p = dta.fdata.find("dndndn")) != fend && (f2_p =
      dta.fdata.find("dxdxdx")) != fend) {
    QString cs = QString("%1V%2")
        .arg(f_p->second / 10)
        .arg(f2_p->second / 10);
    printString(gl, cs, xytab(lpos + 2) + QPointF(2, 2));
    dndx = 2;
  }
  //Wind gust
  QPointF xyid = xytab(lpos + 4);
  if (pFlag.count("fmfm") && (f_p = dta.fdata.find("fmfm")) != fend) {
    checkColourCriteria(gl, "fmfm", f_p->second);
    printNumber(gl, f_p->second, xyid + QPointF(2, 2-dndx), "left", true);
    //understrekes
    xyid += QPointF(20 + 15, -dndx + 8);
  } else {
    xyid += QPointF(2 + 15, 2 - dndx + 8);
  }

  //Temperature
  if (pFlag.count("ttt") && (f_p = dta.fdata.find("TTT")) != fend) {
    checkColourCriteria(gl, "TTT", f_p->second);
    //    if( dta.TT>-99.5 && dta.TT<99.5 ) //right align_righted
    printNumber(gl, f_p->second, xytab(lpos + 12) + QPointF(23, 16), "temp");
  }

  //Dewpoint temperature
  if (pFlag.count("tdtdtd") && (f_p = dta.fdata.find("TdTdTd")) != fend) {
    checkColourCriteria(gl, "TdTdTd", f_p->second);
    //    if( dta.TdTd>-99.5 && dta.TdTd<99.5 )  //right align_righted and underlined
    printNumber(gl, f_p->second, xytab(lpos + 14) + QPointF(23, -16), "temp", true);
  }

  PushPopTranslateScale pushpop3(gl, scale*0.8);

  //Significant weather
  int wwshift = 0; //idxm
  if (pFlag.count("ww")) {
    checkColourCriteria(gl, "ww", 0);
    if (dta.ww.size() > 0 && not dta.ww[0].empty()) {
      metarSymbol(gl, dta.ww[0], xytab(lpos + 8), wwshift);
    }
    if (dta.ww.size() > 1 && not dta.ww[1].empty()) {
      metarSymbol(gl, dta.ww[1], xytab(lpos + 10), wwshift);
    }
  }

  //Recent weather
  if (pFlag.count("reww")) {
    checkColourCriteria(gl, "REww", 0);
    if (dta.REww.size() > 0 && not dta.REww[0].empty()) {
      int intREww[5];
      metarString2int(dta.REww[0], intREww);
      if (intREww[0] >= 0 && intREww[0] < 100) {
        symbol(gl, vtab(40 + intREww[0]), xytab(lpos + 30) + QPointF(0, 2));
      }
    }
    if (dta.REww.size() > 1 && not dta.REww[1].empty()) {
      int intREww[5];
      metarString2int(dta.REww[1], intREww);
      if (intREww[0] >= 0 && intREww[0] < 100) {
        symbol(gl, vtab(40 + intREww[0]), xytab(lpos + 30) + QPointF(15, 2));
      }
    }
  }
  pushpop3.PopMatrix();

  //Visibility (worst)
  if (pFlag.count("vvvv/dv")) {
    if ((f_p = dta.fdata.find("VVVV")) != fend) {
      checkColourCriteria(gl, "VVVV/Dv", 0);
      const QPointF xy = xytab(lpos + 12) + QPointF(22 + wwshift, 2);
      if ((f2_p = dta.fdata.find("Dv")) != fend) {
        printNumber(gl, float(int(f_p->second) / 100), xy);
        printNumber(gl, vis_direction(f2_p->second), xy);
      } else {
        printNumber(gl, float(int(f_p->second) / 100), xy);
      }
    }
  }

  //Visibility (best)
  if (pFlag.count("vxvxvxvx/dvx")) {
    if ((f_p = dta.fdata.find("VxVxVxVx")) != fend) {
      checkColourCriteria(gl, "VVVV/Dv", 0);
      const QPointF dxy(22 + wwshift, 0);
      if ((f2_p = dta.fdata.find("Dvx")) != fend) {
        printNumber(gl, float(int(f_p->second) / 100), xytab(lpos + 12, lpos + 15) + dxy);
        printNumber(gl, f2_p->second, xytab(lpos + 12) + dxy);
      } else {
        printNumber(gl, float(int(f_p->second) / 100), xytab(lpos + 14) + dxy);
      }
    }
  }

  //CAVOK
  if (pFlag.count("clouds")) {
    checkColourCriteria(gl, "Clouds", 0);

    if (dta.CAVOK) {
      printString(gl, "CAVOK", xytab(lpos + 18) + QPointF(2, 2));
    } else { //Clouds
      int ncl = dta.cloud.size();
      for (int i = 0; i < ncl; i++)
        printString(gl, decodeText(dta.cloud[i]), xytab(lpos + 18 + i * 4) + QPointF(2, 2));
    }
  }

  //QNH ??
  if (pFlag.count("phphphph")) {
    if ((f_p = dta.fdata.find("PHPHPHPH")) != fend) {
      checkColourCriteria(gl, "PHPHPHPH", f_p->second);
      int pp = (int) f_p->second;
      pp -= (pp / 100) * 100;
      printNumber(gl, pp, xytab(lpos + 32) + QPointF(2, 2), "fill_2");
    }
  }

  //Id
  if (pFlag.count("id")) {
    checkColourCriteria(gl, "Id", 0);
    printString(gl, decodeText(dta.metarId), xyid);
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
    dy -= 2;
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

  vector<metarww> ia;
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

  ostringstream cs;

  if (align == "temp") {
    cs.setf(ios::fixed);
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
    cs.setf(ios::fixed);
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
    cs.setf(ios::fixed);
    cs.setf(ios::showpoint);
    if (f < 1) {
      cs.precision(1);
    } else {
      cs.precision(0);
    }
    cs << f;
  } else if (align == "PPPP_mslp") {
    cs.setf(ios::fixed);
    cs.precision(1);
    cs.setf(ios::showpos);
    cs << f;
  } else
    cs << f;

  const std::string str = cs.str();
  float cw, ch;
  if (mark || line)
    gl->getTextSize(str, cw, ch);

  if (mark) {
    Colour col("white");
    if (!(colour == col))
      gl->setColour(col); //white
    else
      gl->Color3ub(0, 0, 0); //black
    gl->drawRect(true, x, y - 0.2 * ch, x + cw, y + 0.8 * ch);

    gl->Color3ub(0, 0, 0); //black
    gl->drawRect(false, x, y - 0.2 * ch, x + cw, y + 0.8 * ch);
    gl->setColour(colour);
  }

  gl->drawText(str, x, y, 0.0);

  if (line)
    gl->drawLine(x, (y - ch / 6), (x + cw), (y - ch / 6));
}

void ObsPlot::printString(DiGLPainter* gl, const QString& c, QPointF xy, bool align_right, bool line)
{
  float x = xy.x() * scale;
  float y = xy.y() * scale;

  float w, h;
  if (align_right || line)
    gl->getTextSize(c, w, h);
  if (align_right)
    x -= w;

  gl->drawText(c, x, y, 0.0);

  if (line)
    gl->drawLine(x, (y - h / 6), (x + w), (y - h / 6));
}

void ObsPlot::printTime(DiGLPainter* gl, const miTime& time,
    QPointF xy, bool align_right, const std::string& format)
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

int ObsPlot::visibility(float VV, bool ship)
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

void ObsPlot::amountOfClouds_1(DiGLPainter* gl, short int Nh, short int h, QPointF xy, bool metar)
{
  float x = xy.x(), y = xy.y();

  QString ost;
  if (Nh > -1)
    if (metar) {
      if (Nh == 8)
        ost = "O";
      else if (Nh == 11)
        ost = "S";
      else if (Nh == 12)
        ost = "B";
      else if (Nh == 13)
        ost = "F";
      else
        ost.setNum(Nh);
    } else
      ost.setNum(Nh);
  else
    ost = "x";

  gl->drawText(ost, x * scale, y * scale, 0.0);

  x += 8;
  y -= 2;
  gl->drawText("/", x * scale, y * scale, 0.0);

  x += 6; // += 8;
  y -= 2;
  if (h > -1)
    ost.setNum(h);
  else
    ost = "x";

  gl->drawText(ost, x * scale, y * scale, 0.0);
}

void ObsPlot::amountOfClouds_1_4(DiGLPainter* gl, short int Ns1, short int hs1, short int Ns2,
    short int hs2, short int Ns3, short int hs3, short int Ns4, short int hs4,
    QPointF xy, bool metar)
{
  float x = xy.x(), y = xy.y();
  const float x_org = x;

  if (Ns4 != undef || hs4 != undef) {
    x = x_org;
    QString ost;
    if (Ns4 > -1)
      if (metar) {
        if (Ns4 == 8)
          ost = "O";
        else if (Ns4 == 11)
          ost = "S";
        else if (Ns4 == 12)
          ost = "B";
        else if (Ns4 == 13)
          ost = "F";
        else
          ost.setNum(Ns4);
      } else
        ost.setNum(Ns4);
    else
      ost = "x";

    gl->drawText(ost, x * scale, y * scale, 0.0);
    x += 10 * ost.size();
    gl->drawText("-", x * scale, y * scale, 0.0);

    x += 8;
    if (hs4 != undef)
      ost.setNum(hs4);
    else
      ost = "x";

    gl->drawText(ost, x * scale, y * scale, 0.0);
    y -= 12;
  }
  if (Ns3 != undef || hs3 != undef) {
    x = x_org;
    QString ost;
    if (Ns3 > -1)
      if (metar) {
        if (Ns3 == 8)
          ost = "O";
        else if (Ns3 == 11)
          ost = "S";
        else if (Ns3 == 12)
          ost = "B";
        else if (Ns3 == 13)
          ost = "F";
        else
          ost.setNum(Ns3);
      } else
        ost.setNum(Ns3);
    else
      ost = "x";

    gl->drawText(ost, x * scale, y * scale, 0.0);

    x += 10 * ost.size();
    gl->drawText("-", x * scale, y * scale, 0.0);

    x += 8;
    if (hs3 != undef)
      ost.setNum(hs3);
    else
      ost = "x";

    gl->drawText(ost, x * scale, y * scale, 0.0);
    y -= 12;
  }
  if (Ns2 != undef || hs2 != undef) {
    x = x_org;
    QString ost;
    if (Ns2 > -1)
      if (metar) {
        if (Ns2 == 8)
          ost = "O";
        else if (Ns2 == 11)
          ost = "S";
        else if (Ns2 == 12)
          ost = "B";
        else if (Ns2 == 13)
          ost = "F";
        else
          ost.setNum(Ns2);
      } else
        ost.setNum(Ns2);
    else
      ost = "x";

    gl->drawText(ost, x * scale, y * scale, 0.0);

    x += 10 * ost.size();
    gl->drawText("-", x * scale, y * scale, 0.0);

    x += 8;
    if (hs2 != undef)
      ost.setNum(hs2);
    else
      ost = "x";

    gl->drawText(ost, x * scale, y * scale, 0.0);
    y -= 12;
  }
  if (Ns1 != undef || hs1 != undef) {
    x = x_org;
    QString ost;
    if (Ns1 > -1)
      if (metar) {
        if (Ns1 == 8)
          ost = "O";
        else if (Ns1 == 11)
          ost = "S";
        else if (Ns1 == 12)
          ost = "B";
        else if (Ns1 == 13)
          ost = "F";
        else
          ost.setNum(Ns1);
      } else
        ost.setNum(Ns1);
    else
      ost = "x";

    gl->drawText(ost, x * scale, y * scale, 0.0);

    x += 10 * ost.size();
    gl->drawText("-", x * scale, y * scale, 0.0);

    x += 8;
    if (hs1 != undef)
      ost.setNum(hs1);
    else
      ost = "x";

    gl->drawText(ost, x * scale, y * scale, 0.0);
    y -= 12;
  }
}

void ObsPlot::checkAccumulationTime(ObsData &dta)
{

  // todo: include this if all data sources reports time info
  //  if (dta.fdata.find("RRR")!= dta.fdata.end()){
  //    dta.fdata.erase(dta.fdata.find("RRR"));
  //  }

  int hour = Time.hour();
  if ((hour == 6 || hour == 18) && dta.fdata.count("RRR_12")) {

    dta.fdata["RRR"] = dta.fdata["RRR_12"];

  } else if ((hour == 0 || hour == 12) && dta.fdata.count("RRR_6")) {
    dta.fdata["RRR"] = dta.fdata["RRR_6"];

  } else if (dta.fdata.count("RRR_1")) {
    dta.fdata["RRR"] = dta.fdata["RRR_1"];

  }
}

void ObsPlot::checkGustTime(ObsData &dta)
{
  // todo: include this if all data sources reports time info
  //  if (dta.fdata.find("911ff")!= dta.fdata.end()){
  //    dta.fdata.erase(dta.fdata.find("911ff"));
  //  }
  int hour = Time.hour();
  if ((hour == 0 || hour == 6 || hour == 12 || hour == 18)
      && dta.fdata.count("911ff_360")) {

    dta.fdata["911ff"] = dta.fdata["911ff_360"];

  } else if ((hour == 3 || hour == 9 || hour == 15 || hour == 21)
      && dta.fdata.count("911ff_180")) {

    dta.fdata["911ff"] = dta.fdata["911ff_180"];

  } else if (dta.fdata.count("911ff_60")) {

    dta.fdata["911ff"] = dta.fdata["911ff_60"];

  }
}

bool ObsPlot::updateDeltaTimes()
{
  bool updated = false;
  if (plottype() == OPT_ASCII
      && std::find(columnName.begin(), columnName.end(), "DeltaTime")
  != columnName.end()) {
    miutil::miTime nowTime = miutil::miTime::nowTime();
    for (int i = 0; i < getObsCount(); i++) {
      if (updateDeltaTime(obsp[i], nowTime))
        updated = true;
    }
  }
  return updated;
}

bool ObsPlot::updateDeltaTime(ObsData &dta, const miutil::miTime& nowTime)
{
  if (dta.obsTime.undef())
    return false;

  dta.stringdata["DeltaTime"] = miutil::from_number(
      miutil::miTime::secDiff(nowTime, dta.obsTime));
  return true;
}

void ObsPlot::checkMaxWindTime(ObsData &dta)
{
  // todo: include this if all data sources reports time info
  //  if (dta.fdata.find("fxfx")!= dta.fdata.end()){
  //    dta.fdata.erase(dta.fdata.find("fxfx"));
  //  }

  int hour = Time.hour();
  if ((hour == 0 || hour == 6 || hour == 12 || hour == 18)
      && dta.fdata.count("fxfx_360")) {

    dta.fdata["fxfx"] = dta.fdata["fxfx_360"];

  } else if ((hour == 3 || hour == 9 || hour == 15 || hour == 21)
      && dta.fdata.count("fxfx_180")) {

    dta.fdata["fxfx"] = dta.fdata["fxfx_180"];

  } else if (dta.fdata.count("fxfx_60")) {

    dta.fdata["fxfx"] = dta.fdata["fxfx_60"];
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
  int N = diutil::float2int(fN);

  float x;
  DiGLPainter::GLfloat x1, x2, x3, y1, y2, y3;

  // Total cloud cover N
  // Dont fill anything
  if (N >= 0 && N <= 2)
    return;

  if (N < 0 || N > 9) { //cloud cover not observed
    x = radius * 1.1 / sqrt((float) 2);
    gl->drawCross(0, 0, x, true);
  } else if (N == 9) {
    // some special code.., fog perhaps
    x = radius / sqrt((float) 2);
    gl->drawCross(0, 0, x, true);
  } else if (N >= 6 && N <= 8) {
    DiGLPainter::GLfloat tmp_radius = 0.6 * radius;
    y1 = y2 = -1.1 * tmp_radius;
    x1 = y1 * sqrtf(3.0);
    x2 = -1 * x1;
    x3 = 0;
    y3 = tmp_radius * 2.2;
    gl->drawTriangle(true, QPointF(x1, y1), QPointF(x2, y2), QPointF(x3, y3));

  } else if (N >= 3 && N <= 5) {
    DiGLPainter::GLfloat tmp_radius = 0.6 * radius;
    y1 = y2 = -1.1 * tmp_radius;
    x1 = y1 * sqrtf(3.0);
    x2 = -1 * x1;
    x1 = 0;
    x3 = 0;
    y3 = tmp_radius * 2.2;
    gl->drawTriangle(true,  QPointF(x1, y1), QPointF(x2, y2), QPointF(x3, y3));
  }
}

void ObsPlot::plotWind(DiGLPainter* gl, int dd, float ff_ms, bool ddvar, float radius, float current)
{
  METLIBS_LOG_SCOPE();
  //full feather = current
  if (current > 0)
    ff_ms = ff_ms * 10.0 / current;

  float ff; //wind in knots (current in m/s)

  if (current < 0)
    ff = diutil::ms2knots(ff_ms);
  else
    ff = int(ff_ms);

  // just a guess of the max possible in plotting below
  if (ff > 200)
    ff = 200;

  diutil::GlMatrixPushPop pushpop1(gl);

  const bool type_list_ascii = (plottype() == OPT_LIST || plottype() == OPT_ASCII);
  const bool no_cirle = (type_list_ascii || current > 0);
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

    const bool arrow = poptions.arrowstyle == arrow_wind_arrow
        && (plottype() == OPT_LIST || plottype() == OPT_ASCII);

    const float angle = (270-dd)*DEG_TO_RAD, ac = std::cos(angle), as = std::sin(angle);
    const float u = ff*ac, v = ff*as;
    gl->drawWindArrow(u, v, -r*ac, -r*as, 47-r, arrow, 1);
  }
}

void ObsPlot::weather(DiGLPainter* gl, short int ww, float TTT, bool show_time_id,
    QPointF xypos, float scale, bool align_right)
{
  const int auto2man[100] = { 0, 1, 2, 3, 4, 5, 0, 0, 0, 0, 10, 76, 13, 0, 0, 0,
      0, 0, 18, 0, 28, 21, 20, 21, 22, 24, 29, 38, 38, 37, 41, 41, 43, 45, 47,
      49, 0, 0, 0, 0, 63, 63, 65, 63, 65, 74, 75, 66, 67, 0, 53, 51, 53, 55, 56,
      57, 57, 58, 59, 0, 63, 61, 63, 65, 66, 67, 67, 68, 69, 0, 73, 71, 73, 75,
      79, 79, 79, 0, 0, 0, 81, 80, 81, 82, 85, 86, 86, 0, 0, 0, 17, 17, 95, 96,
      17, 97, 99, 0, 0, 0 };

  if (ww > 99)  {
    ww = auto2man[ww - 100];
  }

  //do not plot ww<3
  if (ww < 3) {
    return;
  }

  int index = iptab->at(1247 + ww);
  if (ww == 7 && show_time_id)
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
  if (ww == 7 && show_time_id)
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

void ObsPlot::pastWeather(DiGLPainter* gl, int w, QPointF xypos, float scale,
    bool align_right)
{
  const int auto2man[10] = { 0, 4, 3, 4, 6, 5, 6, 7, 8, 9 };

  if (w > 9)
    w = auto2man[w - 10];

  if (w < 2)
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

  printListString(gl, cs, xypos, align_right);
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
  string line;
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

  if (type == OPT_SYNOP || type == OPT_LIST || type == OPT_ROADOBS)
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
  vector<std::string> vstr = miutil::split(sortStr, ";");
  int nvstr = vstr.size();
  sortcriteria.clear();

  for (int i = 0; i < nvstr; i++) {
    vector<std::string> vcrit = miutil::split(vstr[i], ",");
    if (vcrit.size() > 1 && miutil::to_lower(vcrit[1]) == "desc") {
      sortcriteria[vcrit[0]] = false;
    } else {
      sortcriteria[vcrit[0]] = true;
    }
  }
}

void ObsPlot::decodeCriteria(const std::string& critStr)
{
  vector<std::string> vstr = miutil::split(critStr, ";");
  int nvstr = vstr.size();
  for (int i = 0; i < nvstr; i++) {
    vector<std::string> vcrit = miutil::split(vstr[i], ",");
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
      vector<std::string> sstr = miutil::split(vcrit[0], sep);
      if (sstr.size() != 2)
        continue;
      parameter = sstr[0];
      limit = atof(sstr[1].c_str());
      if (!unit_ms && knotParameters.count(parameter)) {
        limit = diutil::knots2ms(limit);
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

  std::map<std::string, vector<colourCriteria> >::iterator p = colourcriteria.find(param);
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

bool ObsPlot::getValueForCriteria(int index, const std::string& param,
    float& value)
{
  value = 0;

  if (obsp[index].fdata.count(param)) {
    value = obsp[index].fdata[param];
  } else if (obsp[index].stringdata.count(param)) {
    value = atof(obsp[index].stringdata[param].c_str());
  } else if (miutil::to_lower(param) != obsp[index].dataType) {
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

bool ObsPlot::checkPlotCriteria(int index)
{
  if (plotcriteria.empty())
    return true;

  bool doPlot = false;

  map<std::string, vector<plotCriteria> >::iterator p = plotcriteria.begin();

  for (; p != plotcriteria.end(); p++) {
    float value = 0;
    if (not getValueForCriteria(index, p->first, value))
      continue;
    if (p->first == "RRR")
      adjustRRR(value);

    bool bplot = true;
    for (size_t i = 0; i < p->second.size(); i++)
      bplot &= p->second[i].match(value);

    doPlot |= bplot;
  }

  return doPlot;
}

void ObsPlot::checkTotalColourCriteria(DiGLPainter* gl, int index)
{
  if (totalcolourcriteria.empty())
    return;

  map<std::string, vector<colourCriteria> >::iterator p =
      totalcolourcriteria.begin();

  for (; p != totalcolourcriteria.end(); p++) {
    float value = 0;
    if (not getValueForCriteria(index, p->first, value))
      continue;
    if (p->first == "RRR")
      adjustRRR(value);

    for (size_t i = 0; i < p->second.size(); i++) {
      if (p->second[i].match(value))
        colour = p->second[i].colour;
    }
  }

  gl->setColour(colour);
}

std::string ObsPlot::checkMarkerCriteria(int index)
{
  std::string marker = image;
  if (markercriteria.empty())
    return marker;

  map<std::string, vector<markerCriteria> >::iterator p =
      markercriteria.begin();

  for (; p != markercriteria.end(); p++) {
    float value = 0;
    if (not getValueForCriteria(index, p->first, value))
      continue;
    if (p->first == "RRR")
      adjustRRR(value);

    for (size_t i = 0; i < p->second.size(); i++) {
      if (p->second[i].match(value))
        marker = p->second[i].marker;
    }
  }

  return marker;
}

float ObsPlot::checkMarkersizeCriteria(int index)
{
  if (markersizecriteria.empty())
    return markerSize;

  float relSize = 1.0;
  map<std::string, vector<markersizeCriteria> >::iterator p =
      markersizecriteria.begin();

  for (; p != markersizecriteria.end(); p++) {
    float value = 0;
    if (not getValueForCriteria(index, p->first, value))
      continue;
    if (p->first == "RRR")
      adjustRRR(value);

    for (size_t i = 0; i < p->second.size(); i++) {
      if (p->second[i].match(value))
        relSize = p->second[i].size;
    }
  }

  return relSize * markerSize;
}

void ObsPlot::changeParamColour(const std::string& param, bool select)
{
  if (select) {
    colourCriteria cc;
    cc.sign = no_sign;
    cc.colour = Colour("red");
    ;
    colourcriteria[param].push_back(cc);
  } else {
    colourcriteria.erase(param);
  }
}

void ObsPlot::parameterDecode(const std::string& parameter, bool add)
{
  paramColour[parameter] = colour;
  if (parameter == "txtxtx")
    paramColour["tntntn"] = colour;
  if (parameter == "tntntn")
    paramColour["txtxtx"] = colour;
  if (parameter == "pwapwa")
    paramColour["hwahwa"] = colour;
  if (parameter == "hwahwa")
    paramColour["pwapwa"] = colour;

  std::string par;
  if (parameter == "txtxtx" || parameter == "tntntn")
    par = "txtn";
  else if (parameter == "pwapwa" || parameter == "hwahwa")
    par = "hwahwa";
  else if (parameter == "hw1Pw1" || parameter == "hw1hw1")
    par = "pw1hw1";
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

const std::vector<std::string>& ObsPlot::getFileNames() const
{
  return fileNames;
}
