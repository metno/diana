
#include "math_util.h"

#include <puDatatypes/miCoordinates.h>

namespace diutil {

static const double DEG_TO_RAD = M_PI/180;

//! haversine function
double hav(double theta)
{
  return square(std::sin(theta/2));
}

float GreatCircleDistance(float lat1_deg, float lat2_deg, float lon1_deg, float lon2_deg)
{
  // great-circle distance, see http://en.wikipedia.org/wiki/Great-circle_distance
#if 1
  const double lat1 = lat1_deg * DEG_TO_RAD, lat2 = lat2_deg * DEG_TO_RAD;
  const double dlon = (lon2_deg - lon1_deg) * DEG_TO_RAD;
#if 1
  return EARTH_RADIUS_M * 2 * std::asin(std::sqrt(hav(lat2 - lat1) + cos(lat1)*cos(lat2)*hav(dlon)));
#else
  // fimex mifi_great_circle_angle uses this formula, but it also uses double
  return EARTH_RADIUS_M * acos(sin(lat0)*sin(lat1) + cos(lat0)*cos(lat1)*cos(dlon));
#endif
#else
  // this uses a much more complex formula
  return LonLat::fromDegrees(lon1_deg, lat1_deg).distanceTo(LonLat::fromDegrees(lon2_deg, lat2_deg));
#endif
}

} // namespace diutil
