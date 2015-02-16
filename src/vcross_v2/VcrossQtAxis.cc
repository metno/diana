
#include "VcrossQtAxis.h"

#include <diField/VcrossUtil.h>

#define MILOGGER_CATEGORY "vcross.Axis"
#include <miLogger/miLogging.h>

namespace vcross {
namespace detail {

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
  if (mType == EXNER and mQuantity == PRESSURE)
    return vcross::util::exnerFunction(x);
  return x; // LINEAR
}

float Axis::functionInverse(float x) const
{
  if (mType == EXNER and mQuantity == PRESSURE)
    return vcross::util::exnerFunctionInverse(x);
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
    return -1e35;

  const float vv = function(v)-fValueMin(),
      sfvv = mScale * vv,
      p = sfvv + paintMin;
  return p;
}

float Axis::paint2value(float p, bool check) const
{
  if (check and not legalPaint(p))
    return -1e35;

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
  if (t == "exner")
    mType = EXNER;
  else if (t == "linear")
    mType = LINEAR;
  else
    return false;

  calculateScale();
  return true;
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
  else if (q == "Pressure")
    mQuantity = PRESSURE;
  else
    return false;

  return true;
}

bool Axis::increasing() const
{
  return mQuantity != PRESSURE; // FIXME
}

} /*namespace detail*/
} /*namespace vcross*/
