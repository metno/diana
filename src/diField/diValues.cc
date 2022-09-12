/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2014-2022 met.no

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

#include "diValues.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric> // std::accumulate

namespace diutil {

namespace {
const float NANF = nanf("");
static size_t calculate_volume(const Values::Shape::size_v& counts)
{
  // TODO use this function in DataReshape::reshape
  return std::accumulate(counts.begin(), counts.end(), 1, std::multiplies<size_t>());
}
} // namespace

Values::Shape::Shape() {}

Values::Shape::Shape(const std::string& name0, size_t length0)
{
  add(name0, length0);
}

Values::Shape::Shape(const std::string& name0, size_t length0, const std::string& name1, size_t length1)
{
  add(name0, length0);
  add(name1, length1);
}

Values::Shape::Shape(const string_v& names, const size_v& lengths)
    : mNames(names)
    , mLengths(lengths)
{
  if (mLengths.size() != mNames.size())
    throw std::runtime_error("mismatch in number of names and lengths");
}

Values::Shape& Values::Shape::add(const std::string& name, size_t length)
{
  mNames.push_back(name);
  mLengths.push_back(length);
  return *this;
}

int Values::Shape::position(const std::string& name) const
{
  string_v::const_iterator it = std::find(mNames.begin(), mNames.end(), name);
  if (it != mNames.end())
    return (it - mNames.begin());
  else
    return -1;
}

int Values::Shape::length(int position) const
{
  if (position >= 0 and position < (int)mLengths.size())
    return mLengths[position];
  else
    return 1;
}

size_t Values::Shape::volume() const
{
  return calculate_volume(mLengths);
}

Values::ShapeIndex::ShapeIndex(const Values::Shape& shape)
    : mShape(shape)
    , mElements(mShape.rank(), 0)
{
}

Values::ShapeIndex& Values::ShapeIndex::set(int position, size_t element)
{
  if (position >= 0 and position < (int)mShape.rank()) {
    if (int(element) < mShape.length(position))
      mElements[position] = element;
    else
      mElements[position] = 0;
  }
  return *this;
}

size_t Values::ShapeIndex::index() const
{
  size_t i = 0, s = 1;
  for (size_t j = 0; j < mShape.rank(); ++j) {
    i += mElements[j] * s;
    s *= mShape.length(j);
  }
  return i;
}

Values::ShapeSlice::ShapeSlice(const Shape& shape)
    : mShape(shape)
    , mStarts(mShape.rank(), 0)
    , mLengths(mShape.lengths())
{
}

Values::ShapeSlice& Values::ShapeSlice::cut(int position, size_t start, size_t length)
{
  if (position >= 0 and position < int(mStarts.size())) {
    if (int(start) >= mShape.length(position) or int(start + length) > mShape.length(position))
      start = length = 0;
    mStarts[position] = start;
    mLengths[position] = length;
  }
  return *this;
}

size_t Values::ShapeSlice::volume() const
{
  return calculate_volume(mLengths);
}

const char* Values::GEO_X = "GEO_X";
const char* Values::GEO_Y = "GEO_Y";
const char* Values::GEO_Z = "GEO_Z";
const char* Values::REALIZATION = "REALIZATION";
const char* Values::TIME = "TIME";

Values::Values(const Shape& shape, ValueArray v)
    : mShape(shape)
    , mValues(v)
    , mUndefValue(NANF)
{
}

Values::Values(const Shape& shape)
    : mShape(shape)
    , mValues(new float[mShape.volume()])
    , mUndefValue(NANF)
{
}

std::ostream& operator<<(std::ostream& out, const Values::Shape& shp)
{
  out << "[vc shape rank=" << shp.rank();
  for (size_t i = 0; i < shp.rank(); ++i)
    out << ' ' << shp.name(i) << ':' << shp.length(i);
  out << ']';
  return out;
}

} // namespace diutil
