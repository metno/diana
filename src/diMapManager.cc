/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2016 met.no

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

#include "diMapManager.h"

#include "diKVListPlotCommand.h"
#include "diMapAreaSetup.h"
#include "miSetupParser.h"

#include <puTools/miStringFunctions.h>

#include <sstream>

#define MILOGGER_CATEGORY "diana.MapManager"
#include <miLogger/miLogging.h>

using namespace::miutil;

std::vector<MapInfo> MapManager::mapfiles;

const std::string SectMapTypes = "MAP_TYPE";

MapManager::MapManager()
{
}

bool MapManager::parseSetup()
{
  MapInfo mapinfo;

  mapfiles.clear();

  std::vector<std::string> strlist;
  if (!SetupParser::getSection(SectMapTypes, strlist))
    return true;

  for (size_t i = 0; i < strlist.size(); i++) {
    const miutil::KeyValue_v kvs = miutil::splitKeyValue(strlist[i]);
    if (find(kvs, "map") != size_t(-1)) {
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
      mapinfo.reset();
    }
    // parse string and fill mapinfo-struct
    fillMapInfo(kvs, mapinfo);
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

const std::vector<MapInfo>& MapManager::getMapInfo()
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

bool MapManager::fillMapInfo(const miutil::KeyValue_v& kvs, MapInfo& mi)
{
  PlotOptions a,b,c,d,e;
  return fillMapInfo(kvs,mi,a,b,c,d,e);
}

bool MapManager::fillMapInfo(const miutil::KeyValue_v& kvs, MapInfo& mi,
    PlotOptions& contopts, PlotOptions& landopts, PlotOptions& lonopts,
    PlotOptions& latopts, PlotOptions& ffopts)
{
  const std::string key_name = "map";
  const std::string key_type = "type";
  const std::string key_file = "file";
  const std::string key_limit = "limit";

  MapFileInfo mfi;
  bool newfile = false;

  mfi.fname = "";
  mfi.sizelimit = 0.0;

  // default values
  mi.lon.fontsize = 10;
  mi.lat.fontsize = 10;

  for (const miutil::KeyValue& kv : kvs) {
    if (kv.key() == "no.log" && kv.value().empty()) {
      mi.logok = false;
      continue;
    }

    if (!kv.value().empty()) {
      if (kv.key() == key_name) {
        mi.name = kv.value();
      } else if (kv.key() == key_type) {
        mi.type = kv.value();
      } else if (kv.key() == key_file) {
        newfile = true;
        mfi.fname = kv.value();
      } else if (kv.key() == key_limit) {
        newfile = true;
        mfi.sizelimit = kv.toDouble();

      } else if (kv.key() == "contour") {
        mi.contour.ison = kv.toBool();
      } else if (kv.key() == "cont.colour") {
        mi.contour.linecolour = kv.value();
        contopts.linecolour = Colour(kv.value());
      } else if (kv.key() == "cont.linewidth") {
        mi.contour.linewidth = kv.value();
        contopts.linewidth = kv.toInt();
      } else if (kv.key() == "cont.linetype") {
        mi.contour.linetype = kv.value();
        contopts.linetype = Linetype(kv.value());
      } else if (kv.key() == "cont.zorder") {
        mi.contour.zorder = kv.toInt();

      } else if (kv.key() == "land") {
        mi.land.ison = kv.toBool();
      } else if (kv.key() == "land.colour") {
        mi.land.fillcolour = kv.value();
        landopts.fillcolour = Colour(kv.value());
      } else if (kv.key() == "land.zorder") {
        mi.land.zorder = kv.toInt();

        // Old combined latlon (deprecated)
      } else if (kv.key() == "latlon") {
        mi.lon.ison = mi.lat.ison = kv.toBool();
      } else if (kv.key() == "latlon.colour") {
        mi.lon.linecolour = mi.lat.linecolour = kv.value();
        lonopts.linecolour = latopts.linecolour = Colour(kv.value());
      } else if (kv.key() == "latlon.linewidth") {
        mi.lon.linewidth = mi.lat.linewidth = kv.value();
        lonopts.linewidth = latopts.linewidth = kv.toInt();
      } else if (kv.key() == "latlon.linetype") {
        mi.lon.linetype = mi.lat.linetype = kv.value();
        lonopts.linetype = latopts.linetype = Linetype(kv.value());
      } else if (kv.key() == "latlon.density") {
        mi.lon.density = mi.lat.density = kv.toFloat();
      } else if (kv.key() == "latlon.zorder") {
        mi.lon.zorder = mi.lat.zorder = kv.toInt();

      } else if (kv.key() == "lon") {
        mi.lon.ison = kv.toBool();
      } else if (kv.key() == "lon.colour") {
        mi.lon.linecolour = kv.value();
        lonopts.linecolour = Colour(kv.value());
      } else if (kv.key() == "lon.linewidth") {
        mi.lon.linewidth = kv.value();
        lonopts.linewidth = kv.toInt();
      } else if (kv.key() == "lon.linetype") {
        mi.lon.linetype = kv.value();
        lonopts.linetype = Linetype(kv.value());
      } else if (kv.key() == "lon.density") {
        mi.lon.density = kv.toFloat();
      } else if (kv.key() == "lon.zorder") {
        mi.lon.zorder = kv.toInt();
      } else if (kv.key() == "lon.showvalue") {
        mi.lon.showvalue = kv.toBool();
      } else if (kv.key() == "lon.value_pos") {
        mi.lon.value_pos = kv.value();
      } else if (kv.key() == "lon.fontsize") {
        mi.lon.fontsize = kv.toFloat();

      } else if (kv.key() == "lat") {
        mi.lat.ison = kv.toBool();
      } else if (kv.key() == "lat.colour") {
        mi.lat.linecolour = kv.value();
        latopts.linecolour = Colour(kv.value());
      } else if (kv.key() == "lat.linewidth") {
        mi.lat.linewidth = kv.value();
        latopts.linewidth = kv.toInt();
      } else if (kv.key() == "lat.linetype") {
        mi.lat.linetype = kv.value();
        latopts.linetype = Linetype(kv.value());
      } else if (kv.key() == "lat.density") {
        mi.lat.density = kv.toFloat();
      } else if (kv.key() == "lat.zorder") {
        mi.lat.zorder = kv.toInt();
      } else if (kv.key() == "lat.showvalue") {
        mi.lat.showvalue = kv.toBool();
      } else if (kv.key() == "lat.value_pos") {
        mi.lat.value_pos = kv.value();
      } else if (kv.key() == "lat.fontsize") {
        mi.lat.fontsize = kv.toFloat();

      } else if (kv.key() == "frame") {
        mi.frame.ison = kv.toBool();
      } else if (kv.key() == "frame.colour") {
        mi.frame.linecolour = kv.value();
        ffopts.linecolour = Colour(kv.value());
      } else if (kv.key() == "frame.linewidth") {
        mi.frame.linewidth = kv.value();
        ffopts.linewidth = kv.toInt();
      } else if (kv.key() == "frame.linetype") {
        mi.frame.linetype = kv.value();
        ffopts.linetype = Linetype(kv.value());
      } else if (kv.key() == "frame.zorder") {
        mi.frame.zorder = kv.toInt();
      } else if (kv.key() == "symbol") {
        mi.symbol = kv.toInt();
        mi.special = true;
      } else if (kv.key() == "dbfcol") {
      }
    }
  }
  if (newfile && mfi.fname.length() > 0)
    mi.mapfiles.push_back(mfi);
  return true;
}

inline miutil::KeyValue KeyBool(const std::string& key, bool on_off)
{
  return miutil::KeyValue(key, on_off ? "on" : "off");
}

inline miutil::KeyValue KeyFloat(const std::string& key, float f)
{
  return miutil::KeyValue(key, miutil::from_number(f));
}

inline miutil::KeyValue KeyInt(const std::string& key, int i)
{
  return miutil::KeyValue(key, miutil::from_number(i));
}

miutil::KeyValue_v MapManager::MapInfo2str(const MapInfo& mi)
{
  using miutil::KeyValue;
  miutil::KeyValue_v ost;

  ost.push_back(KeyValue("map", mi.name));
  ost.push_back(KeyBool("contour", mi.contour.ison));
  if (mi.contour.ison) {
    ost.push_back(KeyValue("cont.colour", mi.contour.linecolour));
    ost.push_back(KeyValue("cont.linewidth", mi.contour.linewidth));
    ost.push_back(KeyValue("cont.linetype", mi.contour.linetype));
    ost.push_back(KeyInt("cont.zorder", mi.contour.zorder));
  }
  ost.push_back(KeyBool("land", mi.land.ison));
  if (mi.land.ison) {
    ost.push_back(KeyValue("land.colour", mi.land.fillcolour));
    ost.push_back(KeyInt("land.zorder", mi.land.zorder));
  }

  return ost;
}

miutil::KeyValue_v MapManager::MapExtra2str(const MapInfo& mi)
{
  using miutil::KeyValue;
  miutil::KeyValue_v ost;

  ost.push_back(KeyBool("lon", mi.lon.ison));
  if (mi.lon.ison) {
    ost.push_back(KeyValue("lon.colour", mi.lon.linecolour));
    ost.push_back(KeyValue("lon.linewidth", mi.lon.linewidth));
    ost.push_back(KeyValue("lon.linetype", mi.lon.linetype));
    ost.push_back(KeyFloat("lon.density", mi.lon.density));
    ost.push_back(KeyInt("lon.zorder", mi.lon.zorder));
    ost.push_back(KeyBool("lon.showvalue", mi.lon.showvalue));
    ost.push_back(KeyValue("lon.value_pos", mi.lon.value_pos));
    ost.push_back(KeyFloat("lon.fontsize", mi.lon.fontsize));
  }
  ost.push_back(KeyBool("lat", mi.lat.ison));
  if (mi.lat.ison) {
    ost.push_back(KeyValue("lat.colour", mi.lat.linecolour));
    ost.push_back(KeyValue("lat.linewidth", mi.lat.linewidth));
    ost.push_back(KeyValue("lat.linetype", mi.lat.linetype));
    ost.push_back(KeyFloat("lat.density", mi.lat.density));
    ost.push_back(KeyInt("lat.zorder", mi.lat.zorder));
    ost.push_back(KeyBool("lat.showvalue", mi.lat.showvalue));
    ost.push_back(KeyValue("lat.value_pos", mi.lat.value_pos));
    ost.push_back(KeyFloat("lat.fontsize", mi.lat.fontsize));
  }
  ost.push_back(KeyBool("frame", mi.frame.ison));
  if (mi.frame.ison) {
    ost.push_back(KeyValue("frame.colour", mi.frame.linecolour));
    ost.push_back(KeyValue("frame.linewidth", mi.frame.linewidth));
    ost.push_back(KeyValue("frame.linetype", mi.frame.linetype));
    ost.push_back(KeyInt("frame.zorder", mi.frame.zorder));
  }

  return ost;
}

MapDialogInfo MapManager::getMapDialogInfo()
{
  MapDialogInfo MapDI;
  MapDI.areas = MapAreaSetup::instance()->getMapAreaNames();
  MapDI.maps = getMapInfo();

  MapDI.default_area = (MapDI.areas.size() > 0 ? MapDI.areas[0] : "");
  if (MapDI.maps.size() > 0)
    MapDI.default_maps.push_back(MapDI.maps[0].name);

  MapDI.backcolour = "white";

  return MapDI;
}
