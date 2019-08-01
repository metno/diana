/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2019 met.no

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

#include "diObsPlotCluster.h"

#include "diEditManager.h"
#include "diKVListPlotCommand.h"
#include "diObsManager.h"
#include "diObsPlot.h"
#include "diUtilities.h"
#include "util/was_enabled.h"
#include "util/misc_util.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.ObsPlotCluster"
#include <miLogger/miLogging.h>

namespace {
const std::string OBS = "OBS";
} // namespace

using diutil::static_content_cast;

ObsPlotCluster::ObsPlotCluster(ObsManager* obsm, EditManager* editm)
    : PlotCluster(OBS)
    , hasDevField_(false)
    , collider_(new ObsPlotCollider)
    , obsm_(obsm)
    , editm_(editm)
{
}

ObsPlotCluster::~ObsPlotCluster()
{
}

void ObsPlotCluster::processInputPE(const PlotCommand_cpv& inp)
{
  // for now -- erase all obsplots etc..
  //first log stations plotted
  for (ObsPlot* op : static_content_cast<ObsPlot*>(plots_))
    op->logStations();
  cleanup();
  hasDevField_ = false;

  for (PlotCommand_cp pc : inp) {
    if (KVListPlotCommand_cp cmd = std::dynamic_pointer_cast<const KVListPlotCommand>(pc)) {
      const std::string dialogname = ObsPlot::extractDialogname(cmd->all());
      if (dialogname.empty()) {
        METLIBS_LOG_WARN("probably malformed observation plot specification");
        continue;
      }
      const ObsPlotType plottype = ObsPlot::extractPlottype(dialogname);
      std::unique_ptr<ObsPlot> op(new ObsPlot(dialogname, plottype));
      obsm_->setPlotDefaults(op.get());
      op->setPlotInfo(cmd->all());
      op->setCollider(collider_.get());
      hasDevField_ |= op->mslp();

      add(op.release());
    }
  }
  collider_->clear();
}

void ObsPlotCluster::changeTime(const miutil::miTime& mapTime)
{
  update(false, mapTime);
}

void ObsPlotCluster::update(bool ifNeeded, const miutil::miTime& t)
{
  if (!ifNeeded) {
    for (ObsPlot* op : static_content_cast<ObsPlot*>(plots_))
      op->logStations();
  }
  for (ObsPlot* op : static_content_cast<ObsPlot*>(plots_)) {
    if (!ifNeeded || obsm_->updateTimes(op)) {
      obsm_->prepare(op, t);
    }
    //update list of positions ( used in "PPPP-mslp")
    // TODO this is kind of prepares changeProjection of all ObsPlot's with mslp() == true, to be used in EditManager::interpolateEditFields
    op->updateObsPositions();
  }
}

void ObsPlotCluster::plot(DiGLPainter* gl, PlotOrder zorder)
{
  if (zorder != PO_LINES && zorder != PO_OVERLAY)
    return;

  collider_->clear();

  // plot observations (if in fieldEditMode  and the option obs_mslp is true, plot observations in overlay)

  const bool obsedit = (hasDevField_ && editm_->isObsEdit());
  const bool plotoverlay = (zorder == PO_OVERLAY && obsedit);
  const bool plotunderlay = (zorder == PO_LINES && !obsedit);

  if (plotoverlay) {
    for (ObsPlot* op : static_content_cast<ObsPlot*>(plots_)) {
      if (editm_->interpolateEditField(op->getObsPositions()))
        op->updateFromEditField();
    }
  }
  if (plotunderlay || plotoverlay)
    PlotCluster::plot(gl, zorder);
}

std::vector<AnnotationPlot*> ObsPlotCluster::getExtraAnnotations() const
{
  std::vector<AnnotationPlot*> vap;
  for (ObsPlot* op : static_content_cast<ObsPlot*>(plots_)) {
    if (!op->isEnabled())
      continue;
    for (PlotCommand_cp pc : op->getObsExtraAnnotations())
      vap.push_back(new AnnotationPlot(pc));
  }
  return vap;
}

plottimes_t ObsPlotCluster::getTimes()
{
  std::set<std::string> readernames;
  for (ObsPlot* op : static_content_cast<ObsPlot*>(plots_))
    diutil::insert_all(readernames, op->readerNames());
  return obsm_->getObsTimes(readernames);
}

bool ObsPlotCluster::findObs(int x, int y)
{
  bool found = false;

  for (ObsPlot* op : static_content_cast<ObsPlot*>(plots_))
    if (op->showpos_findObs(x, y))
      found = true;

  return found;
}

std::string ObsPlotCluster::getObsPopupText(int x, int y)
{
  std::string obsText;

  for (ObsPlot* op : static_content_cast<ObsPlot*>(plots_))
    if (op->getObsPopupText(x, y, obsText))
      break;

  return obsText;
}

void ObsPlotCluster::nextObs(bool next)
{
  for (ObsPlot* op : static_content_cast<ObsPlot*>(plots_))
    op->nextObs(next);
}

std::vector<ObsPlot*> ObsPlotCluster::getObsPlots() const
{
  const auto ops = static_content_cast<ObsPlot*>(plots_);
  return std::vector<ObsPlot*>(ops.begin(), ops.end());
}
