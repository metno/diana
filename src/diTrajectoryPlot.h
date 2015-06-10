/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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
#ifndef diTrajectoryPlot_h
#define diTrajectoryPlot_h

#include "diPlot.h"
#include "diLinetype.h"
#include "diTrajectoryGenerator.h"

#include <QPolygonF>
#include <vector>

/**
   \brief Computes and plots 2-D trajectories from wind fields shown
*/
class TrajectoryPlot : public Plot {
public:
  TrajectoryPlot();
  ~TrajectoryPlot();

  void plot(DiGLPainter* gl, PlotOrder zorder);

  ///change projection
  bool prepare(void);

  ///Start positions, colours, lines, field, etc
  int trajPos(const std::vector<std::string>&);

  const std::string& getFieldName() const
    { return fieldStr; }

  const TrajectoryGenerator::LonLat_v& getStartPositions() const
    { return startPositions; }

  int getIterationCount() const
    { return numIterations; }

  float getTimeStep() const
    { return timeStep; }

  void setTrajectoryData(const TrajectoryData_v& t);

  void getTrajectoryAnnotation(std::string& s, Colour& c);

  bool printTrajectoryPositions(const std::string& filename);

private:
  TrajectoryPlot(const TrajectoryPlot &rhs); // not implemented
  TrajectoryPlot& operator=(const TrajectoryPlot &rhs); // not implemented

  void clearData();

private:
  std::string fieldStr;

  Colour colourPast;
  Colour colourFuture;
  int lineWidth;
  Linetype lineType;
  bool plot_on;

  Area oldArea;
  bool mNeedPrepare;

  TrajectoryGenerator::LonLat_v startPositions;

  TrajectoryData_v trajectories;
  std::vector<QPolygonF> reprojectedXY;

  float  timeStep;      // timestep in seconds
  int    numIterations; // fixed no. of iterations each timestep
};

#endif
