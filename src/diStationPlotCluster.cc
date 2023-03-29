/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2019 met.no

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

#include "diStationPlotCluster.h"

#include "diStationManager.h"
#include "diStationPlot.h"
#include "diStationPlotCommand.h"
#include "util/misc_util.h"

#include <puTools/miStringFunctions.h>
#include <puTools/miTime.h>

#define MILOGGER_CATEGORY "diana.StationPlotCluster"
#include <miLogger/miLogging.h>

using namespace miutil;

static const std::string STATION = "STATION";

StationPlotCluster::StationPlotCluster(StationManager* stam)
    : PlotCluster(STATION)
    , stam_(stam)
{
}

StationPlotCluster::~StationPlotCluster() {}

StationPlotCluster::Plot_xv::iterator StationPlotCluster::findStationPlot(const std::string& name, int id, Plot_xv::iterator begin)
{
  for (; begin != plots_.end(); ++begin) {
    StationPlot* sp = static_cast<StationPlot*>(*begin);
    if (sp->getId() == id && sp->getName() == name)
      break;
  }
  return begin;
}

/**
 * Updates the vector of \a plots with the data provided in \a inp.
 */
void StationPlotCluster::processInputPE(const PlotCommand_cpv& inp)
{
  METLIBS_LOG_SCOPE();
  // Hide all the station plots to begin with so that we can later
  // show and select/unselect them.

  for (StationPlot* sp : diutil::static_content_cast<StationPlot*>(plots_))
    sp->unselect();

  for (PlotCommand_cp pc : inp) {
    StationPlotCommand_cp c = std::dynamic_pointer_cast<const StationPlotCommand>(pc);
    if (!c)
      continue;

    Plot_xv::iterator it = findStationPlot(c->name, -1, plots_.begin());
    StationPlot* plot = 0;

    if (it == plots_.end()) {
      // No existing plot exists. If the select part of the plot command
      // contains the word "hidden" then we ignore this set of stations.

      if (c->select != "hidden") {
        // Load the stations.
        plot = stam_->importStations(c->name, c->url);
        if (plot)
          putStations(plot);
      }

    } else {
      // An existing plot containing a loaded list of stations.
      plot = static_cast<StationPlot*>(*it);
    }

    if (plot) {
      const bool visible = (c->select != "hidden");
      plot->setVisible(visible);
      stam_->initDialog().chosen[c->url] = visible;

      if (c->select == "selected") {
        stam_->initDialog().selected = c->name;
        for (Station* s : plot->getStations())
          s->isSelected = true;

      } else
        plot->unselect();
    }
  }
}

//********** plotting and selecting stations on the map***************

void StationPlotCluster::putStations(StationPlot* stationPlot)
{
  METLIBS_LOG_SCOPE();

  Plot_xv::iterator p = findStationPlot(stationPlot->getName(), stationPlot->getId(), plots_.begin());
  if (p != plots_.end()) {
    StationPlot* old = static_cast<StationPlot*>(*p);
    if (!old->isVisible())
      stationPlot->setVisible(false);
    stationPlot->setEnabled(old->isEnabled());
    delete old;
    *p = stationPlot;
  } else {
    add(stationPlot);
  }
}

void StationPlotCluster::makeStationPlot(const std::string& commondesc, const std::string& common, const std::string& description, int from,
                                         const std::vector<std::string>& data)
{
  putStations(new StationPlot(commondesc, common, description, from, data));
}

Station* StationPlotCluster::findStation(int x, int y)
{
  for (StationPlot* sp : diutil::static_content_cast<StationPlot*>(plots_)) {
    Station* station = sp->stationAt(x, y);
    if (station)
      return station;
  }
  return 0;
}

std::vector<Station*> StationPlotCluster::findStations(int x, int y)
{
  std::vector<Station*> stations;
  for (StationPlot* sp : diutil::static_content_cast<StationPlot*>(plots_)) {
    if (sp->isVisible()) {
      diutil::insert_all(stations, sp->stationsAt(x, y, 5));
    }
  }
  return stations;
}

