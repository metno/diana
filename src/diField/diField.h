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
#ifndef diField_h
#define diField_h

#include "diArea.h"
#include "diFieldFwd.h"
#include "diValues.h"

#include <mi_fieldcalc/FieldDefined.h>

#include <puTools/miTime.h>

class GridConverter;

/**

  \brief Field data and information

  Contains one scalar field and information about size, projection, time
  and vertical coordinate values (pressure etc. as needed for computations).
  May also contain names and text for annotations. When plotting wind etc.
  using more than one scalar field, only the first contains annotation info.
*/
class Field {
public:
  //.................... set when reading: ..........................................
  float *data;
  GridArea area;

  void checkDefined();
  void forceDefined(miutil::ValuesDefined d)
    { defined_ = d; }
  miutil::ValuesDefined defined() const
    { return defined_; }
  bool allDefined() const // true if none undefined/missing values in the field
    { return defined_ == miutil::ALL_DEFINED; }

  int  level;            // vertical level
  int  idnum;            // level2, used for EPS clusters and members
  int  forecastHour;
  miutil::miTime validFieldTime;
  miutil::miTime analysisTime;   // time of model analysis

  // set if applicable (used in computations, and to show obs. in same level)
  float aHybrid;        // pressure[] = aHybrid + bHybrid * pSurface[]  ("eta levels")
  float bHybrid;        //
  std::string unit;     //unit of field (celsius, kelvin, m, mm, ...?)
  diutil::Values_p palette; // colour palette from file
  //.................................................................................

  int numSmoothed;
  bool turnWaveDirection; //In some fields wave direction need to be turned
  bool vectorProjectionLonLat; // in some datasets, vectors are east-north while the grid has a different projection than lonlat

  std::string name;
  std::string text;
  std::string fulltext;
  std::string modelName;
  std::string paramName;
  std::string fieldText;
  std::string leveltext;
  std::string idnumtext;
  std::string progtext;
  std::string timetext;

  /**
   * Copies all members, but performs a shallow copy on the actual data.
   * @param rhs Field to copy
   */
  void shallowMemberCopy(const Field& rhs);

private:
  // Copy members
  void memberCopy(const Field& rhs);

  miutil::ValuesDefined defined_;

public:
  Field();
  ~Field();

  Field(const Field &rhs);
  Field& operator=(const Field &rhs);

private:
  // Equality operator (only checking gridarea)
  bool operator==(const Field &rhs) const;

  // Inequality operator (only checking gridarea)
  bool operator!=(const Field &rhs) const;

public:
  /// Delete the data
  void cleanup();

  /// Add dataspace for a xdim*ydim field
  void reserve(int xdim, int ydim);

  void fill(float value);

  /// Subtract a field
  bool subtract(const Field &rhs);
#if 0
  /// Add a field
  bool add(const Field &rhs);
  /// Multiply by a field
  bool multiply(const Field &rhs);
  /// Divide by a field (division by 0 = fieldUndef)
  bool divide(const Field &rhs);
  /// Difference between another field and this
  bool differ(const Field &rhs);
  /// Negate (the negative)
  void negate();
  /// Set value
  void setValue(float value);
#endif

  /// Set all values undefined
  void setUndefined();

  /// smooth the field in nsmooth iterations
  bool smooth(int nsmooth);
  /// smooth the field in nsmooth iterations
  bool smooth(int nsmooth, float *work, float *worku1, float *worku2);

  enum InterpolationType {
    I_BILINEAR    =   0, //!< bilinear interpolation (2x2 points used)
    I_BESSEL      =   1, //!< bessel   interpolation (4x4 points used, or 2x2 points near borders and undefined points)
    I_NEAREST     =   2, //!< nearest gridpoint
    I_BILINEAR_EX = I_BILINEAR + 100, //!< bilinear with extrapolation beyond boundaries
    I_BESSEL_EX   = I_BESSEL   + 100, //!< bessel with extrapolation beyond boundaries
    I_NEAREST_EX  = I_NEAREST  + 100, //!< nearest with extrapolation beyond boundaries
  };

  /// interpolation in the field to selected positions
  bool interpolate(int npos, const float *xpos, const float *ypos,
      float *fpos, InterpolationType itype) const;

  /// interpolate to another grid
  bool changeGrid(GridConverter& gc, const GridArea& anew, bool fine_interpolation);

  /// Return x,y in proj-coord
  void convertFromGrid(int npos, float* xpos, float* ypos) const;

  /// Return x,y in grid-coord
  void convertToGrid(int npos, float* xpos, float* ypos) const;
};

#endif
