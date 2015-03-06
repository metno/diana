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
#include "diStationPlot.h"
#include "diUtilities.h"

#include <puTools/miSetupParser.h>
#include <puTools/miTime.h>
#include <boost/algorithm/string/join.hpp>

#define MILOGGER_CATEGORY "diana.StationManager"
#include <miLogger/miLogging.h>

using namespace miutil;
using namespace std;

StationManager::StationManager()
{
}

/**
 * Updates the vector of \a plots with the data provided in \a inp.
*/
bool StationManager::init(const vector<string>& inp)
{
  // Hide all the station plots to begin with so that we can later
  // show and select/unselect them.

  for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it) {
    if (it->second->isVisible())
      it->second->show();
    else
      it->second->hide();
  }

  for (unsigned int i = 0; i < inp.size(); ++i) {

    std::string plotStr = inp[i];

    vector<std::string> pieces = miutil::split(plotStr, " ");
    pieces.erase(pieces.begin());

    std::string select = pieces.back();
    pieces.pop_back();

    std::string url = pieces.back();
    pieces.pop_back();

    std::string name = boost::algorithm::join(pieces, " ");

    stationPlots_t::iterator it = stationPlots.find(name);
    StationPlot* plot = 0;

    if (it == stationPlots.end()) {
      // No existing plot exists. If the select part of the plot command
      // contains the word "hidden" then we ignore this set of stations.

      if (select != "hidden") {
        // Load the stations.
        plot = importStations(name, url);
        if (plot)
          putStations(plot);
      }

    } else {
      // An existing plot containing a loaded list of stations.
      plot = it->second;
    }

    if (plot) {
      if (select == "hidden") {
        plot->hide();
        m_info.chosen[url] = false;
      } else {
        plot->show();
        m_info.chosen[url] = true;
      }

      if (select == "selected") {
        m_info.selected = name;
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
StationPlot* StationManager::importStations(std::string& name, std::string& url)
{
  vector<std::string> lines;
  if (not diutil::getFromAny(url, lines)) {
#ifdef DEBUGPRINT
    METLIBS_LOG_DEBUG("*** Failed to open " << url);
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
      station->lat = atof(pieces[0].c_str());
      station->lon = atof(pieces[1].c_str());
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

Station* StationManager::parseSMHI(std::string& miLine, std::string& url)
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
      st->lat = miutil::to_double(stationVector[1]);
      st->lon = miutil::to_double(stationVector[2]);
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
      st->lat = miutil::to_double(stationVector[1]);
      st->lon = miutil::to_double(stationVector[2]);
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
    return stationPlots.begin()->second->getImageScale(0);
  } else
    return .0;
}

void StationManager::setStationsScale(float new_scale)
{
  for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it)
    it->second->setImageScale(new_scale);
}

//********** plotting and selecting stations on the map***************

void StationManager::putStations(StationPlot* stationPlot)
{
  METLIBS_LOG_SCOPE();

  std::string name = stationPlot->getName();
  map <std::string,StationPlot*>::iterator p = stationPlots.begin();
  map <std::string,StationPlot*>::iterator pend = stationPlots.end();

  //delete old stationPlot
  while (p != pend && name != (*p).second->getName())
    p++;
  if (p != pend) {
    if (!((*p).second->isVisible()))
      stationPlot->hide();
    stationPlot->setEnabled((*p).second->isEnabled());
    delete (*p).second;
    stationPlots.erase(p);
  }

  //put new stationPlot into vector (sorted by priority and number of stations)
  p = stationPlots.begin();
  pend = stationPlots.end();

  int pri = stationPlot->getPriority();
  while (p != pend && pri > (*p).second->getPriority())
    p++;
  if (p != pend && pri == (*p).second->getPriority())
    while (p != pend && (*stationPlot) < *((*p).second))
      p++;
  stationPlots[stationPlot->getName()] = stationPlot;
}

void StationManager::makeStationPlot(const std::string& commondesc,
    const std::string& common, const std::string& description, int from,
    const vector<std::string>& data)
{
  StationPlot* stationPlot = new StationPlot(commondesc, common, description,
      from, data);
  putStations(stationPlot);
}

Station* StationManager::findStation(int x, int y)
{
  for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it) {
    Station* station = it->second->stationAt(x, y);
    if (station)
      return station;
  }
  return 0;
}

vector<Station*> StationManager::findStations(int x, int y)
{
  vector<Station*> stations;
  for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it) {
    if (it->second->isVisible()) {
      vector<Station*> found = it->second->stationsAt(x, y);
      stations.insert(stations.end(), found.begin(), found.end());
    }
  }
  return stations;
}

