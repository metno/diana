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
#ifndef diProjection_h
#define diProjection_h

#include "diRectangle.h"

#include <proj_api.h>
#include <boost/shared_ptr.hpp>
#include <iosfwd>
#include <string>

/**
  \brief Map Projection

  Specification of map projection
    - using proj4
*/
class Projection {
public:
  Projection();
  explicit Projection(const std::string& projStr);

  ~Projection();

  bool operator==(const Projection &rhs) const;
  bool operator!=(const Projection &rhs) const;

  friend std::ostream& operator<<(std::ostream& output, const Projection& p);

  /// set proj4 definitions
  bool set_proj_definition(const std::string& proj4str);

  const std::string& getProjDefinition() const
    { return projDefinition; }

  std::string getProjDefinitionExpanded() const;

  /// return true if projection is defined
  bool isDefined() const
    { return projObject != 0; }

  /**
   * Return true if geographic projection
   */
  bool isGeographic() const;

  void setDefault();

  /// Convert Points to this projection
  int convertPoints(const Projection& srcProj, int npos, float * x, float * y,
      bool silent = false) const;

  /// Convert Vectors to this projection
  int convertVectors(const Projection& srcProj, int nvec, const float * to_x,
      const float * to_y, float * from_u, float * from_v) const;

  int calculateVectorRotationElements(const Projection& srcProj, int nvec,
      const float * to_x, const float * to_y, float * from_u, float * from_v) const;

  /**
   * Check if position (geographic) is legal/non-singular in this projection
   */
  bool isLegal(float lon, float lat) const;

  /**
   * Check if projection p is equal to this projection with the exception of x_0 and y_0
   */
  bool isAlmostEqual(const Projection& p) const;

  /*
   * Convert point data to geographic values
   */
  int convertToGeographic(int n, float* x, float* y) const;

  /*
   * Convert geographic points to this projection
   */
  int convertFromGeographic(int n, float* x, float* y) const;

  /**
   * get max legal jump-distance for a line on the map
   */
  float getMapLinesJumpLimit() const;

  /**
   * calculate geographic extension
   */
  bool calculateLatLonBoundingBox(const Rectangle & maprect,
      float & lonmin, float & lonmax, float & latmin, float & latmax) const;

  /**
   * return adjusted geographic extension
   */
  bool adjustedLatLonBoundingBox(const Rectangle & maprect,
      float & lonmin, float & lonmax, float & latmin, float & latmax) const;

  /// Calculate mapratios and coriolis factors
  int getMapRatios(int nx, int ny, float gridResolutionX, float gridResolutionY,
      float* xmapr, float* ymapr, float* coriolis) const;

  static bool getLatLonIncrement(float lat, float lon, float& dlat, float& dlon);

  static const Projection& geographic();

private:
  std::string projDefinition;

#if !defined(PROJECTS_H)
  typedef void PJ;
#endif
  boost::shared_ptr<PJ> projObject;

  static boost::shared_ptr<Projection> sGeographic;
};

#endif
