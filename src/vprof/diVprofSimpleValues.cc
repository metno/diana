/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

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

#include "diVprofSimpleValues.h"

#include "diVprofCalculations.h"
#include "diVprofUtils.h"

#define MILOGGER_CATEGORY "diana.VprofSimpleValues"
#include <miLogger/miLogging.h>

VprofSimpleValues::VprofSimpleValues()
    : defined_(difield::NONE_DEFINED)
{
}

void VprofSimpleValues::add(VprofGraphData_cp series)
{
  if (!series)
    return;

  series_v::iterator it = std::find_if(series_.begin(), series_.end(), [&](VprofGraphData_cp s) { return s->id() == series->id(); });
  if (it == series_.end()) {
    if (series_.empty()) {
      defined_ = series->defined();
    } else {
      defined_ = difield::combineDefined(defined_, series->defined());
    }

    series_.push_back(series);
  } else {
    const bool recheck_defined = ((*it)->defined() != series->defined());

    *it = series;

    if (recheck_defined) {
      defined_ = series->defined();
      for (VprofGraphData_cp s : series_)
        defined_ = difield::combineDefined(defined_, series->defined());
    }
  }
}

VprofGraphData_cp VprofSimpleValues::series(const std::string& id) const
{
  series_v::const_iterator it = std::find_if(series_.begin(), series_.end(), [&](VprofGraphData_cp s) { return s->id() == id; });
  if (it != series_.end())
    return *it;
  else
    return VprofGraphData_cp();
}

void VprofSimpleValues::calculate()
{
  VprofGraphData_cp tt = series(vprof::VP_AIR_TEMPERATURE), td = series(vprof::VP_DEW_POINT_TEMPERATURE);
  if (!series(vprof::VP_RELATIVE_HUMIDITY)) {
    if (VprofGraphData_cp rh = vprof::relhum(tt, td))
      add(rh);
  }
  if (!series(vprof::VP_DUCTING_INDEX)) {
    if (VprofGraphData_cp duct = vprof::ducting(tt, td))
      add(duct);
  }
  if (!series(vprof::VP_CLOUDBASE)) {
    if (VprofGraphData_cp cb = vprof::cloudbase(tt, td))
      add(cb);
  }

  if (!series(vprof::VP_WIND_X) && !series(vprof::VP_WIND_Y)) {
    VprofGraphData_cp wind_dd = series(vprof::VP_WIND_DD), wind_ff = series(vprof::VP_WIND_FF);
    if (wind_dd && wind_ff) {
      const std::tuple<VprofGraphData_cp, VprofGraphData_cp> wind_uv = vprof::wind_uv(wind_dd, wind_ff);
      add(std::get<0>(wind_uv));
      add(std::get<1>(wind_uv));
    }
  }
  if (!series(vprof::VP_WIND_DD) && !series(vprof::VP_WIND_FF)) {
    VprofGraphData_cp wind_x = series(vprof::VP_WIND_X), wind_y = series(vprof::VP_WIND_Y);
    if (wind_x && wind_y) {
      const std::tuple<VprofGraphData_cp, VprofGraphData_cp> wind_dd_ff = vprof::wind_dd_ff(wind_x, wind_y);
      add(std::get<0>(wind_dd_ff));
      add(std::get<1>(wind_dd_ff));
    }
  }
  if (!series(vprof::VP_WIND_SIG)) {
    if (VprofGraphData_cp wind_ff = series(vprof::VP_WIND_FF))
      add(vprof::wind_sig(wind_ff));
  }

  text.kindexValue = vprof::kindex(tt, td);
  text.kindexFound = !vprof::is_invalid(text.kindexValue);
}
