
#include "VcrossQtAxis.h"

#include <diField/VcrossUtil.h>

#define MILOGGER_CATEGORY "diana.VcrossAxis"
#include <miLogger/miLogging.h>

namespace vcross { namespace detail {

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
  switch (type) {
  case LINEAR:
    return x;
  case EXNER:
    return vcross::util::exnerFunction(x);
  }
  return x; // not reached
}

float Axis::functionInverse(float x) const
{
  switch (type) {
  case LINEAR:
    return x;
  case EXNER:
    return vcross::util::exnerFunctionInverse(x);
  }
  return x; // not reached
}

void Axis::calculateScale()
{
  //METLIBS_LOG_SCOPE();
  const float dp = (paintMax - paintMin),
      dv = (valueMax-valueMin),
      fdv = function(dv);
  scale = dp / fdv;
  //METLIBS_LOG_DEBUG(LOGVAL(dp) << LOGVAL(dv) << LOGVAL(fdv) << LOGVAL(scale) << LOGVAL(valueMin) << LOGVAL(valueMax));
}


float Axis::value2paint(float v, bool check) const
{
  //METLIBS_LOG_SCOPE();
  if (check and not legalValue(v))
    return -1e35;
  const float vv = (v-valueMin),
      fvv = function(vv),
      sfvv = scale * fvv;
  //METLIBS_LOG_DEBUG(LOGVAL(vv) << LOGVAL(fvv) << LOGVAL(sfvv) << LOGVAL(scale));
  return sfvv + paintMin;
}

float Axis::paint2value(float p, bool check) const
{
  METLIBS_LOG_SCOPE();
  if (check and not legalPaint(p))
    return -1e35;

  const float pp = (p-paintMin),
      spp = pp/scale,
      ispp = functionInverse(spp),
      v = ispp + valueMin;
  METLIBS_LOG_DEBUG(LOGVAL(p) << LOGVAL(pp) << LOGVAL(spp) << LOGVAL(ispp) << LOGVAL(v));
  return v;

  return functionInverse((p-paintMin)/scale) + valueMin;
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

bool Axis::setType( std::string t)
{

  if ( t == "exner" ) {
    type = EXNER;
    return true;
  }

  if (t == "linear" ) {
    type = LINEAR;
    return true;
  }

  return false;
}

bool Axis::setQuantity( std::string q)
{

  if (q == "time" ) {
    quantity = TIME;
    return true;
  }

  if (q == "distance" ) {
    quantity = DISTANCE;
    return true;
  }

  if (q == "Height" ) {
    quantity = HEIGHT;
    return true;
  }

  if (q == "Pressure" ) {
    quantity = PRESSURE;
    return true;
  }

  return false;
}

} /*namespace detail*/ } /*namespace vcross*/
