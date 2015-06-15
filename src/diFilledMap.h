/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2013 met.no

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
#ifndef _diFilledMap_h
#define _diFilledMap_h

#include "diGLPainter.h"

#include <diField/diGridConverter.h>
#include <vector>

/**
   \brief Maps with filled land

   Data and plotter for maps with filled land (pre-calculated triangles)
*/

class FilledMap {
private:
  std::string filename; // name of map file
  long timestamp; // file's change-time
  GridConverter gc;

  struct tile_group {
    int numtiles; // number of tiles in group
    float *tilex; // group + tile borders, X
    float *tiley; // group + tile borders, Y
    float *midlat; // tile midpoint, latitude
    float *midlon; // tile midpoint, longitude
    float *mmx; // tile minimum-maximum X in last used projection
    float *mmy; // tile minimum-maximum Y in last used projection
    bool *use; // use tile in this projection
    int *tiletype; // 0=normal, 1=near north pole, 2=near south pole
    int *crecnr; // record number for start of tile
    int *cwp; // word number for start of tile
    tile_group() :
      numtiles(0), tilex(0), tiley(0), midlat(0), midlon(0), mmx(0), mmy(0),
          crecnr(0), cwp(0)
    {
    }
  };

  // file parameters
  float scale; // data scale
  float tscale; // tile-border scale
  int numGroups;
  tile_group *groups; // tiles
  void clearGroups();

  Projection proj; // last used projection

  struct tile_data {
    int np;
    std::vector<int> polysize;
    float *polyverx;
    float *polyvery;
    tile_data() :
      np(0), polyverx(0), polyvery(0)
    {
    }
  };
  int numPolytiles;
  tile_data* polydata;
  void clearPolys();

  bool opened; // file opened and header read
  bool contexist; // contours exist ready for plot

  long gettimestamp();
  bool readheader();

  void clipTriangles(DiGLPainter* gl, int i1, int i2, float * x, float * y, float xylim[4],
      float jumplimit);
  void clipPrimitiveLines(DiGLPainter* gl, int i1, int i2, float *, float *, float xylim[4],
      float jumplimit);

public:
  FilledMap(const std::string fn);
  ~FilledMap();

  const std::string& getFilename() const
    { return filename; }

  /**
   * Plot map (OpenGL)
   * @param area, current area
   * @param maprect, the visible rectangle
   * @param gcd, size of plot area in m
   * @param land, plot triangles
   * @param cont, plot contour-lines
   * @param keepcont, keep contour lines for later
   * @param linetype, contour line type
   * @param linewidth, contour line width
   * @param lcolour, contour line color
   * @param fcolour, triangles fill color
   * @param bcolour, background color
   * @return successful plot
   */
  bool plot(DiGLPainter* gl, const Area& area, const Rectangle& maprect,
      double gcd, bool land, bool cont,
      bool keepcont, DiGLPainter::GLushort linetype, float linewidth,
      const unsigned char* lcolour, const unsigned char* fcolour, const unsigned char* bcolour);
};

#endif
