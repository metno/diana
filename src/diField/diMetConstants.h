/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2006 met.no

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
#ifndef _MetConstants_h
#define _MetConstants_h

#include <string>

namespace MetNo {
namespace Constants {

  /// Misc. constants and tables (meteorology, thermodynamics etc.)

  const float r = 287., cp = 1004., p0 = 1000., t0 = 273.15;
  const float eps = 0.622;
  const float xlh = 2.501e+6;
  const float rcp = r/cp, cplr = xlh/rcp, exl = eps*xlh;
  const float p0inv = 1./p0;
  const float kappa = r/cp;

  const float g = 9.8;
  const float ginv = 1./g;

  const float rhmin=0.02, rhmax=1.00;  // limits in computations

  const double ft_per_m = 3.2808399; // feet per meter

  // saturation pressure (water) for t=-100,-95,-90,...+100 deg.Celsius
  const int N_EWT = 41;
  const float ewt[N_EWT] = {
    .000034,.000089,.000220,.000517,.001155,.002472,
    .005080,.01005, .01921, .03553, .06356, .1111,
    .1891,  .3139,  .5088,  .8070,  1.2540, 1.9118,
    2.8627, 4.2148, 6.1078, 8.7192, 12.272, 17.044,
    23.373, 31.671, 42.430, 56.236, 73.777, 95.855,
    123.40, 157.46, 199.26, 250.16, 311.69, 385.56,
    473.67, 578.09, 701.13, 845.28, 1013.25
  };

  class ewt_calculator {
  public:
    ewt_calculator(float t_celsius)
      : x((t_celsius + 100.) * 0.2), l(int(x)) { }
    bool defined() const
      { return l>=0 and l<N_EWT-1; }
    bool defined(bool& allDefined, float undef, float& out) const
      { if (defined()) return true; allDefined = false; out = undef; return false; }
    float value() const
      { return ewt[l] + (ewt[l + 1] - ewt[l]) * (x - l); }
    float inverse(float et) const;
  private:
    float x;
    int l;
  };

  // connection between standard pressure (hPa) levels and FlightLevels (100 feet)
  const int   nLevelTable= 16;
  const float pLevelTable[nLevelTable]= {1000, 925, 850, 800, 700, 500, 400, 300,
                                          250, 200, 150, 100,  70,  50,  30,  10 };
  const float fLevelTable[nLevelTable]= {   5,  25,  50, 65, 100, 185, 235, 300,
                                          340, 385, 445, 530, 605, 675, 780, 1020 };
  //obsolete table, kept in order to support old input files
  const float fLevelTable_old[nLevelTable]= {   0,  25,  50, 70, 100, 180, 240, 300,
                                              340, 390, 450, 530, 600, 700, 800, 999 };

/*! Convert a pressure value to geopotential altitude according to the
 *  ICAO standard atmosphere.
 *
 * \param pressure pressure in hPa
 * \return geopotential altitude in m
 */
double ICAO_geo_altitude_from_pressure(double pressure);

/*! Convert a geopotential altitude value to a pressure according to
 *  the ICAO standard atmosphere. Should be the inverse of
 *  ICAO_geo_altitude_from_pressure.
 *
 * \param geopotential altitude in m
 * \return pressure in hPa
 */
double ICAO_pressure_from_geo_altitude(double altitude);

/*!
 * Convert from geopotential altitude to flight level, rounding to
 * 500ft.
 *
 * \param a geopotential altitude in m
 * \return flight level
 */
int FL_from_geo_altitude(double a);

/*!
 * Convert from flight level to geopotential altitude, without
 * rounding.
 *
 * \param flight level
 * \return a geopotential altitude in m
 */
double geo_altitude_from_FL(double fl);

  const std::string VerticalName[7] = { "none", "pressure", "hybrid",
      "atmospheric", "isentropic", "oceandepth", "other" };

} // namespace Constants
} // namespace MetNo

#endif
