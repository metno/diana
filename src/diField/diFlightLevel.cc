
#include "diFlightLevel.h"

#include "diMetConstants.h"

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
  using namespace MetNo::Constants;
  for (int i=0; i<nLevelTable; i++) {
    std::ostringstream pstr, fstr, fstr_old;
    pstr << pLevelTable[i];
    fstr << std::setw(3) << std::setfill('0') << fLevelTable[i];
    fstr_old << std::setw(3) << std::setfill('0') << fLevelTable_old[i];

    pLevel2flightLevel[pstr.str()+"hPa"] = "FL"+fstr.str();
    flightLevel2pLevel["FL"+fstr.str() ] = pstr.str()+"hPa";
    //obsolete table
    flightLevel2pLevel["FL"+fstr_old.str()] = pstr.str()+"hPa";
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

  METLIBS_LOG_WARN(" Flightlevel: "<<flightlevel<<". No pressure level found.");
  return flightlevel;
}

std::string getFlightLevel(const std::string& pressurelevel)
{
  METLIBS_LOG_SCOPE(LOGVAL(pressurelevel));
  buildPLevelsToFlightLevelsTable();

  const std::map<std::string, std::string>::const_iterator it = pLevel2flightLevel.find(pressurelevel);
  if (it != pLevel2flightLevel.end())
    return it->second;

  METLIBS_LOG_WARN(" pressurelevel: "<<pressurelevel<<". No pressure level found.");
  return pressurelevel;
}

} // namespace FlightLevel
