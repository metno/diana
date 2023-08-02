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

#ifndef VCROSSDATA_HH
#define VCROSSDATA_HH 1

#include "DataReshape.h"
#include "diField/diValues.h"

#include <puDatatypes/miCoordinates.h>

#include <map>
#include <memory>
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

enum Z_AXIS_TYPE { Z_TYPE_UNKNOWN, Z_TYPE_PRESSURE, Z_TYPE_ALTITUDE, Z_TYPE_HEIGHT, Z_TYPE_DEPTH, Z_TYPE_LEVEL };

// ================================================================================

struct ZAxisType {
  Z_AXIS_TYPE type;
  Z_DIRECTION direction;
  ZAxisType(Z_AXIS_TYPE t, Z_DIRECTION d)
    : type(t), direction(d) { }
};

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
typedef std::shared_ptr<InventoryBase> InventoryBase_p;
typedef std::shared_ptr<const InventoryBase> InventoryBase_cp;
typedef std::vector<InventoryBase_p> InventoryBase_pv;
typedef std::vector<InventoryBase_cp> InventoryBase_cpv;

struct InventoryById
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

  std::string hybridType() const
    { return mHybridType; }
  void setHybridType(std::string ht)
    { mHybridType = ht; }
  Id_t hybridParameterId(std::string hybridParameterName) const;
  void setHybridParameterId(std::string hybridParameterName, Id_t id);

  virtual std::string dataType() const
    { return DATA_TYPE(); }
  static std::string DATA_TYPE();

  InventoryBase_cp pressureField() const { return getField(Z_TYPE_PRESSURE); }
  void setPressureField(InventoryBase_cp f) { setField(Z_TYPE_PRESSURE, f); };

  InventoryBase_cp altitudeField() const { return getField(Z_TYPE_ALTITUDE); }
  void setAltitudeField(InventoryBase_cp f) { setField(Z_TYPE_ALTITUDE, f); };

  InventoryBase_cp depthField() const { return getField(Z_TYPE_DEPTH); }
  void setDepthField(InventoryBase_cp f) { setField(Z_TYPE_DEPTH, f); };

  InventoryBase_cp getField(Z_AXIS_TYPE t) const;
  void setField(Z_AXIS_TYPE t, InventoryBase_cp f);

private:
  Z_DIRECTION mZDirection;
  std::string mHybridType;
  typedef std::map<std::string, Id_t> hybridparameterids_t;
  hybridparameterids_t mHybridParameterIds;

  typedef std::map<Z_AXIS_TYPE, InventoryBase_cp> fields_t;
  fields_t mFields;
};

typedef std::shared_ptr<ZAxisData>       ZAxisData_p;
typedef std::shared_ptr<const ZAxisData> ZAxisData_cp;

// ================================================================================

// data with horizontal and vertical dimension
class FieldData : public InventoryBase
{
public:
  FieldData(Id_t id, Unit_t unit)
    : InventoryBase(id, unit) { }

  bool hasZAxis() const
    { return !!mZAxis; }
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
typedef std::shared_ptr<FieldData> FieldData_p;
typedef std::shared_ptr<const FieldData> FieldData_cp;
typedef std::vector<FieldData_cp> FieldData_cpv;

// ================================================================================

typedef std::map<std::string, diutil::Values_cp> name2value_t;

template <typename T>
std::shared_ptr<T[]> reshape(const diutil::Values::Shape& shapeIn, const diutil::Values::Shape& shapeOut, std::shared_ptr<T[]> dataIn)
{
  return ::reshape(shapeIn.names(), shapeIn.lengths(), shapeOut.names(), shapeOut.lengths(), dataIn);
}

template <typename T>
std::shared_ptr<T[]> reshape(const diutil::Values::ShapeSlice& sliceIn, const diutil::Values::Shape& shapeOut, std::shared_ptr<T[]> dataIn)
{
  return ::reshape(sliceIn.shape().names(), sliceIn.lengths(), shapeOut.names(), shapeOut.lengths(), dataIn);
}

diutil::Values_p reshape(diutil::Values_p valuesIn, const diutil::Values::Shape& shapeOut);

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
typedef std::shared_ptr<Crossection> Crossection_p;
typedef std::shared_ptr<const Crossection> Crossection_cp;
typedef std::vector<Crossection_cp> Crossection_cpv;

// ================================================================================

struct Time
{
  typedef double timevalue_t;
  std::string unit;
  timevalue_t value;
  Time(const std::string& u, timevalue_t v)
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
  int realizationCount;
  Crossection_cpv crossections;
  fields_t fields;

  Inventory();
  void clear();
  FieldData_cp findFieldById(InventoryBase::Id_t id) const;
  Crossection_cp findCrossectionByLabel(const std::string& cslabel) const;
  Crossection_cp findCrossectionPoint(const LonLat& point, size_t& best_index) const;
  Crossection_cp findCrossectionPoint(const LonLat& point) const;
};
typedef std::shared_ptr<Inventory> Inventory_p;
typedef std::shared_ptr<const Inventory> Inventory_cp;

// ================================================================================

const vcross::InventoryBase::Unit_t& zAxisUnit(Z_AXIS_TYPE zType);

} // namespace vcross

#endif
