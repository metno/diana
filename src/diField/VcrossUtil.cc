
#include "VcrossUtil.h"

#include <puTools/mi_boost_compatibility.hh>
#include "diMetConstants.h"
#include <udunits2.h>
#include <boost/foreach.hpp>
#include <cmath>

#define MILOGGER_CATEGORY "vcross.Util"
#include "miLogger/miLogging.h"

namespace vcross {
namespace util {

float exnerFunction(float p)
{
  // cp, p0inv, and kappa are defined in metlibs/diField/diMetConstants.h
  return MetNo::Constants::cp * std::pow(p * MetNo::Constants::p0inv, MetNo::Constants::kappa);
}

float exnerFunctionInverse(float e)
{
  // invert e=cp*exp(kappa*log(p*p0inv))
  return std::exp(std::log(e/MetNo::Constants::cp)/MetNo::Constants::kappa)*MetNo::Constants::p0;
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

  boost::shared_ptr<MetNoFimex::UnitsConverter> uconv = units.getConverter(ua, ub);
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

    boost::shared_ptr<ut_unit> aUnit(ut_parse(utSystem, ua.empty() ? "1" : ua.c_str(), UT_UTF8), ut_free);
    MetNoFimex::handleUdUnitError(ut_get_status(), ua);
    boost::shared_ptr<ut_unit> bUnit(ut_parse(utSystem, ub.c_str(), UT_UTF8), ut_free);
    MetNoFimex::handleUdUnitError(ut_get_status(), ub);
    boost::shared_ptr<ut_unit> mUnit((multiply ? ut_multiply : ut_divide)(aUnit.get(), bUnit.get()), ut_free);
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

    boost::shared_ptr<ut_unit> iUnit(ut_parse(utSystem, u.c_str(), UT_UTF8), ut_free);
    MetNoFimex::handleUdUnitError(ut_get_status(), u);
    boost::shared_ptr<ut_unit> rUnit(ut_root(iUnit.get(), root), ut_free);
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

UnitsConverterPtr unitConverter(const std::string& ua, const std::string& ub)
{
  return UnitsConverterPtr(MetNoFimex::Units().getConverter(ua, ub));
}

// ------------------------------------------------------------------------

Values_cp unitConversion(Values_cp valuesIn, const std::string& unitIn, const std::string& unitOut)
{
  if (not valuesIn or unitsIdentical(unitIn, unitOut))
    return valuesIn;
  util::UnitsConverterPtr uconv = util::unitConverter(unitIn, unitOut);
  if (not uconv)
    return Values_cp();
  const float udI = valuesIn->undefValue();
  Values_p valuesOut = miutil::make_shared<Values>(valuesIn->shape(), false);

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
  util::UnitsConverterPtr uconv = util::unitConverter(unitIn, unitOut);
  if (not uconv)
    return valueIn;
  return uconv->convert(valueIn);
}

// ------------------------------------------------------------------------

bool startsWith(const std::string& text, const std::string& start)
{
  return text.substr(0, start.size()) == start;
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
  BOOST_FOREACH(Crossection_cp cs, inv->crossections) {
    csidx += 1;
    if (cs->label() == ucs)
      return csidx;
  }
  return -1;
}

// ------------------------------------------------------------------------

const char SECONDS_SINCE_1970[] = "seconds since 1970-01-01 00:00:00";
const char DAYS_SINCE_1900[] = "days since 1900-01-01 00:00:00";

static const miutil::miTime time0("1970-01-01 00:00:00"), day0("1900-01-01 00:00:00");

Time from_miTime(const miutil::miTime& mitime)
{
  if (mitime >= time0)
    return Time(SECONDS_SINCE_1970, miutil::miTime::secDiff(mitime, time0));
  else
    return Time(DAYS_SINCE_1900, miutil::miTime::hourDiff(mitime, day0)/24);
}

// ------------------------------------------------------------------------

miutil::miTime to_miTime(const std::string& unit, Time::timevalue_t value)
{
  if (unit.empty())
    return miutil::miTime();

  if (startsWith(unit, "days")) {
    if (UnitsConverterPtr uconv = unitConverter(unit, DAYS_SINCE_1900)) {
      miutil::miTime t(day0);
      t.addHour(value*24);
      return t;
    }
  }
  UnitsConverterPtr uconv = unitConverter(unit, SECONDS_SINCE_1970);
  if (uconv) {
    value = uconv->convert(value);
    miutil::miTime t(time0);
    t.addSec(value);
    return t;
  } else {
    return miutil::miTime();
  }
}

// ------------------------------------------------------------------------

WindArrowFeathers countFeathers(float ff)
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

} // namespace util
} // namespace vcross
