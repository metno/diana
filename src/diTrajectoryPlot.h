/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2006 met.no

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

#include <diPlot.h>
#include <vector>
#include <deque>
#include <diLinetype.h>

class Field;

/**
   \brief Computes and plots 2-D trajectories from wind fields shown
*/
class TrajectoryPlot : public Plot {
private:
  std::string fieldStr;
  Colour colour;
  int lineWidth;
  Linetype lineType;
  int numMarker;
  int markerRadius;
  std::vector<float> x;
  std::vector<float> y;
  std::vector<float> lat;
  std::vector<float> lon;
  Area oldArea;
  bool plot_on;
  int timeMarker;

 struct TrajectoryData {
  Area    area;
  int     ndata;
  miutil::miTime *time;   // time[ndata]
  int    *first;  // first existing index, "x[i][first[i]]"
  int    *last;   // last  existing index, "x[i][ last[i]]"
  float  *x;  // "x[numTraj][ndata]"
  float  *y;  // "y[numTraj][ndata]"
};


  Field *fu1;
  Field *fv1;

  miutil::miTime firstTime;
  miutil::miTime lastTime;
  float  timeStep;      // timestep in seconds
  int    numIterations; // fixed no. of iterations each timestep
		        // (no convergence test)
  int    numTraj;       // no. of trajectories (start positions)

  bool computing;
  bool firstStep;

  Area fieldArea;

  bool* runningForward;
  bool* runningBackward;

  std::deque<TrajectoryData*> vtrajdata;

  TrajectoryPlot(const TrajectoryPlot &rhs){}

public:
  TrajectoryPlot();
  ~TrajectoryPlot();

  void plot(PlotOrder zorder);

  ///change projection
  bool prepare(void);
  ///Start positions, colours, lines, field, etc
  int  trajPos(const std::vector<std::string>&);

  bool startComputation(std::vector<Field*> vf);
  void stopComputation();
  void clearData();
  bool compute(std::vector<Field*> vf);
  void getTrajectoryAnnotation(std::string& s, Colour& c);

  bool inComputation() { return computing; }
  std::string getFieldName() { return fieldStr; }
  bool printTrajectoryPositions(const std::string& filename);
};

#endif
