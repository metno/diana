/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2019-2021 met.no

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

#ifndef diObjectPlotCluster_h
#define diObjectPlotCluster_h

#include "diPlotCluster.h"

class ObjectManager;

class ObjectPlotCluster : public PlotCluster
{
public:
  ObjectPlotCluster(ObjectManager* objm);
  ~ObjectPlotCluster();

  void processInput(const PlotCommand_cpv& cmds) override;
  void changeProjection(const Area& mapArea, const Rectangle& plotSize, const diutil::PointI& physSize) override;
  void changeTime(const miutil::miTime& mapTime) override;
  plottimes_t getTimes() override;
  void addAnnotations(std::vector<AnnotationPlot::Annotation>& annotations) override;
  void getDataAnnotations(std::vector<std::string>& anno) const override;
  std::vector<AnnotationPlot*> getExtraAnnotations() const override;
  void addPlotElements(std::vector<PlotElement>& pel) override;
  bool enablePlotElement(const PlotElement& pe) override;
  void plot(DiGLPainter* gl, PlotOrder zorder) override;
  void cleanup() override;
  PlotStatus getStatus() override;

private:
  ObjectManager* objm;
};

#endif // diObjectPlotCluster_h
