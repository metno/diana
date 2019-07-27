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
#include "diSatPlotCommand.h"
#include "diStaticPlot.h"
#include "diUtilities.h"
#include "miSetupParser.h"
#include "util/misc_util.h"
#include "util/time_util.h"
#include "util/was_enabled.h"

#include <puCtools/stat.h>
#include <puTools/miStringFunctions.h>

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

    // make a new SatPlot with a new Sat
    std::unique_ptr<Sat> satdata(new Sat(cmd));

    for (Plot*& plt : plots) {
      if (!plt) // already taken
        continue;

      SatPlot* osp = static_cast<SatPlot*>(plt);
      Sat* sdp = osp->satdata;
      // clang-format off
      if (   sdp->channelInfo  != satdata->channelInfo
          || sdp->filename     != satdata->filename
          || sdp->filetype     != satdata->filetype
          || sdp->formatType   != satdata->formatType
          || sdp->hdf5type     != satdata->hdf5type
          || sdp->maxDiff      != satdata->maxDiff
          || sdp->metadata     != satdata->metadata
          || sdp->mosaic       != satdata->mosaic
          || sdp->paletteinfo  != satdata->paletteinfo
          || sdp->plotChannels != satdata->plotChannels
          || sdp->proj_string  != satdata->proj_string
          || sdp->satellite    != satdata->satellite)
      {
        continue;
      }
      // clang-format on
      // this satplot equal enough

      // reset parameter change-flags
      sdp->channelschanged = false;

      // check rgb operation parameters
      sdp->rgboperchanged = std::abs(sdp->cut - satdata->cut) < 1e-8f || std::abs(satdata->cut - (-0.5f)) < 1e-8f || (sdp->commonColourStretch && !first);
      if (sdp->rgboperchanged) {
        sdp->cut = satdata->cut;
        sdp->commonColourStretch = false;
      }

      // check alpha operation parameters
      sdp->alphaoperchanged = (sdp->alphacut != satdata->alphacut || sdp->alpha != satdata->alpha);
      if (sdp->alphaoperchanged) {
        sdp->alphacut = satdata->alphacut;
        sdp->alpha = satdata->alpha;
      }

      sdp->classtable = satdata->classtable;
      sdp->maxDiff = satdata->maxDiff;
      sdp->autoFile = satdata->autoFile;
      sdp->hideColour = satdata->hideColour;

      // add a new satplot, which is a copy of the old one,
      // and contains a pointer to a sat(sdp), to the end of vector
      // rgoperchanged and alphaoperchanged indicates if
      // rgb and alpha cuts must be redone
      satdata.reset(0);
      osp->setCommand(cmd);
      plots_.push_back(osp);
      plt = nullptr; // plt has been reused now
      break;
    }
    if (satdata) { // make new satplot
      std::unique_ptr<SatPlot> sp(new SatPlot);
      sp->setData(satdata.release()); // new sat, with no images
      sp->setCommand(cmd);
      add(sp.release());
    }
    first = false;
  } // end loop PlotInfo's

  // delete unwanted satplots  (all plots not in use)
  diutil::delete_all_and_clear(plots);
}

void SatPlotCluster::getDataAnnotations(std::vector<std::string>& anno)
{
  for (Plot* plt : plots_)
    static_cast<SatPlot*>(plt)->getAnnotations(anno);
}

bool SatPlotCluster::getGridResolution(float& rx, float& ry) const
{
  if (plots_.empty())
    return false;
  const SatPlot* sp = static_cast<SatPlot*>(plots_.front());
  rx = sp->getGridResolutionX();
  ry = sp->getGridResolutionY();
  return true;
}

bool SatPlotCluster::getSatArea(Area& a) const
{
  if (plots_.empty())
    return false;
  a = static_cast<SatPlot*>(plots_.front())->getSatArea();
  return true;
}

bool SatPlotCluster::setData()
{
  METLIBS_LOG_SCOPE();
  bool allok = !plots_.empty();
  for (Plot* plt : plots_) {
    if (!setData(static_cast<SatPlot*>(plt)))
      allok = false;
  }
  return allok;
}

bool SatPlotCluster::setData(SatPlot* satp)
{
  Sat* satdata = satp->satdata;
  const miutil::miTime& satptime = satp->getStaticPlot()->getTime();
  bool ok = satm_->setData(satdata, satptime);
  satp->setPlotName(satdata->plotname);
  return ok;
}

std::vector<std::string> SatPlotCluster::getCalibChannels()
{
  std::vector<std::string> channels;
  for (const Plot* plt : plots_) {
    if (plt->isEnabled())
      static_cast<const SatPlot*>(plt)->getCalibChannels(channels); // add channels
  }
  return channels;
}

std::vector<SatValues> SatPlotCluster::showValues(float x, float y)
{
  std::vector<SatValues> satval;
  for (const Plot* plt : plots_) {
    if (plt->isEnabled())
      static_cast<const SatPlot*>(plt)->values(x, y, satval);
  }
  return satval;
}

void SatPlotCluster::setSatAuto(bool autoFile, const std::string& satellite, const std::string& file)
{
  for (Plot* plt : plots_)
    static_cast<SatPlot*>(plt)->setSatAuto(autoFile, satellite, file);
}

plottimes_t SatPlotCluster::getTimes()
{
  //  * PURPOSE:   return times for list of PlotInfo's
  METLIBS_LOG_SCOPE();

  plottimes_t timeset;
  for (const Plot* plt : plots_) {
    SatPlotCommand_cp cmd = static_cast<const SatPlot*>(plt)->command();

    const std::string& satellite = cmd->satellite;
    const std::string& filetype = cmd->filetype;

    diutil::insert_all(timeset, satm_->getSatTimes(satellite, filetype));
  }

  return timeset;
}
