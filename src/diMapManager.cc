/*
 Diana - A Free Meteorological Visualisation Tool

 $Id$

 Copyright (C) 2006 met.no

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diMapManager.h>
#include <puTools/miSetupParser.h>

using namespace::miutil;
using namespace std;

vector<Area> MapManager::mapareas;
vector<Area> MapManager::mapareas_Fkeys;
vector<MapInfo> MapManager::mapfiles;

const std::string SectMapAreas = "MAP_AREA";
const std::string SectMapTypes = "MAP_TYPE";

bool MapManager::parseSetup()
{
  if (!parseMapAreas())
    return false;
  if (!parseMapTypes())
    return false;
  return true;
}

// parse section containing definitions of map-areas
bool MapManager::parseMapAreas()
{
  mapareas.clear();

  vector<std::string> setuplist;
  if (!SetupParser::getSection(SectMapAreas, setuplist)) {
    return true;
  }

  for (size_t i = 0; i < setuplist. size(); i++) {
    Area area;

    if (area.setAreaFromString(setuplist[i])) {
      std::string name = area.Name();
      if (miutil::contains(name, "[F5]") || miutil::contains(name, "[F6]") || miutil::contains(name, 
          "[F7]") || miutil::contains(name, "[F8]")) {
        std::string Fkey = name.substr(name.find("["), 4);
        miutil::replace(name, Fkey, "");
        Fkey.erase(0, 1);
        Fkey.erase(2, 1);
        Area area_Fkey = area;
        area_Fkey.setName(Fkey);;
        area.setName(name);;

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
      //cerr << "Adding area:" << name << " defined by:" << area << endl;

    } else {
      SetupParser::errorMsg(SectMapAreas, i, "Incomplete maparea-specification");
      return true;
    }

  }
  return true;
}

bool MapManager::parseMapTypes()
{
  const std::string key_name = "map=";

  MapInfo mapinfo;

  PlotOptions a, b, c, d, e;

  mapfiles.clear();

  vector<std::string> strlist;
  if (!SetupParser::getSection(SectMapTypes, strlist))
    return true;

  for (size_t i = 0; i < strlist.size(); i++) {
    if (miutil::contains(strlist[i], key_name)) {
      // save previous map and set defaults
      if (not mapinfo.name.empty()) {
        // find duplicate
        size_t q = 0;
        for (; q < mapfiles.size(); q++)
          if (mapfiles[q].name == mapinfo.name)
            break;
        if (q != mapfiles.size())
          mapfiles[q] = mapinfo;
        else
          mapfiles.push_back(mapinfo);
      }
      // set default values ( mapfiles[m] --> mapinfo )
      mapinfo.name = "";
      mapinfo.mapfiles.clear();
      mapinfo.type = "pland";
      mapinfo.logok = true;
      mapinfo.special = false;
      mapinfo.symbol = 0;
      mapinfo.dbfcol = "";
      mapinfo.contour.ison = true;
      mapinfo.contour.linecolour = "black";
      mapinfo.contour.linewidth = "1";
      mapinfo.contour.linetype = "solid";
      mapinfo.contour.zorder = 1;

      mapinfo.land.ison = false;
      mapinfo.land.fillcolour = "white";
      mapinfo.land.zorder = 0;

      mapinfo.lon.ison = false;
      mapinfo.lon.linecolour = "black";
      mapinfo.lon.linewidth = "1";
      mapinfo.lon.linetype = "solid";
      mapinfo.lon.zorder = 2;
      mapinfo.lon.density = 10.0;
      mapinfo.lon.showvalue = false;
      mapinfo.lon.value_pos = "bottom";
      mapinfo.lon.fontsize=10;

      mapinfo.lat.ison = false;
      mapinfo.lat.linecolour = "black";
      mapinfo.lat.linewidth = "1";
      mapinfo.lat.linetype = "solid";
      mapinfo.lat.zorder = 2;
      mapinfo.lat.density = 10.0;
      mapinfo.lat.showvalue = false;
      mapinfo.lat.value_pos = "left";
      mapinfo.lat.fontsize=10;

      mapinfo.frame.ison = false;
      mapinfo.frame.linecolour = "black";
      mapinfo.frame.linewidth = "1";
      mapinfo.frame.linetype = "solid";
      mapinfo.frame.zorder = 2;
    }
    // parse string and fill mapinfo-struct
    fillMapInfo(strlist[i], mapinfo, a, b, c, d, e);
  }

  // add final map to list
  if (not mapinfo.name.empty()) {
    // find duplicate
    size_t q = 0;
    for (; q < mapfiles.size(); q++)
      if (mapfiles[q].name == mapinfo.name)
        break;
    if (q != mapfiles.size())
      mapfiles[q] = mapinfo;
    else
      mapfiles.push_back(mapinfo);
  }

  return true;
}

vector<std::string> MapManager::getMapAreaNames()
{
  vector<std::string> areanames;
  for (size_t i = 0; i < mapareas.size(); i++)
    areanames.push_back(mapareas[i].Name());

  return areanames;
}

bool MapManager::getMapAreaByName(const std::string& name, Area& a)
{
  //return first map
  if (miutil::to_lower(name) == "default" and not mapareas.empty()) {
    a = mapareas[0];
    return true;
  }
  for (size_t i = 0; i < mapareas.size(); i++) {
    if (name == mapareas[i].Name()) {
      a = mapareas[i];
      return true;
    }
  }
  return false;
}

bool MapManager::getMapAreaByFkey(const std::string& name, Area& a)
{
  //     cerr<<"getMapAreaByFkey:"<<name<<endl;
  for (size_t i = 0; i < mapareas_Fkeys.size(); i++) {
    if (name == mapareas_Fkeys[i].Name()) {
      a = mapareas_Fkeys[i];
      return true;
    }
  }
  return false;
}

vector<MapInfo> MapManager::getMapInfo()
{
  return mapfiles;
}

bool MapManager::getMapInfoByName(const std::string& name, MapInfo& mapinfo)
{
  const std::string lname = miutil::to_lower(name);
  for (size_t i = 0; i < mapfiles.size(); i++) {
    if (miutil::to_lower(mapfiles[i].name) == lname) {
      mapinfo = mapfiles[i];
      return true;
    }
  }
  return false;
}

bool MapManager::fillMapInfo(const std::string& str, MapInfo& mi,
    PlotOptions& contopts, PlotOptions& landopts, PlotOptions& lonopts,
    PlotOptions& latopts, PlotOptions& ffopts)
{
  const std::string key_name = "map";
  const std::string key_type = "type";
  const std::string key_file = "file";
  const std::string key_limit = "limit";

  vector<std::string> tokens, stokens;
  std::string key, value;
  MapFileInfo mfi;
  bool newfile = false;
  int m, j, o;

  mfi.fname = "";
  mfi.sizelimit = 0.0;

  // default values
  mi.lon.fontsize = 10;
  mi.lat.fontsize = 10;

  tokens = miutil::split(str, " ");
  m = tokens.size();
  for (j = 0; j < m; j++) {

    if (miutil::to_upper(tokens[j]) == "NO.LOG") {
      mi.logok = false;
      continue;
    }

    stokens = miutil::split(tokens[j], 0, "=");
    o = stokens.size();
    if (o > 1) {
      key = miutil::to_lower(stokens[0]);
      miutil::trim(key);
      value = stokens[1];
      miutil::trim(value);

      if (key == key_name) {
        mi.name = value;
      } else if (key == key_type) {
        mi.type = value;
      } else if (key == key_file) {
        newfile = true;
        mfi.fname = value;
      } else if (key == key_limit) {
        newfile = true;
        mfi.sizelimit = atof(value.c_str());

      } else if (key == "contour") {
        mi.contour.ison = (miutil::to_upper(value) == "ON");
      } else if (key == "cont.colour") {
        mi.contour.linecolour = value;
        contopts.linecolour = Colour(value);
      } else if (key == "cont.linewidth") {
        mi.contour.linewidth = value;
        contopts.linewidth = atoi(value.c_str());
      } else if (key == "cont.linetype") {
        mi.contour.linetype = value;
        contopts.linetype = Linetype(value);
      } else if (key == "cont.zorder") {
        mi.contour.zorder = atoi(value.c_str());

      } else if (key == "land") {
        mi.land.ison = (miutil::to_upper(value) == "ON");
      } else if (key == "land.colour") {
        mi.land.fillcolour = value;
        landopts.fillcolour = Colour(value);
      } else if (key == "land.zorder") {
        mi.land.zorder = atoi(value.c_str());

        // Old combined latlon (deprecated)
      } else if (key == "latlon") {
        mi.lon.ison = mi.lat.ison = (miutil::to_upper(value) == "ON");
      } else if (key == "latlon.colour") {
        mi.lon.linecolour = mi.lat.linecolour = value;
        lonopts.linecolour = latopts.linecolour = Colour(value);
      } else if (key == "latlon.linewidth") {
        mi.lon.linewidth = mi.lat.linewidth = value;
        lonopts.linewidth = latopts.linewidth = atoi(value.c_str());
      } else if (key == "latlon.linetype") {
        mi.lon.linetype = mi.lat.linetype = value;
        lonopts.linetype = latopts.linetype = Linetype(value);
      } else if (key == "latlon.density") {
        mi.lon.density = mi.lat.density = atof(value.c_str());
      } else if (key == "latlon.zorder") {
        mi.lon.zorder = mi.lat.zorder = atoi(value.c_str());

      } else if (key == "lon") {
        mi.lon.ison = (miutil::to_upper(value) == "ON");
      } else if (key == "lon.colour") {
        mi.lon.linecolour = value;
        lonopts.linecolour = Colour(value);
      } else if (key == "lon.linewidth") {
        mi.lon.linewidth = value;
        lonopts.linewidth = atoi(value.c_str());
      } else if (key == "lon.linetype") {
        mi.lon.linetype = value;
        lonopts.linetype = Linetype(value);
      } else if (key == "lon.density") {
        mi.lon.density = atof(value.c_str());
      } else if (key == "lon.zorder") {
        mi.lon.zorder = atoi(value.c_str());
      } else if (key == "lon.showvalue") {
        mi.lon.showvalue = (miutil::to_upper(value) == "ON");
      } else if (key == "lon.value_pos") {
        mi.lon.value_pos = value;
      } else if (key == "lon.fontsize") {
        mi.lon.fontsize = atof(value.c_str());

      } else if (key == "lat") {
        mi.lat.ison = (miutil::to_upper(value) == "ON");
      } else if (key == "lat.colour") {
        mi.lat.linecolour = value;
        latopts.linecolour = Colour(value);
      } else if (key == "lat.linewidth") {
        mi.lat.linewidth = value;
        latopts.linewidth = atoi(value.c_str());
      } else if (key == "lat.linetype") {
        mi.lat.linetype = value;
        latopts.linetype = Linetype(value);
      } else if (key == "lat.density") {
        mi.lat.density = atof(value.c_str());
      } else if (key == "lat.zorder") {
        mi.lat.zorder = atoi(value.c_str());
      } else if (key == "lat.showvalue") {
        mi.lat.showvalue = (miutil::to_upper(value) == "ON");
      } else if (key == "lat.value_pos") {
        mi.lat.value_pos = value;
      } else if (key == "lat.fontsize") {
        mi.lat.fontsize = atof(value.c_str());

      } else if (key == "frame") {
        mi.frame.ison = (miutil::to_upper(value) == "ON");
      } else if (key == "frame.colour") {
        mi.frame.linecolour = value;
        ffopts.linecolour = Colour(value);
      } else if (key == "frame.linewidth") {
        mi.frame.linewidth = value;
        ffopts.linewidth = atoi(value.c_str());
      } else if (key == "frame.linetype") {
        mi.frame.linetype = value;
        ffopts.linetype = Linetype(value);
      } else if (key == "frame.zorder") {
        mi.frame.zorder = atoi(value.c_str());
      } else if (key == "symbol") {
        mi.symbol = atoi(value.c_str());
        mi.special = true;
      } else if (key == "dbfcol") {
        mi.dbfcol = value;
        mi.special = true;
      }
    }
  }
  if (newfile && mfi.fname.length() > 0)
    mi.mapfiles.push_back(mfi);
  return true;
}

std::string MapManager::MapInfo2str(const MapInfo& mi)
{
  ostringstream ost;
  ost << "map=" << mi.name;
  ost << " contour=" << (mi.contour.ison ? "on" : "off");
  if (mi.contour.ison) {
    ost << " cont.colour=" << mi.contour.linecolour;
    ost << " cont.linewidth=" << mi.contour.linewidth;
    ost << " cont.linetype=" << mi.contour.linetype;
    ost << " cont.zorder=" << mi.contour.zorder;
  }
  ost << " land=" << (mi.land.ison ? "on" : "off");
  if (mi.land.ison) {
    ost << " land.colour=" << mi.land.fillcolour;
    ost << " land.zorder=" << mi.land.zorder;
  }
  ost << " lon=" << (mi.lon.ison ? "on" : "off");
  if (mi.lon.ison) {
    ost << " lon.colour=" << mi.lon.linecolour;
    ost << " lon.linewidth=" << mi.lon.linewidth;
    ost << " lon.linetype=" << mi.lon.linetype;
    ost << " lon.density=" << mi.lon.density;
    ost << " lon.zorder=" << mi.lon.zorder;
    ost << " lon.showvalue=" << (mi.lon.showvalue ? "on" : "off");
    ost << " lon.value_pos=" << mi.lon.value_pos;
    ost << " lon.fontsize=" << mi.lon.fontsize;
  }
  ost << " lat=" << (mi.lat.ison ? "on" : "off");
  if (mi.lat.ison) {
    ost << " lat.colour=" << mi.lat.linecolour;
    ost << " lat.linewidth=" << mi.lat.linewidth;
    ost << " lat.linetype=" << mi.lat.linetype;
    ost << " lat.density=" << mi.lat.density;
    ost << " lat.zorder=" << mi.lat.zorder;
    ost << " lat.showvalue=" << (mi.lat.showvalue ? "on" : "off");
    ost << " lat.value_pos=" << mi.lat.value_pos;
    ost << " lat.fontsize=" << mi.lat.fontsize;
  }
  ost << " frame=" << (mi.frame.ison ? "on" : "off");
  if (mi.frame.ison) {
    ost << " frame.colour=" << mi.frame.linecolour;
    ost << " frame.linewidth=" << mi.frame.linewidth;
    ost << " frame.linetype=" << mi.frame.linetype;
    ost << " frame.zorder=" << mi.frame.zorder;
  }

  return ost.str();
}

MapDialogInfo MapManager::getMapDialogInfo()
{
  vector<string> areas = getMapAreaNames();
  vector<MapInfo> maps = getMapInfo();

  MapDialogInfo MapDI;

  MapDI.areas = areas;
  MapDI.default_area = (areas.size() > 0 ? areas[0] : "");
  MapDI.maps = maps;
  if (maps.size() > 0)
    MapDI.default_maps.push_back(maps[0].name);

  MapDI.backcolour = "white";

  return MapDI;
}
