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
#include <diFilledMap.h>
#include <diShapeObject.h>

#include <GL/gl.h>

#include <vector>
#include <map>
#include <set>

/**
 \brief Map layer plotting

 plots the map layer
 - simple coastlines plotting
 - filled land type (precalculated triangles)
 - lat/lon lines
 */
class MapPlot : public Plot {
private:
  /// Lat/Lon Value Annotation with position on map
  struct ValueAnno {
    std::string t;
    float x;
    float y;
  };
private:
  bool mapchanged; // redraw needed
  bool haspanned;
  MapInfo mapinfo;
  PlotOptions contopts; // contour options
  PlotOptions landopts; // land plot options
  PlotOptions lonopts; // lon options
  PlotOptions latopts; // lat options
  PlotOptions ffopts; // frame options
  bool areadefined; // area explicitly defined
  Area reqarea; // requested area
  bool isactive[3]; // active data for zorder
  bool usedrawlists; // use OpenGL drawlists
  GLuint drawlist[3]; // openGL drawlists
  std::vector<ValueAnno> value_annotations;

  static std::map<std::string,FilledMap> filledmaps;
  static std::set<std::string> usedFilledmaps;
  static std::map<std::string,ShapeObject> shapemaps;
  static std::map<std::string,Area> shapeareas;

  /**
  * remove large jumps in a set of lines. Calls xyclip
  *
  * @param npos
  * @param x
  * @param y
  * @param xylim
  * @param jumplimit
  * @param plotanno
  * @param anno_position
  * @param anno
  */
  void clipPrimitiveLines(int npos, float *, float *, float xylim[4],
      float jumplimit, bool plotanno=false, int anno_position=2, std::string anno="");
  /**
   * clip a set a lines to the viewport
   * @param npos
   * @param x
   * @param y
   * @param xylim
   * @param plotanno
   * @param anno_position
   * @param anno
   */
  void xyclip(int, float[], float[], float[], bool, int, std::string);
  /**
  * plot a map from a Land4 formatted file
  * @param filename
  * @param
  * @param
  * @param
  * @param
  * @return
  */

  //convert position "bottom", left" etc to MapValuePosition
  int convertLatLonPos(const std::string& pos);

  bool plotMapLand4(const std::string&, float[], const Linetype&, float,
      const Colour&);
  /**
   * Plot Lat/Lon lines with optional numbering
   * @param mapinfo
   * @param plot_lon
   * @param plot_lat
   * @param plotResolution
   * @return
   */
  bool plotGeoGrid(const MapInfo & mapinfo, bool plot_lon, bool plot_lat, int plotResolution = 100);
  /**
   * plot a map from a simple text formatted file
   * @param filename
   * @return
   */
  bool plotLinesSimpleText(const std::string& filename);

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
  bool prepare(const std::string&, Area rarea, bool ifequal =true);
  /// return the area asked for
  bool requestedArea(Area& rarea); // return requested area
};

#endif
