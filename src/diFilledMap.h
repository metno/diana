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
#ifndef _diFilledMap_h
#define _diFilledMap_h


#include <puTools/miString.h>
#include <diField/diGridConverter.h>
#include <GL/gl.h>
#include <puCtools/porttypes.h>

using namespace std;

/**
   \brief Maps with filled land

   Data and plotter for maps with filled land (precalculated triangles)

*/

class FilledMap {
private:
  miString filename; // name of mapfile
  long timestamp;    // file's change-time
  GridConverter gc;

  struct tile_group {
    int numtiles;     // number of tiles in group
    float *tilex;     // group + tile borders
    float *tiley;     // --- " ---
    float *midlat;    // tile midpoint
    float *midlon;    // --- " ---
    float *mmx;       // tile min-max in last used projection
    float *mmy;       // --- " ---
    bool *use;        // use tile in this projection
    int *tiletype;    // 0=normal, 1=near northpole, 2=near southpole
    int *crecnr;      // recordnumber for start of tile
    int *cwp;         // wordnumber for start of tile
    tile_group(): numtiles(0),
		  tilex(0),tiley(0),
		  midlat(0),midlon(0),
		  mmx(0),mmy(0),
		  crecnr(0),cwp(0)
    {}
  };

  // file parameters
  float scale;   // datascale
  float tscale;  // tile-border scale
  int numGroups;
  tile_group *groups; // tiles
  void clearGroups();

  Projection proj;  // last used projection

  struct tile_data {
    int np;
    vector<int> polysize;
    float *polyverx;
    float *polyvery;
    tile_data():np(0),polyverx(0),polyvery(0){}
  };
  int numPolytiles;
  tile_data* polydata;
  void clearPolys();

  bool opened; // file opened and header read
  bool contexist; // contours exist ready for plot

  long gettimestamp();
  bool readheader();

  void clipTriangles(int i1, int i2, float * x, float * y, float xylim[4],
      float jumplimit);
  void clipPrimitiveLines(int i1, int i2, float *, float *, float xylim[4],
      float jumplimit);
  void xyclip(int npos, float *x, float *y, float xylim[4]);

public:

  FilledMap();
  FilledMap(const miString fn);
  ~FilledMap();

  /// Plot map (OpenGL)
  bool plot(Area area,               // current area
      Rectangle maprect,       // the visible rectangle
	    double gcd,              // size of plotarea in m
	    bool land,               // plot triangles
	    bool cont,               // plot contour-lines
	    bool keepcont,           // keep contourlines for later
	    GLushort linetype,       // contour line type
	    float linewidth,         // contour linewidth
	    const uchar_t* lcolour,  // contour linecolour
	    const uchar_t* fcolour,  // triangles fill colour
	    const uchar_t* bcolour); // background color

};

#endif
