// -*- c++ -*-
/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#ifndef diRectangle_h
#define diRectangle_h

#include <iosfwd>

/**
   \brief A rectangle

   Four corners define a rectangle
*/

class Rectangle {
public:
  float x1, y1, x2, y2;

public:
  Rectangle();
  Rectangle(float x1, float y1, float x2, float y2);

  bool operator==(const Rectangle &rhs) const;
  inline bool operator!=(const Rectangle& rhs) const { return !(*this == rhs); }

  /// return width of rectangle
  inline float width() const
    { return (x2-x1); }

  /// return height of rectangle
  inline float height() const
    { return (y2-y1); }

  /// return whether a point is inside rectangle
  inline bool isinside(float x, float y) const
    { return ((x>=x1)&&(x<=x2)&&(y>=y1)&&(y<=y2)); }

  bool intersects(const Rectangle& other) const;

  /// move rectangle so that x,y is inside
  void putinside(float x, float y);

  /// set rectangle from string (x1:x2:y1:y2) or (x2:y2)
  bool setRectangle(const std::string& rectangleString);

  /// get string (x1:x2:y1:y2)
  std::string toString() const;

  static const float EQUAL_TOLERANCE;
};

/// insert Rectangle into stream in the format "rectangle=x1:x2:y1:y2" (using toString)
std::ostream& operator<<(std::ostream& output, const Rectangle& r);

#endif
