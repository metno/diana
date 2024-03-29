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

#ifndef DIANA_UTIL_GEO_UTIL_H
#define DIANA_UTIL_GEO_UTIL_H 1

class Area;
class Rectangle;

namespace diutil {

float GreatCircleDistance(float lat1_deg, float lat2_deg, float lon1_deg, float lon2_deg);

/*! Convert area from rad to deg if projection is in degrees
 *
 * Used for backward compatibility after transition from proj 4 (radians) to proj >= 6 (degrees)
 */
void convertAreaRectToDegrees(Area& area);

//! Convert rect from rad to deg.
void convertRectToDegrees(Rectangle& rect);

} // namespace diutil

#endif // DIANA_UTIL_GEO_UTIL_H
