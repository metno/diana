/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2011 met.no

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

#include "diStationManager.h"
#include "diStationPlotCommand.h"
#include "diStationPlot.h"
#include "diUtilities.h"
#include "miSetupParser.h"

#include <puTools/miTime.h>
#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.StationManager"
#include <miLogger/miLogging.h>

using namespace miutil;
using namespace std;

StationManager::StationManager()
{
}


StationManager::~StationManager()
{
  cleanup();
}

void StationManager::cleanup()
{
  diutil::delete_all_and_clear(stationPlots);
}

StationManager::stationPlots_t::iterator StationManager::findStationPlot(const std::string& name, int id, stationPlots_t::iterator begin)
{
  for (; begin != stationPlots.end(); ++begin) {
    StationPlot* sp = *begin;
    if (sp->getId() == id && sp->getName() == name)
      break;
  }
  return begin;
}

/**
 * Updates the vector of \a plots with the data provided in \a inp.
*/
bool StationManager::init(const PlotCommand_cpv& inp)
{
  METLIBS_LOG_SCOPE();
  // Hide all the station plots to begin with so that we can later
  // show and select/unselect them.

  for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it) {
    StationPlot* sp = *it;
    if (sp->isVisible())
      sp->show();
    else
      sp->hide();
  }

  for (PlotCommand_cp pc : inp) {
    StationPlotCommand_cp c = std::dynamic_pointer_cast<const StationPlotCommand>(pc);
    if (!c)
      continue;

    stationPlots_t::iterator it = findStationPlot(c->name, -1, stationPlots.begin());
    StationPlot* plot = 0;

    if (it == stationPlots.end()) {
      // No existing plot exists. If the select part of the plot command
      // contains the word "hidden" then we ignore this set of stations.

      if (c->select != "hidden") {
        // Load the stations.
        plot = importStations(c->name, c->url);
        if (plot)
          putStations(plot);
      }

    } else {
      // An existing plot containing a loaded list of stations.
      plot = *it;
    }

    if (plot) {
      if (c->select == "hidden") {
        plot->hide();
        m_info.chosen[c->url] = false;
      } else {
        plot->show();
        m_info.chosen[c->url] = true;
      }

      if (c->select == "selected") {
        m_info.selected = c->name;
        vector<Station*> stations = plot->getStations();
        for (unsigned int k = 0; k < stations.size(); ++k)
          stations[k]->isSelected = true;

      } else
        plot->unselect();
    }
  }
  return true;
}

/**
 * Returns information about the StationPlot objects held by this manager.
 */
