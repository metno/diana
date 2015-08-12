
#ifndef VCROSSDATA_HH
#define VCROSSDATA_HH 1

#include "DataReshape.h"

#include <puDatatypes/miCoordinates.h>
#include <puCtools/deprecated.h>

#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>

#include <map>
#include <set>
#include <string>
#include <vector>

namespace vcross {

typedef std::vector<LonLat> LonLat_v;

// ================================================================================

enum Z_DIRECTION {
  Z_DIRECTION_UP = 0,   //!< higher level index means higher altitude
  Z_DIRECTION_DOWN = 1  //!< higher level index means lower altitude
};

// ================================================================================

enum Z_AXIS_TYPE {
  Z_TYPE_UNKNOWN  = 0,
  Z_TYPE_PRESSURE = 1,
  Z_TYPE_ALTITUDE = 2,
  Z_TYPE_LEVEL    = 3
};

// ================================================================================

struct ZAxisType {
  Z_AXIS_TYPE type;
  Z_DIRECTION direction;
  ZAxisType(Z_AXIS_TYPE t, Z_DIRECTION d)
    : type(t), direction(d) { }
};

// ================================================================================

ZAxisType zAxisTypeFromUnitAndDirection(std::string unit, Z_DIRECTION direction);

// ================================================================================

//! base class for vcross data
class InventoryBase
{
public:
  typedef std::string Id_t;
  typedef std::string Unit_t;

  InventoryBase(Id_t id, Unit_t unit="")
    : mId(id), mUnit(unit), mNLevel(1) { }

  virtual ~InventoryBase() { }

  virtual Id_t id() const
    { return mId; }

  virtual Unit_t unit() const
    { return mUnit; }
  void setUnit(Unit_t u)
    { mUnit = u; }

  virtual std::string dataType() const = 0; //! one of ZLEVEL, FIELD, FUNCTION

  size_t nlevel() const
    { return mNLevel; }
  void setNlevel(size_t nl)
    { mNLevel = nl; }

private:
  Id_t mId;
  Unit_t mUnit;
  size_t mNLevel;
};
typedef boost::shared_ptr<InventoryBase> InventoryBase_p;
typedef boost::shared_ptr<const InventoryBase> InventoryBase_cp;
typedef std::vector<InventoryBase_p> InventoryBase_pv;
typedef std::vector<InventoryBase_cp> InventoryBase_cpv;

struct InventoryById : public std::binary_function<bool, InventoryBase_cp, InventoryBase_cp>
{
  bool operator()(const InventoryBase_cp& a, const InventoryBase_cp& b) const
    { return a and b and a->id() < b->id(); }
};
typedef std::set<InventoryBase_cp, InventoryById> InventoryBase_cps;

InventoryBase_cp findItemById(const InventoryBase_cpv& items, InventoryBase::Id_t id);
InventoryBase_cp findItemById(const InventoryBase_cps& items, InventoryBase::Id_t id);

// ================================================================================

class ZAxisData : public InventoryBase
{
public:
  ZAxisData(Id_t id, Unit_t unit)
    : InventoryBase(id, unit) { }
  virtual Z_DIRECTION zdirection() const
    { return mZDirection; }
  void setZDirection(Z_DIRECTION zd)
    { mZDirection = zd; }
  virtual ZAxisType zAxisType() const;
  Z_AXIS_TYPE zType() const
    { return zAxisType().type; }

  std::string hybridType() const
    { return mHybridType; }
  void setHybridType(std::string ht)
    { mHybridType = ht; }
  Id_t hybridParameterId(std::string hybridParameterName) const;
  void setHybridParameterId(std::string hybridParameterName, Id_t id);

  virtual std::string dataType() const
    { return DATA_TYPE(); }
  static std::string DATA_TYPE();

  InventoryBase_cp pressureField() const
    { return mPressureField; }
  void setPressureField(InventoryBase_cp p)
    { mPressureField = p; };

  InventoryBase_cp altitudeField() const
    { return mAltitudeField; }
  void setAltitudeField(InventoryBase_cp h)
    { mAltitudeField = h; }

private:
  Z_DIRECTION mZDirection;
  std::string mHybridType;
  typedef std::map<std::string, Id_t> hybridparameterids_t;
  hybridparameterids_t mHybridParameterIds;

