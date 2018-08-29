/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2018 met.no

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

#ifndef OBSPLOTCLUSTER_H
#define OBSPLOTCLUSTER_H

#include "diPlotCluster.h"

#include "diTimeTypes.h"

class EditManager;
class ObsManager;
class ObsPlot;
struct ObsPlotCollider;

class ObsPlotCluster : public PlotCluster
{
public:
  ObsPlotCluster(ObsManager* obsm, EditManager* editm);
  ~ObsPlotCluster();

  const std::string& plotCommandKey() const override;

  void prepare(const PlotCommand_cpv& cmds) override;

  //! returns true iff there are any obs plots with data
  bool update(bool ifNeeded, const miutil::miTime& t);

  void plot(DiGLPainter* gl, PlotOrder zorder) override;

  void getDataAnnotations(std::vector<std::string>& anno) const;

  void getExtraAnnotations(std::vector<AnnotationPlot*>& vap);

  plottimes_t getTimes();

  const std::string& keyPlotElement() const override;

  bool findObs(int x, int y);

  std::string getObsPopupText(int x, int y);

  void nextObs(bool next);

  std::vector<ObsPlot*> getObsPlots() const;

protected:
  ObsPlot* at(size_t i) const;

private:
  bool hasDevField_;
  std::unique_ptr<ObsPlotCollider> collider_;

  ObsManager* obsm_;
  EditManager* editm_;
};

#endif // OBSPLOTCLUSTER_H
