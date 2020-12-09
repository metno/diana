/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2020 met.no

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

#include "diFieldPlotCluster.h"

#include "diField/diField.h"
#include "diFieldPlot.h"
#include "diFieldPlotManager.h"
#include "util/misc_util.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.FieldPlotCluster"
#include <miLogger/miLogging.h>

namespace {
const std::string FIELD = "FIELD";
}

FieldPlotCluster::FieldPlotCluster(FieldPlotManager* fieldplotm)
    : PlotCluster(FIELD)
    , fieldplotm_(fieldplotm)
{
}

FieldPlotCluster::~FieldPlotCluster()
{
}

void FieldPlotCluster::plot(DiGLPainter* gl, PlotOrder zorder)
{
  if (zorder != PO_OVERLAY && zorder != PO_OVERLAY_TOP)
    PlotCluster::plot(gl, zorder);
}

void FieldPlotCluster::processInputPE(const PlotCommand_cpv& inp)
{
  cleanup();

  for (auto pc : inp) {
    std::unique_ptr<FieldPlot> fp(fieldplotm_->createPlot(pc));
    if (fp.get())
      add(fp.release());
  }
}

plottimes_t FieldPlotCluster::getTimes()
{
  std::vector<FieldPlotCommand_cp> commands;
  for (FieldPlot* fp : diutil::static_content_cast<FieldPlot*>(plots_))
    commands.push_back(fp->command());
  return fieldplotm_->getFieldTime(commands);
}

plottimes_t FieldPlotCluster::fieldAnalysisTimes() const
{
  plottimes_t fat;
  for (FieldPlot* fp : diutil::static_content_cast<FieldPlot*>(plots_)) {
    const miutil::miTime& ti = fp->getAnalysisTime();
    if (!ti.undef())
      fat.insert(ti);
  }
  return fat;
}

int FieldPlotCluster::getVerticalLevel() const
{
  if (!plots_.empty())
    return static_cast<const FieldPlot*>(plots_.back())->getLevel(); // TODO why last and not first?
  else
    return 0;
}

bool FieldPlotCluster::getRealFieldArea(Area& newMapArea) const
{
  // set area equal to first EXISTING field-area ("all timesteps"...)
  for (FieldPlot* fp : diutil::static_content_cast<FieldPlot*>(plots_)) {
    if (fp->getRealFieldArea(newMapArea))
      return true;
  }
  return false;
}


bool FieldPlotCluster::MapToGrid(const Projection& plotproj, float xmap, float ymap, float& gridx, float& gridy) const
{
  FieldPlot* fp = first();
  if (fp && plotproj == fp->getFieldArea().P()) {
    const Field_pv& ff = fp->getFields();
    if (!ff.empty()) {
      gridx = ff[0]->area.toGridX(xmap);
      gridy = ff[0]->area.toGridY(ymap);
      return true;
    }
  }
  return false;
}

miutil::miTime FieldPlotCluster::getFieldReferenceTime() const
{
  if (plots_.empty())
    return miutil::miTime();

  return first()->getReferenceTime();
}

std::vector<std::string> FieldPlotCluster::getTrajectoryFields()
{
  std::vector<std::string> vstr;
  for (FieldPlot* fp : diutil::static_content_cast<FieldPlot*>(plots_)) {
    std::string fname = fp->getTrajectoryFieldName();
    if (!fname.empty())
      vstr.push_back(fname);
  }
  return vstr;
}

const FieldPlot* FieldPlotCluster::findTrajectoryPlot(const std::string& fieldname)
{
  for (FieldPlot* fp : diutil::static_content_cast<FieldPlot*>(plots_)) {
    if (miutil::to_lower(fp->getTrajectoryFieldName()) == fieldname)
      return fp;
  }
  return 0;
}

std::vector<FieldPlot*> FieldPlotCluster::getFieldPlots() const
{
  const auto fps = diutil::static_content_cast<FieldPlot*>(plots_);
  return std::vector<FieldPlot*>(fps.begin(), fps.end());
}

FieldPlot* FieldPlotCluster::first() const
{
  return plots_.empty() ? nullptr : static_cast<FieldPlot*>(plots_.front());
}
