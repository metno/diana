// -*- c++ -*-
/*

 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2013 met.no

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
#ifndef diGridConverter_h
#define diGridConverter_h

#include "diArea.h"

#include "puTools/miRing.h"

/**
 \brief Array of points for grid conversion
 */
struct Points {
  GridArea area; ///< input (field) projection
  Area map_area; ///< output (map) projection
  Rectangle maprect; ///< current map rect.
  int npos; ///< number of points/vectors
  bool gridboxes; ///< gridpoints(false) gridbox corners(true)
  float *x; ///< position x
  float *y; ///< position y
  int ixmin, ixmax, iymin, iymax; ///< index range to cover current map rect.
  Points() :
    npos(0), gridboxes(false), x(0), y(0), ixmin(0), ixmax(0), iymin(0), iymax(0)
  {
  }
  ~Points()
  {
    delete[] x;
    delete[] y;
  }
  Points& operator=(const Points& rhs);
};

/**
 \brief Data on fields for grid conversion
 */
struct MapFields {
  GridArea area; ///< input (field) projection
  float *xmapr; ///< map ratio x-direction
  float *ymapr; ///< map ratio y-direction
  float *coriolis; ///< coriolis parameter

  MapFields();
  ~MapFields();
};

/**
 \brief Projection conversion of gridpositions

 The GridConverter manages caches of calculated gridvalues for different projections
 - single x/y values for grids
 - map ratio and/or coriolis parameter fields

 */
class GridConverter {
private:
  enum {
    defringsize = 10
  };
  enum {
    defringsize_angles = 10
  };
  enum {
    defringsize_mapfields = 4
  };

  bool doGetGridPoints(const GridArea& area, const Area& map_area,
      bool gridboxes, float**x, float**y, int& ipb);

  static void doFindGridLimits(const GridArea& area, const Rectangle& maprect,
      bool gridboxes, const float* x, const float* y, size_t xy_offset,
      int& ix1, int& ix2, int& iy1, int& iy2);

  /// get vector rotation elements from angle cache
  bool getVectorRotationElements(const Area& data_area, const Area& map_area,
      int nvec, const float *x, const float *y, float ** cosx, float ** sinx);

public:
  GridConverter();
  GridConverter(const int s, const int smf);
  GridConverter(const int s);
  ~GridConverter();

  /// set size of cache for lists of x/y values
  void setBufferSize(const int);
  /// set size of cache for lists of rotation/angle values
  void setAngleBufferSize(const int);
  /// set size of cache for map fields data
  void setBufferSizeMapFields(const int);

  bool getGridPoints(const GridArea& area, const Area& map_area,
      bool gridboxes, float**x, float**y);

  /// assume regular grid...keep calculations in ringbuffer
  bool getGridPoints(const GridArea& area, const Area& map_area,
      const Rectangle& maprect, bool gridboxes,
      float** x, float** y, int& ix1, int& ix2, int& iy1, int& iy2);

  static void findGridLimits(const GridArea& area, const Rectangle& maprect,
      bool gridboxes, const float* xy,
      int& ix1, int& ix2, int& iy1, int& iy2)
    { doFindGridLimits(area, maprect, gridboxes, xy, xy+1, 2, ix1, ix2, iy1, iy2); }

  static void findGridLimits(const GridArea& area, const Rectangle& maprect,
      bool gridboxes, const float* x, const float* y,
      int& ix1, int& ix2, int& iy1, int& iy2)
    { doFindGridLimits(area, maprect, gridboxes, x, y, 1, ix1, ix2, iy1, iy2); }

  /// convert arbitrary set of points
  bool getPoints(const Area&, const Area&, int, float*, float*) const;
  /// convert arbitrary set of points
  bool getPoints(const Projection&, const Projection&, int, float*, float*) const;
  /// convert u,v vector coordinates for points x,y - obsolete syntax, to be removed
  bool getVectors(const Area&, const Area&, int,
      const float*, const float*, float*, float*);
  /// convert true north direction and velocity (dd,ff) to u,v vector coordinates for points x,y
  bool getDirectionVectors(const Area&, const bool, int,
      const float*, const float*, float*, float*);
  /// convert true north direction and velocity (dd,ff) to u,v vector coordinates for one point
  /// Specific point given by index
  bool getDirectionVector(const Area&, const bool, int,
      const float *, const float *, int index, float &, float &);
  /// convert from geo to xy
  bool geo2xy(const Area&, int, float*, float*);
  /// convert from xy to geo
  bool xy2geo(const Area&, int, float*, float*);
  /// convert geographical u,v vector coordinates for points x,y
  bool geov2xy(const Area&, int, const float*, const float*, float*, float*);
  /// convert xy vector coordinates for points x,y to geographical
  /// for an entire field
  bool xyv2geo(const Area&, int, int, float*, float*);

  /// map ratio and/or coriolis parameter fields
  bool getMapFields(const GridArea& gridarea,
      const float** xmapr, const float** ymapr, const float** coriolis);

private:
  ring<Points> *pointbuffer;
  ring<Points> *anglebuffer;
  ring<MapFields> *mapfieldsbuffer;
};

#endif
