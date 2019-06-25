/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2018 met.no

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

#include "VcrossUtil.h"

#include "../util/string_util.h"
#include "../util/time_util.h"

#include <mi_fieldcalc/MetConstants.h>

#include <fimex/UnitsConverter.h>
#include <fimex/UnitsException.h>

#include <udunits2.h>

#include <cmath>
#include <istream>

#define MILOGGER_CATEGORY "vcross.Util"
#include "miLogger/miLogging.h"

namespace vcross {
namespace util {

static const float ambleLimit = 500, ambleLimitLog = std::log(ambleLimit), ambleD = 10;
static const float ambleDelta = (std::log(ambleLimit - ambleD) - ambleLimitLog) / ambleD;

float ambleFunction(float p)
{
  if (p > ambleLimit)
    return std::log(p);
  else
    return ambleLimitLog + (ambleLimit - p) * ambleDelta;
}

float ambleFunctionInverse(float x)
{
  if (x > ambleLimitLog)
    return std::exp(x);
  else
    return ambleLimit - (x - ambleLimitLog) / ambleDelta;
}

float exnerFunction(float p)
{
  // cp, p0inv, and kappa are defined in metlibs/diField/diMetConstants.h
  return miutil::constants::cp * std::pow(p * miutil::constants::p0inv, miutil::constants::kappa);
}

float exnerFunctionInverse(float e)
{
  // invert e=cp*exp(kappa*log(p*p0inv))
  return std::exp(std::log(e / miutil::constants::cp) / miutil::constants::kappa) * miutil::constants::p0;
}

float coriolisFactor(float lat /* radian */)
{
  // FIXME see diField/diProjection.cc: Projection::getMapRatios
  const float EARTH_OMEGA = 2*M_PI / (24*3600); // earth rotation in radian / s
  return 2 * EARTH_OMEGA * sin(lat);
}

int stepped_index(int idx, int step, int max)
{
  if (step == 0 || max == 0)
    return idx;
  idx += step;
  if (idx < 0 or idx >= max) {
    const int astep = std::abs(step), smax = (max % astep);
    int mmax = max;
    if (smax != 0)
      mmax += astep - smax;
    while (idx < 0)
      idx += mmax;
    while (idx >= mmax)
      idx -= mmax;
  }
  return idx;
}

bool step_index(int& idx, int step, int max)
{
  int oidx = idx;
  idx = stepped_index(idx, step, max);
  return idx != oidx;
}

UnitConvertibility unitConvertibility(const std::string& ua, const std::string& ub)
{
  if (ua == ub)
    return UNITS_IDENTICAL;

  MetNoFimex::Units units;
  if (not units.areConvertible(ua, ub))
    return UNITS_MISMATCH;

  MetNoFimex::UnitsConverter_p uconv = units.getConverter(ua, ub);
  if (not uconv)
    return UNITS_MISMATCH;
  if (not uconv->isLinear())
    return UNITS_CONVERTIBLE;

  double scale=0, offset=12354;
  uconv->getScaleOffset(scale, offset);
  if (scale == 1 and offset == 0)
    return UNITS_IDENTICAL;
  return UNITS_LINEAR;
}

static ut_system* utSystem = 0;
static void getUtSystem()
{
  if (not utSystem) {
    ut_set_error_message_handler(&ut_ignore);
    utSystem = ut_read_xml(0);
    MetNoFimex::handleUdUnitError(ut_get_status(), "init");
  }
}

std::string unitsMultiplyDivide(const std::string& ua, const std::string& ub, bool multiply)
{
  if (ub.empty())
    return ua;

  try {
    getUtSystem();

    std::shared_ptr<ut_unit> aUnit(ut_parse(utSystem, ua.empty() ? "1" : ua.c_str(), UT_UTF8), ut_free);
    MetNoFimex::handleUdUnitError(ut_get_status(), ua);
    std::shared_ptr<ut_unit> bUnit(ut_parse(utSystem, ub.c_str(), UT_UTF8), ut_free);
    MetNoFimex::handleUdUnitError(ut_get_status(), ub);
    std::shared_ptr<ut_unit> mUnit((multiply ? ut_multiply : ut_divide)(aUnit.get(), bUnit.get()), ut_free);
    MetNoFimex::handleUdUnitError(ut_get_status(), "result");

    char buf[128];
    const int len = ut_format(mUnit.get(), buf, sizeof(buf), UT_ASCII | UT_DEFINITION);
    if (len >= 0 and len < int(sizeof(buf)))
      return buf;
  } catch (MetNoFimex::UnitException& ue) {
    //METLIBS_LOG_ERROR(ue.what());
  }
  return "";
}

std::string unitsRoot(const std::string& u, int root)
{
  if (u.empty())
    return u;

  try {
    getUtSystem();

    std::shared_ptr<ut_unit> iUnit(ut_parse(utSystem, u.c_str(), UT_UTF8), ut_free);
    MetNoFimex::handleUdUnitError(ut_get_status(), u);
    std::shared_ptr<ut_unit> rUnit(ut_root(iUnit.get(), root), ut_free);
    MetNoFimex::handleUdUnitError(ut_get_status(), "result");

    char buf[128];
    const int len = ut_format(rUnit.get(), buf, sizeof(buf), UT_ASCII | UT_DEFINITION);
    if (len >= 0 and len < int(sizeof(buf)))
      return buf;
  } catch (MetNoFimex::UnitException& ue) {
    //METLIBS_LOG_ERROR(ue.what());
  }
  return "";
}

// ------------------------------------------------------------------------

MetNoFimex::UnitsConverter_p unitConverter(const std::string& ua, const std::string& ub)
{
  return MetNoFimex::Units().getConverter(ua, ub);
}

// ------------------------------------------------------------------------

Values_cp unitConversion(Values_cp valuesIn, const std::string& unitIn, const std::string& unitOut)
{
  if (not valuesIn or unitsIdentical(unitIn, unitOut))
    return valuesIn;
  MetNoFimex::UnitsConverter_p uconv = util::unitConverter(unitIn, unitOut);
  if (not uconv)
    return Values_cp();
  const float udI = valuesIn->undefValue();
  Values_p valuesOut = std::make_shared<Values>(valuesIn->shape(), false);

  const size_t volume = valuesOut->shape().volume();
  const Values::ValueArray vIn = valuesIn->values();
  Values::ValueArray vOut = valuesOut->values();
  for (size_t i = 0; i < volume; ++i) {
    const float pX = vIn[i];
    const float v = (pX != udI) ? uconv->convert(pX): udI;
    vOut[i] = v;
  }
  return valuesOut;
}

// ------------------------------------------------------------------------

float unitConversion(float valueIn, const std::string& unitIn, const std::string& unitOut)
{
  if (unitsIdentical(unitIn, unitOut))
    return valueIn;
  MetNoFimex::UnitsConverter_p uconv = util::unitConverter(unitIn, unitOut);
  if (not uconv)
    return valueIn;
  return uconv->convert(valueIn);
}

// ------------------------------------------------------------------------

bool isCommentLine(const std::string& line)
{
  return not line.empty() and line[0] == '#';
}

// ------------------------------------------------------------------------

bool nextline(std::istream& config, std::string& line)
{
  while (std::getline(config, line)) {
    if (not (line.empty() or line[0] == '#'))
      return true;
  }
  return false;
}

// ------------------------------------------------------------------------

int vc_select_cs(const std::string& ucs, Inventory_cp inv)
{
  int csidx = -1;
  for (Crossection_cp cs : inv->crossections) {
    csidx += 1;
    if (cs->label() == ucs)
      return csidx;
  }
  return -1;
}

// ------------------------------------------------------------------------

const char SECONDS_SINCE_1970[] = "seconds since 1970-01-01 00:00:00";
const char DAYS_SINCE_1900[] = "days since 1900-01-01 00:00:00";

static const miutil::miTime time0 = miutil::unix_t0, day0("1900-01-01 00:00:00");

Time from_miTime(const miutil::miTime& mitime)
{
  if (mitime.undef())
    return Time();
  else if (mitime >= time0)
    return Time(SECONDS_SINCE_1970, miutil::miTime::secDiff(mitime, time0));
  else
    return Time(DAYS_SINCE_1900, miutil::miTime::hourDiff(mitime, day0)/24);
}

// ------------------------------------------------------------------------

miutil::miTime to_miTime(const std::string& unit, Time::timevalue_t value)
{
  if (!unit.empty()) {
    if (diutil::startswith(unit, "days")) {
      if (MetNoFimex::UnitsConverter_p uconv = unitConverter(unit, DAYS_SINCE_1900))
        return miutil::addHour(day0, 24 * uconv->convert(value));
    }
    if (MetNoFimex::UnitsConverter_p uconv = unitConverter(unit, SECONDS_SINCE_1970))
      return miutil::addSec(time0, uconv->convert(value));
  }
  return miutil::miTime();
}

// ------------------------------------------------------------------------

WindArrowFeathers countFeathers(float ff /* knots */)
{
  WindArrowFeathers waf;
  if (ff < 182.49) {
    waf.n05 = int(ff * 0.2 + 0.5);
    waf.n50 = waf.n05 / 10;
    waf.n05 -= waf.n50 * 10;
    waf.n10 = waf.n05 / 2;
    waf.n05 -= waf.n10 * 2;
  } else if (ff < 190.) {
    waf.n50 = 3;
    waf.n10 = 3;
  } else if (ff < 205.) {
    waf.n50 = 4;
  } else if (ff < 225.) {
    waf.n50 = 4;
    waf.n10 = 1;
  } else {
    waf.n50 = 5;
  }
  return waf;
}

float FL_to_hPa(float fl)
{
  const double a = miutil::constants::geo_altitude_from_FL(fl);
  return miutil::constants::ICAO_pressure_from_geo_altitude(a);
}

float hPa_to_FL(float hPa)
{
  const double a = miutil::constants::ICAO_geo_altitude_from_pressure(hPa);
  return miutil::constants::FL_from_geo_altitude(a);
}

} // namespace util
} // namespace vcross
