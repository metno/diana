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

#include "diana_config.h"

#include "diVprofAxesStandard.h"

VprofAxesStandard::VprofAxesStandard()
    : z(std::make_shared<vcross::detail::Axis>(false))
    , x(std::make_shared<vcross::detail::Axis>(true))
{
}

void VprofAxesStandard::setPaintRange(const Rectangle& rect)
{
  x->setPaintRange(rect.x1, rect.x2);
  z->setPaintRange(rect.y1, rect.y2);
}

Rectangle VprofAxesStandard::paintRange() const
{
  return Rectangle(x->getPaintMin(), z->getPaintMin(), x->getPaintMax(), z->getPaintMax());
}

diutil::PointF VprofAxesStandard::value2paint(const diutil::PointF& value) const
{
  return diutil::PointF(x->value2paint(value.x(), false), z->value2paint(value.y(), false));
}

diutil::PointF VprofAxesStandard::paint2value(const diutil::PointF& paint) const
{
  return diutil::PointF(x->paint2value(paint.x(), false), z->paint2value(paint.y(), false));
}