  InventoryBase_cp mPressureField, mAltitudeField;
};

typedef boost::shared_ptr<ZAxisData>       ZAxisData_p;
typedef boost::shared_ptr<const ZAxisData> ZAxisData_cp;

// ================================================================================

// data with horizontal and vertical dimension
class FieldData : public InventoryBase
{
public:
  FieldData(Id_t id, Unit_t unit)
    : InventoryBase(id, unit) { }

  bool hasZAxis() const
    { return mZAxis; }
  ZAxisData_cp zaxis() const
    { return mZAxis; }
  void setZAxis(ZAxisData_cp z)
    { mZAxis = z; if (z) setNlevel(z->nlevel()); }
  virtual std::string dataType() const
    { return DATA_TYPE(); }
  static std::string DATA_TYPE();
  
private:
  ZAxisData_cp mZAxis;
};
typedef boost::shared_ptr<FieldData> FieldData_p;
typedef boost::shared_ptr<const FieldData> FieldData_cp;
typedef std::vector<FieldData_cp> FieldData_cpv;

// ================================================================================

// wrapper around shared_array<float>
class Values
{
public:
  typedef float value_t;
  typedef boost::shared_array<value_t> ValueArray;

public:
  class Shape {
  public:
    typedef std::vector<std::size_t> size_v;
    typedef std::vector<std::string> string_v;

    Shape();
    Shape(const string_v& names, const size_v& lengths);
    Shape(const std::string& name0, size_t length0);
    Shape(const std::string& name0, size_t length0, const std::string& name1, size_t length1);

    Shape& add(const std::string& name, size_t length);

    const string_v& names() const
      { return mNames; }
    const std::string& name(int position) const
      { return mNames.at(position); }
    size_t rank() const
      { return mNames.size(); }

    int position(const std::string& name) const;

    int length(int position) const;
    int length(const std::string& name) const
      { return length(position(name)); }
    const size_v& lengths() const
      { return mLengths; }
    size_t volume() const;

  private:
    string_v mNames;
    size_v mLengths;
  };

  class ShapeIndex {
  public:
    ShapeIndex(const Shape& shape);
    ShapeIndex& set(int position, size_t element);
    ShapeIndex& set(const std::string& name, size_t element)
      { return set(mShape.position(name), element); }
    size_t index() const;
  private:
    const Shape& mShape;

    typedef std::vector<std::size_t> size_v;
    size_v mElements;
  };

  class ShapeSlice {
  public:
    typedef std::vector<size_t> size_v;

  public:
    ShapeSlice(const Shape& shape);

    ShapeSlice& cut(int position, size_t start, size_t length);
    ShapeSlice& cut(const std::string& name, size_t start, size_t length)
      { return cut(mShape.position(name), start, length); }

    const Shape& shape() const
      { return mShape; }

    size_t start(int position) const
      { return get(mStarts, position, 0); }
    size_t start(const std::string& name) const
      { return start(mShape.position(name)); }
    const size_v& starts() const
      { return mStarts; }

    size_t length(int position) const
      { return get(mLengths, position, 1); }
    size_t length(const std::string& name) const
      { return length(mShape.position(name)); }
    const size_v& lengths() const
      { return mLengths; }

    size_t end(int position) const
      { return start(position) + length(position); }
    size_t end(const std::string& name) const
      { return end(mShape.position(name)); }

    size_t volume() const;

  private:
    size_t get(const size_v& v, int position, size_t missing) const
      { return (position >= 0 and position < (int)v.size()) ? v[position] : missing; }

  private:
    const Shape& mShape;
    size_v mStarts, mLengths;
  };

  METLIBS_DEPRECATED(METLIBS_CONCAT(Values(int np, int nl, ValueArray v)),   "use Values(const Shape&, ValueArray)");
  METLIBS_DEPRECATED(METLIBS_CONCAT(Values(int np, int nl, bool fill=true)), "use Values(const Shape&, bool)");

  Values(const Shape& shape, ValueArray v);
  Values(const Shape& shape, bool fill=true);

  static const char *GEO_X, *GEO_Y, *GEO_Z, *TIME;

  METLIBS_DEPRECATED(size_t npoint() const, "use shape().length(...)")
    { return mShape.length(0); }
  METLIBS_DEPRECATED(size_t nlevel() const, "use shape().length(...)")
    { return mShape.length(1); }

