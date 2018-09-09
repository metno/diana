/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018 met.no

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

#ifndef DITRAJECTORYGENRATOR_H
#define DITRAJECTORYGENRATOR_H 1

#include <util/diKeyValue.h>
#include <puDatatypes/miCoordinates.h>
#include <puTools/miTime.h>
#include <string>
#include <vector>

class FieldPlotManager;
class FieldPlot;
class Field;


struct TrajectoryPoint {
  miutil::miTime time;
  LonLat position;
  TrajectoryPoint(const miutil::miTime& t, const LonLat& p)
    : time(t), position(p) { }
};

typedef std::vector<TrajectoryPoint> TrajectoryPoint_v;

// ========================================================================

class TrajectoryData {
public:
  int size() const
    { return mPoints.size(); }

  const LonLat& position(int i) const
    { return mPoints.at(i).position; }

  const miutil::miTime& time(int i) const
    { return mPoints.at(i).time; }

  std::string mLabel;
  TrajectoryPoint_v mPoints;
  int mStartIndex; //! which of the points is the one that was specified
};

typedef std::vector<TrajectoryData> TrajectoryData_v;

// ========================================================================

class TrajectoryGenerator {
public:
  typedef std::vector<LonLat> LonLat_v;

public:
  TrajectoryGenerator(FieldPlotManager* fpm, const FieldPlot* fp,
      const miutil::miTime& t);
  ~TrajectoryGenerator();

  void setIterationCount(int c)
    { numIterations = c; }

  void setTimeStep(float s)
    { timeStep = s; }

  void addPosition(const LonLat& position)
    { mStartPositions.push_back(position); }

  TrajectoryData_v compute();

private:
  void timeLoop(int i0, int di, const std::vector<miutil::miTime>& times);
  void reprojectStartPositions();
  void initAborted();

  //! returns true iff there are any trajectories that are not aborted
  bool haveTrajectories() const;

  LonLat_v reprojectStepPositions();

  /*! For positions outside the field grid, reproject to lon-lat and back
   *  so that the position might move back into the field grid (eg at the date line).
   * */
  void reprojectRoundTripLonLat();

  void calculateMapFields();
  void computeSingleStep(const miutil::miTime& t1, const miutil::miTime& t2,
      const std::vector<Field*>& fields1, const std::vector<Field*>& fields2,
      TrajectoryData_v& tracjectories);

private:
  FieldPlotManager* fpm;
  const FieldPlot* fp;
  miutil::miTime mPositionTime; //!< time for which the positions apply
  LonLat_v mStartPositions;     //!< a list of start positions
  int numIterations;
  float timeStep; //!< timestep in seconds

  // data used during compute()

  TrajectoryData_v trajectories;
  std::vector<bool> mAborted;
  float *xt, *yt, *u1, *v1, *u2, *v2, *rx, *ry;
  const Field *frx, *fry;
};

#endif // DITRAJECTORYGENRATOR_H
