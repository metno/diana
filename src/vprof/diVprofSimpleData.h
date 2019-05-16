/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018 met.no

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

#ifndef diVprofSimpleData_h
#define diVprofSimpleData_h

#include "diVprofGraphData.h"

#include "diVprofUtils.h"

#include <vector>

class VprofSimpleData : public VprofGraphData
{
private:
  struct ZX
  {
    float z_, x_;
    ZX(float z, float x)
        : z_(z)
        , x_(x)
    {
    }
    bool is_invalid() const;
  };

  typedef std::vector<ZX> ZX_v;

public:
  VprofSimpleData(const std::string& id, const std::string& z_unit, const std::string& x_unit);

  void clear() { points_.clear(); }
  void reserve(size_t expected) { points_.reserve(expected); }
  void add(float z, float x);

  float z(size_t index) const override { return points_[index].z_; }
  float x(size_t index) const override { return points_[index].x_; }
  void setX(size_t index, float x) { points_[index].x_ = x; }

  const std::string& zUnit() const override { return z_unit_; }
  const std::string& xUnit() const override { return x_unit_; }

  bool empty() const override { return points_.empty(); }
  size_t length() const override { return points_.size(); }
  miutil::ValuesDefined defined() const override { return defined_; }

private:
  std::string z_unit_;
  std::string x_unit_;

  ZX_v points_;
  miutil::ValuesDefined defined_;
};

typedef std::shared_ptr<VprofSimpleData> VprofSimpleData_p;
typedef std::shared_ptr<const VprofSimpleData> VprofSimpleData_cp;

#endif // diVprofSimpleData_h
