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

#ifndef PLOTCLUSTER_H
#define PLOTCLUSTER_H

#include "diAnnotationPlot.h" // AnnotationPlot::Annotation
#include "diPlot.h"
#include "diPlotCommand.h"
#include "diPlotElement.h"
#include "diPlotOrder.h"

#include <vector>

class PlotCluster
{
public:
  PlotCluster(const std::string& pck, const std::string& pek);
  PlotCluster(const std::string& key);

  virtual ~PlotCluster();

  const std::string& plotCommandKey() const { return pck_; }
  const std::string& keyPlotElement() const { return pek_; }

  virtual void cleanup();

  virtual void processInput(const PlotCommand_cpv& cmds);

  virtual void setCanvas(DiCanvas* canvas);

  virtual void plot(DiGLPainter* gl, PlotOrder zorder);

  virtual plottimes_t getTimes();

  virtual void addAnnotations(std::vector<AnnotationPlot::Annotation>& annotations);

  virtual void addPlotElements(std::vector<PlotElement>& pel);

  virtual bool enablePlotElement(const PlotElement& pe);

  virtual void changeProjection(const Area& mapArea, const Rectangle& plotSize);
  virtual void changeTime(const miutil::miTime& mapTime);

  bool empty() const { return plots_.empty(); }

protected:
  virtual void processInputPE(const PlotCommand_cpv& cmds);
  virtual void add(Plot* plot);

protected:
  typedef std::vector<Plot*> Plot_xv;

  Plot_xv plots_;
  DiCanvas* canvas_;

private:
  const std::string& pck_;
  const std::string& pek_;
};

#endif // PLOTCLUSTER_H
