/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2014 met.no

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

#ifndef DILOCATIONDATA_H
#define DILOCATIONDATA_H

#include <diField/diArea.h>

#include <vector>

/// Types of locations to be displayed
enum LocationType {
  location_line,
  location_unknown
};

/// Description of one location (name and position)
struct LocationElement {
  std::string name;
  std::vector<float> xpos;    // usually longitude
  std::vector<float> ypos;    // usually latitude
};

/// Data and info for a set of locations
struct LocationData {
  std::string         name;
  LocationType     locationType; // for all elements
  Area             area;      // only Projection used (usually geo)
  std::vector<LocationElement> elements;
  std::string         annotation;
  std::string         colour;
  std::string         linetype;
  float            linewidth;
  std::string         colourSelected;
  std::string         linetypeSelected;
  float            linewidthSelected;
};

#endif // DILOCATIONDATA_H
