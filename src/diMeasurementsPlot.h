/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diTrajectoryPlot.h 1 2007-09-12 08:06:42Z lisbethb $

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
#ifndef diMeasurementsPlot_h
#define diMeasurementsPlot_h

#include <diPlot.h>
#include <vector>
#include <deque>
#include <diLinetype.h>

/**
   \brief plots positions used for distance and velocity measurements
*/
class MeasurementsPlot : public Plot {

private:

  Colour colour;
  int lineWidth;
  Linetype lineType;
  std::vector<float> x;
  std::vector<float> y;
  std::vector<float> lat;
  std::vector<float> lon;
  Area oldArea;

public:
  // Constructors
  MeasurementsPlot();
  // Destructor
  ~MeasurementsPlot();

  bool plot();
  bool plot(const int){return false;}
  ///change projection
  bool prepare(void);
  ///Start positions, colours, lines, field, etc
  void measurementsPos(const std::vector<std::string>&);
};

#endif
