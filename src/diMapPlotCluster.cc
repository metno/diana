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

#include "diMapPlotCluster.h"

#include "diMapPlot.h"
#include "util/misc_util.h"

#include <vector>

#define MILOGGER_CATEGORY "diana.MapPlotCluster"
#include <miLogger/miLogging.h>

namespace {
const std::string MAP = "MAP";
}

MapPlotCluster::MapPlotCluster()
    : PlotCluster(MAP)
{
}

MapPlotCluster::~MapPlotCluster() {}

void MapPlotCluster::addPlotElements(std::vector<PlotElement>&) {}

bool MapPlotCluster::enablePlotElement(const PlotElement&)
{
  return false;
}

const std::string& MapPlotCluster::getBackColour() const
{
  return bgcolourname_;
}

void MapPlotCluster::setBackColourFromPlot(const MapPlot* mp)
{
  if (!mp->bgcolourname().empty())
    bgcolourname_ = mp->bgcolourname();
}

void MapPlotCluster::processInputPE(const PlotCommand_cpv& inp)
{
  METLIBS_LOG_SCOPE();

  std::vector<Plot*> plots = std::move(plots_); // old vector of map plots

  for (PlotCommand_cp pc : inp) { // loop through all plotinfo's
    Plot* to_reuse = nullptr;
    for (Plot*& p : plots) {
      if (p) { // not already taken
        MapPlot* mp = static_cast<MapPlot*>(p);
        const bool use_plot = mp->prepare(pc, true);
        setBackColourFromPlot(mp);
        if (use_plot) {
          std::swap(p, to_reuse); // re-use, p = nullptr
          break;
        }
      }
    }
    if (to_reuse) {
      plots_.push_back(to_reuse);
    } else {
      // make new mapPlot object and push it on the list
      std::unique_ptr<MapPlot> mp(new MapPlot());
      const bool use_plot = mp->prepare(pc, false);
      setBackColourFromPlot(mp.get());
      if (use_plot)
        add(mp.release());
    }
  }

  diutil::delete_all_and_clear(plots); // delete unwanted mapplots
}
