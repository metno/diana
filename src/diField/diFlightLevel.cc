
#include "diFlightLevel.h"

#include "diField/VcrossUtil.h"
#include "util/string_util.h"

#include <mi_fieldcalc/MetConstants.h>

#include <puTools/miStringFunctions.h>

#include <cmath>
#include <iomanip>
#include <map>
#include <sstream>

#define MILOGGER_CATEGORY "diField.FieldFunctions"
#include "miLogger/miLogging.h"

namespace {

std::map<std::string,std::string> pLevel2flightLevel;
std::map<std::string,std::string> flightLevel2pLevel;

void buildPLevelsToFlightLevelsTable()
{
  if (!pLevel2flightLevel.empty() && !flightLevel2pLevel.empty())
    return;

  pLevel2flightLevel.clear();
  flightLevel2pLevel.clear();

  // make conversion tables between pressureLevels and flightLevels
  using namespace miutil::constants;
  for (int i=0; i<nLevelTable; i++) {
    std::ostringstream pstr, fstr, fstr_old;
    pstr << pLevelTable[i] << "hPa";
    fstr << "FL" << std::setw(3) << std::setfill('0') << fLevelTable[i];
    fstr_old << "FL" << std::setw(3) << std::setfill('0') << fLevelTable_old[i];

    pLevel2flightLevel[pstr.str()] = fstr.str();
    flightLevel2pLevel[fstr.str()] = pstr.str();
    flightLevel2pLevel[fstr_old.str()] = pstr.str(); // obsolete table
  }
}
} // anonymous namespace

namespace FlightLevel {

std::string getPressureLevel(const std::string& flightlevel)
{
  METLIBS_LOG_SCOPE(LOGVAL(flightlevel));
  buildPLevelsToFlightLevelsTable();

  const std::map<std::string, std::string>::const_iterator it = flightLevel2pLevel.find(flightlevel);
  if (it != flightLevel2pLevel.end())
    return it->second;

  if (flightlevel.size() >= 5 && diutil::startswith(flightlevel, "FL")) {
    const float FL = miutil::to_float(flightlevel.substr(2));
    const float ROUND = 5;
    const float hPa = ROUND * std::round(vcross::util::FL_to_hPa(FL) / ROUND + 0.5f);
    return miutil::from_number(hPa) + "hPa";
  } else {
    METLIBS_LOG_WARN("Flightlevel: " << flightlevel << ". No pressure level found.");
  }
  return flightlevel;
}

std::string getFlightLevel(const std::string& pressurelevel)
{
  METLIBS_LOG_SCOPE(LOGVAL(pressurelevel));
  buildPLevelsToFlightLevelsTable();

  const std::map<std::string, std::string>::const_iterator it = pLevel2flightLevel.find(pressurelevel);
  if (it != pLevel2flightLevel.end())
    return it->second;

  if (diutil::endswith(pressurelevel, "hPa")) {
    const float hPa = miutil::to_float(pressurelevel.substr(0, pressurelevel.size() - 3));
    return "FL" + miutil::from_number(vcross::util::hPa_to_FL(hPa));
  } else {
    METLIBS_LOG_WARN("Pressure level: " << pressurelevel << ". No flightlevel found.");
  }
  return pressurelevel;
}

} // namespace FlightLevel
