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
#ifndef diMeasurementsPlot_h
#define diMeasurementsPlot_h

#include "diPlot.h"
#include "diLinetype.h"

#include <vector>

/**
   \brief plots positions used for distance and velocity measurements
*/
class MeasurementsPlot : public Plot {

private:
  Colour colour;
  int lineWidth;
  Linetype lineType;
  float *xpos;
  float *ypos;
  std::vector<float> lat;
  std::vector<float> lon;
  Projection oldProjection;
  bool needReprojection;

public:
  MeasurementsPlot();
  ~MeasurementsPlot();

  void plot(Plot::PlotOrder porder);

  void changeProjection();

  /// define positions, colours, lines, field, etc
  void measurementsPos(const std::vector<std::string>&);

private:
  void reproject();

  // deallocate xpos and ypos
  void clearXY();

  MeasurementsPlot(const MeasurementsPlot&);
  MeasurementsPlot& operator=(const MeasurementsPlot&);
};

#endif
