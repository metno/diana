/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2018 met.no

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

#ifndef DIANA_DIMAPAREASETUP_H
#define DIANA_DIMAPAREASETUP_H

#include "diField/diArea.h"
#include <vector>

class MapAreaSetup
{
public:
  /// get list of predefined areas (names)
  std::vector<std::string> getMapAreaNames();

  /// get predefined area from name
  bool getMapAreaByName(const std::string&, Area&);

  /// get predefined area from accelerator
  bool getMapAreaByFkey(const std::string&, Area&);

  // parse section containing definitions of map-areas
  bool parseSetup();

  static MapAreaSetup* instance();
  static void destroy();

private:
  static MapAreaSetup* self_;

  std::vector<Area> mapareas;
  std::vector<Area> mapareas_Fkeys;
};

#endif
