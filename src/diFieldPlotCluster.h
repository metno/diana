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

#ifndef FIELDPLOTCLUSTER_H
#define FIELDPLOTCLUSTER_H

#include "diPlotCluster.h"

class FieldPlot;
class FieldPlotManager;
class Projection;

class FieldPlotCluster : public PlotCluster
{
public:
  FieldPlotCluster(FieldPlotManager* fieldplotm);
  ~FieldPlotCluster();

  void changeTime(const miutil::miTime& mapTime) override;

  void getDataAnnotations(std::vector<std::string>& anno) override;

  plottimes_t getTimes() override;

  plottimes_t fieldAnalysisTimes() const;

  int getVerticalLevel() const;

  //! set area equal to first EXISTING field-area ("all timesteps"...)
  bool getRealFieldArea(Area&) const;

  bool MapToGrid(const Projection& plotproj, float xmap, float ymap, float& gridx, float& gridy) const;

  miutil::miTime getFieldReferenceTime() const;

  std::vector<std::string> getTrajectoryFields();

  const FieldPlot* findTrajectoryPlot(const std::string& fieldname);

  std::vector<FieldPlot*> getFieldPlots() const;

protected:
  void processInputPE(const PlotCommand_cpv& cmds) override;

  /// \returns first plot if not empty, else nullptr
  FieldPlot* first() const;

private:
  FieldPlotManager* fieldplotm_;
};

#endif // FIELDPLOTCLUSTER_H
