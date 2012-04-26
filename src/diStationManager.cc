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

#include <puTools/miSetupParser.h>

#include <diCommonTypes.h>
#include <diStationManager.h>
#include <diStationPlot.h>
#include <diObsAscii.h>

using namespace miutil;

StationManager::StationManager()
{
}

/**
 * Updates the vector of \a plots with the data provided in \a inp.
*/
bool StationManager::init(const vector<miutil::miString>& inp)
{
  map <miutil::miString,StationPlot*>::iterator it;

  // Hide all the station plots to begin with so that we can later
  // show and select/unselect them.

  for (it = stationPlots.begin(); it != stationPlots.end(); ++it)
    (*it).second->hide();

  for (unsigned int i = 0; i < inp.size(); ++i) {

    miutil::miString plotStr = inp[i];

    vector<miutil::miString> pieces = plotStr.split(" ");
    pieces.erase(pieces.begin());

    miutil::miString select = pieces.back();
    pieces.pop_back();

    miutil::miString url = pieces.back();
    pieces.pop_back();

    miutil::miString name;
    name.join(pieces, " ");

    it = stationPlots.find(name);
    StationPlot* plot;

    if (it == stationPlots.end()) {
      plot = importStations(name, url);
      putStations(plot);
    } else
      plot = it->second;

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
  // Create stationSetInfo objects to be stored for later retrieval by the StationDialog.
  vector<miString> section;

  // Return true if there is no STATIONS section.
  if (!SetupParser::getSection("STATIONS", section))
    return true;

  for (unsigned int i = 0; i < section.size(); ++i) {
    vector<miString> pieces = section[i].split("=");
    if (pieces.size() == 2) {
      stationSetInfo s_info;
      s_info.name = pieces[0];
      s_info.url = pieces[1];
      m_info.chosen[s_info.url] = false;
      m_info.sets.push_back(s_info);
    } else
      cerr << __FUNCTION__ << ": Invalid line in setup file: " << section[i] << endl;
  }

  return true;
}

/**
 * Imports the set of stations from the specified \a url.
 */
StationPlot* StationManager::importStations(miutil::miString& name, miutil::miString& url)
{
  vector<miutil::miString> lines;
  bool success = false;

  if (url.find("http://") == 0)
    success = ObsAscii::getFromHttp(url, lines);
  else
    success = ObsAscii::getFromFile(url, lines);

  if (!success) {
#ifdef DEBUGPRINT
    cerr << "*** Failed to open " << url << endl;
#endif
    return 0;
  }

  vector<Station*> stations;

  for (unsigned int i = 0; i < lines.size(); ++i) {

    vector<miutil::miString> pieces = lines[i].split(";");
    if (pieces.size() >= 4) {

      // Create a station with the latitude, longitude and a combination of the name and station number.
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

      stations.push_back(station);
    }
  }

  // Construct a new StationPlot object.
  StationPlot *plot = new StationPlot(stations);
  plot->setName(name);

  return plot;
}

float StationManager::getStationsScale()
{
  if(!stationPlots.empty()) {
    map <miutil::miString,StationPlot*>::iterator it = stationPlots.begin();
    return (*it).second->getImageScale(0);
  } else
    return .0;
}

void StationManager::setStationsScale(float new_scale)
{
  map <miutil::miString,StationPlot*>::iterator p = stationPlots.begin();

  while (p != stationPlots.end()) {
    (*p).second->setImageScale(new_scale);
    ++p;
  }
}

//********** plotting and selecting stations on the map***************

void StationManager::putStations(StationPlot* stationPlot)
{
#ifdef DEBUGPRINT
  cerr << "PlotModule::putStations"<< endl;
#endif

  miutil::miString name = stationPlot->getName();
  map <miutil::miString,StationPlot*>::iterator p = stationPlots.begin();
  map <miutil::miString,StationPlot*>::iterator pend = stationPlots.end();

  //delete old stationPlot
  while (p != pend && name != (*p).second->getName())
    p++;
  if (p != pend) {
    if (!((*p).second->isVisible()))
      stationPlot->hide();
    stationPlot->enable((*p).second->Enabled());
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

void StationManager::makeStationPlot(const miutil::miString& commondesc,
    const miutil::miString& common, const miutil::miString& description, int from,
    const vector<miutil::miString>& data)
{
  StationPlot* stationPlot = new StationPlot(commondesc, common, description,
      from, data);
  putStations(stationPlot);
}

Station* StationManager::findStation(int x, int y)
{
  map <miutil::miString,StationPlot*>::iterator it;
  for (it = stationPlots.begin(); it != stationPlots.end(); ++it) {
    Station* station = (*it).second->stationAt(x, y);
    if (station)
      return station;
  }
  return 0;
}

vector<Station*> StationManager::findStations(int x, int y)
{
  map <miutil::miString,StationPlot*>::iterator it;
  vector<Station*> stations;
  for (it = stationPlots.begin(); it != stationPlots.end(); ++it) {
    if ((*it).second->isVisible()) {
      vector<Station*> found = (*it).second->stationsAt(x, y);
      stations.insert(stations.end(), found.begin(), found.end());
    }
  }
  return stations;
}

miutil::miString StationManager::findStation(int x, int y, miutil::miString name, int id)
{
  map <miutil::miString,StationPlot*>::iterator it;
  for (it = stationPlots.begin(); it != stationPlots.end(); ++it) {
    if ((id == -1 || id == (*it).second->getId()) && (name == (*it).second->getName())) {
      vector<miutil::miString> st = (*it).second->findStation(x, y);
      if (st.size() > 0)
        return st[0];
    }
  }
  return miutil::miString();
}

void StationManager::findStations(int x, int y, bool add, vector<miutil::miString>& name,
    vector<int>& id, vector<miutil::miString>& station)
{
  map <miutil::miString,StationPlot*>::iterator it;

  int ii;
  for (it = stationPlots.begin(); it != stationPlots.end(); ++it) {
    vector<miutil::miString> st = (*it).second->findStation(x, y, add);
    if ((ii = (*it).second->getId()) > -1) {
      for (unsigned int j = 0; j < st.size(); j++) {
        name.push_back((*it).second->getName());
        id.push_back(ii);
        station.push_back(st[j]);
      }
    }
  }

}

bool StationManager::getEditStation(int step, miutil::miString& name, int& id, vector<
    miutil::miString>& stations)
{

  bool updateArea = false;
  map <miutil::miString,StationPlot*>::iterator it = stationPlots.begin();
  while (it != stationPlots.end() && !(*it).second->getEditStation(step, name, id, stations,
      updateArea))
    it++;

  return updateArea;
}

void StationManager::stationCommand(const miutil::miString& command,
    vector<miutil::miString>& data, const miutil::miString& name, int id, const miutil::miString& misc)
{
  map <miutil::miString,StationPlot*>::iterator it;

  if (command == "selected") {
    for (it = stationPlots.begin(); it != stationPlots.end(); ++it) {
      data.push_back((*it).second->stationRequest(command));
    }
  } else { // use stationPlot with name and id
    for (it = stationPlots.begin(); it != stationPlots.end(); ++it) {
      if ((id == -1 || id == (*it).second->getId()) &&
          (name == (*it).second->getName() || !name.exists())) {
        (*it).second->stationCommand(command, data, misc);
        break;
      }
    }
  }
}

void StationManager::stationCommand(const miutil::miString& command, const miutil::miString& name,
    int id)
{
  if (command == "delete") {
    map <miutil::miString,StationPlot*>::iterator p = stationPlots.begin();

    while (p != stationPlots.end()) {
      if ((name == "all" && (*p).second->getId() != -1) ||
          (id == (*p).second->getId() && (name == (*p).second->getName() || !name.exists()))) {
        delete (*p).second;
        stationPlots.erase(p);
      } else {
        p++;
      }
    }

  } else {
    map <miutil::miString,StationPlot*>::iterator it;

    for (it = stationPlots.begin(); it != stationPlots.end(); ++it) {
      if ((id == -1 || id == (*it).second->getId()) &&
          (name == (*it).second->getName() || !name.exists()))
        (*it).second->stationCommand(command);
    }
  }
}

vector <StationPlot*> StationManager::plots()
{
  vector <StationPlot*> values;
  map <miutil::miString,StationPlot*>::iterator it;
  for (it = stationPlots.begin(); it != stationPlots.end(); ++it)
    values.push_back((*it).second);

  return values;
}
