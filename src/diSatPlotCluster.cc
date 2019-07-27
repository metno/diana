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

#include "diSatPlotCluster.h"

#include "diSatManager.h"
#include "diSatPlot.h"
#include "diUtilities.h"
#include "util/misc_util.h"

#define MILOGGER_CATEGORY "diana.SatPlotCluster"
#include <miLogger/miLogging.h>

using namespace miutil;

static const std::string SAT = "SAT";
static const std::string RASTER = "RASTER";

SatPlotCluster::SatPlotCluster(SatManager* satm)
    : PlotCluster(SAT, RASTER)
    , satm_(satm)
{
}

void SatPlotCluster::processInputPE(const PlotCommand_cpv& pinfo)
{
  //     PURPOSE:   Decode PlotInfo &pinfo
  //                - make a new SatPlot for each SAT entry in pinfo
  //                - if similar plot already exists, just make a copy of the
  //                  old one (satellite, filetype and channel the same)
  METLIBS_LOG_SCOPE();

  // FIXME this is almost the same as PlotModule::prepareMap

  std::vector<Plot*> plots = std::move(plots_);

  // loop through all PlotInfo's
  bool first = true;
  for (PlotCommand_cp pc : pinfo) {
    SatPlotCommand_cp cmd = std::dynamic_pointer_cast<const SatPlotCommand>(pc);
    if (!cmd)
      continue;

    bool reused = false;
    for (Plot*& plt : plots) {
      if (!plt) // already taken
        continue;

      SatPlot* osp = static_cast<SatPlot*>(plt);
      if (satm_->reusePlot(osp, cmd, first)) {
        plots_.push_back(osp);
        plt = nullptr; // plt has been reused now
        reused = true;
        break;
      }
    }
    if (!reused)
      add(satm_->createPlot(cmd));

    first = false;
  } // end loop PlotInfo's

  // delete unwanted satplots  (all plots not in use)
  diutil::delete_all_and_clear(plots);
}

bool SatPlotCluster::MapToGrid(const Projection& plotproj, float xmap, float ymap, float& gridx, float& gridy) const
{
  if (!plots_.empty()) {
    const GridArea& ga = static_cast<SatPlot*>(plots_.front())->getSatArea();
    if (ga.P() == plotproj) {
      gridx = ga.toGridX(xmap);
      gridy = ga.toGridY(ymap);
      return true;
    }
  }
  return false;
}

bool SatPlotCluster::getSatArea(Area& a) const
{
  if (plots_.empty())
    return false;
  a = static_cast<SatPlot*>(plots_.front())->getSatArea();
  return true;
}

std::vector<std::string> SatPlotCluster::getCalibChannels()
{
  std::vector<std::string> channels;
  for (const SatPlot* sp : diutil::static_content_cast<SatPlot*>(plots_)) {
    if (sp->isEnabled())
      sp->getCalibChannels(channels); // add channels
  }
  return channels;
}

std::vector<SatValues> SatPlotCluster::showValues(float x, float y)
{
  std::vector<SatValues> satval;
  for (const SatPlot* sp : diutil::static_content_cast<SatPlot*>(plots_)) {
    if (sp->isEnabled())
      sp->values(x, y, satval);
  }
  return satval;
}

plottimes_t SatPlotCluster::getTimes()
{
  //  * PURPOSE:   return times for list of PlotInfo's
  METLIBS_LOG_SCOPE();

  plottimes_t timeset;
  for (const SatPlot* sp : diutil::static_content_cast<SatPlot*>(plots_)) {
    SatPlotCommand_cp cmd = sp->command();
    diutil::insert_all(timeset, satm_->getSatTimes(cmd->image_name, cmd->subtype_name));
  }

  return timeset;
}
