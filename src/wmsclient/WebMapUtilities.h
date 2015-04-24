/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015 MET Norway

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

#ifndef WebMapUtilities_h
#define WebMapUtilities_h 1

#include <puTools/miTime.h>

#include <QString>

#include <set>
#include <string>

class Projection;
class Rectangle;


#define QDOM_FOREACH_CHILD(eChild, eParent, name)                       \
  for (QDomElement eChild = eParent.firstChildElement(name);            \
       !eChild.isNull(); eChild = eChild.nextSiblingElement(name))


namespace diutil {

extern const double WMTS_M_PER_PIXEL;
extern const double WMTS_EARTH_RADIUS_M;

inline std::string qs(const QString& q)
{ return q.toStdString(); }
inline QString sq(const std::string& s)
{ return QString::fromStdString(s); }

std::string textAfter(const std::string& haystack, const std::string& key);

double metersPerUnit(const Projection& proj);

Projection projectionForCRS(const std::string& crs);

// ========================================================================

struct WmsTime {
  enum Resolution {
    INVALID, YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, SUBSECOND
  };

  Resolution resolution;

  int year, month, day, hour, minute;
  double second;

  int timezone_hours, timezone_minutes;

  WmsTime();
};

struct WmsInterval {
  WmsTime::Resolution resolution;
  double year, month, day, hour, minute, second;

  WmsInterval();
};

namespace detail {
bool parseDecimals(int& value, const std::string& text, size_t& idx,
    int count, bool count_is_minimum = false);
bool parseDouble(double& value, const std::string& text, size_t& idx);
}
WmsTime parseWmsIso8601(const std::string& wmsIso8601);
miutil::miTime to_miTime(const WmsTime& wmstime);
std::string to_wmsIso8601(const miutil::miTime& t, WmsTime::Resolution resolution);

WmsInterval parseWmsIso8601Interval(const std::string& text);

QStringList expandWmsTimes(const QString& values);
QStringList expandWmsValues(const QString& values);

// ========================================================================

struct tilexy {
  int x, y;
  tilexy(int x_, int y_) : x(x_), y(y_) { }
  bool operator<(const tilexy& o) const
    { return x < o.x || (x == o.x && y < o.y); }
};

typedef std::set<tilexy> tilexy_s;

void select_tiles(tilexy_s& tiles,
    int ix0, int nx, float x0, float dx, int iy0, int ny, float y0, float dy,
    const Projection& p_tiles, const Rectangle& r_view, const Projection& p_view);

} // namespace diutil


#endif // WebMapUtilities_h