stationDialogInfo StationManager::initDialog()
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
  vector<std::string> section;

  // Return true if there is no STATIONS section.
  if (!SetupParser::getSection("STATIONS", section))
    return true;

  set<std::string> urls;

  for (unsigned int i = 0; i < section.size(); ++i) {
    if (section[i].find("image") != string::npos) {
      // split on blank, preserve ""
      vector<std::string> tokens = miutil::split_protected(section[i], '"','"'," ",true);
      stationSetInfo s_info;
      for (size_t k = 0; k < tokens.size(); k++) {
     //  METLIBS_LOG_DEBUG("TOKENS = " << tokens[k]);
        vector<std::string> pieces = miutil::split(tokens[k], "=");
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
    }
    else {
      vector<std::string> pieces = miutil::split(section[i], "=");
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
  vector<std::string> lines;
  if (not diutil::getFromAny(url, lines)) {
#ifdef DEBUGPRINT
    METLIBS_LOG_DEBUG("*** Failed to open '" << url << "'");
#endif
    return 0;
  }

  vector<Station*> stations;
  bool useImage = false;
  for (unsigned int i = 0; i < lines.size(); ++i) {

    vector<std::string> pieces = miutil::split(lines[i], ";");
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
      vector<std::string> timePieces = miutil::split(pieces[6], " ");
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
  vector<std::string> stationVector;

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

float StationManager::getStationsScale()
{
  if (!stationPlots.empty()) {
    return stationPlots.front()->getImageScale(0);
  } else
    return 0;
}

void StationManager::setStationsScale(float new_scale)
{
  for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it)
    (*it)->setImageScale(new_scale);
}

//********** plotting and selecting stations on the map***************

void StationManager::putStations(StationPlot* stationPlot)
{
  METLIBS_LOG_SCOPE();

  stationPlots_t::iterator p = findStationPlot(stationPlot->getName(), stationPlot->getId(), stationPlots.begin());
  if (p != stationPlots.end()) {
    StationPlot* old = *p;
    if (!old->isVisible())
      stationPlot->hide();
    stationPlot->setEnabled(old->isEnabled());
    delete old;
    *p = stationPlot;
  } else {
    stationPlots.push_back(stationPlot);
  }
}

void StationManager::makeStationPlot(const std::string& commondesc,
    const std::string& common, const std::string& description, int from,
    const vector<std::string>& data)
{
  StationPlot* stationPlot = new StationPlot(commondesc, common, description, from, data);
  putStations(stationPlot);
}

Station* StationManager::findStation(int x, int y)
{
  for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it) {
    Station* station = (*it)->stationAt(x, y);
    if (station)
      return station;
  }
  return 0;
}

vector<Station*> StationManager::findStations(int x, int y)
{
  vector<Station*> stations;
  for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it) {
    if ((*it)->isVisible()) {
      vector<Station*> found = (*it)->stationsAt(x, y, 5);
      stations.insert(stations.end(), found.begin(), found.end());
    }
  }
  return stations;
}

string StationManager::findStation(int x, int y, const std::string& name, int id)
{
  for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it) {
    if ((id == -1 || id == (*it)->getId())
        && (name == (*it)->getName()))
    {
      vector<std::string> st = (*it)->findStation(x, y);
      if (st.size() > 0)
        return st[0];
    }
  }
  return std::string();
}

vector<string> StationManager::findStations(int x, int y, const std::string& name, int id)
{
  for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it) {
    if ((id == -1 || id == (*it)->getId()) && (name == (*it)->getName())) {
      vector<std::string> st = (*it)->findStations(x, y);
      if (st.size() > 0)
        return st;
    }
  }
  return vector<std::string>();
}

void StationManager::findStations(int x, int y, bool add, std::vector<std::string>& name,
    std::vector<int>& id, std::vector<std::string>& station)
{
  for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it) {
    StationPlot* sp = *it;
    const int ii = sp->getId();
    if (ii > -1) {
      const vector<std::string> st = sp->findStation(x, y, add);
      for (size_t j = 0; j < st.size(); j++) {
        name.push_back(sp->getName());
        id.push_back(ii);
        station.push_back(st[j]);
      }
    }
  }
}

void StationManager::getStationData(vector< vector<std::string> >& data)
{
  for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it)
    data.push_back((*it)->stationRequest("selected"));
}

void StationManager::stationCommand(const std::string& command,
    const vector<std::string>& data, const std::string& name, int id, const std::string& misc)
{
  for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it) {
    StationPlot* sp = *it;
    if ((id == -1 || id == sp->getId()) &&
        (name == sp->getName() || name.empty()))
    {
      sp->stationCommand(command, data, misc);
      break;
    }
  }
}

void StationManager::stationCommand(const std::string& command, const std::string& name, int id)
{
  if (command == "delete") {
    // name == "all" && id == -1 => coserver disconnect => delete all plots with id >= 0
    // name == "" && id >= 0 => client disconnect => delete all plots with getId() == id
    for (stationPlots_t::iterator p = stationPlots.begin(); p != stationPlots.end(); /*nothing*/) {
      StationPlot* sp = *p;
      if ((name == "all" && sp->getId() != -1) ||
          (id == sp->getId() && (name == sp->getName() || name.empty())))
      {
        delete *p;
        p = stationPlots.erase(p);
      } else {
        p++;
      }
    }

  } else {
    for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it) {
      StationPlot* sp = *it;
      if ((id == -1 || id == sp->getId()) &&
          (name == sp->getName() || name.empty()))
        sp->stationCommand(command);
    }
  }
}