string StationManager::findStation(int x, int y, std::string name, int id)
{
  for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it) {
    if ((id == -1 || id == it->second->getId())
        && (name == it->second->getName()))
    {
      vector<std::string> st = it->second->findStation(x, y);
      if (st.size() > 0)
        return st[0];
    }
  }
  return std::string();
}

vector<string> StationManager::findStations(int x, int y, std::string name, int id)
{
  for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it) {
    if ((id == -1 || id == it->second->getId()) && (name == it->second->getName())) {
      vector<std::string> st = (*it).second->findStations(x, y);
      if (st.size() > 0)
        return st;
    }
  }
  return vector<std::string>();
}

void StationManager::findStations(int x, int y, bool add, std::vector<std::string>& name,
    std::vector<int>& id, std::vector<std::string>& station)
{
  int ii;
  for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it) {
    vector<std::string> st = it->second->findStation(x, y, add);
    if ((ii = it->second->getId()) > -1) {
      for (unsigned int j = 0; j < st.size(); j++) {
        name.push_back(it->second->getName());
        id.push_back(ii);
        station.push_back(st[j]);
      }
    }
  }
}

void StationManager::getEditStation(int step, std::string& name, int& id,
    vector<std::string>& stations)
{
  for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it) {
    if (it->second->getEditStation(step, name, id, stations))
      return;
  }
}

void StationManager::getStationData(vector<std::string>& data)
{
  for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it)
    data.push_back(it->second->stationRequest("selected"));
}

void StationManager::stationCommand(const std::string& command,
    const vector<std::string>& data, const std::string& name, int id, const std::string& misc)
{
  for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it) {
    if ((id == -1 || id == it->second->getId()) &&
        (name == it->second->getName() || name.empty())) {
      it->second->stationCommand(command, data, misc);
      break;
    }
  }
}

void StationManager::stationCommand(const std::string& command, const std::string& name,
    int id)
{
  if (command == "delete") {
    map <std::string,StationPlot*>::iterator p = stationPlots.begin();

    while (p != stationPlots.end()) {
      if ((name == "all" && (*p).second->getId() != -1) ||
          (id == (*p).second->getId() && (name == (*p).second->getName() || name.empty()))) {
        delete (*p).second;
        stationPlots.erase(p);
        if (name != "all")
          break;
        else
          p++;
      } else {
        p++;
      }
    }

  } else {
    for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it) {
      if ((id == -1 || id == it->second->getId()) &&
          (name == it->second->getName() || name.empty()))
        it->second->stationCommand(command);
    }
  }
}

vector <StationPlot*> StationManager::plots()
{
  vector <StationPlot*> values;
  for (stationPlots_t::iterator it = stationPlots.begin(); it != stationPlots.end(); ++it)
    values.push_back(it->second);

  return values;
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
      if (stations[i]->lat >= 0)
        stationsText += QString("%1&nbsp;N,&nbsp;").arg(stations[i]->lat);
      else
        stationsText += QString("%1&nbsp;S,&nbsp;").arg(-stations[i]->lat);
      if (stations[i]->lon >= 0)
        stationsText += QString("%1&nbsp;E").arg(stations[i]->lon);
      else
        stationsText += QString("%1&nbsp;W").arg(-stations[i]->lon);
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
