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

#ifndef diVprofCalculations_h
#define diVprofCalculations_h

#include "diVprofGraphData.h"

namespace vprof {

VprofGraphData_cp relhum(VprofGraphData_cp tt, VprofGraphData_cp td);
VprofGraphData_cp cloudbase(VprofGraphData_cp tt, VprofGraphData_cp td);
VprofGraphData_cp ducting(VprofGraphData_cp tt, VprofGraphData_cp td);

//! convert to east/west and north/south component
std::tuple<VprofGraphData_cp, VprofGraphData_cp> wind_uv(VprofGraphData_cp wind_dd, VprofGraphData_cp wind_ff);

std::tuple<VprofGraphData_cp, VprofGraphData_cp> wind_dd_ff(VprofGraphData_cp wind_u, VprofGraphData_cp wind_v);

VprofGraphData_cp wind_sig(VprofGraphData_cp wind_ff);

float kindex(VprofGraphData_cp tt, VprofGraphData_cp td);

bool valid_content(VprofGraphData_cp data);

inline bool all_valid(VprofGraphData_cp s)
{
  return s && s->defined() == difield::ALL_DEFINED;
}

bool check_same_z(VprofGraphData_cp a, VprofGraphData_cp b);

} // namespace vprof

#endif // diVprofCalculations_h
