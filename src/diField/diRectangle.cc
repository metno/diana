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

#include "diana_config.h"

#include "diRectangle.h"

#include <puTools/miString.h>
#include <cmath>

const float Rectangle::EQUAL_TOLERANCE = 1e-7;

namespace {
/// comparing floats
bool AlmostEqual(float A, float B)
{
  return (fabs(A - B) < Rectangle::EQUAL_TOLERANCE);
}
} // namespace

Rectangle::Rectangle()
    : x1(0)
    , y1(0)
    , x2(0)
    , y2(0)
{
}

Rectangle::Rectangle(float _x1, float _y1, float _x2, float _y2)
    : x1(_x1)
    , y1(_y1)
    , x2(_x2)
    , y2(_y2)
{
}

bool Rectangle::operator==(const Rectangle& rhs) const
{
  return AlmostEqual(x1, rhs.x1) && AlmostEqual(x2, rhs.x2) && AlmostEqual(y1, rhs.y1) && AlmostEqual(y2, rhs.y2);
}

std::ostream& operator<<(std::ostream& output, const Rectangle& r)
{
  return output << "rectangle=" << r.toString();
}

void Rectangle::putinside(float x, float y)
{
  float xdiff = x2 - x1;
  x1 += x - (x2 + x1) / 2.;
  x2 = x1 + xdiff;

  float ydiff = y2 - y1;
  y1 += y - (y2 + y1) / 2.;
  y2 = y1 + ydiff;
}

bool Rectangle::setRectangle(const std::string& rectangleString)
{
  //If no string, set default rectangle
  if (rectangleString.empty()) {
    x1 = 0.0;
    x2 = 0.0;
    y1 = 0.0;
    y2 = 0.0;
    return true;
  }

  const std::vector<std::string> token = miutil::split(rectangleString, ":");

  if (token.size() == 2) {
    x1 = 0.;
    y1 = 0.;
    x2 = miutil::to_double(token[0]);
    y2 = miutil::to_double(token[1]);
    return true;
  } else if (token.size() == 4) {
    x1 = miutil::to_double(token[0]);
    x2 = miutil::to_double(token[1]);
    y1 = miutil::to_double(token[2]);
    y2 = miutil::to_double(token[3]);
    return true;
  } else {
    return false;
  }
}

// get string (x1:x2:y1:y2)
std::string Rectangle::toString() const
{
  return  miutil::from_number(x1, 5)
      + ":" + miutil::from_number(x2, 5)
      + ":" + miutil::from_number(y1, 5)
      + ":" + miutil::from_number(y2, 5);
}

bool Rectangle::intersects(const Rectangle& other) const
{
  return !(other.x1 > x2 || other.x2 < x1 || other.y1 > y2 || other.y2 < y1);
}
