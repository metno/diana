// -*- c++ -*-
/*

 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2013-2020 met.no

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

#include <mutex>

/**
 \brief Array of points for grid conversion
 */
struct Points {
  GridArea area; ///< input (field) projection
  Projection map_proj; ///< output (map) projection
  Rectangle maprect; ///< current map rect.
  int npos; ///< number of points/vectors
  bool gridboxes; ///< gridpoints(false) gridbox corners(true)
  const float* x; ///< position x
  const float* y; ///< position y
  int ixmin, ixmax, iymin, iymax; ///< index range to cover current map rect.

  Points();
  ~Points();

  Points& operator=(const Points&) = delete;
  Points(const Points&) = delete;
  Points(Points&&) = delete;
};

typedef std::shared_ptr<Points> Points_p;
typedef std::shared_ptr<Points> Points_cp;

/**
 \brief Data on fields for grid conversion
 */
struct MapFields {
  GridArea area; ///< input (field) projection
  const float* xmapr;    ///< map ratio x-direction
  const float* ymapr;    ///< map ratio y-direction
  const float* coriolis; ///< coriolis parameter

  MapFields();
  ~MapFields();

  MapFields& operator=(const MapFields&) = delete;
  MapFields(const MapFields&) = delete;
  MapFields(MapFields&&) = delete;
};

typedef std::shared_ptr<MapFields> MapFields_p;
typedef std::shared_ptr<MapFields> MapFields_cp;

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

  Points_p doGetGridPoints(const GridArea& area, const Projection& map_proj, bool gridboxes);

  /// get vector rotation elements from angle cache
  Points_p getVectorRotationElements(const Area& data_area, const Projection& map_proj, int nvec, const float* x, const float* y);

  /// set size of cache for lists of x/y values
  void setBufferSize(const int);
  /// set size of cache for lists of rotation/angle values
  void setAngleBufferSize(const int);
  /// set size of cache for map fields data
  void setBufferSizeMapFields(const int);

public:
  GridConverter();
  ~GridConverter();

  Points_cp getGridPoints(const GridArea& area, const Area& map_area, bool gridboxes);

  /// assume regular grid...keep calculations in ringbuffer
  Points_cp getGridPoints(const GridArea& area, const Area& map_area, const Rectangle& maprect, bool gridboxes, int& ix1, int& ix2, int& iy1, int& iy2);

  static void findGridLimits(const GridArea& area, const Rectangle& maprect, bool gridboxes, const float* x, const float* y, int& ix1, int& ix2, int& iy1,
                             int& iy2);

  /// convert u,v vector coordinates for points x,y - obsolete syntax, to be removed
  bool getVectors(const Area&, const Projection&, int,
      const float*, const float*, float*, float*);

  /// convert true north direction and velocity (dd,ff) to u,v vector coordinates for points x,y
  bool getDirectionVectors(const Area&, const bool, int,
      const float*, const float*, float*, float*);

  /// map ratio and/or coriolis parameter fields
  MapFields_cp getMapFields(const GridArea& gridarea);

private:
  std::unique_ptr<ring<Points_p>> pointbuffer;
  std::unique_ptr<ring<Points_p>> anglebuffer;
  std::unique_ptr<ring<MapFields_p>> mapfieldsbuffer;

  std::mutex point_mutex;
  std::mutex angle_mutex;
  std::mutex mapfields_mutex;
};

#endif
