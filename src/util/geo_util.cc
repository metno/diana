/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2019-2022 met.no

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

#include "geo_util.h"

#include "diField/diArea.h"

#include <mi_fieldcalc/math_util.h>

#include <puDatatypes/miCoordinates.h>

#include <cmath> // hope for M_PI

namespace diutil {

static const double DEG_TO_RAD = M_PI/180;

//! haversine function
double hav(double theta)
{
  return miutil::square(std::sin(theta / 2));
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

void convertRectToDegrees(Rectangle& r)
{
  const float RAD_TO_DEG = 180 / M_PI;
  r.x1 *= RAD_TO_DEG;
  r.y1 *= RAD_TO_DEG;
  r.x2 *= RAD_TO_DEG;
  r.y2 *= RAD_TO_DEG;
}

void convertAreaRectToDegrees(Area& area)
{
  if (area.P().isDegree()) {
    auto r = area.R();
    convertRectToDegrees(r);
    area.setR(r);
  }
}

} // namespace diutil
