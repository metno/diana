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

#include "diVprofAxesPT.h"

namespace {
const float DEG_TO_RAD = M_PI / 180;
} // namespace

VprofAxesPT::VprofAxesPT()
{
  setAngleT(45);
}

void VprofAxesPT::setAngleT(float angle_deg)
{
  if (std::abs(angle_deg) > 0.1)
    tan_tangle_ = std::tan(angle_deg * DEG_TO_RAD);
  else
    tan_tangle_ = 0;
}

diutil::PointF VprofAxesPT::value2paint(const diutil::PointF& value) const
{
  diutil::PointF paint = VprofAxesStandard::value2paint(value);
  if (tan_tangle_ != 0) {
    paint.rx() += paint.y() * tan_tangle_;
  }
  return paint;
}

diutil::PointF VprofAxesPT::paint2value(const diutil::PointF& p) const
{
  diutil::PointF paint(p);
  if (tan_tangle_ != 0) {
    paint.rx() -= paint.y() * tan_tangle_;
  }
  return VprofAxesStandard::paint2value(paint);
}
