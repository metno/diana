/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2020-2022 met.no

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

#ifndef diana_diField_diFieldVerticalAxes_h
#define diana_diField_diFieldVerticalAxes_h

#include <map>
#include <string>
#include <vector>

class FieldVerticalAxes {
public:
  /// Vertical coordinate types
  enum VerticalType {
    vctype_none, ///< surface and other single level fields
    vctype_pressure, ///< pressure levels
    vctype_hybrid, ///< model levels, eta(Hirlam,ECMWF,...) and norlam_sigma
    vctype_atmospheric, ///< other model levels (needing pressure in each level)
    vctype_isentropic, ///< isentropic (constant pot.temp.) levels
    vctype_oceandepth, ///< ocean model depth levels
    vctype_other ///< other multilevel fields (without dedicated compute functions)
  };

  struct Zaxis_info {
    std::string name;
    VerticalType vctype;
    std::string levelprefix;
    std::string levelsuffix;
    bool index;

    Zaxis_info()
        : vctype(vctype_none), index(false) {}
  };

  static std::string FIELD_VERTICAL_COORDINATES_SECTION();

  static bool parseVerticalSetup(const std::vector<std::string>& lines, std::vector<std::string>& errors);

  static const Zaxis_info* findZaxisInfo(const std::string& name);

  static VerticalType getVerticalType(const std::string& vctype);

private:
  static std::map<std::string, Zaxis_info> Zaxis_info_map;
};

#endif // diana_diField_diFieldVerticalAxes_h
