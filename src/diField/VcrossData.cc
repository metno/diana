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

#include "VcrossData.h"

#include "VcrossUtil.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric> // std::accumulate

#define MILOGGER_CATEGORY "vcross.VcrossData"
#include "miLogger/miLogging.h"

static const float NANF = nanf("");
static const double MAX_DISTANCE_M = 10000;

namespace vcross {

const InventoryBase::Unit_t& zAxisUnit(Z_AXIS_TYPE zType)
{
  static const std::string units[] = {"", "hPa", "m", "m", "m", ""};
  return units[zType];
}

// ================================================================================

namespace {
struct find_id {
  InventoryBase::Id_t id;
  find_id(InventoryBase::Id_t i) : id(i) { }
  bool operator()(const InventoryBase_cp ip) const
    { return ip and ip->id() == id; }
};
}

InventoryBase_cp findItemById(const InventoryBase_cpv& items, InventoryBase::Id_t id)
{
  InventoryBase_cpv::const_iterator it = std::find_if(items.begin(), items.end(), find_id(id));
  if (it == items.end())
    return InventoryBase_cp();
  else
    return *it;
}

InventoryBase_cp findItemById(const InventoryBase_cps& items, InventoryBase::Id_t id)
{
  InventoryBase_cps::const_iterator it = std::find_if(items.begin(), items.end(), find_id(id));
  if (it == items.end())
    return InventoryBase_cp();
  else
    return *it;
}

// ================================================================================

InventoryBase::Id_t ZAxisData::hybridParameterId(std::string hybridParameterName) const
{
  hybridparameterids_t::const_iterator it = mHybridParameterIds.find(hybridParameterName);
  if (it != mHybridParameterIds.end())
    return it->second;
  return "";
}

void ZAxisData::setHybridParameterId(std::string hybridParameterName, InventoryBase::Id_t id)
{
  mHybridParameterIds[hybridParameterName] = id;
}

InventoryBase_cp ZAxisData::getField(Z_AXIS_TYPE t) const
{
  fields_t::const_iterator it = mFields.find(t);
  if (it != mFields.end())
    return it->second;
  else
    return InventoryBase_cp();
}

void ZAxisData::setField(Z_AXIS_TYPE t, InventoryBase_cp f)
{
  mFields.insert(std::make_pair(t, f));
}

std::string ZAxisData::DATA_TYPE()
{
  return "ZLEVEL";
}

// ================================================================================

std::string FieldData::DATA_TYPE()
{
  return "FIELD";
}

// ================================================================================

Crossection::Crossection(const std::string& label, const LonLat_v& points,
    const LonLat_v& pointsRequested)
  : mLabel(label)
  , mPoints(points)
  , mPointsRequested(pointsRequested)
{
}

Crossection::~Crossection()
{
}

// ================================================================================

static size_t calculate_volume(const Values::Shape::size_v& counts)
{
  // TODO use this function in DataReshape::reshape
  return std::accumulate(counts.begin(), counts.end(), 1, std::multiplies<size_t>());
}


Values::Shape::Shape()
{
}

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
  for (size_t j=0; j<mShape.rank(); ++j) {
    i += mElements[j]*s;
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
    if (int(start) >= mShape.length(position) or int(start+length) > mShape.length(position))
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

const char *Values::GEO_X = "GEO_X";
const char *Values::GEO_Y = "GEO_Y";
const char *Values::GEO_Z = "GEO_Z";
const char *Values::REALIZATION = "REALIZATION";
const char *Values::TIME = "TIME";

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

Values_p reshape(Values_p valuesIn, const Values::Shape& shapeOut)
{
  return std::make_shared<Values>(shapeOut, reshape(valuesIn->shape(), shapeOut, valuesIn->values()));
}

// ================================================================================

bool Time::valid() const
{
  return not unit.empty();
}

bool Time::operator==(const Time& other) const
{
  const bool v = valid(), ov = other.valid();
  if (!v)
    return !ov;
  else if (!ov)
    return false;

  if (unit == other.unit)
    return value == other.value;
  else
    return util::to_miTime(*this) == util::to_miTime(other);
}

bool Time::operator<(const Time& other) const
{
  const bool v = valid(), ov = other.valid();
  // define invalid < valid
  if (!v)
    return ov;
  else if (!ov)
    return false;

  if (!valid())
    return other.valid(); // define invalid < valid

  if (unit == other.unit)
    return value < other.value;
  else
    return util::to_miTime(*this) < util::to_miTime(other);
}

// ================================================================================

Inventory::Inventory()
  : realizationCount(1)
{
}

void Inventory::clear()
{
  realizationCount = 1;
  times.values.clear();
  crossections.clear();
  fields.clear();
}

FieldData_cp Inventory::findFieldById(std::string id) const
{
  fields_t::const_iterator it = std::find_if(fields.begin(), fields.end(), find_id(id));
  if (it == fields.end())
    return FieldData_cp();
  else
    return *it;
}

Crossection_cp Inventory::findCrossectionByLabel(const std::string& cslabel) const
{
  for (Crossection_cp cs : crossections) {
    if (cs->label() == cslabel)
      return cs;
  }
  return Crossection_cp();
}

Crossection_cp Inventory::findCrossectionPoint(const LonLat& point, size_t& best_index) const
{
  Crossection_cp best_cs;
  double best_distance = 0;

  for (Crossection_cp cs : crossections) {
    for (size_t index = 0; index < cs->length(); ++index) {
      const double distance = point.distanceTo(cs->point(index));
      if (distance >= MAX_DISTANCE_M)
        continue;
      if (not best_cs or distance < best_distance) {
        best_cs = cs;
        best_index = index;
        best_distance = distance;
      }
    }
  }
  return best_cs;
}

Crossection_cp Inventory::findCrossectionPoint(const LonLat& point) const
{
  Crossection_cp best_cs;
  double best_distance = 0;

  for (Crossection_cp cs : crossections) {
    if (cs->length() != 1)
      continue;
    const double distance = point.distanceTo(cs->point(0));
    if (distance >= MAX_DISTANCE_M)
      continue;
    if (not best_cs or distance < best_distance) {
      best_cs = cs;
      best_distance = distance;
    }
  }
  return best_cs;
}

std::ostream& operator<<(std::ostream& out, const Values::Shape& shp)
{
  out << "[vc shape rank=" << shp.rank();
  for (size_t i=0; i<shp.rank(); ++i)
    out << ' ' << shp.name(i) << ':' << shp.length(i);
  out << ']';
  return out;
}

} // namespace vcross
