/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2021 met.no

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

#include "diPlotCluster.h"

#include "diUtilities.h" // delete_all_and_clear
#include "util/was_enabled.h"

#include <puTools/miStringFunctions.h>

PlotCluster::PlotCluster(const std::string& pck, const std::string& pek)
    : canvas_(nullptr)
    , pck_(pck)
    , pek_(pek)
{
}

PlotCluster::PlotCluster(const std::string& key)
    : PlotCluster(key, key)
{
}

PlotCluster::~PlotCluster()
{
  cleanup();
}

void PlotCluster::cleanup()
{
  diutil::delete_all_and_clear(plots_);
}

void PlotCluster::setCanvas(DiCanvas* canvas)
{
  if (canvas == canvas_)
    return;
  canvas_ = canvas;
  for (Plot* plt : plots_) {
    plt->setCanvas(canvas_);
  }
}

void PlotCluster::processInput(const PlotCommand_cpv& cmds)
{

  diutil::was_enabled plotenabled;
  for (Plot* plt : plots_)
    plotenabled.save(plt);

  processInputPE(cmds);

  for (Plot* plt : plots_)
    plotenabled.restore(plt);
}

plottimes_t PlotCluster::getTimes()
{
  return plottimes_t();
}

void PlotCluster::plot(DiGLPainter* gl, PlotOrder zorder)
{
  for (Plot* plt : plots_)
    plt->plot(gl, zorder);
}

void PlotCluster::addAnnotations(std::vector<AnnotationPlot::Annotation>& annotations)
{
  AnnotationPlot::Annotation ann;
  for (Plot* plt : plots_) {
    if (!plt->isEnabled())
      continue;

    plt->getAnnotation(ann.str, ann.col);
    if (!ann.str.empty())
      annotations.push_back(ann);
  }
}

void PlotCluster::getDataAnnotations(std::vector<std::string>& anno) const
{
  for (Plot* p : plots_)
    p->getDataAnnotations(anno);
}

std::vector<AnnotationPlot*> PlotCluster::getExtraAnnotations() const
{
  return std::vector<AnnotationPlot*>();
}

void PlotCluster::addPlotElements(std::vector<PlotElement>& pel)
{
  const std::string& key = keyPlotElement();
  for (size_t j = 0; j < plots_.size(); j++) {
    const Plot* p = plots_[j];
    const std::string& nm = p->getPlotName();
    if (!nm.empty()) {
      const std::string& icon_ = p->getIconName();
      const std::string& icon = icon_.empty() ? key : icon_;
      std::string str = nm + "# " + miutil::from_number(int(j));
      pel.push_back(PlotElement(key, str, icon, p->isEnabled()));
      pel.back().status = p->getStatus();
    }
  }
}

bool PlotCluster::enablePlotElement(const PlotElement& pe)
{
  if (pe.type != keyPlotElement())
    return false;
  for (unsigned int i = 0; i < plots_.size(); i++) {
    std::string str = plots_[i]->getPlotName() + "# " + miutil::from_number(int(i));
    if (str == pe.str) {
      Plot* op = plots_[i];
      if (op->isEnabled() != pe.enabled) {
        op->setEnabled(pe.enabled);
        return true;
      } else {
        break;
      }
    }
  }
  return false;
}

void PlotCluster::changeProjection(const Area& mapArea, const Rectangle& plotSize, const diutil::PointI& physSize)
{
  mapArea_ = mapArea;
  plotSize_ = plotSize;
  physSize_ = physSize;
  for (Plot* p : plots_)
    p->changeProjection(mapArea, plotSize, physSize);
}

void PlotCluster::changeTime(const miutil::miTime& mapTime)
{
  mapTime_ = mapTime;
  for (Plot* p : plots_)
    p->changeTime(mapTime);
}

PlotStatus PlotCluster::getStatus()
{
  PlotStatus pcs;
  for (Plot* p : plots_) {
    pcs.add(p->getStatus());
  }
  return pcs;
}

void PlotCluster::processInputPE(const PlotCommand_cpv&) {}

void PlotCluster::add(Plot* plot)
{
  plot->setCanvas(canvas_);
  plot->changeProjection(mapArea_, plotSize_, physSize_);
  plots_.push_back(plot);
}
