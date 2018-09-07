/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2018 met.no

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

#ifndef VcrossVerticalTicks_h
#define VcrossVerticalTicks_h

#include <memory>
#include <vector>

namespace vcross {

namespace detail {
struct Axis;
typedef std::shared_ptr<Axis> AxisPtr;
typedef std::shared_ptr<const Axis> AxisCPtr;
} // namespace detail

typedef std::vector<float> ticks_t;
typedef float (*tick_to_axis_f)(float);

float identity(float x);

float foot_to_meter(float ft);

float meter_to_foot(float m);

void generateVerticalTicks(vcross::detail::AxisCPtr mAxisY, ticks_t& tickValues, tick_to_axis_f& tta);
void generateVerticalTicks(detail::AxisCPtr zAxis, const std::string& zLabel, ticks_t& tickValues, tick_to_axis_f& tta);

} // namespace vcross

#endif // VcrossVerticalTicks_h