  METLIBS_DEPRECATED(METLIBS_CONCAT(value_t value(int point, int level) const), "use value(const ShapeIndex&) const")
    { ShapeIndex si(mShape); si.set(0, point).set(1, level); return value(si); }
  METLIBS_DEPRECATED(METLIBS_CONCAT(void setValue(value_t v, int point, int level)), "use setValue(value_t, const ShapeIndex&)")
    { ShapeIndex si(mShape); si.set(0, point).set(1, level); setValue(v, si); }

  value_t value(const ShapeIndex& si) const
    { return mValues[si.index()]; }
  void setValue(value_t v, const ShapeIndex& si)
    { mValues[si.index()] = v; }

  value_t undefValue() const
    { return mUndefValue; }
  void setUndefValue(value_t u)
    { mUndefValue = u; }

  /** direct access to the values -- make sure that you know the shape if you use this! */
  ValueArray values() const
    { return mValues; }
  const Shape& shape() const
    { return mShape; }

private:
  Shape mShape;
  ValueArray mValues;
  float mUndefValue;
};
typedef boost::shared_ptr<Values> Values_p;
typedef std::vector<Values_p> Values_pv;

typedef boost::shared_ptr<const Values> Values_cp;
typedef std::vector<Values_cp> Values_cpv;

typedef std::map<std::string, Values_cp> name2value_t;

template<typename T>
boost::shared_array<T> reshape(const Values::Shape& shapeIn, const Values::Shape& shapeOut, boost::shared_array<T> dataIn)
{
  return ::reshape(shapeIn.names(), shapeIn.lengths(), shapeOut.names(), shapeOut.lengths(), dataIn);
}

template<typename T>
boost::shared_array<T> reshape(const Values::ShapeSlice& sliceIn, const Values::Shape& shapeOut, boost::shared_array<T> dataIn)
{
  return ::reshape(sliceIn.shape().names(), sliceIn.lengths(), shapeOut.names(), shapeOut.lengths(), dataIn);
}

Values_p reshape(Values_p valuesIn, const Values::Shape& shapeOut);

// ================================================================================

class Crossection
{
public:
  Crossection(const std::string& label, const LonLat_v& points,
      const LonLat_v& pointsRequested);
  virtual ~Crossection();

  const std::string& label() const
    { return mLabel; }

  size_t length() const
    { return mPoints.size(); }
  const LonLat& point(int i) const
    { return mPoints.at(i); }

  size_t lengthRequested() const
    { return mPointsRequested.size(); }
  const LonLat& pointRequested(int i) const
    { return mPointsRequested.at(i); }

  virtual bool dynamic() const = 0;

private:
  std::string mLabel;
  LonLat_v mPoints;
  LonLat_v mPointsRequested;
};
typedef boost::shared_ptr<Crossection> Crossection_p;
typedef boost::shared_ptr<const Crossection> Crossection_cp;
typedef std::vector<Crossection_cp> Crossection_cpv;

// ================================================================================

struct Time
{
  typedef double timevalue_t;
  std::string unit;
  timevalue_t value;
  Time(std::string u, timevalue_t v)
    : unit(u), value(v) { }
  Time()
    : value(0) { }

  bool valid() const;

  bool operator==(const Time& other) const;
  bool operator!=(const Time& other) const
    { return !(*this == other); }

  bool operator<(const Time& other) const;
  bool operator>(const Time& other) const
    { return other < *this; }
};

typedef std::vector<Time> Time_v;
typedef std::set<Time> Time_s;

// ================================================================================

struct Times
{
  typedef std::vector<Time::timevalue_t> timevalue_v;

  std::string unit;
  timevalue_v values;
  int npoint() const
    { return values.size(); }
  Time at(size_t i) const
    { return Time(unit, values.at(i)); }
};

// ================================================================================

struct Inventory
{
  typedef std::vector<FieldData_cp> fields_t;

  Times times;
  Crossection_cpv crossections;
  fields_t fields;

  void clear();
  FieldData_cp findFieldById(InventoryBase::Id_t id) const;
  Crossection_cp findCrossectionByLabel(const std::string& cslabel) const;
  Crossection_cp findCrossectionPoint(const LonLat& point, size_t& best_index) const;
  Crossection_cp findCrossectionPoint(const LonLat& point) const;
};
typedef boost::shared_ptr<Inventory> Inventory_p;
typedef boost::shared_ptr<const Inventory> Inventory_cp;

} // namespace vcross

#endif