std::string StationPlotCluster::findStation(int x, int y, const std::string& name, int id)
{
  for (StationPlot* sp : diutil::static_content_cast<StationPlot*>(plots_)) {
    if ((id == -1 || id == sp->getId()) && (name == sp->getName())) {
      std::vector<std::string> st = sp->findStation(x, y);
      if (st.size() > 0)
        return st[0];
    }
  }
  return std::string();
}

std::vector<std::string> StationPlotCluster::findStations(int x, int y, const std::string& name, int id)
{
  for (StationPlot* sp : diutil::static_content_cast<StationPlot*>(plots_)) {
    if ((id == -1 || id == sp->getId()) && (name == sp->getName())) {
      std::vector<std::string> st = sp->findStations(x, y);
      if (st.size() > 0)
        return st;
    }
  }
  return std::vector<std::string>();
}

void StationPlotCluster::findStations(int x, int y, bool add, std::vector<std::string>& name, std::vector<int>& id, std::vector<std::string>& station)
{
  for (StationPlot* sp : diutil::static_content_cast<StationPlot*>(plots_)) {
    const int ii = sp->getId();
    if (ii > -1) {
      const std::vector<std::string> st = sp->findStation(x, y, add);
      for (size_t j = 0; j < st.size(); j++) {
        name.push_back(sp->getName());
        id.push_back(ii);
        station.push_back(st[j]);
      }
    }
  }
}

void StationPlotCluster::stationCommand(const std::string& command, const std::vector<std::string>& data, const std::string& name, int id, const std::string& misc)
{
  for (StationPlot* sp : diutil::static_content_cast<StationPlot*>(plots_)) {
    if ((id == -1 || id == sp->getId()) && (name == sp->getName() || name.empty())) {
      sp->stationCommand(command, data, misc);
      break;
    }
  }
}

void StationPlotCluster::stationCommand(const std::string& command, const std::string& name, int id)
{
  if (command == "delete") {
    // name == "all" && id == -1 => coserver disconnect => delete all plots with id >= 0
    // name == "" && id >= 0 => client disconnect => delete all plots with getId() == id
    for (Plot_xv::iterator it = plots_.begin(); it != plots_.end(); /*nothing*/) {
      StationPlot* sp = static_cast<StationPlot*>(*it);
      if ((name == "all" && sp->getId() != -1) || (id == sp->getId() && (name == sp->getName() || name.empty()))) {
        delete *it;
        it = plots_.erase(it);
      } else {
        it++;
      }
    }

  } else {
    for (StationPlot* sp : diutil::static_content_cast<StationPlot*>(plots_)) {
      if ((id == -1 || id == sp->getId()) && (name == sp->getName() || name.empty()))
        sp->stationCommand(command);
    }
  }
}

QString StationPlotCluster::getStationsText(int x, int y)
{
  QString stationsText;
  std::vector<Station*> allStations = findStations(x, y);
  std::vector<Station*> stations;
  for (unsigned int i = 0; i < allStations.size(); ++i) {
    if (allStations[i]->status != Station::noStatus)
      stations.push_back(allStations[i]);
  }

  if (stations.size() > 0) {

    // Count the number of times each station name appears in the list.
    // This is used later to decide whether or not to show the "auto" or
    // "vis" text.
    std::map<std::string, unsigned int> stationNames;
    for (unsigned int i = 0; i < stations.size(); ++i) {
      unsigned int number = stationNames.count(stations[i]->name);
      stationNames[stations[i]->name] = number + 1; // FIXME this is always == 2
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
      default:;
      }
      stationsText += "</td>";

      stationsText += "<td>";
      stationsText += QString("<a href=\"%1\">%2</a>").arg(QString::fromStdString(stations[i]->url)).arg(QString::fromStdString(stations[i]->name));
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
    }
    stationsText += "</table>";
  }

  return stationsText;
}

std::vector<StationPlot*> StationPlotCluster::getStationPlots() const
{
  const auto sps = diutil::static_content_cast<StationPlot*>(plots_);
  return std::vector<StationPlot*>(sps.begin(), sps.end());
}
