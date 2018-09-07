/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013-2018 met.no

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

#include "VcrossQtAxis.h"

#include <diField/VcrossUtil.h>

#define MILOGGER_CATEGORY "vcross.Axis"
#include <miLogger/miLogging.h>

namespace vcross {
namespace detail {

static const float INVALID = -1e35;

Axis::Axis(bool h)
  : horizontal(h)
  , mType(LINEAR)
  , mQuantity(h ? DISTANCE : PRESSURE)
  , valueMin(0)
  , valueMax(1)
  , paintMin(0)
  , paintMax(1)
  , mScale(1)
{
  setDefaultLabel();
}

bool Axis::legalPaint(float p) const
{
  return vcross::util::value_between(p, paintMin, paintMax);
}

bool Axis::legalValue(float v) const
{
  return vcross::util::value_between(v, valueMin, valueMax);
}

bool Axis::legalData(float d) const
{
  return vcross::util::value_between(d, dataMin, dataMax);
}

float Axis::function(float x) const
{
  if (mQuantity == PRESSURE) {
    if (mType == EXNER)
      return vcross::util::exnerFunction(x);
    else if (mType == AMBLE)
      return vcross::util::ambleFunction(x);
  }
  if (mType == LOGARITHMIC) {
    if (x > 0)
      return std::log(x);
    else
      return INVALID;
  }
  return x; // LINEAR
}

float Axis::functionInverse(float x) const
{
  if (mQuantity == PRESSURE) {
    if (mType == EXNER)
      return vcross::util::exnerFunctionInverse(x);
    else if (mType == AMBLE)
      return vcross::util::ambleFunctionInverse(x);
  }
  if (mType == LOGARITHMIC) {
    if (x != INVALID)
      return std::exp(x);
    else
      return INVALID;
  }
  return x; // LINEAR
}

void Axis::calculateScale()
{
  const float dp = (paintMax - paintMin),
      dv = fValueMax()-fValueMin();
  mScale = dp / dv;
}

float Axis::value2paint(float v, bool check) const
{
  if (check and not legalValue(v))
    return INVALID;

  const float vv = function(v)-fValueMin(),
      sfvv = mScale * vv,
      p = sfvv + paintMin;
  return p;
}

float Axis::paint2value(float p, bool check) const
{
  if (check and not legalPaint(p))
    return INVALID;

  const float pp = (p-paintMin),
      spp = pp/mScale + fValueMin(),
      v = functionInverse(spp);
  return v;
}

bool Axis::zoomIn(float paint0, float paint1)
{
  METLIBS_LOG_SCOPE();
  if ((horizontal and paint0 > paint1) or (not horizontal and paint0 < paint1))
    std::swap(paint0, paint1);
  float v0 = paint2value(paint0), v1 = paint2value(paint1);
  const bool l0 = legalData(v0), l1 = legalData(v1);
  if (l0 or l1) {
    if (not l0)
      v0 = valueMin;
    if (not l1)
      v1 = valueMax;
    setValueRange(v0, v1);
    return true;
  } else {
    return false;
  }
}

bool Axis::zoomOut()
{
  const float delta = (valueMax - valueMin) * 0.3 * 0.5;
  const float vmin = vcross::util::constrain_value(valueMin - delta, dataMin, dataMax);
  const float vmax = vcross::util::constrain_value(valueMax + delta, dataMin, dataMax);
  if (vmin != valueMin or vmax != valueMax) {
    setValueRange(vmin, vmax);
    return true;
  } else {
    return false;
  }
}

bool Axis::pan(float delta)
{
  if (delta == 0)
    return false;
  if (not horizontal)
    delta = -delta;

  float vMax = paint2value(paintMax + delta, false);
  float vMin = paint2value(paintMin + delta, false);
  if (legalData(vMax) and legalData(vMin)) {
    setValueRange(vMin, vMax);
    return true;
  } else {
    return false;
  }
}

bool Axis::setType(const std::string& t)
{
  METLIBS_LOG_SCOPE(LOGVAL(t));
  if (mQuantity == PRESSURE) {
    if (t == "exner")
      setType(EXNER);
    else if (t == "amble")
      setType(AMBLE);
  }
  if (t == "log")
    setType(LOGARITHMIC);
  else if (t == "linear")
    setType(LINEAR);
  else
    return false;
  return true;
}

void Axis::setType(Type t)
{
  if (mQuantity != PRESSURE && (mType == EXNER || mType == AMBLE))
    mType = LINEAR;
  else
    mType = t;
  calculateScale();
}

bool Axis::setQuantity(const std::string& q)
{
  METLIBS_LOG_SCOPE(LOGVAL(q));
  if (q == "time")
    mQuantity = TIME;
  else if (q == "distance")
    mQuantity = DISTANCE;
  else if (q == "Altitude")
    mQuantity = ALTITUDE;
  else if (q == "Depth")
    mQuantity = DEPTH;
  else if (q == "Pressure")
    mQuantity = PRESSURE;
  else
    return false;

  setType(mType);
  setDefaultLabel();

  return true;
}

void Axis::setDefaultLabel()
{
  if (mQuantity == ALTITUDE || mQuantity == DEPTH)
    mLabel = "m";
  else if (mQuantity == PRESSURE)
    mLabel = "hPa";
  else
    mLabel.clear();
}

bool Axis::increasing() const
{
  if (mQuantity == PRESSURE || mQuantity == DEPTH)
    return false;
  // FIXME
  return true;
}

} /*namespace detail*/
} /*namespace vcross*/
