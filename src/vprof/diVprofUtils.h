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

#ifndef VPROFUTILS_H
#define VPROFUTILS_H

#include "diVprofPainter.h"
#include "diVprofPlotCommand.h"

#include <QSize>

#include <vector>

class VprofOptions;

namespace vprof {

const float UNDEF = 1e35;

inline bool is_invalid(float f)
{
  return std::isnan(f) || std::abs(f) >= 1e20;
}

inline float invalid_to_undef(float f)
{
  return is_invalid(f) ? UNDEF : f;
}

extern const float chxbas, chybas;

extern const std::string VP_WIND_X;                // x wind speed in m/s
extern const std::string VP_WIND_Y;                // y wind speed in m/s
extern const std::string VP_WIND_DD;               // wind direction in meteorological degrees (0 == north)
extern const std::string VP_WIND_FF;               // wind speed in m/s
extern const std::string VP_WIND_SIG;              // wind significance, 1=local-minimum, 2=local-maximum, 3=global-maximum
extern const std::string VP_RELATIVE_HUMIDITY;     // in %
extern const std::string VP_OMEGA;                 // in hPa/s
extern const std::string VP_AIR_TEMPERATURE;       // in degC
extern const std::string VP_DEW_POINT_TEMPERATURE; // in degC
extern const std::string VP_DUCTING_INDEX;
extern const std::string VP_CLOUDBASE;

extern const std::string VP_UNIT_COMPASS_DEGREES; // meteorological degrees (0 == north, 90 = east, i.e. like a clock)

extern const std::vector<int> default_flightlevels;

Colour alternateColour(const Colour& c);

struct TextSpacing
{
  float next, last, spacing;
  TextSpacing(float n, float l, float s);
  bool accept(float v);

  /** Flip direction by exchanging next and last and changing the sign of spacing. */
  void flip();
};

diutil::PointF interpolateY(float y, const diutil::PointF& p1, const diutil::PointF& p2);

diutil::PointF interpolateX(float x, const diutil::PointF& p1, const diutil::PointF& p2);

typedef std::vector<diutil::PointF> PointF_v;

diutil::PointF scaledTextSize(int length, float width);

extern const std::string kv_linestyle_colour;
extern const std::string kv_linestyle_linewidth;
extern const std::string kv_linestyle_linetype;

bool kvLinestyle(Linestyle& ls, const std::string& prefix, const miutil::KeyValue& kv);

PlotCommand_cpv createCommandsFromOptions(const VprofOptions* vpopt);
VprofPlotCommand_cpv createVprofCommandsFromOptions(const VprofOptions* vpopt);

} // namespace vprof

#endif // VPROFUTILS_H
