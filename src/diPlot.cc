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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diPlot.h"

#include "diGLPainter.h"
#include "diKVListPlotCommand.h"
#include "diPlotModule.h"
#include "util/math_util.h"
#include "util/string_util.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.Plot"
#include <miLogger/miLogging.h>

using namespace ::miutil;
using namespace ::std;

GridConverter StaticPlot::gc; // Projection-converter

StaticPlot::StaticPlot()
  : mPhys(0, 0)       // physical plot size
  , mPhysToMapScale(1, 1)
  , dirty(true)       // plotsize has changed
  , verticalLevel(-1) // current vertical level
  , gcd(0)            // great circle distance (corner to corner)
  , panning(false)    // panning in progress
{
}

StaticPlot::~StaticPlot()
{
}

Plot::Plot()
  : enabled(true)
{
  METLIBS_LOG_SCOPE();
}

void Plot::setCanvas(DiCanvas* canvas)
{
}

bool Plot::operator==(const Plot &rhs) const{
  return false;
}

StaticPlot* Plot::getStaticPlot() const
{
  return PlotModule::instance()->getStaticPlot();
}

void Plot::setEnabled(bool e)
{
  enabled = e;
}

void StaticPlot::setBgColour(const std::string& cn)
{
  backgroundColour = Colour(cn);
  backContrastColour = backgroundColour.contrastColour();
}

const Colour& StaticPlot::notBackgroundColour(const Colour& c) const
{
  if (c == getBackgroundColour())
    return getBackContrastColour();
  else
    return c;
}

void StaticPlot::setMapArea(const Area& a)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_INFO(LOGVAL(a));
  if (!a.P().isDefined())
    return;

  // change plot-Area
  area = a;
  setDirty(true);
}

inline float oneIf0(float f)
{
  return (f <= 0) ? 1 : f;
}

void StaticPlot::updatePhysToMapScale()
{
  mPhysToMapScale = XY(oneIf0(plotsize.width()) / oneIf0(mPhys.x()),
      oneIf0(plotsize.height()) / oneIf0(mPhys.y()));
}

void StaticPlot::setMapPlotSize(const Rectangle& mr, const Rectangle& pr)
{
  if (plotsize != pr) {
    plotsize = pr;
    updatePhysToMapScale();
    setDirty(true);
  }
  if (maprect != mr) {
    maprect= mr;
    setDirty(true);
  }
}

void StaticPlot::setPhysSize(int w, int h)
{
  mPhys = diutil::PointI(w, h);
  updatePhysToMapScale();
  setDirty(true);
}

float StaticPlot::getPhysDiagonal() const
{
  return diutil::absval(getPhysWidth(), getPhysHeight());
}

Area StaticPlot::findBestMatch(const Area& newa){

  if (!area.P().isDefined())
    return newa;

  const int npos= 4;
  float xpos[npos], ypos[npos];
  xpos[0]= area.R().x1;
  ypos[0]= area.R().y1;
  xpos[1]= area.R().x1;
  ypos[1]= area.R().y2;
  xpos[2]= area.R().x2;
  ypos[2]= area.R().y2;
  xpos[3]= area.R().x2;
  ypos[3]= area.R().y1;

  if (!gc.getPoints(area.P(),newa.P(),npos,xpos,ypos)) {
    METLIBS_LOG_ERROR("findBestMatch: getPoints error");
    return newa;
  }

  const float MAX=100000000;
  float maxx= -100000000, maxy= -1000000000,
    minx= 100000000, miny= 1000000000;
  for (int i=0; i<npos; i++){
    // check for impossible numbers
    if (xpos[i] < -MAX || xpos[i] > MAX){
      return newa;
    }
    if (xpos[i] < minx) minx= xpos[i];
    if (ypos[i] < miny) miny= ypos[i];
    if (xpos[i] > maxx) maxx= xpos[i];
    if (ypos[i] > maxy) maxy= ypos[i];
  }

  Area a = newa;
  a.setR(Rectangle(minx,miny,maxx,maxy));
  return a;
}


void StaticPlot::setDirty(bool f)
{
  //METLIBS_LOG_SCOPE(LOGVAL(f));
  dirty= f;
}