QString StationManager::getStationsText(int x, int y)
{
  QString stationsText;
  vector<Station*> allStations = findStations(x, y);
  vector<Station*> stations;
  for (unsigned int i = 0; i < allStations.size(); ++i) {
    if (allStations[i]->status != Station::noStatus)
      stations.push_back(allStations[i]);
  }

  if (stations.size() > 0) {

    // Count the number of times each station name appears in the list.
    // This is used later to decide whether or not to show the "auto" or
    // "vis" text.
    map<std::string, unsigned int> stationNames;
    for (unsigned int i = 0; i < stations.size(); ++i) {
      unsigned int number = stationNames.count(stations[i]->name);
      stationNames[stations[i]->name] = number + 1;
    }

    stationsText = "<table>";
    for (unsigned int i = 0; i < stations.size(); ++i) {
      if (!stations[i]->isVisible)
        continue;

      stationsText += "<tr>";
      stationsText += "<td>";
      switch (stations[i]->status) {
      case Station::failed:
        stationsText += "<span style=\"background: red; color: red\">X</span>";
        break;
      case Station::underRepair:
        stationsText += "<span style=\"background: yellow; color: yellow\">X</span>";
        break;
      case Station::working:
        stationsText += "<span style=\"background: lightgreen; color: lightgreen\">X</span>";
        break;
      default:
        ;
      }
      stationsText += "</td>";

      stationsText += "<td>";
      stationsText += QString("<a href=\"%1\">%2</a>").arg(
          QString::fromStdString(stations[i]->url)).arg(QString::fromStdString(stations[i]->name));
      if (stationNames[stations[i]->name] > 1) {
        if (stations[i]->type == Station::automatic)
          stationsText += "&nbsp;auto";
        else
          stationsText += "&nbsp;vis";
      }
      stationsText += "</td>";

      stationsText += "<td>";
      const float lat = stations[i]->lat(), lon = stations[i]->lon();
      if (lat >= 0)
        stationsText += QString("%1&nbsp;N,&nbsp;").arg(lat);
      else
        stationsText += QString("%1&nbsp;S,&nbsp;").arg(-lat);
      if (lon >= 0)
        stationsText += QString("%1&nbsp;E").arg(lon);
      else
        stationsText += QString("%1&nbsp;W").arg(-lon);
      stationsText += "</td>";

      stationsText += "<td>";
      stationsText += QString("%1&nbsp;m").arg(stations[i]->height);
      stationsText += "</td>";

      stationsText += "</tr>";
   
//Here comes stations info for climate number, wmo number and nationalid.
      // stationsText += "<tr>";
      // stationsText += "<td>";
      // stationsText += "</td>";
      // stationsText += "<td>";
      // stationsText += QString(tr("ClimateNr:  "));
      // stationsText += "</td>";
      // stationsText += "<td>";
      // stationsText +=  tr("%1&nbsp;").arg(stations[i]->height);
      // stationsText += "</td>";
      // stationsText += "</tr>";

      // stationsText += "<tr>";
      // stationsText += "<td>";
      // stationsText += "</td>";
      // stationsText += "<td>";
      // stationsText += QString(tr("WMO Nr  :"));
      // stationsText += "</td>";
      // stationsText += "<td>";
      // stationsText +=  tr("%1&nbsp;").arg(stations[i]->barHeight);
      // stationsText += "</td>";
      // stationsText += "</tr>";
      // stationsText += "<tr>";
      // stationsText += "<td>";
      // stationsText += "</td>";
      // stationsText += "<td>";
      // stationsText += QString(tr("NationalId:"));
      // stationsText += "</td>";
      // stationsText += "<td>";
      // stationsText +=  tr("%1&nbsp;").arg(QString::fromStdString(stations[i]->id));
      // stationsText += "</td>";
      // stationsText += "</tr>";
   }
    stationsText += "</table>";

  }

  return stationsText;
}
