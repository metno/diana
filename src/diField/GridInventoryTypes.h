/*
 * GridInventoryTypes.h
 *
 *  Created on: Mar 11, 2010
 *      Author: audunc
 */
/*
 Copyright (C) 2013 met.no

 Contact information:
 Norwegian Meteorological Institute
 Box 43 Blindern
 0313 OSLO
 NORWAY
 email: diana@met.no

 This is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef GRIDINVENTORYTYPES_H_
#define GRIDINVENTORYTYPES_H_

#include "diFieldVerticalAxes.h"

#include "puTools/miTime.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace gridinventory {

/**
 * Inventory super class
 */
class InventoryBase {
public:
  std::string id; ///< identifier
  std::string name; ///< name from data
  std::vector<double> values; ///< the values
  std::vector<std::string> stringvalues; ///< the values with suffix/prefix

  const std::string& getName() const
    { return name; }

  const std::string& getId() const
    { return id; }

  InventoryBase()
  {
  }
  explicit InventoryBase(const std::string& id_) :
    id(id_)
  {
  }
  InventoryBase(const std::string& id_, const std::string& na) :
    id(id_), name(na)
  {
  }
  InventoryBase(const std::string& id_, const std::string& na, const std::vector<double>& va) :
    id(id_),name(na), values(va)
  {
  }
  InventoryBase(const std::string& na, const std::vector<double>& va) :
    name(na), values(va)
  {
  }
  virtual ~InventoryBase()
  {
  }

  std::vector<std::string> getStringValues() const;
  bool valueExists(const std::string&) const;

  friend bool operator==(const InventoryBase& lhs, const InventoryBase& rhs)
  {
    return lhs.sortkey() == rhs.sortkey();
  }
  friend bool operator!=(const InventoryBase& lhs, const InventoryBase& rhs)
  {
    return lhs.sortkey() != rhs.sortkey();
  }
  friend bool operator>(const InventoryBase& lhs, const InventoryBase& rhs)
  {
    return lhs.sortkey() > rhs.sortkey();
  }
  friend bool operator<(const InventoryBase& lhs, const InventoryBase& rhs)
  {
    return lhs.sortkey() < rhs.sortkey();
  }
  friend bool operator>=(const InventoryBase& lhs, const InventoryBase& rhs)
  {
    return lhs.sortkey() >= rhs.sortkey();
  }
  friend bool operator<=(const InventoryBase& lhs, const InventoryBase& rhs)
  {
    return lhs.sortkey() <= rhs.sortkey();
  }

  virtual std::string sortkey() const
  {
    return id;
  }
};

/**
 * Grid
 */
class Grid: public InventoryBase {
public:
  int nx; ///< number of grid points in x
  int ny; ///< number of grid points in y
  float x_0; ///< first grid point in x
  float y_0; ///< first grid point in y
  float x_resolution; ///< distance between grid points in x
  float y_resolution; ///< distance between grid points in y
  std::string projection; ///< projection specification
  bool y_direction_up; //if false -> must to turn field upsidedown

  Grid() :
    InventoryBase(""), nx(0), ny(0), x_0(0), y_0(0), x_resolution(0),
        y_resolution(0)
  {
  }
  explicit Grid(const std::string& id_) :
    InventoryBase(id_), nx(0), ny(0), x_0(0), y_0(0), x_resolution(0),
        y_resolution(0)
  {
  }
  Grid(const std::string& id_, const std::string& /*na*/, const int x, const int y, const float x0,
      const float y0, const float xr, const float yr, const std::string& pr, const bool up) :
    InventoryBase(id_), nx(x), ny(y), x_0(x0), y_0(y0), x_resolution(xr),
        y_resolution(yr), projection(pr), y_direction_up(up)
  {
  }
};

/**
 * Z-axis
 */
class Zaxis: public InventoryBase {
public:
  FieldVerticalAxes::VerticalType vc_type; ///< coordinate type
  bool positive; //direction of axsis
  std::string verticalType;

