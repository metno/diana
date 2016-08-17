/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 20016 met.no

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
#ifndef DISTATIONINFO_H
#define DISTATIONINFO_H

#include <string>

struct stationInfo {
    std::string name;
    std::string url;
    float lat;
    float lon;
    stationInfo(const std::string& n, float lo, float la)
      : name(n), lat(la), lon(lo) { }
};

namespace diutil {

/** Equality comparator for stationInfo, comparing by name. */
struct eq_StationName {
  eq_StationName(const std::string& name) : name_(name) { }
  bool operator()(const stationInfo& si) const
    { return si.name == name_; }
  const std::string& name_;
};

/** Less-than comparator for stationInfo, comparing by name. */
struct lt_StationName {
  bool operator()(const stationInfo& s1, const stationInfo& s2) const
    { return s1.name < s2.name; }
};

} // namespace diutil

#endif // DISTATIONINFO_H
