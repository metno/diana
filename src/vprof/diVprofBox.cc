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

#include "diVprofBox.h"

#include "diVprofAxesStandard.h"

const std::string VprofBox::key_id = "id";
const std::string VprofBox::key_type = "type";
const std::string VprofBox::key_width = "width";

VprofBox::VprofBox()
    : width_(0)
{
}

VprofBox::~VprofBox()
{
}

bool VprofBox::separate() const
{
  return false;
}

void VprofBox::configure(const miutil::KeyValue_v& options)
{
  for (const auto& kv : options) {
    if (kv.key() == key_width)
      setWidth(kv.toFloat());
  }
}

Rectangle VprofBox::size() const
{
  const Rectangle a = area();
  return Rectangle(a.x1 - margin.x1, a.y1 - margin.y1, a.x2 + margin.x2, a.y2 + margin.y2);
}

void VprofBox::updateLayout()
{
  margin = Rectangle();
}

void VprofBox::setId(const std::string& id)
{
  id_ = id;
}

size_t VprofBox::graphComponentCount(size_t /*graph*/)
{
  return 0;
}

std::string VprofBox::graphComponentVarName(size_t /*graph*/, size_t /*component*/)
{
  return "";
}
