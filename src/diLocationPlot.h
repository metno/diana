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
#ifndef LocationPlot_h
#define LocationPlot_h

#include <iostream>
#include <vector>
#include <diPlot.h>
#include <puTools/miString.h>
#include <diField/diArea.h>

using namespace std;

  /// Types of locations to be displayed
  enum LocationType {
    location_line,
    location_unknown
  };

  /// Description of one location (name and position)
  struct LocationElement {
    miString name;
    vector<float> xpos;    // usually longitude
    vector<float> ypos;    // usually latitude
  };

  /// Data and info for a set of locations
  struct LocationData {
    miString         name;
    LocationType     locationType; // for all elements
    Area             area;      // only Projection used (usually geo)
    vector<LocationElement> elements;
    miString         annotation;
    miString         colour;
    miString         linetype;
    float            linewidth;
    miString         colourSelected;
    miString         linetypeSelected;
    float            linewidthSelected;
  };


/**
   \brief Plot pickable Locations on the map

   Contains data for a set of locations.
   At present only used to display lines of Vertical Crossections
   on the map.
*/
class LocationPlot : public Plot {

public:

  // constructor
  LocationPlot();

  // destructor
  ~LocationPlot();

  bool setData(const LocationData& locationdata);
  void updateOptions(const LocationData& locationdata);

  void setSelected(const miString& name)
	{ selectedName= name; }

  void hide() { visible= false; }
  void show() { visible= true; }
  bool isVisible() { return visible; }
  bool plot();
  bool plot(const int) { return false; }
  bool changeProjection();
  miString getName() { return locdata.name; }
  miString find(int x, int y);
  void getAnnotation(miString &str, Colour &col);

private:

  struct InternalLocationInfo {
    int   beginpos;
    int   endpos;
    float xmin,xmax;
    float ymin,ymax;
    float dmax;
  };

  bool visible;

  LocationData locdata;

  vector<InternalLocationInfo> locinfo;

  miString selectedName;

  Area  posArea;
  int   numPos;
  float *px;
  float *py;

  void cleanup();

};

#endif




