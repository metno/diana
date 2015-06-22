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

#include "diFilledMap.h"
#include "diGlUtilities.h"
#include "diPlot.h"
#include "diShapeObject.h"
#include "diUtilities.h"

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
  bool mapchanged; // redraw needed
  bool haspanned;
  MapInfo mapinfo;
  PlotOptions contopts; // contour options
  PlotOptions landopts; // land plot options
  PlotOptions lonopts; // lon options
  PlotOptions latopts; // lat options
  PlotOptions ffopts; // frame options
  bool isactive[3]; // active data for zorder
  diutil::MapValueAnno_v value_annotations;

  DiGLCanvas* mCanvas;
  DiGLPainter::GLuint drawlist[3]; // openGL drawlists

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
  void clipPrimitiveLines(DiGLPainter* gl, int npos, float *, float *, const float xylim[4],
      float jumplimit, bool plotanno=false,
      diutil::MapValuePosition anno_position = diutil::map_right, const std::string& anno="");
  /**
  * plot a map from a Land4 formatted file
  * @param filename
  * @param
  * @param
  * @param
  * @param
  * @return
  */

  bool plotMapLand4(DiGLPainter* gl, const std::string&, const float[], const Linetype&, float,
      const Colour&);
  /**
   * Plot Lat/Lon lines with optional numbering
   * @param mapinfo
   * @param plot_lon
   * @param plot_lat
   * @param plotResolution
   * @return
   */
  bool plotGeoGrid(DiGLPainter* gl, const MapInfo & mapinfo, bool plot_lon, bool plot_lat, int plotResolution = 100);
  /**
   * plot a map from a simple text formatted file
   * @param filename
   * @return
   */
  bool plotLinesSimpleText(DiGLPainter* gl, const std::string& filename);

public:
  MapPlot();
  ~MapPlot();

  void setCanvas(DiCanvas* canvas) /*Q_DECL_OVERRIDE*/;

  /// plot map in a specific zorder layer
  void plot(DiGLPainter* gl, PlotOrder zorder);

  /// parse plotinfo
  bool prepare(const std::string&, bool ifequal =true);

private:
  static void referenceFilledMaps(const MapInfo& mi);
  static void dereferenceFilledMaps(const MapInfo& mi);

  static FilledMap* fetchFilledMap(const std::string& filename);

  typedef std::map<std::string, FilledMap> fmObjects_t;
  static fmObjects_t filledmapObjects; // filename -> map

  // filename -> reference count; separate from "filledmaps" because it may contain more elements
  typedef std::map<std::string, int> fmRefCounts_t;
  static fmRefCounts_t filledmapRefCounts;
};

#endif
