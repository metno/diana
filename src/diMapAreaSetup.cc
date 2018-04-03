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

#include "diana_config.h"

#include "diMapAreaSetup.h"

#include "miSetupParser.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.MapAreaManager"
#include <miLogger/miLogging.h>

namespace {

const std::string SectMapAreas = "MAP_AREA";

} // anonymous namespace

MapAreaSetup* MapAreaSetup::self_ = 0;

// static
MapAreaSetup* MapAreaSetup::instance()
{
  if (!self_)
    self_ = new MapAreaSetup();
  return self_;
}

// static
void MapAreaSetup::destroy()
{
  delete self_;
  self_ = 0;
}

std::vector<std::string> MapAreaSetup::getMapAreaNames()
{
  std::vector<std::string> areanames;
  for (const Area& a : mapareas)
    areanames.push_back(a.Name());

  return areanames;
}

bool MapAreaSetup::getMapAreaByName(const std::string& name, Area& area)
{
  // return first map
  if (miutil::to_lower(name) == "default" and !mapareas.empty()) {
    area = mapareas[0];
    return true;
  }
  for (const Area& a : mapareas) {
    if (name == a.Name()) {
      area = a;
      return true;
    }
  }
  return false;
}

bool MapAreaSetup::getMapAreaByFkey(const std::string& name, Area& area)
{
  for (const Area& a : mapareas_Fkeys) {
    if (name == a.Name()) {
      area = a;
      return true;
    }
  }
  return false;
}

// parse section containing definitions of map-areas
bool MapAreaSetup::parseSetup()
{
  mapareas.clear();

  std::vector<std::string> setuplist;
  if (!miutil::SetupParser::getSection(SectMapAreas, setuplist)) {
    return true;
  }

  for (size_t i = 0; i < setuplist.size(); i++) {
    Area area;

    if (area.setAreaFromString(setuplist[i])) {
      std::string name = area.Name();
      if (miutil::contains(name, "[F5]") || miutil::contains(name, "[F6]") || miutil::contains(name, "[F7]") || miutil::contains(name, "[F8]")) {
        std::string Fkey = name.substr(name.find("["), 4);
        miutil::replace(name, Fkey, "");
        Fkey.erase(0, 1);
        Fkey.erase(2, 1);
        Area area_Fkey = area;
        area_Fkey.setName(Fkey);
        area.setName(name);

        // find duplicate
        size_t q = 0;
        for (; q < mapareas_Fkeys.size(); q++) {
          if (mapareas_Fkeys[q].Name() == Fkey)
            break;
        }
        if (q != mapareas_Fkeys.size()) {
          mapareas_Fkeys[q] = area_Fkey;
        } else {
          mapareas_Fkeys.push_back(area_Fkey);
        }
      }

      // find duplicate
      size_t q = 0;
      for (q = 0; q < mapareas.size(); q++) {
        if (mapareas[q].Name() == name)
          break;
      }
      if (q != mapareas.size()) {
        mapareas[q] = area;
      } else {
        mapareas.push_back(area);
      }

    } else {
      miutil::SetupParser::errorMsg(SectMapAreas, i, "Incomplete maparea-specification");
      return true;
    }
  }
  return true;
}