void StaticPlot::updateGcd(DiGLPainter* gl)
{
  gl->setVpGlSize(mPhys.x(), mPhys.y(), plotsize.width(), plotsize.height());

  // lat3,lon3, point where ratio between window scale and geographical scale
  // is computed, set to Oslo coordinates, can be changed according to area
  const float lat3 = 60,
      lon3 = 10,
      lat1 = lat3 - 10,
      lat2 = lat3 + 10,
      lon1 = lon3 - 10,
      lon2 = lon3 + 10;

  //gcd is distance between lower left and upper right corners
  float ngcd = diutil::GreatCircleDistance(lat1, lat2, lon1, lon2);
  float x1, y1, x2, y2;
  GeoToPhys(lat1, lon1, x1, y1);
  GeoToPhys(lat2, lon2, x2, y2);
  float distGeoSq = (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
  float width = getPhysWidth(), height = getPhysHeight();
  float distWindowSq = width * width + height * height;
  float ratio = sqrtf(distWindowSq / distGeoSq);
  gcd = ngcd * ratio;
}

std::string Plot::getEnabledStateKey() const
{
  return miutil::mergeKeyValue(getPlotInfo());
}

void Plot::setPlotInfo(const miutil::KeyValue_v& kvs)
{
  // fill poptions with values from pinfo
  ooptions.clear();
  PlotOptions::parsePlotOption(kvs, poptions, ooptions);

  enabled= poptions.enabled;
}

miutil::KeyValue_v Plot::getPlotInfo(int n) const
{
  miutil::KeyValue_v::const_iterator itB = ooptions.begin(), itE = itB;
  std::advance(itE, std::min(size_t(n), ooptions.size()));
  return miutil::KeyValue_v(itB, itE);
}

void Plot::getAnnotation(string& s, Colour& c) const
{
  c = Colour("black");
  s = getPlotName();
}

// panning in progress
void StaticPlot::panPlot(bool b)
{
  panning= b;
}


bool StaticPlot::GeoToMap(int n, float* x, float* y) const
{
  return gc.geo2xy(getMapArea(), n, x, y);
}

bool StaticPlot::GeoToMap(int n, const float* x, const float* y,
    float* u, float* v) const
{
  return gc.geov2xy(getMapArea(), n, x, y, u, v);
}

XY StaticPlot::GeoToMap(const XY& lonlatdeg) const
{
  XY map(lonlatdeg);
  GeoToMap(1, &map.rx(), &map.ry());
  return map;
}

bool StaticPlot::MapToGeo(int n, float* x, float* y) const
{
  return gc.xy2geo(getMapArea(), n, x, y);
}

XY StaticPlot::MapToGeo(const XY& map) const
{
  XY lonlatdeg(map);
  MapToGeo(1, &lonlatdeg.rx(), &lonlatdeg.ry());
  return lonlatdeg;
}

bool StaticPlot::PhysToGeo(const float x, const float y, float& lat, float& lon) const
{
  bool ret = false;
  if (hasPhysSize()) {
    PhysToMap(x, y, lon, lat);
    ret = MapToGeo(1, &lon, &lat);
  }
  return ret;
}

bool StaticPlot::GeoToPhys(float lat, float lon, float& x, float& y) const
{
  bool ret = false;
  if (hasPhysSize()) {
    ret = GeoToMap(1, &lon, &lat);
    MapToPhys(lon, lat, x, y);
  }
  return ret;
}

XY StaticPlot::PhysToMap(const XY& phys) const
{
  if (hasPhysSize())
    return phys*mPhysToMapScale + XY(getPlotSize().x1, getPlotSize().y1);
  else
    return phys;
}

XY StaticPlot::MapToPhys(const XY& map) const
{
  if (hasPhysSize())
    return (map - XY(getPlotSize().x1, getPlotSize().y1)) / mPhysToMapScale;
  else
    return map;
}

bool StaticPlot::ProjToMap(const Projection& srcProj, int n, float* x, float* y) const
{
  return gc.getPoints(srcProj, getMapArea().P(), n, x, y);
}

bool StaticPlot::ProjToMap(const Area& srcArea, int n,
    const float* x, const float* y, float* u, float* v) const
{
  return gc.getVectors(srcArea, getMapArea().P(), n, x, y, u, v);
}

bool StaticPlot::MapToProj(const Projection& targetProj, int n, float* x, float* y) const
{
  return gc.getPoints(getMapArea().P(), targetProj, n, x, y);
}

bool StaticPlot::MapToProj(const Projection& targetProj, int n, diutil::PointD* xy) const
{
  return targetProj.convertPoints(getMapArea().P(), n, xy);
}
