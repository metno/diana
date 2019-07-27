/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2019 met.no

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

#include "diObjectPlotCluster.h"

#include "diObjectManager.h"

#define MILOGGER_CATEGORY "diana.ObjectPlotCluster"
#include <miLogger/miLogging.h>

static const std::string OBJECTS = "OBJECTS";

ObjectPlotCluster::ObjectPlotCluster(ObjectManager* objm)
    : PlotCluster(OBJECTS)
    , objm(objm)
{
  METLIBS_LOG_SCOPE();
}

ObjectPlotCluster::~ObjectPlotCluster() {}

void ObjectPlotCluster::changeProjection(const Area& mapArea, const Rectangle& plotSize)
{
  objm->changeProjection(mapArea, plotSize);
}

void ObjectPlotCluster::changeTime(const miutil::miTime& mapTime)
{
  objm->prepareObjects(mapTime);
}

void ObjectPlotCluster::processInput(const PlotCommand_cpv& cmds)
{
  objm->prepareObjects(cmds);
}

void ObjectPlotCluster::addPlotElements(std::vector<PlotElement>& pel)
{
  objm->addPlotElements(pel);
}

bool ObjectPlotCluster::enablePlotElement(const PlotElement& pe)
{
  return objm->enablePlotElement(pe);
}

plottimes_t ObjectPlotCluster::getTimes()
{
  return objm->getTimes();
}

void ObjectPlotCluster::addAnnotations(std::vector<AnnotationPlot::Annotation>& annotations)
{
  AnnotationPlot::Annotation ann;
  objm->getObjAnnotation(ann.str, ann.col);
  if (!ann.str.empty())
    annotations.push_back(ann);
}

void ObjectPlotCluster::getDataAnnotations(std::vector<std::string>& anno)
{
  objm->getDataAnnotations(anno);
}

std::vector<AnnotationPlot*> ObjectPlotCluster::getExtraAnnotations() const
{
  std::vector<AnnotationPlot*> vap;
  PlotCommand_cpv objLabels = objm->getObjectLabels();
  for (PlotCommand_cp pc : objLabels) {
    std::unique_ptr<AnnotationPlot> ap(new AnnotationPlot());
    if (ap->prepare(pc))
      vap.push_back(ap.release());
  }
  return vap;
}

void ObjectPlotCluster::plot(DiGLPainter* gl, PlotOrder zorder)
{
  objm->plotObjects(gl, zorder);
}

void ObjectPlotCluster::cleanup()
{
  objm->clearObjects();
}

bool ObjectPlotCluster::hasData()
{
  return objm->objectsDefined();
}
