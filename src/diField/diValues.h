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

#ifndef DIANA_DIFIELD_VALUES_H
#define DIANA_DIFIELD_VALUES_H 1

#include <memory>
#include <string>
#include <vector>

namespace diutil {

// wrapper around shared_array<float>
class Values
{
public:
  typedef float value_t;
  typedef std::shared_ptr<value_t[]> ValueArray;

public:
  class Shape
  {
  public:
    typedef std::vector<std::size_t> size_v;
    typedef std::vector<std::string> string_v;

    Shape();
    Shape(const string_v& names, const size_v& lengths);
    Shape(const std::string& name0, size_t length0);
    Shape(const std::string& name0, size_t length0, const std::string& name1, size_t length1);

    Shape& add(const std::string& name, size_t length);

    const string_v& names() const { return mNames; }
    const std::string& name(int position) const { return mNames.at(position); }
    size_t rank() const { return mNames.size(); }

    int position(const std::string& name) const;

    int length(int position) const;
    int length(const std::string& name) const { return length(position(name)); }
    const size_v& lengths() const { return mLengths; }
    size_t volume() const;

  private:
    string_v mNames;
    size_v mLengths;
  };

  class ShapeIndex
  {
  public:
    ShapeIndex(const Shape& shape);
    ShapeIndex& set(int position, size_t element);
    ShapeIndex& set(const std::string& name, size_t element) { return set(mShape.position(name), element); }
    size_t index() const;

  private:
    const Shape& mShape;

    typedef std::vector<std::size_t> size_v;
    size_v mElements;
  };

  class ShapeSlice
  {
  public:
    typedef std::vector<size_t> size_v;

  public:
    ShapeSlice(const Shape& shape);

    ShapeSlice& cut(int position, size_t start, size_t length);
    ShapeSlice& cut(const std::string& name, size_t start, size_t length) { return cut(mShape.position(name), start, length); }

    const Shape& shape() const { return mShape; }

    size_t start(int position) const { return get(mStarts, position, 0); }
    size_t start(const std::string& name) const { return start(mShape.position(name)); }
    const size_v& starts() const { return mStarts; }

    size_t length(int position) const { return get(mLengths, position, 1); }
    size_t length(const std::string& name) const { return length(mShape.position(name)); }
    const size_v& lengths() const { return mLengths; }

    size_t end(int position) const { return start(position) + length(position); }
    size_t end(const std::string& name) const { return end(mShape.position(name)); }

    size_t volume() const;

  private:
    size_t get(const size_v& v, int position, size_t missing) const { return (position >= 0 and position < (int)v.size()) ? v[position] : missing; }

  private:
    const Shape& mShape;
    size_v mStarts, mLengths;
  };

  Values(const Shape& shape, ValueArray v);
  Values(const Shape& shape);

  static const char *GEO_X, *GEO_Y, *GEO_Z, *TIME, *REALIZATION;

  value_t value(const ShapeIndex& si) const { return values()[si.index()]; }
  void setValue(value_t v, const ShapeIndex& si) { values()[si.index()] = v; }

  value_t undefValue() const { return mUndefValue; }
  void setUndefValue(value_t u) { mUndefValue = u; }

  /** direct access to the values -- make sure that you know the shape if you use this! */
  const ValueArray values() const { return mValues; }
  ValueArray values() { return mValues; }
  const Shape& shape() const { return mShape; }

private:
  Shape mShape;
  ValueArray mValues;
  float mUndefValue;
};
typedef std::shared_ptr<Values> Values_p;
typedef std::vector<Values_p> Values_pv;

typedef std::shared_ptr<const Values> Values_cp;
typedef std::vector<Values_cp> Values_cpv;

// ================================================================================

std::ostream& operator<<(std::ostream& out, const Values::Shape& shp);

} // namespace diutil

#endif // DIANA_DIFIELD_VALUES_H
