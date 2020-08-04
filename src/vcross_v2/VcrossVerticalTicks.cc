/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2018-2020 met.no

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

#include "VcrossVerticalTicks.h"

#include "diField/VcrossUtil.h"
#include "vcross_v2/VcrossQtAxis.h"

#include <mi_fieldcalc/MetConstants.h>

#include <boost/range/size.hpp>

#include <cmath>

#define MILOGGER_CATEGORY "diana.VcrossVerticalTicks"
#include <miLogger/miLogging.h>

namespace vcross {
namespace {

const float FLTABLE[] = {25, 50, 100, 140, 180, 240, 300, 340, 390, 450, 600, 700, 800, 999};

namespace Unit {
const float m = 1;
const float nm = 1852 * m; // nautical mile
const float km = 1000 * m;
}

// begin utility functions for y ticks

ticks_t ticks_table(const float* table, int size)
{
  return ticks_t(table, table + size);
}

ticks_t ticks_auto(float start, float end, float scale, float offset)
{
  scale = std::abs(scale);
  if ((start > end) != (offset < 0))
    offset *= -1;
  ticks_t ticks(1, start);
  while ((offset > 0 && ticks.back() < end) || (offset < 0 && ticks.back() > end))
    ticks.push_back(ticks.back() * scale + offset);
  return ticks;
}

} // namespace

float identity(float x)
{
  return x;
}

float foot_to_meter(float ft)
{
  return ft / miutil::constants::ft_per_m;
}

float meter_to_foot(float m)
{
  return m * miutil::constants::ft_per_m;
}

void generateVerticalTicks(vcross::detail::AxisCPtr zAxis, ticks_t& tickValues, tick_to_axis_f& tta)
{
  generateVerticalTicks(zAxis, zAxis->label(), tickValues, tta);
}

void generateVerticalTicks(vcross::detail::AxisCPtr zAxis, const std::string& zLabel, ticks_t& tickValues, tick_to_axis_f& tta)
{
  METLIBS_LOG_SCOPE(LOGVAL(zAxis->quantity()) << LOGVAL(zLabel));
  using namespace miutil::constants;

  tickValues.clear();
  tta = identity;

  float autotick_offset = 0, autotick_scale = 1;

  if (zAxis->quantity() == vcross::detail::Axis::PRESSURE) {
    if (zLabel == "hPa") {
      tickValues = ticks_table(pLevelTable, nLevelTable);
      autotick_offset = -5;
    } else if (zLabel == "FL") {
      tickValues = ticks_table(FLTABLE, boost::size(FLTABLE));
      tta = vcross::util::FL_to_hPa;
      autotick_offset = 10;
    } else {
      METLIBS_LOG_WARN("unknown z label '" << zLabel << "'");
      return;
    }
  } else if (zAxis->quantity() == vcross::detail::Axis::ALTITUDE || zAxis->quantity() == vcross::detail::Axis::HEIGHT) {
    if (zLabel == "m") {
      const float zsteps[] = {100., 500., 1000., 2500., 5000., 10000, 15000, 20000, 25000, 30000, 35000, 40000};
      tickValues = ticks_table(zsteps, boost::size(zsteps));
      autotick_offset = 100;
    } else if (zLabel == "Ft") {
      const float ftsteps[] = {0, 100, 1500, 3000, 8000, 15000, 30000, 50000, 60000, 70000, 80000, 90000};
      tickValues = ticks_table(ftsteps, boost::size(ftsteps));
      tta = foot_to_meter;
      autotick_offset = 500;
    } else {
      METLIBS_LOG_WARN("unknown z label '" << zLabel << "'");
      return;
    }
  } else if (zAxis->quantity() == vcross::detail::Axis::DEPTH) {
    if (zLabel == "m") {
      const float zsteps[] = {0, 3, 10, 15, 25, 50, 75, 100, 150, 200, 250, 300, 400, 500, 750, 1000, 1500, 2000, 3000, 4000, 5000};
      tickValues = ticks_table(zsteps, boost::size(zsteps));
      autotick_offset = 100;
    } else {
      METLIBS_LOG_WARN("unknown z label '" << zLabel << "'");
      return;
    }
  } else {
    METLIBS_LOG_WARN("unsupported z quantity");
    return;
  }

  int visibleTicks = 0;
  for (size_t i = 0; i < tickValues.size(); ++i) {
    const float axisValue = tta(tickValues[i]);
    const float axisY = zAxis->value2paint(axisValue);
    if (zAxis->legalPaint(axisY))
      visibleTicks += 1;
  }
  METLIBS_LOG_DEBUG(LOGVAL(visibleTicks));
  if (visibleTicks < 6) {
    tickValues = ticks_auto(tickValues.front(), tickValues.back(), autotick_scale, autotick_offset);
    METLIBS_LOG_DEBUG(LOGVAL(tickValues.size()));
  }
}

} // namespace vcross
