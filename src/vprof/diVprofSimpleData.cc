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

#include "diVprofSimpleData.h"

bool VprofSimpleData::ZX::is_invalid() const
{
  return vprof::is_invalid(z_) || vprof::is_invalid(x_);
}

VprofSimpleData::VprofSimpleData(const std::string& id, const std::string& z_unit, const std::string& x_unit)
    : VprofGraphData(id)
    , z_unit_(z_unit)
    , x_unit_(x_unit)
    , defined_(miutil::ALL_DEFINED)
{
}

void VprofSimpleData::add(float z, float x)
{
  points_.push_back(ZX(z, x));
  const bool invalid = points_.back().is_invalid();
  if (points_.size() == 1) {
    defined_ = invalid ? miutil::NONE_DEFINED : miutil::ALL_DEFINED;
  } else if ((invalid && defined_ == miutil::ALL_DEFINED) || (!invalid && defined_ == miutil::NONE_DEFINED)) {
    defined_ = miutil::SOME_DEFINED;
  }
}
