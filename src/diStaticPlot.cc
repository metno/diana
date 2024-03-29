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

#include "diStaticPlot.h"

#include "diana_config.h"

#include "diField/VcrossUtil.h"
#include "util/geo_util.h"
#include "diGLPainter.h"

#include <mi_fieldcalc/math_util.h>

#define MILOGGER_CATEGORY "diana.StaticPlot"
#include <miLogger/miLogging.h>

GridConverter StaticPlot::gc; // Projection-converter

StaticPlot::StaticPlot()
    : dirty(true)       // plotsize has changed
    , verticalLevel(-1) // current vertical level
    , gcd(0)            // great circle distance (corner to corner)
    , panning(false)    // panning in progress
{
}

StaticPlot::~StaticPlot()
{
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

float StaticPlot::getPhysDiagonal() const
{
  return miutil::absval(getPhysWidth(), getPhysHeight());
}

bool StaticPlot::setMapArea(const Area& area)
{
  const bool changed = pa_.setMapArea(area);
  if (changed)
    updateGcd();
  return changed;
}

bool StaticPlot::setPhysSize(int w, int h)
{
  const bool changed = pa_.setPhysSize(w, h);
  if (changed)
    updateGcd();
  return changed;
}

Area StaticPlot::findBestMatch(const Area& newa) const
{
  if (!getMapProjection().isDefined())
    return newa;

  const Area& area = pa_.getMapArea();
  if (!newa.P().isDefined() || getMapProjection() == newa.P())
    return area;

  const int npos = 4;
  float xpos[npos] = {area.R().x1, area.R().x1, area.R().x2, area.R().x2};
  float ypos[npos] = {area.R().y1, area.R().y2, area.R().y2, area.R().y1};
  if (!newa.P().convertPoints(getMapProjection(), npos, xpos, ypos)) {
    METLIBS_LOG_ERROR("findBestMatch: convertPoints error");
    return newa;
  }

  const float MAX = 100000000;
  float maxx = xpos[0], minx = xpos[0], maxy = ypos[0], miny = ypos[0];
  for (int i = 0; i < npos; i++) {
    // check for invalid numbers
    if (xpos[i] < -MAX || xpos[i] > MAX || ypos[i] < -MAX || ypos[i] > MAX) {
      return newa;
    }
    miutil::minimaximize(minx, maxx, xpos[i]);
    miutil::minimaximize(miny, maxy, ypos[i]);
  }

  return Area(newa.P(), Rectangle(minx, miny, maxx, maxy));
}

void StaticPlot::updateGcd()
{
  // lat3,lon3, point where ratio between window scale and geographical scale
  // is computed, set to Oslo coordinates, can be changed according to area
  const float lat3 = 60, lon3 = 10, lat1 = lat3 - 10, lat2 = lat3 + 10, lon1 = lon3 - 10, lon2 = lon3 + 10;

  // gcd is distance between lower left and upper right corners
  float ngcd = diutil::GreatCircleDistance(lat1, lat2, lon1, lon2);
  float x1, y1, x2, y2;
  GeoToPhys(lat1, lon1, x1, y1);
  GeoToPhys(lat2, lon2, x2, y2);
  float distGeoSq = miutil::absval2(x2 - x1, y2 - y1);
  float width = getPhysWidth(), height = getPhysHeight();
  float distWindowSq = miutil::absval2(width, height);
  float ratio = sqrtf(distWindowSq / distGeoSq);
  gcd = ngcd * ratio;
}

void StaticPlot::setPanning(bool p)
{
  panning = p;
}

bool StaticPlot::GeoToMap(int n, float* x, float* y) const
{
  return getMapProjection().convertFromGeographic(n, x, y);
}

bool StaticPlot::GeoToMap(int n, const float* x, const float* y, float* u, float* v) const
{
  return getMapProjection().convertVectors(Projection::geographic(), n, x, y, u, v);
}

XY StaticPlot::GeoToMap(const XY& lonlatdeg) const
{
  XY map(lonlatdeg);
  GeoToMap(1, &map.rx(), &map.ry());
  return map;
}

bool StaticPlot::MapToGeo(int n, float* x, float* y) const
{
  return getMapProjection().convertToGeographic(n, x, y);
}

XY StaticPlot::MapToGeo(const XY& map) const
{
  XY lonlatdeg(map);
  MapToGeo(1, &lonlatdeg.rx(), &lonlatdeg.ry());
  return lonlatdeg;
}

bool StaticPlot::PhysToGeo(float x, float y, float& lat, float& lon) const
{
  bool ret = false;
  if (pa_.hasPhysSize()) {
    PhysToMap(x, y, lon, lat);
    ret = MapToGeo(1, &lon, &lat);
  }
  return ret;
}

bool StaticPlot::GeoToPhys(float lat, float lon, float& x, float& y) const
{
  bool ret = false;
  if (pa_.hasPhysSize()) {
    ret = GeoToMap(1, &lon, &lat);
    MapToPhys(lon, lat, x, y);
  }
  return ret;
}
