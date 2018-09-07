/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018 met.no

  Contact information:
  Norwegian Meteorological Institute
  BoxFactory 43 Blindern
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

#ifndef VPROFBOXFACTORY_H
#define VPROFBOXFACTORY_H

#include "diVprofBox.h"

/*
 * VPROF_DIAGRAM z.min=10 z.max=1015 frame.colour=black
 *
 * VPROF_BOX type=significant_wind id=slwind separate=true graph.id=sigwind
 * VPROF_BOX type=fl id=fl flevels=....
 * VPROF_BOX type=pt id=pt t.min=-30 t.max=30
 * VPROF_BOX type=wind id=wind separate=true graph.id=wind
 * VPROF_BOX type=vertical_wind id=vwind graph.id=omega
 * VPROF_BOX type=line id=relhum x.min=0 x.max=100 x.label=RH
 * VPROF_BOX type=line id=ducting x.min=120 x.max=-120 x.label=DUCT
 *
 * VPROF_GRAPH type=line box=pt id=vp_air_temperature_celsius
 * VPROF_GRAPH type=line box=pt id=vp_dew_point_temperature_celsius style.line=stippled
 * VPROF_GRAPH type=line box=pt id=vp_cloudbase style.marker.type=dot style.marker.size=1 style.line=off
 * VPROF_GRAPH type=line box=relhum id=vp_relative_humidity
 * VPROF_GRAPH type=line box=ducting id=vp_relative_humidity
 *
 * VPROF_DATA model=meps-det refhour=0 style.colour=blue realization=all/selected/0 \-
 * TODO: graphs=sigwind,pt.vp_air_temperature_celsius,pt.vp_dew_point_temperature_celsius,wind,vertical_wind
 *
 * VPROF_DATA model=meps-det refhour=12 style.colour=blue realization=5
 *
 * VPROF_DATA model=ec refhour=0 style.colour=green realization=all/selected/0 \-
 * SETUP/COMPUTE graph_data=sigwind(dd_wind,ff_wind,sig_wind) \-
 *    graph_data=pt.vp_air_temperature_celsius(airtemp) \-
 *    graph_data=pt.vp_dew_point_temperature_celsius(rh) \-
 *    graph_data=wind(x_wind_ms,y_wind_ms) \-
 *    graph_data=vertical_wind.wind(vp_omega_pas)
 *
 * VPROF_DATA obs=bufr.temp style.colour=red
 */

namespace vprof {

VprofBox_p createBox(const miutil::KeyValue_v& config);

} // namespace vprof

#endif // VPROFBOXFACTORY_H
