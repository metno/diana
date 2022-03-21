// -*- c++ -*-
/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013-2022 met.no

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

#include "diPoint.h"
#include "diRectangle.h"

#include <iosfwd>
#include <memory>
#include <string>

class Transformation
{
public:
  virtual ~Transformation();

  bool forward(size_t npos, float* x, float* y) const;
  bool forward(size_t npos, double* x, double* y) const;
  bool forward(size_t npos, diutil::PointF* xy) const;
  bool forward(size_t npos, diutil::PointD* xy) const;

  bool inverse(size_t npos, float* x, float* y) const;
  bool inverse(size_t npos, double* x, double* y) const;
  bool inverse(size_t npos, diutil::PointF* xy) const;
  bool inverse(size_t npos, diutil::PointD* xy) const;

protected:
  bool transform(bool fwd, size_t npos, float* x, float* y) const;
  bool transform(bool fwd, size_t npos, double* x, double* y) const;
  bool transform(bool fwd, size_t npos, diutil::PointF* xy) const;
  bool transform(bool fwd, size_t npos, diutil::PointD* xy) const;

  virtual bool defined() const = 0;
  virtual bool transformAndCheck(bool fwd, size_t npos, size_t offset, double* x, double* y) const = 0;
};

typedef std::shared_ptr<const Transformation> Transformation_cp;

/**
  \brief Map Projection

  Specification of map projection
    - using proj4
*/
class Projection {
public:
  Projection();
  /*! Construct instance from a string.
   *
   * If the string contains 'PROJCS[' of 'GEGCS[', is it assumed to be WKT.
   * Else is is assumed to be a proj4 init string.
   */
  explicit Projection(const std::string& projStr);
  Projection(const Projection& o);
  Projection& operator=(const Projection& o);

  ~Projection();

  bool operator==(const Projection &rhs) const;
  bool operator!=(const Projection &rhs) const;

  friend std::ostream& operator<<(std::ostream& output, const Projection& p);

  /*! Define from string.
   * \return false iff the string is not understood.
   */
  bool setFromString(const std::string& projStr);

  //! Set from EPSG code (only number)
  bool setFromEPSG(const std::string& epsg);

  /*! Set from proj init string.
   * Might fail if the proj version used does not understand the init string.
   */
  bool setProj4Definition(const std::string& proj4str);

  /*! Set projection from WKT.
   * With proj 4, this uses `gdalsrsinfo` if available, so it is rather slow.
   */
  bool setFromWKT(const std::string& wkt);

  const std::string& getProj4Definition() const;

  std::string getProj4DefinitionExpanded() const;

  /// return true if projection is defined
  bool isDefined() const;

  /**
   * Return true if geographic projection
   */
  bool isGeographic() const;

  //! \return true if projection uses degrees
  bool isDegree() const;

  Transformation_cp transformationFrom(const Projection& src) const;

  /// Convert Points to this projection
  bool convertPoints(const Projection& srcProj, size_t npos, float* x, float* y) const;

  /// Convert Points to this projection
  bool convertPoints(const Projection& srcProj, size_t npos, double* x, double* y) const;

  /// Convert Points to this projection
  bool convertPoints(const Projection& srcProj, size_t npos, diutil::PointF* xy) const;

  /// Convert Points to this projection
  bool convertPoints(const Projection& srcProj, size_t npos, diutil::PointD* xy) const;

  bool convertVectors(const Projection& srcProj, size_t nvec,
      const float * from_x, const float * from_y,
      const float * to_x, const float * to_y,
      float * u, float * v) const;

  /// Convert Vectors to this projection
  bool convertVectors(const Projection& srcProj, int nvec, const float * to_x,
      const float * to_y, float * from_u, float * from_v) const;

  void calculateVectorRotationElements(const Projection& srcProj, int nvec,
      const float * from_x, const float * from_y,
      const float * to_x, const float * to_y,
      float * cosa, float * sina) const;

  bool calculateVectorRotationElements(const Projection& srcProj, int nvec,
      const float * to_x, const float * to_y, float * from_u, float * from_v) const;

  /**
   * Check if position (geographic) is legal/non-singular in this projection
   * \param lon geographic longitude in degrees
   * \param lat geographic latitude in degrees
   * \return true if lon-lat can be represented in this projection
   */
  bool isLegal(float lon, float lat) const;

  /**
   * Check if projection p is equal to this projection with the exception of x_0 and y_0
   */
  bool isAlmostEqual(const Projection& p) const;

  /*
   * Convert point data to geographic values
   * \param x input is in this projection's units; output is geographic longitude degrees
   * \param y input is in this projection's units; output is geographic latitude degrees
   * \return true if reprojection succeeded
   */
  bool convertToGeographic(int n, float* x, float* y) const;

  /*
   * Convert geographic points to this projection
   * \param x input is geographic longitude degrees; output is in this projection's units
   * \param y input is geographic latitude degrees; output is in this projection's units
   * \return true if reprojection succeeded
   */
  bool convertFromGeographic(int n, float* x, float* y) const;

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
  bool getMapRatios(int nx, int ny, float x0, float y0, float gridResolutionX, float gridResolutionY,
      float* xmapr, float* ymapr, float* coriolis) const;

  static bool getLatLonIncrement(float lat, float lon, float& dlat, float& dlon);

  static const Projection& geographic();

  static bool areDefined(const Projection& srcProj, const Projection& tgtProj);

private:
  struct P;
  std::unique_ptr<P> p_;

  static std::shared_ptr<Projection> sGeographic;
};

#endif
