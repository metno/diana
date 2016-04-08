
#ifndef DIFIELD_DIFLIGHTLEVEL_H
#define DIFIELD_DIFLIGHTLEVEL_H 1

#include <string>

namespace FlightLevel {

std::string getPressureLevel(const std::string& flightlevel);
std::string getFlightLevel(const std::string& pressurelevel);

} // namespace FlightLevel

#endif // DIFIELD_DIFLIGHTLEVEL_H
