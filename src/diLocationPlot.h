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
#ifndef LocationPlot_h
#define LocationPlot_h

#include "diPlot.h"
#include "diLocationData.h"

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
  virtual ~LocationPlot();

  bool setData(const LocationData& locationdata);
  void updateOptions(const LocationData& locationdata);

  void setSelected(const std::string& name)
	{ selectedName= name; }

  void hide() { visible= false; }
  void show() { visible= true; }
  bool isVisible() const { return visible; }
  void plot(PlotOrder zorder);
  bool changeProjection();
  std::string getName() { return locdata.name; }
  std::string find(int x, int y);
  void getAnnotation(std::string &str, Colour &col);

private:
  void drawLineOrPoint(int l);

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

  std::vector<InternalLocationInfo> locinfo;

  std::string selectedName;

  Area  posArea;
  int   numPos;
  float *px;
  float *py;

  void cleanup();

};

#endif // LocationPlot_h
