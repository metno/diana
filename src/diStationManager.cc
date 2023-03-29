/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2011-2019 met.no

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

#include "diStationManager.h"
#include "diStationPlot.h"
#include "diStationPlotCommand.h"
#include "diUtilities.h"
#include "miSetupParser.h"
#include "util/misc_util.h"

#include <puTools/miTime.h>
#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.StationManager"
#include <miLogger/miLogging.h>

using namespace miutil;

StationManager::StationManager()
{
}


StationManager::~StationManager()
{
}

/**
 * Returns information about the StationPlot objects held by this manager.
 */
stationDialogInfo& StationManager::initDialog()
{
  return m_info;
}

/**
 * Parses the setup file and reads in information about the stations
 * declared there.
 * Returns true if the file is parsed without error; otherwise returns false.
 */

bool StationManager::parseSetup()
{
  METLIBS_LOG_SCOPE();

  // Create stationSetInfo objects to be stored for later retrieval by the StationDialog.
  std::vector<std::string> section;

  // Return true if there is no STATIONS section.
  if (!SetupParser::getSection("STATIONS", section))
    return true;

  std::set<std::string> urls;

  for (unsigned int i = 0; i < section.size(); ++i) {
    if (section[i].find("image") != std::string::npos) {
      // split on blank, preserve ""
      std::vector<std::string> tokens = miutil::split_protected(section[i], '"', '"', " ", true);
      stationSetInfo s_info;
      for (size_t k = 0; k < tokens.size(); k++) {
     //  METLIBS_LOG_DEBUG("TOKENS = " << tokens[k]);
        std::vector<std::string> pieces = miutil::split(tokens[k], "=");
        // tag name=url
        if (k == 0 && pieces.size() == 2) {
          if (urls.find(pieces[1]) == urls.end()) {
       //     METLIBS_LOG_DEBUG("ADDS = " << pieces[0]);
            s_info.name = pieces[0];
            s_info.url = pieces[1];
            m_info.chosen[s_info.url] = false;

            // Record the URL of the set to avoid potential duplication.
            urls.insert(pieces[1]);
          }
        }
        // tag image=??.xpm
        else if (k==1 && pieces.size() == 2) {
         // METLIBS_LOG_DEBUG("IMAGE = " << pieces[0]);
          s_info.image = pieces[1];
        }
      }
      m_info.sets.push_back(s_info);
    } else {
      std::vector<std::string> pieces = miutil::split(section[i], "=");
      if (pieces.size() == 2) {
        if (urls.find(pieces[1]) == urls.end()) {
          stationSetInfo s_info;
          s_info.name = pieces[0];
          s_info.url = pieces[1];
          m_info.chosen[s_info.url] = false;
          m_info.sets.push_back(s_info);

          // Record the URL of the set to avoid potential duplication.
          urls.insert(pieces[1]);
        }
      } else {
        METLIBS_LOG_ERROR("Invalid line in setup file: " << section[i]);
      }
    }
  }

  return true;
}

/**
 * Imports the set of stations from the specified \a url.
 */
StationPlot* StationManager::importStations(const std::string& name, const std::string& url)
{
  std::vector<std::string> lines;
  if (not diutil::getFromAny(url, lines)) {
#ifdef DEBUGPRINT
    METLIBS_LOG_DEBUG("*** Failed to open '" << url << "'");
#endif
    return 0;
  }

  std::vector<Station*> stations;
  bool useImage = false;
  for (unsigned int i = 0; i < lines.size(); ++i) {

    std::vector<std::string> pieces = miutil::split(lines[i], ";");
    if (pieces.size() >= 7) {

      // Create a station with the latitude, longitude and a combination of
      // the name and station number. We also record the URL, status, type
      // of station and update time.
      Station *station = new Station;
      station->name = pieces[2] + " " + pieces[3];
      const float lat = miutil::to_float(pieces[0]), lon = miutil::to_float(pieces[1]);
      station->pos = miCoordinates(lon, lat);
      station->url = pieces[5];
      station->isVisible = true;
      switch (atoi(pieces[4].c_str())) {
      case 1:
        station->status = Station::working;
        break;
      case 2:
        station->status = Station::underRepair;
        break;
      case 3:
        station->status = Station::failed;
        break;
      case 4:
      default:
        station->status = Station::unknown;
        break;
      }
      if (station->url.rfind("&sauto=1") == station->url.length() - 8)
        station->type = Station::automatic;
      else
        station->type = Station::visual;

      std::string timeString = pieces[6];
      std::vector<std::string> timePieces = miutil::split(pieces[6], " ");
      if (timePieces.size() == 2 && miutil::count_char(timePieces[0], '-') == 2) {
        if (miutil::count_char(timePieces[1], ':') == 1)
          timeString += ":00";
        if (miutil::count_char(timePieces[1], ':') == 2)
          station->time = miutil::miTime(timeString.c_str());
      }

      if (pieces.size() >= 8)
        station->height = atof(pieces[7].c_str());

      stations.push_back(station);

    } else {
      Station *station = parseSMHI(lines[i], url);
      if (station != NULL) {
        useImage = true;
        stations.push_back(station);
      }
    }
  }

  // Construct a new StationPlot object.
  StationPlot *plot = new StationPlot(stations);
  plot->setName(name);
  plot->setUseImage(useImage);

  return plot;
}

Station* StationManager::parseSMHI(const std::string& miLine, const std::string& url)
{
  Station* st = NULL;
  std::vector<std::string> stationVector;

  std::string image = "";
  for (size_t i  = 0; i<m_info.sets.size(); ++i) {
    if (m_info.sets[i].url == url) {
        image = m_info.sets[i].image;
    }
  }

  // the old format
  if (miutil::contains(miLine, ";")) {
    stationVector = miutil::split(miLine, ";", false);
    if (stationVector.size() == 6) {
      st = new Station();
      st->name = stationVector[0];
      const float lat = miutil::to_float(stationVector[1]), lon = miutil::to_float(stationVector[2]);
      st->pos = miCoordinates(lon, lat);
      st->height = miutil::to_int(stationVector[3], -1);
      st->barHeight = miutil::to_int(stationVector[4], -1);
      st->id = stationVector[5];
      st->isVisible = true;
      st->status = Station::working;
      st->type = Station::visual;
      st->image = image;
    } else {
      METLIBS_LOG_ERROR("Something is wrong with: " << miLine);
    }
  }
  // the old format
  else if (miutil::contains(miLine, ",")) {
    stationVector = miutil::split(miLine, ",", false);
    if (stationVector.size() == 6) {
      st = new Station();
      st->name = stationVector[0];
      const float lat = miutil::to_float(stationVector[1]), lon = miutil::to_float(stationVector[2]);
      st->pos = miCoordinates(lon, lat);
      st->height = miutil::to_int(stationVector[3], -1);
      st->barHeight = miutil::to_int(stationVector[4], -1);
      st->id = stationVector[5];
      st->isVisible = true;
      st->status = Station::working;
      st->type = Station::visual;
      st->image = image;
    } else {
      METLIBS_LOG_ERROR("Something is wrong with: " << miLine);
    }
  }
  return st;
}