  Zaxis() :
    vc_type(FieldVerticalAxes::vctype_other), positive(true)
  {
  }
  explicit Zaxis(const std::string& id_) :
    InventoryBase(id_), vc_type(FieldVerticalAxes::vctype_other), positive(true)
  {
  }
  Zaxis(const std::string& id_, const std::string& na, const bool p,
      const std::vector<double>& le, const std::string& verticaltype="") :
    InventoryBase(id_, na, le), vc_type(FieldVerticalAxes::vctype_other), positive(p), verticalType(verticaltype)
  {
    setStringValues();
  }

  void setStringValues();
};

/**
 * Time-axis
 */
class Taxis: public InventoryBase {
public:

  Taxis()
  {
  }
  explicit Taxis(const std::string id_) :
    InventoryBase(id_)
  {
  }
  Taxis(const std::string& na, const std::vector<double>& va) :
    InventoryBase(na, na, va)
  {
  }

};

/**
 * Extra-axis
 */
class ExtraAxis: public InventoryBase {
public:

  ExtraAxis()
  {
  }
  explicit ExtraAxis(const std::string& id_) :
    InventoryBase(id_)
  {
  }
  ExtraAxis(const std::string& id_, const std::string& na, const std::vector<double>& va) :
    InventoryBase(id_, na, va)
  {
    setStringValues();
  }

  void setStringValues();
};

/**
 * Unique parameter definition
 */
class GridParameterKey: public InventoryBase {
public:
  std::string zaxis; ///< Z-axis identifier
  std::string taxis; ///< time-axis identifier
  std::string extraaxis; ///< run-axis identifier

  GridParameterKey()
  {
  }
  GridParameterKey(const std::string& na, const std::string& za, const std::string& ta,
      const std::string& ea) :
    InventoryBase(na,na), zaxis(za), taxis(ta), extraaxis(ea)
  {
  }
  std::string sortkey() const
  {
    return (name + zaxis + extraaxis);
  }
};

/**
 * parameter
 */
class GridParameter: public InventoryBase {
public:
  GridParameterKey key;
  std::string grid;
  std::string unit;
  std::string nativename;
  std::string standard_name;
  std::string long_name;
  std::string nativekey;
  std::string zaxis_id;
  std::string taxis_id;
  std::string extraaxis_id;

  GridParameter()
  {
  }
  explicit GridParameter(const GridParameterKey& k) :
    key(k)
  {
  }
  std::string sortkey() const
  {
    return (key.sortkey());
  }
};

/**
 * Inventory for a given reference time
 */
class ReftimeInventory: public InventoryBase {
public:
  std::string referencetime;
  std::set<GridParameter> parameters;
  std::set<Grid> grids;
  std::set<Zaxis> zaxes;
  std::set<Taxis> taxes;
  std::set<ExtraAxis> extraaxes;
  std::map<std::string,std::string> globalAttributes;

  ReftimeInventory()
  {
  }
  explicit ReftimeInventory(const std::string& reftime)
  {
    referencetime = reftime;
  }
  std::string sortkey() const
  {
    return (referencetime);
  }
  const Taxis& getTaxis(const std::string& name) const;
  const ExtraAxis& getExtraAxis(const std::string& name) const;
  const Zaxis& getZaxis(const std::string& name) const;
};

/**
 * Complete inventory for this grid source
 */
class Inventory {
public:
  typedef std::map<std::string, ReftimeInventory> reftimes_t;

public:
  reftimes_t reftimes;

  Inventory()
  {
  }

  explicit Inventory(const ReftimeInventory& ri)
  {
    reftimes[ri.referencetime] = ri;
  }

  void clear()
  {
    reftimes.clear();
  }

  bool isEmpty() const
  {
    return (reftimes.size() == 0);
  }

  /**
   * Merge this inventory with another and return the resulting inventory
   * @param i
   * @return resulting inventory
   */
  Inventory merge(const Inventory& i) const;
};

} // namespace gridinventory

#endif /* GRIDINVENTORYTYPES_H_ */
