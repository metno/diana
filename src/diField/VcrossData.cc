
#include "VcrossData.h"

#include "VcrossUtil.h"

#include <boost/foreach.hpp>
#include <cmath>
#include <numeric> // std::accumulate

#define MILOGGER_CATEGORY "vcross.VcrossData"
#include "miLogger/miLogging.h"

static const float NANF = nanf("");
static const double MAX_DISTANCE_M = 10000;

namespace vcross {

ZAxisType zAxisTypeFromUnitAndDirection(InventoryBase::Unit_t unit, Z_DIRECTION direction)
{
  Z_AXIS_TYPE t = Z_TYPE_UNKNOWN;
  if (unit.empty())
    t = Z_TYPE_LEVEL;
  if (util::unitsConvertible(unit, "hPa"))
    t = Z_TYPE_PRESSURE;
  else if (util::unitsConvertible(unit, "m"))
    t = Z_TYPE_ALTITUDE;

  return ZAxisType(t, direction);
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

ZAxisType ZAxisData::zAxisType() const
{
  return zAxisTypeFromUnitAndDirection(unit(), zdirection());
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
const char *Values::TIME = "TIME";

Values::Values(int np, int nl, ValueArray v)
  : mShape(Shape(GEO_X, np, GEO_Z, nl))
  , mValues(v)
  , mUndefValue(NANF)
{
  assert(mShape.volume() == size_t(np * nl));
}

Values::Values(int np, int nl, bool fill)
  : mShape(Shape(GEO_X, np, GEO_Z, nl))
  , mValues(new float[mShape.volume()])
  , mUndefValue(NANF)
{
  assert(mShape.volume() == size_t(np * nl));
  if (fill)
    std::fill(mValues.get(), mValues.get()+mShape.volume(), mUndefValue);
}

Values::Values(const Shape& shape, ValueArray v)
  : mShape(shape)
  , mValues(v)
  , mUndefValue(NANF)
{
}

Values::Values(const Shape& shape, bool fill)
  : mShape(shape)
  , mValues(new float[mShape.volume()])
  , mUndefValue(NANF)
{
  if (fill)
    std::fill(mValues.get(), mValues.get()+mShape.volume(), mUndefValue);
}

Values_p reshape(Values_p valuesIn, const Values::Shape& shapeOut)
{
  return Values_p(new Values(shapeOut, reshape(valuesIn->shape(), shapeOut, valuesIn->values())));
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

void Inventory::clear()
{
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
  BOOST_FOREACH(Crossection_cp cs, crossections) {
    if (cs->label() == cslabel)
      return cs;
  }
  return Crossection_cp();
}

Crossection_cp Inventory::findCrossectionPoint(const LonLat& point, size_t& best_index) const
{
  Crossection_cp best_cs;
  double best_distance = 0;

  BOOST_FOREACH(Crossection_cp cs, crossections) {
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

  BOOST_FOREACH(Crossection_cp cs, crossections) {
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

} // namespace vcross
