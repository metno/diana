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

#include "diVprofBoxZ.h"

#include "diVprofAxesStandard.h"

const std::string VprofBoxZ::key_separate = "separate";

VprofBoxZ::VprofBoxZ()
    : axis_z(std::make_shared<vcross::detail::Axis>(false))
{
}

void VprofBoxZ::setArea(const Rectangle& area)
{
  x1 = area.x1;
  x2 = area.x2;
  axis_z->setPaintRange(area.y1, area.y2);
}

Rectangle VprofBoxZ::area() const
{
  return Rectangle(x1, axis_z->getPaintMin(), x2, axis_z->getPaintMax());
}

void VprofBoxZ::setVerticalAxis(vcross::detail::AxisPtr zaxis)
{
  axis_z = zaxis;
}

bool VprofBoxZ::addGraph(const miutil::KeyValue_v&)
{
  return false;
}
