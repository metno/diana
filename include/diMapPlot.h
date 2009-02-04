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
#ifndef diMapPlot_h
#define diMapPlot_h

#include <diPlot.h>
#include <miString.h>
#include <vector>
#include <map>
#include <set>
#include <GL/gl.h>
#include <diFilledMap.h>
#include <diShapeObject.h>

using namespace std;

/**

 \brief Map layer plotting
 
 plots the map layer
 - simple coastlines plotting
 - filled land type (precalculated triangles)
 - lat/lon lines

 */

class MapPlot : public Plot {
private:
  bool mapchanged; // redraw needed
  bool haspanned;
  MapInfo mapinfo;
  PlotOptions contopts; // contour options
  PlotOptions landopts; // land plot options
  PlotOptions llopts; // latlon options
  PlotOptions ffopts; // frame options
  bool areadefined; // area explicitly defined
  Area reqarea; // requested area
  bool isactive[3]; // active data for zorder
  bool usedrawlists; // use OpenGL drawlists
  GLuint drawlist[3]; // openGL drawlists

  static map<miString,FilledMap> filledmaps;
  static set<miString> usedFilledmaps;
  static map<miString,ShapeObject> shapemaps;
  static map<miString,Area> shapeareas;

  void xyclip(int, float[], float[], float[]);
  bool pland4(const miString&, int, float[], float[], const Linetype&, float,
      const Colour&);
  bool geoGrid(float latitudeStep, float longitudeStep, int plotResolution= 10);
  bool plotLinesSimpleText(const miString& filename);

public:
  MapPlot();
  ~MapPlot();

  // check for changing mapfiles
  static bool checkFiles(bool);
  void markFiles();

  bool plot()
  {
    return false;
  }
  /// plot map in a specific zorder layer
  bool plot(const int zorder);
  /// parse plotinfo
  bool prepare(const miString&, bool ifequal =true);
  /// return the area asked for
  bool requestedArea(Area& rarea); // return requested area
};

#endif
