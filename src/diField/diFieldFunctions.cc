/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2015 met.no

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diMetConstants.h"
#include "diFieldFunctions.h"
#include "diField.h"

#include <puTools/miString.h>

#include <cmath>
#include <iomanip>
#include <cfloat>

//#define DEBUGPRINT 1
#define ENABLE_FIELDFUNCTIONS_TIMING 1

using namespace miutil;
using namespace std;
using namespace MetNo::Constants;

#define MILOGGER_CATEGORY "diField.FieldFunctions"
#include "miLogger/miLogging.h"

// static data (setup)
vector<FieldFunctions::FieldCompute>          FieldFunctions::vFieldCompute;
map<FieldFunctions::VerticalType,map<std::string,vector<int> > > FieldFunctions::mFieldCompute;

std::map<FieldFunctions::Function, FieldFunctions::Function> FieldFunctions::functionMap;
vector<std::string> FieldFunctions::vFieldName;
map<std::string,int> FieldFunctions::mFieldName;

map<std::string,std::string> FieldFunctions::pLevel2flightLevel;
map<std::string,std::string> FieldFunctions::flightLevel2pLevel;

map< std::string, FieldFunctions::Zaxis_info > FieldFunctions::Zaxis_info_map;

namespace calculations {

inline bool is_defined(bool allDefined, float in1, float undef)
{
  return allDefined or in1 != undef;
}

inline bool is_defined(bool allDefined, float in1, float in2, float undef)
{
  return allDefined or (in1 != undef and in2 != undef);
}

inline bool is_defined(bool allDefined, float in1, float in2, float in3, float undef)
{
  return allDefined or (in1 != undef and in2 != undef and in3 != undef);
}

inline bool is_defined(bool allDefined, float in1, float in2, float in3, float in4, float undef)
{
  return allDefined or (in1 != undef and in2 != undef and in3 != undef and in4 != undef);
}

inline bool is_defined(bool allDefined, float in1, float in2, float in3, float in4, float in5, float undef)
{
  return allDefined or (in1 != undef and in2 != undef and in3 != undef and in4 != undef and in5 != undef);
}

inline bool is_defined(bool allDefined, float in1, float in2, float in3, float in4, float in5, float in6, float undef)
{
  return allDefined or (in1 != undef and in2 != undef and in3 != undef and in4 != undef and in5 != undef and in6 != undef);
}

inline float clamp_rh(float rh)
{
  if (rh < rhmin)
    return rhmin;
  else if (rh > rhmax)
    return rhmax;
  else
    return rh;
}

inline float t_thesat(float tk, float p, float pi, float undef, bool& allDefined)
{
  ewt_calculator ewt(tk-t0);
  if (not ewt.defined()) {
    allDefined = false;
    return undef;
  }
  const float qsat = eps * ewt.value() / p;
  return (cp * tk + xlh * qsat) / pi;
}

inline float th_thesat(float th, float p, float pi, float undef, bool& allDefined)
{
  ewt_calculator ewt(th * pi / cp - t0);
  if (not ewt.defined()) {
    allDefined = false;
    return undef;
  }
  const float qsat = eps * ewt.value() / p;
  return th + xlh * qsat / pi;
}

inline float tk_q_rh(float tk, float q, float p, float undef, bool& allDefined)
{
  ewt_calculator ewt(tk - t0);
  if (not ewt.defined()) {
    allDefined = false;
    return undef;
  }
  const float qsat = eps * ewt.value() / p;
  return 100. * q / qsat;
}

inline float tk_rh_q(float tk, float rh, float p, float undef, bool& allDefined)
{
  ewt_calculator ewt(tk - t0);
  if (not ewt.defined()) {
    allDefined = false;
    return undef;
  }
  const float qsat = eps * ewt.value() / p;
  return 0.01 * rh * qsat;
}

inline float tk_q_td(float tk, float q, float p, float tdconv, float undef, bool& allDefined)
{
  ewt_calculator ewt(tk - t0);
  if (not ewt.defined()) {
    allDefined = false;
    return undef;
  }

  const float et = ewt.value();
  const float qsat = eps * et / p;
  const float rh = calculations::clamp_rh(q / qsat);
  const float etd = rh * et;
  return ewt.inverse(etd) + tdconv;
}

inline float tk_rh_td(float tk, float rh100, float tdconv, float undef, bool& allDefined)
{
  ewt_calculator ewt(tk - t0);
  if (not ewt.defined()) {
    allDefined = false;
    return undef;
  }

  const float et = ewt.value();
  const float rh = calculations::clamp_rh(0.01 * rh100);
  const float etd = rh * et;
  return ewt.inverse(etd) + tdconv;
}

inline float tk_rh_the(float tk, float rh, float thconv, float undef, bool& allDefined)
{
  ewt_calculator ewt(tk - t0);
  if (not ewt.defined()) {
    allDefined = false;
    return undef;
  }

  return tk*thconv + ewt.value() * rh;
}

inline float tk_q_duct(float tk, float q, float p)
{
  return 77.6 * (p / tk) + 373000. * (q * p) / (eps * tk * tk);
}

inline float tk_rh_duct(float tk, float q, float p, float undef, bool& allDefined)
{
  ewt_calculator ewt(tk - t0);
  if (not ewt.defined()) {
    allDefined = false;
    return undef;
  }

  const float et = ewt.value();
  const float rh = clamp_rh(q * 0.01);
  return 77.6 * (p / tk) + 373000. * rh * et / (tk * tk);
}

float bad_hlevel(float a, float b)
{
  return (a < 0.0) || (b < 0.0) || (a == 0.0 && b == 0.0) || (b > 1.0);
}

inline float p_hlevel(float ps, float a, float b)
{ return a + b * ps; }

inline float pidcp_from_p(float p)
{ return powf(p * p0inv, kappa); }

inline float pi_from_p(float p)
{ return cp * pidcp_from_p(p); }

} // namespace calculations

FieldFunctions::FieldFunctions()
{
  // initialize function texts

  /*
  // simple computations - the functionTexts here are purely experimental - do not add!
  functionText[f_add_f_f] = "add(f1,f2)"; // field - field
  functionText[f_subtract_f_f] = "subtract(f1,f2)"; // field - field
  functionText[f_multiply_f_f] = "multiply(f1,f2)"; // field * field
  functionText[f_divide_f_f] = "divide(f1,f2)"; // field / field
  functionText[f_add_f_c] = "add(f,const:value)"; // field + constant
  functionText[f_subtract_f_c] = "subtract(f,const:value)"; // field - constant
  functionText[f_multiply_f_c] = "multiply(f,const:value)"; // field * constant
  functionText[f_divide_f_c] = "divide(f,const:value)"; // field / constant
  functionText[f_add_c_f] = "add(const:value,f)"; // constant + field
  functionText[f_subtract_c_f] = "subtract(const:value,f)"; // constant - field
  functionText[f_multiply_c_f] = "multiply(const:value,f)"; // constant * field
  functionText[f_divide_c_f] = "divide(const:value,f)"; // constant / field
   */

  functionText[f_sum_f] = "sum(field,...)";
  // pressure level (PLEVEL) functions
  functionText[f_tc_plevel_th] = "tc.plevel_th(th)";
  functionText[f_tk_plevel_th] = "tk.plevel_th(th)";
  functionText[f_th_plevel_tk] = "th.plevel_tk(tk)";
  functionText[f_thesat_plevel_tk] = "thesat.plevel_tk(tk)";
  functionText[f_thesat_plevel_th] = "thesat.plevel_th(th)";
  functionText[f_the_plevel_tk_rh] = "the.plevel_tk_rh(tk,rh)";
  functionText[f_the_plevel_th_rh] = "the.plevel_th_rh(th,rh)";
  functionText[f_rh_plevel_tk_q] = "rh.plevel_tk_q(tk,q)";
  functionText[f_rh_plevel_th_q] = "rh.plevel_th_q(th,q)";
  functionText[f_q_plevel_tk_rh] = "q.plevel_tk_rh(tk,rh)";
  functionText[f_q_plevel_th_rh] = "q.plevel_th_rh(th,rh)";
  functionText[f_tdc_plevel_tk_rh] = "tdc.plevel_tk_rh(tk,rh)";
  functionText[f_tdc_plevel_th_rh] = "tdc.plevel_th_rh(th,rh)";
  functionText[f_tdc_plevel_tk_q] = "tdc.plevel_tk_q(tk,q)";
  functionText[f_tdc_plevel_th_q] = "tdc.plevel_th_q(th,q)";
  functionText[f_tdk_plevel_tk_rh] = "tdk.plevel_tk_rh(tk,rh)";
  functionText[f_tdk_plevel_th_rh] = "tdk.plevel_th_rh(th,rh)";
  functionText[f_tdk_plevel_tk_q] = "tdk.plevel_tk_q(tk,q)";
  functionText[f_tdk_plevel_th_q] = "tdk.plevel_th_q(th,q)";
  functionText[f_tcmean_plevel_z1_z2] = "tcmean.plevel_z1_z2(z1,z2)";
  functionText[f_tkmean_plevel_z1_z2] = "tkmean.plevel_z1_z2(z1,z2)";
  functionText[f_thmean_plevel_z1_z2] = "thmean.plevel_z1_z2(z1,z2)";
  functionText[f_qvector_plevel_z_tk_xcomp] = "qvector.plevel_z_tk_xcomp(z,tk)";
  functionText[f_qvector_plevel_z_th_xcomp] = "qvector.plevel_z_th_xcomp(z,th)";
  functionText[f_qvector_plevel_z_tk_ycomp] = "qvector.plevel_z_tk_ycomp(z,tk)";
  functionText[f_qvector_plevel_z_th_ycomp] = "qvector.plevel_z_th_ycomp(z,th)";
  functionText[f_ducting_plevel_tk_q] = "ducting.plevel_tk_q(tk,q)";
  functionText[f_ducting_plevel_th_q] = "ducting.plevel_th_q(th,q)";
  functionText[f_ducting_plevel_tk_rh] = "ducting.plevel_tk_rh(tk,rh)";
  functionText[f_ducting_plevel_th_rh] = "ducting.plevel_th_rh(th,rh)";
  functionText[f_geostrophic_wind_plevel_z_xcomp] = "geostrophic.wind.plevel_z_xcomp(z)";
  functionText[f_geostrophic_wind_plevel_z_ycomp] = "geostrophic.wind.plevel_z_ycomp(z)";
  functionText[f_geostrophic_vorticity_plevel_z] = "geostrophic.vorticity.plevel_z(z)";
  functionText[f_kindex_plevel_tk_rh] = "kindex.plevel_tk_rh(tk500,tk700,rh700,tk850,rh850)";
  functionText[f_kindex_plevel_th_rh] = "kindex.plevel_th_rh(th500,th700,rh700,th850,rh850)";
  functionText[f_ductingindex_plevel_tk_rh] = "ductingindex.plevel_tk_rh(tk850,rh850)";
  functionText[f_ductingindex_plevel_th_rh] = "ductingindex.plevel_th_rh(th850,rh850)";
  functionText[f_showalterindex_plevel_tk_rh] = "showalterindex.plevel_tk_rh(tk500,tk850,rh850)";
  functionText[f_showalterindex_plevel_th_rh] = "showalterindex.plevel_th_rh(th500,th850,rh850)";
  functionText[f_boydenindex_plevel_tk_z] = "boydenindex.plevel_tk_z(tk700,z700,z1000)";
  functionText[f_boydenindex_plevel_th_z] = "boydenindex.plevel_th_z(th700,z700,z1000)";

  // hybrid model level (HLEVEL) functions
  functionText[f_tc_hlevel_th_psurf] = "tc.hlevel_th_psurf(th,psurf)";
  functionText[f_tk_hlevel_th_psurf] = "tk.hlevel_th_psurf(th,psurf)";
  functionText[f_th_hlevel_tk_psurf] = "th.hlevel_tk_psurf(tk,psurf)";
  functionText[f_thesat_hlevel_tk_psurf] = "thesat.hlevel_tk_psurf(tk,psurf)";
  functionText[f_thesat_hlevel_th_psurf] = "thesat.hlevel_th_psurf(th,psurf)";
  functionText[f_the_hlevel_tk_q_psurf] = "the.hlevel_tk_q_psurf(tk,q,psurf)";
  functionText[f_the_hlevel_th_q_psurf] = "the.hlevel_th_q_psurf(th,q,psurf)";
  functionText[f_rh_hlevel_tk_q_psurf] = "rh.hlevel_tk_q_psurf(tk,q,psurf)";
  functionText[f_rh_hlevel_th_q_psurf] = "rh.hlevel_th_q_psurf(th,q,psurf)";
  functionText[f_q_hlevel_tk_rh_psurf] = "q.hlevel_tk_rh_psurf(tk,rh,psurf)";
  functionText[f_q_hlevel_th_rh_psurf] = "q.hlevel_th_rh_psurf(th,rh,psurf)";
  functionText[f_tdc_hlevel_tk_q_psurf] = "tdc.hlevel_tk_q_psurf(tk,q,psurf)";
  functionText[f_tdc_hlevel_th_q_psurf] = "tdc.hlevel_th_q_psurf(th,q,psurf)";
  functionText[f_tdc_hlevel_tk_rh_psurf] = "tdc.hlevel_tk_rh_psurf(tk,rh,psurf)";
  functionText[f_tdc_hlevel_th_rh_psurf] = "tdc.hlevel_th_rh_psurf(th,rh,psurf)";
  functionText[f_tdk_hlevel_tk_q_psurf] = "tdk.hlevel_tk_q_psurf(tk,q,psurf)";
  functionText[f_tdk_hlevel_th_q_psurf] = "tdk.hlevel_th_q_psurf(th,q,psurf)";
  functionText[f_tdk_hlevel_tk_rh_psurf] = "tdk.hlevel_tk_rh_psurf(tk,rh,psurf)";
  functionText[f_tdk_hlevel_th_rh_psurf] = "tdk.hlevel_th_rh_psurf(th,rh,psurf)";
  functionText[f_ducting_hlevel_tk_q_psurf] = "ducting.hlevel_tk_q_psurf(tk,q,psurf)";
  functionText[f_ducting_hlevel_th_q_psurf] = "ducting.hlevel_th_q_psurf(th,q,psurf)";
  functionText[f_ducting_hlevel_tk_rh_psurf] = "ducting.hlevel_tk_rh_psurf(tk,rh,psurf)";
  functionText[f_ducting_hlevel_th_rh_psurf] = "ducting.hlevel_th_rh_psurf(th,rh,psurf)";
  functionText[f_pressure_hlevel_xx_psurf] = "pressure.hlevel_xx_psurf(xx,psurf)"; // just get eta.a and eta.b from field xx

  // misc atmospheric model level (ALEVEL) functions
  functionText[f_tc_alevel_th_p] = "tc.alevel_th_p(th,p)";
  functionText[f_tk_alevel_th_p] = "tk.alevel_th_p(th,p)";
  functionText[f_th_alevel_tk_p] = "th.alevel_tk_p(tk,p)";
  functionText[f_thesat_alevel_tk_p] = "thesat.alevel_tk_p(tk,p)";
  functionText[f_thesat_alevel_th_p] = "thesat.alevel_th_p(th,p)";
  functionText[f_the_alevel_tk_q_p] = "the.alevel_tk_q_p(tk,q,p)";
  functionText[f_the_alevel_th_q_p] = "the.alevel_th_q_p(th,q,p)";
  functionText[f_rh_alevel_tk_q_p] = "rh.alevel_tk_q_p(tk,q,p)";
  functionText[f_rh_alevel_th_q_p] = "rh.alevel_th_q_p(th,q,p)";
  functionText[f_q_alevel_tk_rh_p] = "q.alevel_tk_rh_p(tk,rh,p)";
  functionText[f_q_alevel_th_rh_p] = "q.alevel_th_rh_p(th,rh,p)";
  functionText[f_tdc_alevel_tk_q_p] = "tdc.alevel_tk_q_p(tk,q,p)";
  functionText[f_tdc_alevel_th_q_p] = "tdc.alevel_th_q_p(th,q,p)";
  functionText[f_tdc_alevel_tk_rh_p] = "tdc.alevel_tk_rh_p(tk,rh,p)";
  functionText[f_tdc_alevel_th_rh_p] = "tdc.alevel_th_rh_p(th,rh,p)";
  functionText[f_tdk_alevel_tk_q_p] = "tdk.alevel_tk_q_p(tk,q,p)";
  functionText[f_tdk_alevel_th_q_p] = "tdk.alevel_th_q_p(th,q,p)";
  functionText[f_tdk_alevel_tk_rh_p] = "tdk.alevel_tk_rh_p(tk,rh,p)";
  functionText[f_tdk_alevel_th_rh_p] = "tdk.alevel_th_rh_p(th,rh,p)";
  functionText[f_ducting_alevel_tk_q_p] = "ducting.alevel_tk_q_p(tk,q,p)";
  functionText[f_ducting_alevel_th_q_p] = "ducting.alevel_th_q_p(th,q,p)";
  functionText[f_ducting_alevel_tk_rh_p] = "ducting.alevel_tk_rh_p(tk,rh,p)";
  functionText[f_ducting_alevel_th_rh_p] = "ducting.alevel_th_rh_p(th,rh,p)";

  // isentropic level (ILEVEL) function NB! functions with two output fields do not work (TODO)
  functionText[f_geostrophic_wind_ilevel_mpot] = "geostrophic_wind.ilevel_mpot(mpot)2";

  // ocean depth level (OZLEVEL) functions
  functionText[f_sea_soundspeed_ozlevel_tc_salt] = "sea.soundspeed.ozlevel_tc_salt(seatemp.c,salt)";
  functionText[f_sea_soundspeed_ozlevel_tk_salt] = "sea.soundspeed.ozlevel_tk_salt(seatemp.k,salt)";

  // level independent functions
  functionText[f_temp_k2c] = "temp_k2c(tk)";
  functionText[f_temp_c2k] = "temp_c2k(tc)";
  functionText[f_temp_k2c_possibly] = "temp_k2c_possibly(tk)";
  functionText[f_temp_c2k_possibly] = "temp_c2k_possibly(tc)";
  functionText[f_tdk_tk_rh] = "tdk.tk_rh(tk,rh)";
  functionText[f_tdc_tk_rh] = "tdc.tk_rh(tk,rh)";
  functionText[f_tdc_tc_rh] = "tdc.tc_rh(tc,rh)";
  functionText[f_rh_tk_td] = "rh.tk_tdk(tk,tdk)";
  functionText[f_rh_tc_td] = "rh.tc_tdc(tc,tdc)";
  functionText[f_vector_abs] = "vector.abs(u,v)";
  functionText[f_direction] = "direction(u,v)";
  functionText[f_rel_vorticity] = "rel.vorticity(u,v)";
  functionText[f_abs_vorticity] = "abs.vorticity(u,v)";
  functionText[f_divergence] = "divergence(u,v)";
  functionText[f_advection] = "advection(f,u,v,const:hours)";
  functionText[f_d_dx] = "d/dx(f)";
  functionText[f_d_dy] = "d/dy(f)";
  functionText[f_abs_del] = "abs.del(f)";
  functionText[f_del_square] = "del.square(f)";
  functionText[f_minvalue_fields] = "minvalue.fields(f1,f2)";
  functionText[f_maxvalue_fields] = "maxvalue.fields(f1,f2)";
  functionText[f_minvalue_field_const] = "minvalue.field.const(f,const:value)";
  functionText[f_maxvalue_field_const] = "maxvalue.field.const(f,const:value)";
  functionText[f_abs] = "abs(f)";
  functionText[f_log10] = "log10(f)";
  functionText[f_pow10] = "pow10(f)";
  functionText[f_log] = "log(f)";
  functionText[f_exp] = "exp(f)";
  functionText[f_power] = "power(f,const:exponent)";
  functionText[f_shapiro2_filter] = "shapiro2.filter(f)";
  functionText[f_smooth] = "smooth(f,const:numsmooth)";
  functionText[f_windcooling_tk_u_v] = "windcooling_tk_u_v(tk2m,u10m,v10m)";
  functionText[f_windcooling_tc_u_v] = "windcooling_tc_u_v(tc2m,u10m,v10m)";
  functionText[f_undercooled_rain] = "undercooled.rain(precip,snow,tk,const:precipMin,const:snowRateMax,const:tcMax)";
  functionText[f_thermal_front_parameter_tx] = "thermal.front.parameter_tx(tx)";
  functionText[f_pressure2flightlevel] = "pressure2flightlevel(p)";
  functionText[f_momentum_x_coordinate] = "momentum.x.coordinate(v,const:coriolisMin)";
  functionText[f_momentum_y_coordinate] = "momentum.y.coordinate(u,const:coriolisMin)";
  functionText[f_jacobian] = "jacobian(fx,fy)";
  functionText[f_vessel_icing_overland] = "vessel.icing.overland(airtemp,seatemp,u10m,v10m,const:freezingPoint)";
  functionText[f_vessel_icing_mertins] = "vessel.icing.mertins(airtemp,seatemp,u10m,v10m,const:freezingPoint)";
  functionText[f_replace_undefined] = "replace.undefined(f,const:value)";
  functionText[f_replace_defined] = "replace.defined(f,const:value)";
  functionText[f_replace_all] = "replace.all(f,const:value)";
  functionText[f_values2classes] = "values2classes(f,const:limits_low_to_high,...)";
  functionText[f_field_diff_forecast_hour] = "field.diff.forecast.hour(field,const:relHourFirst,const:relHourLast)";
  functionText[f_accum_diff_forecast_hour] = "accum.diff.forecast.hour(accumfield,const:relHourFirst,const:relHourLast)";
  functionText[f_sum_of_forecast_hours] = "sum_of_forecast_hours(field,const:forecastHours,...)";
  functionText[f_sum_of_fields] = "sum_of_fields(field)";
  functionText[f_max_of_fields] = "max_of_fields(field)";
  functionText[f_min_of_fields] = "min_of_fields(field)";
  functionText[f_no_of_fields_above] = "no_of_fields_above(field,const:limit)";
  functionText[f_no_of_fields_below] = "no_of_fields_below(field,const:limit)";
  functionText[f_index_of_fields_max] = "index_of_fields_max(field)";
  functionText[f_index_of_fields_min] = "index_of_fields_min(field)";
  functionText[f_mean_value] = "mean_value(field,...)";
  functionText[f_stddev] = "stddev(field)";
  functionText[f_probability_above] = "probability_above(field,const:limit)";
  functionText[f_probability_below] = "probability_below(field,const:limit)";
  functionText[f_probability_between] = "probability_betwwen(field,const:limit,const:limit)";
  functionText[f_number_above] = "number_above(field,const:limit)";
  functionText[f_number_below] = "number_below(field,const:limit)";
  functionText[f_number_between] = "number_betwwen(field,const:limit,const:limit)";
  functionText[f_equivalent_to] = "equivalent_to(field)";
  functionText[f_min_value] = "min_value(field,...)";
  functionText[f_max_value] = "max_value(field,...)";
  functionText[f_min_index] = "min_index(field,...)";
  functionText[f_max_index] = "max_index(field,...)";
  functionText[f_percentile] = "percentile(field,const:value,...)";
  functionText[f_snow_cm_from_snow_water_tk_td] = "snow.cm.from.snow.water(snow,tk,td)";

  functionMap[f_field_diff_forecast_hour] = f_subtract_f_f;
  functionMap[f_accum_diff_forecast_hour] = f_subtract_f_f;
  functionMap[f_sum_of_forecast_hours] = f_sum_f;
  functionMap[f_sum_of_fields] = f_sum_f;
  functionMap[f_max_of_fields] = f_max_value;
  functionMap[f_min_of_fields] = f_min_value;
  functionMap[f_no_of_fields_above] = f_number_above;
  functionMap[f_no_of_fields_below] = f_number_below;
  functionMap[f_index_of_fields_max] = f_max_index;
  functionMap[f_index_of_fields_min] = f_min_index;
}

std::string FieldFunctions::FIELD_COMPUTE_SECTION() { return "FIELD_COMPUTE"; }
std::string FieldFunctions::FIELD_VERTICAL_COORDINATES_SECTION() { return "FIELD_VERTICAL_COORDINATES"; }


struct FunctionHelper {
  FieldFunctions::Function func;
  int numfields;
  int numconsts;
  unsigned int numresult;
  FieldFunctions::VerticalType vertcoord;
  FunctionHelper() :
    func(FieldFunctions::f_undefined), numfields(0), numconsts(0),
    numresult(1), vertcoord(FieldFunctions::vctype_none)
  {
  }
};


// static member
bool FieldFunctions::parseComputeSetup(const vector<std::string>& lines,
    vector<std::string>& errors)
{
  METLIBS_LOG_SCOPE();
  bool ok = true;

  // field functions (not setup input)

  FieldFunctions ffunc;
  std::map<FieldFunctions::Function, std::string> functionTexts =
      ffunc.getFunctionTexts();
  std::map<FieldFunctions::Function, std::string>::iterator fitr;
  map<std::string, FunctionHelper> functions;

  std::string str, functionText, functionName;
  vector<std::string> vstr;
  size_t p1, p2;

  /*
   * Loop over all functions with defined function texts
   * Parse text and register functions in a FunctionHelper struct
   */
  for (fitr = functionTexts.begin(); fitr != functionTexts.end(); ++fitr) {
    bool err = true;
    functionText = fitr->second;
    functionText = functionText;
    p1 = functionText.find('(');
    p2 = functionText.find(')');
    // The function text should contain a variable definition inside a '(' ')' pair
    if (p1 != string::npos && p2 != string::npos && p1 > 0 && p1 < p2 - 1) {
      // the name of the function (must be matched in setup)
      functionName = functionText.substr(0, p1);
      // parse the arguments
      str = functionText.substr(p1 + 1, p2 - p1 - 1);
      vstr = miutil::split(str, 0, ",");
      // set the vertical component type
      VerticalType vctype = vctype_none;
      if (miutil::contains(functionName, ".plevel_"))
        vctype = vctype_pressure;
      else if (miutil::contains(functionName, ".hlevel_"))
        vctype = vctype_hybrid;
      else if (miutil::contains(functionName, ".alevel_"))
        vctype = vctype_atmospheric;
      else if (miutil::contains(functionName, ".ilevel_"))
        vctype = vctype_isentropic;
      else if (miutil::contains(functionName, ".ozlevel_"))
        vctype = vctype_oceandepth;
      int nfields = 0;
      int nconsts = 0;
      int nresult = 1;
      unsigned int i = 0;
      while (i < vstr.size() && (vstr[i].length() < 6 || vstr[i].substr(0, 6)
          != "const:"))
        i++;
      if (i == vstr.size()) {
        nfields = vstr.size();
        if (nfields > 0 && vstr[i-1].find_first_not_of('.') == string::npos) {
          nfields = -1;
        }
      } else {
        nfields = i;
        if (nfields > 0 && vstr[i-1].find_first_not_of('.') == string::npos) {
          METLIBS_LOG_ERROR("Error while parsing functions defined in FieldFunctions:"
              "Functions with both fields and contants cannot have a variable numder of fields: " << functionText);
          ok = false;
        }
        nconsts = vstr.size() - nfields;
        i = vstr.size() - 1;
        if (vstr[i].find_first_not_of('.') == string::npos)
          nconsts = -(nconsts - 1);
      }
      if (p2 + 1 < functionText.length()) {
        str = functionText.substr(p2 + 1);
        if (miutil::is_int(str)) {
          nresult = atoi(str.c_str());
          if (nresult > 0)
            err = false;
        }
      } else {
        err = false;
      }
      if (!err) {
        FunctionHelper fh;
        fh.func = fitr->first;
        fh.numfields = nfields;
        fh.numconsts = nconsts;
        fh.numresult = nresult;
        fh.vertcoord = vctype;
        functions[functionName] = fh;
      }
    }
    if (err) {
      METLIBS_LOG_ERROR("Bad FunctionName " << functionText);
      ok = false;
    }
  }

  if (!ok)
    return false;

  // parse setup

  map<std::string, int> compute;
  compute["add"] = 0;
  compute["subtract"] = 1;
  compute["multiply"] = 2;
  compute["divide"] = 3;
  map<std::string, int>::const_iterator pc, pcend = compute.end();

  map<std::string, FunctionHelper>::const_iterator pf, pfend = functions.end();

  std::string v1, v2, oneline;
  vector<std::string> vspec, vpart;
  bool b0, b1;

  int nlines = lines.size();

  for (int l = 0; l < nlines; l++) {
    oneline = lines[l];
    vstr = miutil::split(oneline, 0, " ", true);
    int n = vstr.size();
    for (int i = 0; i < n; i++) {
      bool err = true;
      vspec = miutil::split(vstr[i], 1, "=", false);
      if (vspec.size() == 2) {
        p1 = vspec[1].find('(');
        p2 = vspec[1].find(')');
        if (p1 != string::npos && p2 != string::npos && p1 > 0 && p1 < p2 - 1) {
          functionName = vspec[1].substr(0, p1);
          str = vspec[1].substr(p1 + 1, p2 - p1 - 1);
          FieldCompute fcomp;
          fcomp.name = vspec[0];
          fcomp.functionName = miutil::to_upper(functionName);
          VerticalType vctype = vctype_none;

          // First check if function is a simple calculation
          if ((pc = compute.find(functionName)) != pcend) {
            fcomp.results.push_back(fcomp.name);
            vpart = miutil::split(str, 1, ",");
            if (vpart.size() == 2) {
              b0 = miutil::is_number(vpart[0]);
              b1 = miutil::is_number(vpart[1]);
              if (!b0 && !b1) {
                fcomp.function = FieldFunctions::Function(
                    FieldFunctions::f_add_f_f + pc->second);
                fcomp.input.push_back(vpart[0]);
                fcomp.input.push_back(vpart[1]);
                err = false;
              } else if (!b0 && b1) {
                fcomp.function = FieldFunctions::Function(
                    FieldFunctions::f_add_f_c + pc->second);
                fcomp.input.push_back(vpart[0]);
                fcomp.constants.push_back(atof(vpart[1].c_str()));
                err = false;
              } else if (b0 && !b1) {
                fcomp.function = FieldFunctions::Function(
                    FieldFunctions::f_add_c_f + pc->second);
                fcomp.input.push_back(vpart[1]);
                fcomp.constants.push_back(atof(vpart[0].c_str()));
                err = false;
              }
            }

            // then check if function is defined in the FunctionHelper container
          } else if ((pf = functions.find(miutil::to_lower(functionName))) != pfend) {
            fcomp.function = pf->second.func;
            // check function arguments
            vpart = miutil::split(str, 0, ",", false);
            int npart = vpart.size();
            int numf = pf->second.numfields;
            int numc = pf->second.numconsts;
            if (numf < 0 ) {
              numf = npart;
            }
            unsigned int numr = pf->second.numresult;
            vctype = pf->second.vertcoord;
            // if any fields or constants as input...
            if ((numc >= 0 && npart == numf + numc) || (numc < 0 && npart
                >= numf - numc)) {
              err = false;
              // extract field names
              for (int i = 0; i < numf; i++) {
                if (not miutil::is_number(vpart[i]))
                  fcomp.input.push_back(vpart[i]);
                else
                  err = true;
              }
              // extract constants
              for (int i = numf; i < npart; i++) {
                if (miutil::is_number(vpart[i]))
                  fcomp.constants.push_back(atof(vpart[i].c_str()));
                else
                  err = true;
              }
            }
            // extract names of output fields
            fcomp.results = miutil::split(fcomp.name, 0, ",", false);
            if (numr != fcomp.results.size())
              err = true;
          }

          if (!err) {
            // add one FieldCompute for each result field
            for (unsigned int i = 0; i < fcomp.results.size(); i++) {
              fcomp.name = fcomp.results[i];
              fcomp.vctype = vctype;
              mFieldCompute[vctype][fcomp.name].push_back(vFieldCompute.size());
              vFieldCompute.push_back(fcomp);
            }
          }
        }
      }
      if (err) {
        std::string errm = FIELD_COMPUTE_SECTION() + "|" + miutil::from_number(l)
        + "|Error in field compute : " + vstr[i];
        errors.push_back(errm);
        ok = true;
      }
    }
  }

#ifdef DEBUGPRINT
  cerr<<endl;
  cerr<<"-------------- "<<FIELD_COMPUTE_SECTION()<<" (1) ------------------"<<endl;
  cerr<<endl;
  map<VerticalType,map<std::string,vector<int> > >::iterator pg= mFieldCompute.begin(),
      pgend= mFieldCompute.end();
  map<std::string,vector<int> >::iterator pfc,pfcend;
  cerr.setf(ios::left);
  for (; pg!=pgend; pg++) {
    pfc= pg->second.begin();
    pfcend= pg->second.end();
    for (; pfc!=pfcend; pfc++) {
      for (size_t i=0; i<pfc->second.size(); i++) {
        int n= pfc->second[i];
        cerr<<setw(18)<<VerticalName[pg->first]<<" "
            <<setw(18)<<vFieldCompute[n].name<<" "
            <<setw(18)<<vFieldCompute[n].functionName<<" input:";
        for (size_t j=0; j<vFieldCompute[n].input.size(); j++)
          cerr<<" "<<vFieldCompute[n].input[j];
        if (vFieldCompute[n].results.size()>1) {
          cerr<<" res:";
          for (size_t j=0; j<vFieldCompute[n].results.size(); j++)
            cerr<<" "<<vFieldCompute[n].results[j];
        }
        if (!vFieldCompute[n].constants.empty()) {
          cerr<<" const:";
          for (size_t j=0; j<vFieldCompute[n].constants.size(); j++)
            cerr<<" "<<vFieldCompute[n].constants[j];
        }
        cerr<<endl;
      }
    }
  }
  cerr.unsetf(ios::left);
  cerr<<endl;
  cerr<<"-------------- "<< FIELD_COMPUTE_SECTION()<<" (2) ------------------"<<endl;
  cerr<<endl;
  for (size_t n=0; n<vFieldCompute.size(); n++) {
    cerr<<setw(4)<<n<<": "<<setw(16)<<vFieldCompute[n].name
        <<setw(18)<<vFieldCompute[n].functionName<<"  input:";
    for (size_t j=0; j<vFieldCompute[n].input.size(); j++)
      cerr<<"  "<<vFieldCompute[n].input[j];
    cerr<<endl;
  }

  cerr<<endl;
  pg= mFieldCompute.begin();
  for (; pg!=pgend; pg++) {
    cerr<<setw(18)<<pg->first<<" size= "<<pg->second.size()<<endl;
  }
  cerr<<endl;
  cerr<<"----------------------------------------------"<<endl;
  cerr<<endl;
#endif

  for (size_t n = 0; n < vFieldCompute.size(); n++) {
    if (!mFieldName.count(vFieldCompute[n].name)) {
      mFieldName[vFieldCompute[n].name] = vFieldName.size();
      vFieldName.push_back(vFieldCompute[n].name);
    }
  }

  buildPLevelsToFlightLevelsTable();

  return ok;
}

bool FieldFunctions::parseVerticalSetup(const std::vector<std::string>& lines,
    std::vector<std::string>& errors)
{
  const std::string key_name = "name";
  const std::string key_vc_type = "vc_type";
  const std::string key_levelprefix = "levelprefix";
  const std::string key_levelsuffix = "levelsuffix";
  const std::string key_index = "index";

  int nlines = lines.size();

  for (int l = 0; l < nlines; l++) {
    Zaxis_info zaxis_info;
    vector<std::string> tokens= miutil::split_protected(lines[l], '"','"');
    for (size_t  i = 0; i < tokens.size(); i++) {
      vector<std::string> stokens= miutil::split_protected(tokens[i], '"','"',"=",true);
      if (stokens.size() == 2 )  {
        if( stokens[0] == key_name ) {
          zaxis_info.name = stokens[1];
        } else  if( stokens[0] == key_vc_type ) {
          if ( stokens[1] == "none" ) {
            zaxis_info.vctype = FieldFunctions::vctype_none;
          } else if ( stokens[1] == "pressure") {
            zaxis_info.vctype = FieldFunctions::vctype_pressure;
          } else if ( stokens[1] == "hybrid") {
            zaxis_info.vctype = FieldFunctions::vctype_hybrid;
          } else if ( stokens[1] == "atmospheric") {
            zaxis_info.vctype = FieldFunctions::vctype_atmospheric;
          } else if ( stokens[1] == "isentropic") {
            zaxis_info.vctype = FieldFunctions::vctype_isentropic;
          } else if ( stokens[1] == "oceandepth") {
            zaxis_info.vctype = FieldFunctions::vctype_oceandepth;
          } else if ( stokens[1] == "other") {
            zaxis_info.vctype = FieldFunctions::vctype_other;
          }
        } else  if( stokens[0] == key_levelprefix ) {
          zaxis_info.levelprefix = stokens[1];
        } else  if( stokens[0] == key_levelsuffix ) {
          zaxis_info.levelsuffix = stokens[1];
        } else  if( stokens[0] == key_index ) {
          zaxis_info.index = (stokens[1]=="true");
        }
      }
    }
    Zaxis_info_map[zaxis_info.name] = zaxis_info;
  }

  return true;
}

// static member
bool FieldFunctions::splitFieldSpecs(const std::string& paramName,FieldSpec& fs)
{

  fs.use_standard_name = false;
  fs.paramName = paramName;
  if (paramName.find(':')==string::npos)
    return false;

  bool levelSpecified= false;
  vector<std::string> vs= miutil::split(paramName, 0, ":");
  fs.paramName= vs[0];
  vector<std::string> vp;
  for (size_t i=1; i<vs.size(); i++) {
    vp= miutil::split(vs[i], 0, "=");
    if ( vp[0]=="level" ) {
      if ( vp.size()==2 ) {
        fs.levelName= vp[1];
        levelSpecified = true;
      } else {
        fs.levelName.clear();
      }
    } else if ( vp[0]=="vcoord" ) {
      if ( vp.size()==2 ) {
        fs.vcoordName= miutil::to_lower(vp[1]);
      } else {
        fs.vcoordName.clear();
      }
    } else if ( vp.size()==2 && vp[0]=="ecoord" ) {
      fs.ecoordName= vp[1];
    } else if (vp.size()==2 && (vp[0]=="unit" || vp[0]=="units") ) {
      fs.unit =  vp[1];
    } else if (vp.size()==2 && vp[0]=="elevel") {
      fs.elevel =  vp[1];
    } else if (vp.size()==1 && vp[0]=="standard_name") {
      fs.use_standard_name = true;
    } else if (vp.size()==2 && (vp[0]=="fchour") ) {
      fs.fcHour =  vp[1];
    } else if (vp.size()==1 ) {
      fs.option =  vp[0];
    }
  }

  return levelSpecified;
}

// static
void FieldFunctions::buildPLevelsToFlightLevelsTable() {
  pLevel2flightLevel.clear();
  flightLevel2pLevel.clear();

  // make conversion tables between pressureLevels and flightLevels
  for (int i=0; i<nLevelTable; i++) {
    ostringstream pstr, fstr, fstr_old;
    pstr<<pLevelTable[i];
    fstr<<setw(3)<<setfill('0')<<fLevelTable[i];
    fstr_old<<setw(3)<<setfill('0')<<fLevelTable_old[i];
    pLevel2flightLevel[std::string(pstr.str()+"hPa")]= std::string("FL"+fstr.str());
    flightLevel2pLevel[std::string("FL"+fstr.str()) ]= std::string(pstr.str()+"hPa");
    //obsolete table
    flightLevel2pLevel[std::string("FL"+fstr_old.str()) ]= std::string(pstr.str()+"hPa");
  }
}

// static
std::string FieldFunctions::getPressureLevel(const std::string& flightlevel)
{
  METLIBS_LOG_SCOPE(LOGVAL(flightlevel));

  if ( flightLevel2pLevel.count(flightlevel))
    return flightLevel2pLevel[flightlevel];

  METLIBS_LOG_WARN(" Flightlevel: "<<flightlevel<<". No pressure level found." );
  return flightlevel;
}

// static
std::string FieldFunctions::getFlightLevel(const std::string& pressurelevel)
{
  METLIBS_LOG_SCOPE(LOGVAL(pressurelevel));

  if ( pLevel2flightLevel.count(pressurelevel))
    return pLevel2flightLevel[pressurelevel];

  METLIBS_LOG_WARN(" pressurelevel: "<<pressurelevel<<". No pressure level found." );
  return pressurelevel;
}

void FieldFunctions::setFieldNames(const vector<std::string>& vfieldname)
{
  vFieldName = vfieldname;
  for(size_t i=0;i<vfieldname.size();i++) {
    mFieldName[vfieldname[i]]=i;
  }
}

const std::string FieldFunctions::getFunctionText(Function f) const
{
  std::map<Function, std::string>::const_iterator i = functionText.find(f);
  if (i != functionText.end())
    return i->second;
  return std::string();
}

bool FieldFunctions::fieldComputer(Function function,
    const std::vector<float>& constants, const std::vector<Field*>& vfinput,
    const std::vector<Field*>& vfres, GridConverter & gc)
{
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  // Perform field computation according to 'function'.
  // All fields must have the same dimensions (nx and ny).
  // All "horizontal" computations assuming nonstaggered fields (A-grid).

#ifdef DEBUGPRINT
  cerr << "FieldFunctions::fieldComputer: " << "fieldName " << fieldName;
  cerr << " FUNCTION  ";
  std::string ftext = getFunctionText(function);
  if (ftext != "") {
    int l = ftext.find('(');
    cerr << ftext.substr(0, l);
  } else {
    cerr << "compute(" << miString(int(function)) << ")";
  }
  if (!constants.empty()) {
    cerr << " const:";
    for (unsigned int i = 0; i < constants.size(); i++)
      cerr << " " << constants[i];
  }
  cerr << endl;
#endif

  if (vfinput.empty() || vfres.empty()) {
    return false;
  }

  int nx = vfinput[0]->area.nx;
  int ny = vfinput[0]->area.ny;
  int fsize = nx * ny;

  vector<float*> finp;
  vector<float*> fout;

  bool allDefined = true;

  bool ok = true;

  int ninp = vfinput.size();
  int nout = vfres.size();
  int nconst = constants.size();

  std::string unit;

  for (int i = 0; i < ninp; i++) {
    finp.push_back(vfinput[i]->data);
    if (!vfinput[i]->allDefined)
      allDefined = false;
    if (vfinput[i]->area.nx != nx || vfinput[i]->area.ny != ny)
      ok = false;
  }

  for (int i = 0; i < nout; i++) {
    fout.push_back(vfres[i]->data);
    vfres[i]->allDefined = allDefined;
    unit = vfres[i]->unit;
    if (vfres[i]->area.nx != nx || vfres[i]->area.ny != ny)
      ok = false;
  }

  if (!ok) {
    METLIBS_LOG_ERROR("Field dimensions not equal");
    return false;
  }

  const float undef = fieldUndef;

  int nsmooth;
  const float *xmapr = 0, *ymapr = 0, *fcoriolis = 0;
  float plevel, plevel1, plevel2, plevel3, alevel, blevel, odepth;
  float hours, constant, precipMin, snowRateMax, tcMax, fcoriolisMin;

  int compute = 0;

  bool res = false;

  // The large function-switch
  switch (function) {

  //---------------------------------------------------
  // pressure level (PLEVEL) functions
  //---------------------------------------------------

  case f_tc_plevel_th:
    if (compute == 0)
      compute = 1;
  case f_tk_plevel_th:
    if (compute == 0)
      compute = 2;
  case f_th_plevel_tk:
    if (compute == 0)
      compute = 3;
  case f_thesat_plevel_tk:
    if (compute == 0)
      compute = 4;
  case f_thesat_plevel_th:
    if (compute == 0)
      compute = 5;
    if (ninp != 1 || nout != 1)
      break;
    plevel = vfinput[0]->level;
    res = pleveltemp(compute, nx, ny, finp[0], fout[0], plevel, allDefined,
        undef,unit);
    break;

  case f_the_plevel_tk_rh:
    if (compute == 0)
      compute = 1;
  case f_the_plevel_th_rh:
    if (compute == 0)
      compute = 2;
    if (ninp != 2 || nout != 1)
      break;
    plevel = vfinput[0]->level;
    res = plevelthe(compute, nx, ny, finp[0], finp[1], fout[0], plevel,
        allDefined, undef);
    break;

  case f_rh_plevel_tk_q:
    if (compute == 0)
      compute = 1;
  case f_rh_plevel_th_q:
    if (compute == 0)
      compute = 2;
  case f_q_plevel_tk_rh:
    if (compute == 0)
      compute = 3;
  case f_q_plevel_th_rh:
    if (compute == 0)
      compute = 4;
  case f_tdc_plevel_tk_rh:
    if (compute == 0)
      compute = 5;
  case f_tdc_plevel_th_rh:
    if (compute == 0)
      compute = 6;
  case f_tdc_plevel_tk_q:
    if (compute == 0)
      compute = 7;
  case f_tdc_plevel_th_q:
    if (compute == 0)
      compute = 8;
  case f_tdk_plevel_tk_rh:
    if (compute == 0)
      compute = 9;
  case f_tdk_plevel_th_rh:
    if (compute == 0)
      compute = 10;
  case f_tdk_plevel_tk_q:
    if (compute == 0)
      compute = 11;
  case f_tdk_plevel_th_q:
    if (compute == 0)
      compute = 12;
    if (ninp != 2 || nout != 1)
      break;
    plevel = vfinput[0]->level;
    res = plevelhum(compute, nx, ny, finp[0], finp[1], fout[0], plevel,
        allDefined, undef, unit);
    break;

  case f_tcmean_plevel_z1_z2:
    if (compute == 0)
      compute = 1;
  case f_tkmean_plevel_z1_z2:
    if (compute == 0)
      compute = 2;
  case f_thmean_plevel_z1_z2:
    if (compute == 0)
      compute = 3;
    if (ninp != 2 || nout != 1)
      break;
    plevel1 = vfinput[0]->level;
    plevel2 = vfinput[1]->level;
    res = pleveldz2tmean(compute, nx, ny, finp[0], finp[1], fout[0], plevel1,
        plevel2, allDefined, undef);
    break;

  case f_qvector_plevel_z_tk_xcomp:
    if (compute == 0)
      compute = 1;
  case f_qvector_plevel_z_th_xcomp:
    if (compute == 0)
      compute = 2;
  case f_qvector_plevel_z_tk_ycomp:
    if (compute == 0)
      compute = 3;
  case f_qvector_plevel_z_th_ycomp:
    if (compute == 0)
      compute = 4;
    if (ninp != 2 || nout != 1)
      break;
    plevel = vfinput[0]->level;
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, &fcoriolis))
      res = plevelqvector(compute, nx, ny, finp[0], finp[1], fout[0],
          xmapr, ymapr, fcoriolis, plevel, allDefined, undef);
    break;

  case f_ducting_plevel_tk_q:
    if (compute == 0)
      compute = 1;
  case f_ducting_plevel_th_q:
    if (compute == 0)
      compute = 2;
  case f_ducting_plevel_tk_rh:
    if (compute == 0)
      compute = 3;
  case f_ducting_plevel_th_rh:
    if (compute == 0)
      compute = 4;
    if (ninp != 2 || nout != 1)
      break;
    plevel = vfinput[0]->level;
    res = plevelducting(compute, nx, ny, finp[0], finp[1], fout[0], plevel,
        allDefined, undef);
    break;

  case f_geostrophic_wind_plevel_z_xcomp:
    if (ninp != 1 || nout != 1)
      break;
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, &fcoriolis))
      res = plevelgwind_xcomp(nx, ny, finp[0], fout[0], xmapr, ymapr,
          fcoriolis, allDefined, undef);
    break;

  case f_geostrophic_wind_plevel_z_ycomp:
    if (ninp != 1 || nout != 1)
      break;
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, &fcoriolis))
      res = plevelgwind_ycomp(nx, ny, finp[0], fout[0], xmapr, ymapr,
          fcoriolis, allDefined, undef);
    break;

  case f_geostrophic_vorticity_plevel_z:
    if (ninp != 1 || nout != 1)
      break;
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, &fcoriolis))
      res = plevelgvort(nx, ny, finp[0], fout[0], xmapr, ymapr, fcoriolis,
          allDefined, undef);
    break;
  case f_kindex_plevel_tk_rh:
    if (compute == 0)
      compute = 1;
  case f_kindex_plevel_th_rh:
    if (compute == 0)
      compute = 2;
    if (ninp != 5 || nout != 1)
      break;
    plevel1 = vfinput[0]->level;
    plevel2 = vfinput[1]->level;
    plevel3 = vfinput[3]->level;
    res = kIndex(compute, nx, ny, finp[0], finp[1], finp[2], finp[3], finp[4],
        fout[0], plevel1, plevel2, plevel3, allDefined, undef);
    break;

  case f_ductingindex_plevel_tk_rh:
    if (compute == 0)
      compute = 1;
  case f_ductingindex_plevel_th_rh:
    if (compute == 0)
      compute = 2;
    if (ninp != 2 || nout != 1)
      break;
    plevel = vfinput[0]->level;
    res = ductingIndex(compute, nx, ny, finp[0], finp[1], fout[0], plevel,
        allDefined, undef);
    break;

  case f_showalterindex_plevel_tk_rh:
    if (compute == 0)
      compute = 1;
  case f_showalterindex_plevel_th_rh:
    if (compute == 0)
      compute = 2;
    if (ninp != 3 || nout != 1)
      break;
    plevel1 = vfinput[0]->level;
    plevel2 = vfinput[1]->level;
    res = showalterIndex(compute, nx, ny, finp[0], finp[1], finp[2], fout[0],
        plevel1, plevel2, allDefined, undef);
    break;

  case f_boydenindex_plevel_tk_z:
    if (compute == 0)
      compute = 1;
  case f_boydenindex_plevel_th_z:
    if (compute == 0)
      compute = 2;
    if (ninp != 3 || nout != 1)
      break;
    plevel1 = vfinput[1]->level;
    plevel2 = vfinput[2]->level;
    res = boydenIndex(compute, nx, ny, finp[0], finp[1], finp[2], fout[0],
        plevel1, plevel2, allDefined, undef);
    break;

    //---------------------------------------------------
    // hybrid model level (HLEVEL) functions
    //---------------------------------------------------

  case f_tc_hlevel_th_psurf:
    if (compute == 0)
      compute = 1;
  case f_tk_hlevel_th_psurf:
    if (compute == 0)
      compute = 2;
  case f_th_hlevel_tk_psurf:
    if (compute == 0)
      compute = 3;
  case f_thesat_hlevel_tk_psurf:
    if (compute == 0)
      compute = 4;
  case f_thesat_hlevel_th_psurf:
    if (compute == 0)
      compute = 5;
    if (ninp != 2 || nout != 1)
      break;
    alevel = vfinput[0]->aHybrid;
    blevel = vfinput[0]->bHybrid;
    res = hleveltemp(compute, nx, ny, finp[0], finp[1], fout[0], alevel,
        blevel, allDefined, undef,unit);
    break;

  case f_the_hlevel_tk_q_psurf:
    if (compute == 0)
      compute = 1;
  case f_the_hlevel_th_q_psurf:
    if (compute == 0)
      compute = 2;
    if (ninp != 3 || nout != 1)
      break;
    alevel = vfinput[0]->aHybrid;
    blevel = vfinput[0]->bHybrid;
    res = hlevelthe(compute, nx, ny, finp[0], finp[1], finp[2], fout[0],
        alevel, blevel, allDefined, undef);
    break;

  case f_rh_hlevel_tk_q_psurf:
    if (compute == 0)
      compute = 1;
  case f_rh_hlevel_th_q_psurf:
    if (compute == 0)
      compute = 2;
  case f_q_hlevel_tk_rh_psurf:
    if (compute == 0)
      compute = 3;
  case f_q_hlevel_th_rh_psurf:
    if (compute == 0)
      compute = 4;
  case f_tdc_hlevel_tk_q_psurf:
    if (compute == 0)
      compute = 5;
  case f_tdc_hlevel_th_q_psurf:
    if (compute == 0)
      compute = 6;
  case f_tdc_hlevel_tk_rh_psurf:
    if (compute == 0)
      compute = 7;
  case f_tdc_hlevel_th_rh_psurf:
    if (compute == 0)
      compute = 8;
  case f_tdk_hlevel_tk_q_psurf:
    if (compute == 0)
      compute = 9;
  case f_tdk_hlevel_th_q_psurf:
    if (compute == 0)
      compute = 10;
  case f_tdk_hlevel_tk_rh_psurf:
    if (compute == 0)
      compute = 11;
  case f_tdk_hlevel_th_rh_psurf:
    if (compute == 0)
      compute = 12;
    if (ninp != 3 || nout != 1)
      break;
    alevel = vfinput[0]->aHybrid;
    blevel = vfinput[0]->bHybrid;
    res = hlevelhum(compute, nx, ny, finp[0], finp[1], finp[2], fout[0],
        alevel, blevel, allDefined, undef, unit);
    break;

  case f_ducting_hlevel_tk_q_psurf:
    if (compute == 0)
      compute = 1;
  case f_ducting_hlevel_th_q_psurf:
    if (compute == 0)
      compute = 2;
  case f_ducting_hlevel_tk_rh_psurf:
    if (compute == 0)
      compute = 3;
  case f_ducting_hlevel_th_rh_psurf:
    if (compute == 0)
      compute = 4;
    if (ninp != 3 || nout != 1)
      break;
    alevel = vfinput[0]->aHybrid;
    blevel = vfinput[0]->bHybrid;
    res = hlevelducting(compute, nx, ny, finp[0], finp[1], finp[2], fout[0],
        alevel, blevel, allDefined, undef);
    break;

  case f_pressure_hlevel_xx_psurf:
    if (ninp != 2 || nout != 1)
      break;
    alevel = vfinput[0]->aHybrid;
    blevel = vfinput[0]->bHybrid;
    res = hlevelpressure(nx, ny, finp[1], fout[0], alevel, blevel, allDefined,
        undef);
    break;

    //---------------------------------------------------
    // atmospheric model level (ALEVEL) functions
    //---------------------------------------------------

  case f_tc_alevel_th_p:
    if (compute == 0)
      compute = 1;
  case f_tk_alevel_th_p:
    if (compute == 0)
      compute = 2;
  case f_th_alevel_tk_p:
    if (compute == 0)
      compute = 3;
  case f_thesat_alevel_tk_p:
    if (compute == 0)
      compute = 4;
  case f_thesat_alevel_th_p:
    if (compute == 0)
      compute = 5;
    if (ninp != 2 || nout != 1)
      break;
    res = aleveltemp(compute, nx, ny, finp[0], finp[1], fout[0], allDefined,
        undef, unit);
    break;

  case f_the_alevel_tk_q_p:
    if (compute == 0)
      compute = 1;
  case f_the_alevel_th_q_p:
    if (compute == 0)
      compute = 2;
    if (ninp != 3 || nout != 1)
      break;
    res = alevelthe(compute, nx, ny, finp[0], finp[1], finp[2], fout[0],
        allDefined, undef);
    break;

  case f_rh_alevel_tk_q_p:
    if (compute == 0)
      compute = 1;
  case f_rh_alevel_th_q_p:
    if (compute == 0)
      compute = 2;
  case f_q_alevel_tk_rh_p:
    if (compute == 0)
      compute = 3;
  case f_q_alevel_th_rh_p:
    if (compute == 0)
      compute = 4;
  case f_tdc_alevel_tk_q_p:
    if (compute == 0)
      compute = 5;
  case f_tdc_alevel_th_q_p:
    if (compute == 0)
      compute = 6;
  case f_tdc_alevel_tk_rh_p:
    if (compute == 0)
      compute = 7;
  case f_tdc_alevel_th_rh_p:
    if (compute == 0)
      compute = 8;
  case f_tdk_alevel_tk_q_p:
    if (compute == 0)
      compute = 9;
  case f_tdk_alevel_th_q_p:
    if (compute == 0)
      compute = 10;
  case f_tdk_alevel_tk_rh_p:
    if (compute == 0)
      compute = 11;
  case f_tdk_alevel_th_rh_p:
    if (compute == 0)
      compute = 12;
    if (ninp != 3 || nout != 1)
      break;
    res = alevelhum(compute, nx, ny, finp[0], finp[1], finp[2], fout[0],
        allDefined, undef, unit);
    break;

  case f_ducting_alevel_tk_q_p:
    if (compute == 0)
      compute = 1;
  case f_ducting_alevel_th_q_p:
    if (compute == 0)
      compute = 2;
  case f_ducting_alevel_tk_rh_p:
    if (compute == 0)
      compute = 3;
  case f_ducting_alevel_th_rh_p:
    if (compute == 0)
      compute = 4;
    if (ninp != 3 || nout != 1)
      break;
    res = alevelducting(compute, nx, ny, finp[0], finp[1], finp[2], fout[0],
        allDefined, undef);
    break;

    //---------------------------------------------------
    // isentropic level (ILEVEL) function
    //---------------------------------------------------

  case f_geostrophic_wind_ilevel_mpot:
    if (ninp != 1 || nout != 2)
      break;
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, &fcoriolis))
      res = ilevelgwind(nx, ny, finp[0], fout[0], fout[1], xmapr, ymapr,
          fcoriolis, allDefined, undef);
    break;

    //---------------------------------------------------
    // ocean depth level (OZLEVEL) functions
    //---------------------------------------------------

  case f_sea_soundspeed_ozlevel_tc_salt:
    if (compute == 0)
      compute = 1;
  case f_sea_soundspeed_ozlevel_tk_salt:
    if (compute == 0)
      compute = 2;
    if (ninp != 2 || nout != 1)
      break;
    odepth = vfinput[0]->level;
    res = seaSoundSpeed(compute, nx, ny, finp[0], finp[1], fout[0], odepth,
        allDefined, undef);
    break;

    //---------------------------------------------------
    // level (pressure) independent functions
    //---------------------------------------------------

  case f_temp_k2c:
    if (compute == 0)
      compute = 1;
  case f_temp_c2k:
    if (compute == 0)
      compute = 2;
  case f_temp_k2c_possibly:
    if (compute == 0)
      compute = 3;
  case f_temp_c2k_possibly:
    if (compute == 0)
      compute = 4;
    if (ninp != 1 || nout != 1)
      break;
    res = cvtemp(compute, nx, ny, finp[0], fout[0], allDefined, undef);
    break;

  case f_tdk_tk_rh:
    if (compute == 0)
      compute = 1;
  case f_tdc_tk_rh:
    if (compute == 0)
      compute = 2;
  case f_tdc_tc_rh:
    if (compute == 0)
      compute = 3;
  case f_rh_tk_td:
    if (compute == 0)
      compute = 4;
  case f_rh_tc_td:
    if (compute == 0)
      compute = 5;
    if (ninp != 2 || nout != 1)
      break;
    res = cvhum(compute, nx, ny, finp[0], finp[1], fout[0], allDefined, undef);
    break;

  case f_vector_abs:
    if (ninp != 2 || nout != 1)
      break;
    res = vectorabs(nx, ny, finp[0], finp[1], fout[0], allDefined, undef);
    break;

  case f_direction:
    if (ninp != 2 || nout != 1)
      break;
    res = direction(nx, ny, finp[0], finp[1], vfinput[0]->area, fout[0],
        allDefined, undef);
    break;

  case f_rel_vorticity:
    if (ninp != 2 || nout != 1)
      break;
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, 0))
      res = relvort(nx, ny, finp[0], finp[1], fout[0], xmapr, ymapr,
          allDefined, undef);
    break;

  case f_abs_vorticity:
    if (ninp != 2 || nout != 1)
      break;
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, &fcoriolis))
      res = absvort(nx, ny, finp[0], finp[1], fout[0], xmapr, ymapr, fcoriolis,
          allDefined, undef);
    break;

  case f_divergence:
    if (ninp != 2 || nout != 1)
      break;
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, 0))
      res = divergence(nx, ny, finp[0], finp[1], fout[0], xmapr, ymapr,
          allDefined, undef);
    break;

  case f_advection:
    if (ninp != 3 || nout != 1 || nconst != 1)
      break;
    hours = constants[0];
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, 0))
      res = advection(nx, ny, finp[0], finp[1], finp[2], fout[0], xmapr, ymapr,
          hours, allDefined, undef);
    break;

  case f_d_dx:
    if (compute == 0)
      compute = 1;
  case f_d_dy:
    if (compute == 0)
      compute = 2;
  case f_abs_del:
    if (compute == 0)
      compute = 3;
  case f_del_square:
    if (compute == 0)
      compute = 4;
    if (ninp != 1 || nout != 1)
      break;
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, 0))
      res = gradient(compute, nx, ny, finp[0], fout[0], xmapr, ymapr,
          allDefined, undef);
    break;

  case f_minvalue_fields:
    if (ninp != 2 || nout != 1)
      break;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < fsize; i++) {
      if (calculations::is_defined(allDefined, finp[0][i], finp[1][i], fieldUndef))
        fout[0][i] = std::min(finp[0][i], finp[1][i]);
      else
        fout[0][i] = fieldUndef;
    }
    res = true;
    break;

  case f_maxvalue_fields:
    if (ninp != 2 || nout != 1)
      break;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < fsize; i++) {
      if (calculations::is_defined(allDefined, finp[0][i], finp[1][i], fieldUndef))
        fout[0][i] = std::max(finp[0][i], finp[1][i]);
      else
        fout[0][i] = fieldUndef;
    }
    res = true;
    break;

  case f_minvalue_field_const:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    constant = constants[0];
    if (constant != fieldUndef) {
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
      for (int i = 0; i < fsize; i++) {
        if (calculations::is_defined(allDefined, finp[0][i], fieldUndef))
          fout[0][i] = std::min(finp[0][i], constant);
        else
          fout[0][i] = fieldUndef;
      }
    } else {
      allDefined = false;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
      for (int i = 0; i < fsize; i++)
        fout[0][i] = fieldUndef;
    }
    res = true;
    break;

  case f_maxvalue_field_const:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    constant = constants[0];
    if (constant != fieldUndef) {
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
      for (int i = 0; i < fsize; i++) {
        if (calculations::is_defined(allDefined, finp[0][i], fieldUndef))
          fout[0][i] = std::max(finp[0][i], constant);
        else
          fout[0][i] = fieldUndef;
      }
    } else {
      allDefined = false;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
      for (int i = 0; i < fsize; i++)
        fout[0][i] = fieldUndef;
    }
    res = true;
    break;

  case f_abs:
    if (ninp != 1 || nout != 1)
      break;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < fsize; i++) {
      if (calculations::is_defined(allDefined, finp[0][i], fieldUndef))
        fout[0][i] = fabs(finp[0][i]);
      else
        fout[0][i] = fieldUndef;
    }
    res = true;
    break;

  case f_log10:
    if (ninp != 1 || nout != 1)
      break;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < fsize; i++) {
      if (calculations::is_defined(allDefined, finp[0][i], fieldUndef))
        fout[0][i] = log10(finp[0][i]);
      else
        fout[0][i] = fieldUndef;
    }
    res = true;
    break;

  case f_pow10:
    if (ninp != 1 || nout != 1)
      break;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < fsize; i++) {
      if (calculations::is_defined(allDefined, finp[0][i], fieldUndef))
        fout[0][i] = pow10(finp[0][i]);
      else
        fout[0][i] = fieldUndef;
    }
    res = true;
    break;

  case f_log:
    if (ninp != 1 || nout != 1)
      break;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < fsize; i++) {
      if (calculations::is_defined(allDefined, finp[0][i], fieldUndef))
        fout[0][i] = log(finp[0][i]);
      else
        fout[0][i] = fieldUndef;
    }
    res = true;
    break;

  case f_exp:
    if (ninp != 1 || nout != 1)
      break;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < fsize; i++) {
      if (calculations::is_defined(allDefined, finp[0][i], fieldUndef))
        fout[0][i] = exp(finp[0][i]);
      else
        fout[0][i] = fieldUndef;
    }
    res = true;
    break;

  case f_power:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    constant = constants[0];
    if (constant != fieldUndef) {
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
      for (int i = 0; i < fsize; i++) {
        if (calculations::is_defined(allDefined, finp[0][i], fieldUndef))
          fout[0][i] = powf(finp[0][i], constant);
        else
          fout[0][i] = fieldUndef;
      }
    } else {
      allDefined = false;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
      for (int i = 0; i < fsize; i++)
        fout[0][i] = fieldUndef;
    }
    res = true;
    break;

  case f_shapiro2_filter:
    if (ninp != 1 || nout != 1)
      break;
    res = shapiro2_filter(nx, ny, finp[0], fout[0], allDefined, undef);
    break;

  case f_smooth:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    nsmooth = int(constants[0] + 0.5);
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < fsize; i++)
      fout[0][i] = finp[0][i];
    res = vfres[0]->smooth(nsmooth);
    break;

  case f_windcooling_tk_u_v:
    if (compute == 0)
      compute = 1;
  case f_windcooling_tc_u_v:
    if (compute == 0)
      compute = 2;
    if (ninp != 3 || nout != 1)
      break;
    res = windCooling(compute, nx, ny, finp[0], finp[1], finp[2], fout[0],
        allDefined, undef);
    break;

  case f_undercooled_rain:
    if (ninp != 3 || nout != 1 || nconst != 3)
      break;
    precipMin = constants[0];
    snowRateMax = constants[1];
    tcMax = constants[2];
    res = underCooledRain(nx, ny, finp[0], finp[1], finp[2], fout[0],
        precipMin, snowRateMax, tcMax, allDefined, undef);
    break;

  case f_thermal_front_parameter_tx:
    if (ninp != 1 || nout != 1)
      break;
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, 0))
      res = thermalFrontParameter(nx, ny, finp[0], fout[0], xmapr, ymapr,
          allDefined, undef);
    break;

  case f_pressure2flightlevel:
    if (ninp != 1 || nout != 1)
      break;
    res = pressure2FlightLevel(nx, ny, finp[0], fout[0], allDefined, undef);
    break;

  case f_momentum_x_coordinate:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    fcoriolisMin = constants[0];
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, &fcoriolis))
      res = momentumXcoordinate(nx, ny, finp[0], fout[0], xmapr, fcoriolis,
          fcoriolisMin, allDefined, undef);
    break;

  case f_momentum_y_coordinate:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    fcoriolisMin = constants[0];
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, &fcoriolis))
      res = momentumYcoordinate(nx, ny, finp[0], fout[0], ymapr, fcoriolis,
          fcoriolisMin, allDefined, undef);
    break;

  case f_jacobian:
    if (ninp != 2 || nout != 1)
      break;
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, 0))
      res = jacobian(nx, ny, finp[0], finp[1], fout[0], xmapr, ymapr,
          allDefined, undef);
    break;

  case f_vessel_icing_overland:
    if (ninp != 4 || nout != 1 || nconst != 1)
      break;
    res = vesselIcingOverland(nx, ny, finp[0], finp[1], finp[2], finp[3], fout[0],
        constants[0], allDefined, undef);
    break;

  case f_vessel_icing_mertins:
    if (ninp != 4 || nout != 1 || nconst != 1)
      break;
    res = vesselIcingMertins(nx, ny, finp[0], finp[1], finp[2], finp[3], fout[0],
        constants[0], allDefined, undef);
    break;

  case f_replace_undefined:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    constant = constants[0];
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < fsize; i++)
      if (finp[0][i] != fieldUndef)
        fout[0][i] = finp[0][i];
      else
        fout[0][i] = constant;
    vfres[0]->allDefined = (constant != fieldUndef);
    res = true;
    break;

  case f_replace_defined:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    constant = constants[0];
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < fsize; i++)
      if (finp[0][i] != fieldUndef)
        fout[0][i] = constant;
      else
        fout[0][i] = fieldUndef;
    res = true;
    break;

  case f_replace_all:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    constant = constants[0];
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < fsize; i++)
      fout[0][i] = constant;
    vfres[0]->allDefined = (constant != fieldUndef);
    res = true;
    break;

  case f_values2classes:
    if (ninp != 1 || nout != 1 || nconst < 2)
      break;
    // constants has at least low_value and high_value
    res = values2classes(nx, ny, finp[0], fout[0], constants,
        allDefined, undef);
    break;

  case f_field_diff_forecast_hour: // not handled here
  case f_accum_diff_forecast_hour: // not handled here
  case f_sum_of_forecast_hours: // not handled here
  case f_sum_of_fields: // not handled here
  case f_max_of_fields: // not handled here
  case f_no_of_fields_above: // not handled here
  case f_no_of_fields_below: // not handled here
  case f_index_of_fields_max: // not handled here
  case f_index_of_fields_min: // not handled here
    break;

  case f_add_f_f:
    if (compute == 0)
      compute = 1; // field + field
  case f_subtract_f_f:
    if (compute == 0)
      compute = 2; // field - field
  case f_multiply_f_f:
    if (compute == 0)
      compute = 3; // field * field
  case f_divide_f_f:
    if (compute == 0)
      compute = 4; // field / field
    if (ninp != 2 || nout != 1)
      break;
    res = fieldOPERfield(compute, nx, ny, finp[0], finp[1], fout[0],
        allDefined, undef);
    break;

  case f_sum_f:
    if (compute == 0)
      compute = 5; // field + field + field +
    res = sumFields(nx, ny, finp, fout[0],
        allDefined, undef);
    break;

  case f_add_f_c:
    if (compute == 0)
      compute = 1; // field + constant
  case f_subtract_f_c:
    if (compute == 0)
      compute = 2; // field - constant
  case f_multiply_f_c:
    if (compute == 0)
      compute = 3; // field * constant
  case f_divide_f_c:
    if (compute == 0)
      compute = 4; // field / constant
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    res = fieldOPERconstant(compute, nx, ny, finp[0], constants[0], fout[0],
        allDefined, undef);
    break;

  case f_add_c_f:
    if (compute == 0)
      compute = 1; // constant + field
  case f_subtract_c_f:
    if (compute == 0)
      compute = 2; // constant - field
  case f_multiply_c_f:
    if (compute == 0)
      compute = 3; // constant * field
  case f_divide_c_f:
    if (compute == 0)
      compute = 4; // constant / field
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    res = constantOPERfield(compute, nx, ny, constants[0], finp[0], fout[0],
        allDefined, undef);
    break;

  case f_equivalent_to :
    if (ninp != 1 || nout != 1)
      break;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < fsize; i++) {
      fout[0][i] = finp[0][i];
    }
    res = true;
    break;

  case f_max_value:
    if (compute == 0)
    compute = 1;
  case f_min_value:
    if (compute == 0)
    compute = 2;
  case f_max_index:
    if (compute == 0)
    compute = 3;
  case f_min_index:
    if (compute == 0)
    compute = 4;
    res = extremeValue(compute, nx, ny, finp, fout[0],
        allDefined, undef);
    break;


  case f_mean_value:
    res = meanValue(nx, ny, finp, fout[0],
        allDefined, undef);
    break;

  case f_stddev:
    res = stddevValue(nx, ny, finp, fout[0],
        allDefined, undef);
    break;

  case f_probability_above:
    if (compute == 0)
    compute = 1;
  case f_probability_below:
    if (compute == 0)
    compute = 2;
  case f_probability_between:
    if (compute == 0)
    compute = 3;
  case f_number_above:
    if (compute == 0)
    compute = 4;
  case f_number_below:
    if (compute == 0)
    compute = 5;
  case f_number_between:
    if (compute == 0)
    compute = 6;
    if (nout != 1 || nconst < 1)
      break;
    res = probability(compute, nx, ny, finp, constants,  fout[0],
        allDefined, undef);
    break;

  case f_percentile:
    if (ninp != 1 || nout != 1 || nconst < 2)
      break;
    if ( nconst == 3)
      compute = int(constants[2]);
    else
      compute = 3;
    res = percentile(nx, ny, finp[0], constants[0], constants[1],  compute, fout[0],
        allDefined, undef);
    break;

   case f_snow_cm_from_snow_water_tk_td:
    if (ninp != 3 || nout != 1)
      break;
    res = snow_in_cm(nx, ny, finp[0], finp[1], finp[2], fout[0], allDefined, undef);
    break;


  default:
    METLIBS_LOG_ERROR("Illegal function='" << function << "'");
    break;
  }

  //alldefined might have changed during computation
  for (int i = 0; i < nout; i++) {
    vfres[i]->allDefined = allDefined;
  }
  return res;
}

//---------------------------------------------------
// pressure level (PLEVEL) functions
//---------------------------------------------------

// static
bool FieldFunctions::pleveltemp(int compute, int nx, int ny, const float *tinp,
    float *tout, float p, bool& allDefined, float undef, const std::string& unit)
{
  //  Pressure levels:
  //     compute=1 : potensiell temp. -> temp. (grader Celsius)
  //     compute=2 : potensiell temp. -> temp. (grader Kelvin)
  //     compute=3 : temp. (grader Kelvin)  -> potensiell temp. (Kelvin)
  //     compute=4 : temp. (grader Kelvin) -> saturated equivalent pot.temp.
  //     compute=5 : potensiell temp.      -> saturated equivalent pot.temp.
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  if (compute < 3) {
    if (unit == "celsius")
      compute = 1;
    else if (unit == "kelvin")
      compute = 2;
  }

  const int fsize = nx * ny;
  if (p <= 0.0)
    return false;

  const bool inAllDefined = allDefined;
  const float pidcp = calculations::pidcp_from_p(p), pi = pidcp*cp;

#ifdef HAVE_OPENMP
#pragma omp parallel for shared(allDefined)
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(inAllDefined, tinp[i], undef)) {
      if (compute == 1) { // TH -> T(Celsius)
        tout[i] = tinp[i] * pidcp - t0;
      } else if (compute == 2) { // TH -> T(Kelvin)
        tout[i] = tinp[i] * pidcp;
      } else if (compute == 3) { // T(Kelvin) -> TH
        tout[i] = tinp[i] / pidcp;
      } else if (compute == 4) { // T(Kelvin) -> THESAT
        tout[i] = calculations::t_thesat(tinp[i], p, pi, undef, allDefined);
      } else if (compute == 5) { // TH -> THESAT
        tout[i] = calculations::th_thesat(tinp[i], p, pi, undef, allDefined);
      }
    } else {
      tout[i] = undef;
    }
  }
  return true;
}

// static
bool FieldFunctions::plevelthe(int compute, int nx, int ny, const float *t,
    const float *rh, float *the, float p, bool& allDefined, float undef)
{
  //  Pressure levels:
  //    compute=1 : temp. (Kelvin)  og RH(%)     -> THE, ekvivalent pot.temp (Kelvin)
  //    compute=2 : pot. temp. (Kelvin) og RH(%) -> THE, ekvivalent pot.temp (Kelvin)
  //
  //  input:
  //    nx,ny      : field dimensions
  //    t[nx*ny]   : temp. (Kelvin)  or  pot.temp. (Kelvin)
  //    rh[nx*ny]  : rel. humidity (%)
  //    p          : pressure (hPa)
  //  output:
  //    the[nx*ny] : equivale pot. temp. (Kelvin)

#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  if (compute != 1 and compute != 2)
    return false;

  if (p <= 0.0)
    return false;

  const float pi = calculations::pi_from_p(p);
  const float cvrh = 0.01 * (xlh / pi) * eps / p;
  const float tconv = (compute == 2) ? (pi / cp) : 1;
  const float thconv = (compute == 1) ? (cp / pi) : 1;

  const int fsize = nx * ny;
  const bool inAllDefined = allDefined;

#ifdef HAVE_OPENMP
#pragma omp parallel for shared(allDefined)
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(inAllDefined, t[i], rh[i], undef))
      // T(Kelvin), RH(%) -> THE  or  TH, RH(%) -> THE
      the[i] = calculations::tk_rh_the(t[i] * tconv, rh[i] * cvrh, thconv, undef, allDefined);
    else
      the[i] = undef;
  }
  return true;
}

// static
bool FieldFunctions::plevelhum(int compute, int nx, int ny, const float *t,
    const float *huminp, float *humout, float p, bool& allDefined, float undef,
    const std::string& unit)
{
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  //  Pressure levels:
  //     compute=1  : temp. (Kelvin) og spes. fukt. -> rel. fuktighet (%)
  //     compute=2  : pot. temp. og spesifikk fukt. -> rel. fuktighet (%)
  //     compute=3  : temp. (Kelvin) og rel. fukt.  -> spesifikk fukt. (kg/kg)
  //     compute=4  : pot. temp. og  relativ fukt.  -> spesifikk fukt. (kg/kg)
  //     compute=5  : temp. (Kelvin) og rel. fukt.  -> duggpunkt, Td (Celsius)
  //     compute=6  : pot. temp. og  relativ fukt.  -> duggpunkt, Td (Celsius)
  //     compute=7  : temp. (Kelvin) og spes. fukt. -> duggpunkt, Td (Celsius)
  //     compute=8  : pot. temp. og spesifikk fukt. -> duggpunkt, Td (Celsius)
  //     compute=9  : temp. (Kelvin) og rel. fukt.  -> duggpunkt, Td (Kelvin)
  //     compute=10 : pot. temp. og  relativ fukt.  -> duggpunkt, Td (Kelvin)
  //     compute=11 : temp. (Kelvin) og spes. fukt. -> duggpunkt, Td (Kelvin)
  //     compute=12 : pot. temp. og spesifikk fukt. -> duggpunkt, Td (Kelvin)
  //
  //  Note: compute=5,9 are pressure independent (and not needed here)

  if (p <= 0 or compute <= 0 or compute >= 13)
    return false;

  if (compute > 8 && unit == "celsius")
    compute -= 4;
  else if (compute > 4 && compute <= 8 && unit == "kelvin")
    compute += 4;

  const int fsize = nx * ny;

  if (p == undef and (compute != 5 and compute != 6 and compute != 9 and compute != 10)) {
    // compute != 5/6/9/10 depends on p, so if p == undef, the result must also be undef
    for (int i = 0; i < fsize; i++)
      humout[i] = undef;
    allDefined = false;
    return true;
  }

  const float pi = calculations::pi_from_p(p);
  const bool inAllDefined = allDefined;

  const float tconv = (compute % 2 == 0) ? (pi / cp) : 1;
  const float tdconv = (compute >= 9) ? t0 : 0;

#ifdef HAVE_OPENMP
#pragma omp parallel for shared(allDefined)
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(inAllDefined, t[i], huminp[i], undef)) { // p checked before loop
      if (compute == 1 or compute == 2) { // T(Kelvin),q -> RH(%)  or  TH,q -> RH(%)
        humout[i] = calculations::tk_q_rh(t[i] * tconv, huminp[i], p, undef, allDefined);
      } else if (compute == 3 or compute == 4) { // T(Kelvin),RH(%) -> q  or  TH,RH(%) -> q
        humout[i] = calculations::tk_rh_q(t[i] * tconv, huminp[i], p, undef, allDefined);
      } else  if (compute == 5 or compute == 6 or compute == 9 or compute == 10) {
        // T(Kelvin),RH(%) -> Td(Celsius)     or  TH,RH(%) -> Td(Celsius)
        // or  T(Kelvin),RH(%) -> Td(Kelvin)  or  TH,RH(%) -> Td(Kelvin)
        humout[i] = calculations::tk_rh_td(t[i] * tconv, huminp[i], tdconv, undef, allDefined);
      } else if (compute == 7 or compute == 8 or compute == 11 or compute == 12) {
        // T(Kelvin),q -> Td(Celsius)     or  TH,q -> Td(Celsius)
        // or  T(Kelvin),q -> Td(Kelvin)  or  TH,q -> Td(Kelvin)
        humout[i] = calculations::tk_q_td(t[i] * tconv, huminp[i], p, tdconv, undef, allDefined);
      }
    } else {
      humout[i] = undef;
    }
  }
  return true;
}

// static
bool FieldFunctions::pleveldz2tmean(int compute, int nx, int ny, const float *z1,
    const float *z2, float *tmean, float p1, float p2, bool& allDefined, float undef)
{
  //  Pressure levels:
  //     compute=1 ; tykkelse -> middel temp. (grader Celsius)
  //     compute=2 ; tykkelse -> middel temp. (grader Kelvin)
  //     compute=3 ; tykkelse -> middel potensiell temp. (grader Kelvin)
  //  input: z1(nx,ny),z2(nx,ny) og p1,p2 (p1<p2 eller p1>p2)
  //  output: tmean(nx,ny) ........ tmean=z1 eller tmean=z2 er o.k.
  //
  //  antar middel-temp. gjelder i nivaa som er middel av exner-funk.
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  const int fsize = nx * ny;

  if (p1 <= 0.0 || p2 <= 0.0 || p1 == p2)
    return false;

  const float pi1 = calculations::pi_from_p(p1);
  const float pi2 = calculations::pi_from_p(p2);

  float convert, tconvert;
  switch (compute) {
  case 1:
    convert = g * 0.5 * (pi1 + pi2) / ((pi2 - pi1) * cp);
    tconvert = -t0;
    break;
  case 2:
    convert = g * 0.5 * (pi1 + pi2) / ((pi2 - pi1) * cp);
    tconvert = 0.;
    break;
  case 3:
    convert = g / (pi2 - pi1);
    tconvert = 0.;
    break;
  default:
    return false;
  }

#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(allDefined, z1[i], z2[i], undef))
      tmean[i] = (z1[i] - z2[i]) * convert + tconvert;
    else
      tmean[i] = undef;
  }
  return true;
}

// static
bool FieldFunctions::plevelqvector(int compute, int nx, int ny, const float *z,
    const float *t, float *qcomp, const float *xmapr, const float *ymapr,
    const float *fcoriolis, float p, bool& allDefined, float undef)
{
  //  Q-vector in pressure levels
  //
  //  compute=1 : input t is abs.temp. (K),       output qcomp (x-comp)
  //  compute=2 : input t is potential temp. (K), output qcomp (x-comp)
  //  compute=3 : input t is abs.temp. (K),       output qcomp (y-comp)
  //  compute=4 : input t is potential temp. (K), output qcomp (y-comp)
  //
  //  input:  z[nx*ny] : height (m) in pressure level
  //          t[nx*ny] : temperature or potential temp.
  //          xmapr[nx*ny],ymapr[nx*ny] : map ratios (xm/(hx*2),ym/(hy*2))
  //          fcoriolis[nx*ny] : coriolis parameter
  //          p: pressure (hPa)
  //  output: qx[nx*ny] : x component of Q-vector
  //          qy[nx*ny] : y component of Q-vector
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  const int fsize = nx * ny;
  float tscale, pi;

  if (p <= 0.0)
    return false;

  if (nx < 3 || ny < 3)
    return false;

  switch (compute) {
  case 1:
    // t is abs.temp. (K)
    tscale = 1.0;
    break;
  case 2:
    // t is pot.temp. (K)
    pi = cp * powf(p / p0, r / cp);
    tscale = pi / cp;
    break;
  case 3:
    // t is abs.temp. (K)
    tscale = 1.0;
    break;
  case 4:
    // t is pot.temp. (K)
    pi = cp * powf(p / p0, r / cp);
    tscale = pi / cp;
    break;
  default:
    return false;
  }

  float *ug = new float[fsize];
  float *vg = new float[fsize];

  if (!plevelgwind_xcomp(nx, ny, z, ug, xmapr, ymapr, fcoriolis, allDefined, undef)
      || !plevelgwind_ycomp(nx, ny, z, vg, xmapr, ymapr, fcoriolis, allDefined, undef)) {
    delete[] ug;
    delete[] vg;
    return false;
  }

  float dugdx, dugdy, dvgdx, dvgdy, dtdx, dtdy;
  float c = -r / (p * 100.);

  if (allDefined) {
    // loop extended, reset bad computations at boundaries later
    if (compute < 3 ) {
      for (int i = nx; i < fsize - nx; i++) {
        dugdx = 0.5 * xmapr[i] * (ug[i + 1] - ug[i - 1]);
        dvgdx = 0.5 * xmapr[i] * (vg[i + 1] - vg[i - 1]);
        dtdx = 0.5 * xmapr[i] * tscale * (t[i + 1] - t[i - 1]);
        dtdy = 0.5 * ymapr[i] * tscale * (t[i + nx] - t[i - nx]);
        qcomp[i] = c * (dugdx * dtdx + dvgdx * dtdy);
      }
    } else {
      for (int i = nx; i < fsize - nx; i++) {
        dugdy = 0.5 * ymapr[i] * (ug[i + nx] - ug[i - nx]);
        dvgdy = 0.5 * ymapr[i] * (vg[i + nx] - vg[i - nx]);
        dtdx = 0.5 * xmapr[i] * tscale * (t[i + 1] - t[i - 1]);
        dtdy = 0.5 * ymapr[i] * tscale * (t[i + nx] - t[i - nx]);
        qcomp[i] = c * (dugdy * dtdx + dvgdy * dtdy);
      }

    }
  } else {
    if (compute < 3 ) {
      for (int i = nx; i < fsize - nx; i++) {
        if (ug[i - nx] != undef && ug[i - 1] != undef && ug[i + 1] != undef
            && ug[i + nx] != undef && vg[i - nx] != undef && vg[i - 1] != undef
            && vg[i + 1] != undef && vg[i + nx] != undef && t[i - nx] != undef
            && t[i - 1] != undef && t[i + 1] != undef && t[i + nx] != undef) {
          dugdx = 0.5 * xmapr[i] * (ug[i + 1] - ug[i - 1]);
          dvgdx = 0.5 * xmapr[i] * (vg[i + 1] - vg[i - 1]);
          dtdx = 0.5 * xmapr[i] * tscale * (t[i + 1] - t[i - 1]);
          dtdy = 0.5 * ymapr[i] * tscale * (t[i + nx] - t[i - nx]);
          qcomp[i] = c * (dugdx * dtdx + dvgdx * dtdy);
        } else {
          qcomp[i] = undef;
        }
      }
    } else {
      for (int i = nx; i < fsize - nx; i++) {
        if (ug[i - nx] != undef && ug[i - 1] != undef && ug[i + 1] != undef
            && ug[i + nx] != undef && vg[i - nx] != undef && vg[i - 1] != undef
            && vg[i + 1] != undef && vg[i + nx] != undef && t[i - nx] != undef
            && t[i - 1] != undef && t[i + 1] != undef && t[i + nx] != undef) {
          dugdy = 0.5 * ymapr[i] * (ug[i + nx] - ug[i - nx]);
          dvgdy = 0.5 * ymapr[i] * (vg[i + nx] - vg[i - nx]);
          dtdx = 0.5 * xmapr[i] * tscale * (t[i + 1] - t[i - 1]);
          dtdy = 0.5 * ymapr[i] * tscale * (t[i + nx] - t[i - nx]);
          qcomp[i] = c * (dugdy * dtdx + dvgdy * dtdy);
        } else {
          qcomp[i] = undef;
        }
      }
    }
  }

  delete[] ug;
  delete[] vg;

  // fill in edge values not computed (or badly computed) above
  fillEdges(nx, ny, qcomp);
  return true;
}

// static
bool FieldFunctions::plevelducting(int compute, int nx, int ny, const float *t,
    const float *h, float *duct, float p, bool& allDefined, float undef)
{
  //  Pressure level:
  //
  //    compute=1 : temp. (Kelvin)      and q (kg/kg)  -> ducting
  //    compute=2 : pot. temp. (Kelvin) and q (kg/kg)  -> ducting
  //    compute=3 : temp. (Kelvin)      and RH(%)      -> ducting
  //    compute=4 : pot. temp. (Kelvin) and RH(%)      -> ducting
  //
  //  input:
  //    nx,ny      : field dimensions
  //    t[nx*ny]   : temp. (Kelvin)  or  pot.temp. (Kelvin)
  //    h[nx*ny]   : specific humidity (kg/kg) or rel.hum.(%)
  //    p          : pressure (hPa)
  //  output:
  //    duct[nx*ny]: ducting
  //...................................................................
  //       t in unit Kelvin, p in unit hPa
  //       duct=77.6*(p/t)+373000.*(q*p)/(eps*t*t)
  //       q*p/eps = rh*qsat*p/eps = rh*(eps*e(t)/p)*p/eps
  //               = rh*e(t) = (e(td)/e(t))*e(t) = e(td)
  //       => duct = 77.6*(p/t)+373000.*e(td)/(t*t)
  //          duct = 77.6*(p/t)+373000.*rh*e(t)/(t*t)
  //...................................................................
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  if (p <= 0.0 or compute <= 0 or compute >= 5)
    return false;

  const int fsize = nx * ny;
  const bool inAllDefined = allDefined;
  const float tconv = (compute % 2 == 0) ? calculations::pidcp_from_p(p) : 1;

#ifdef HAVE_OPENMP
#pragma omp parallel for shared(allDefined)
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(inAllDefined, t[i], h[i], undef)) {
      if (compute == 1 or compute == 2) { // T(Kelvin),q -> ducting  or  TH,q -> ducting
        duct[i] = calculations::tk_q_duct(t[i] * tconv, h[i], p);
      } else if (compute == 3 or compute == 4) { // T(Kelvin),RH(%) -> ducting  or TH,RH(%) -> ducting
        duct[i] = calculations::tk_rh_duct(t[i] * tconv, h[i], p, undef, allDefined);
      }
    } else {
      duct[i] = undef;
    }
  }
  return true;
}

// static
bool FieldFunctions::plevelgwind_xcomp(int nx, int ny, const float *z, float *ug,
    const float */*xmapr*/, const float *ymapr, const float *fcoriolis, bool& allDefined, float undef)
{
  //  Geostophic wind in pressure level
  //  (centered differences)
  //
  //  input:  z[nx*ny] : height (m) in pressure level
  //          xmapr[nx*ny],ymapr[nx*ny] : map ratios (xm/(hx*2),ym/(hy*2))
  //          fcoriolis[nx*ny] : coriolis parameter
  //  output: ug[nx*ny] : x component of geostrophic wind (m/s)
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  if (nx < 3 || ny < 3)
    return false;

  const int fsize = nx * ny;

    // loop extended, reset bad computations at boundaries later
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
  for (int i = nx; i < fsize - nx; i++) {
    // check for y component input, too
    if (calculations::is_defined(allDefined, z[i-nx], z[i-1], z[i+1], z[i+nx], undef))
      ug[i] = -0.5 * ymapr[i] * (z[i+nx] - z[i-nx]) * g / fcoriolis[i];
    else
      ug[i] = undef;
  }

  // fill in edge values not computed (or badly computed) above
  fillEdges(nx, ny, ug);

  return true;
}

// static
bool FieldFunctions::plevelgwind_ycomp(int nx, int ny, const float *z, float *vg,
    const float *xmapr, const float */*ymapr*/, const float *fcoriolis, bool& allDefined, float undef)
{
  //  Geostophic wind in pressure level
  //  (centered differences)
  //
  //  input:  z[nx*ny] : height (m) in pressure level
  //          xmapr[nx*ny],ymapr[nx*ny] : map ratios (xm/(hx*2),ym/(hy*2))
  //          fcoriolis[nx*ny] : coriolis parameter
  //  output: vg[nx*ny] : y component of geostrophic wind (m/s)
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  const int fsize = nx * ny;

    // loop extended, reset bad computations at boundaries later
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
  for (int i = nx; i < fsize - nx; i++) {
    // check for x component input, too
    if (calculations::is_defined(allDefined, z[i-nx], z[i-1], z[i+1], z[i+nx], undef))
      vg[i] = 0.5 * xmapr[i] * (z[i + 1] - z[i - 1]) * g / fcoriolis[i];
    else
      vg[i] = undef;
  }

  // fill in edge values not computed (or badly computed) above
  fillEdges(nx, ny, vg);

  return true;
}

// static
bool FieldFunctions::plevelgvort(int nx, int ny, const float *z, float *gvort,
    const float *xmapr, const float *ymapr, const float *fcoriolis, bool& allDefined, float undef)
{
  //  Geostophic vorticity in pressure level (centered differences)
  //
  //  input:  z[nx*ny] : height (m)
  //          xmapr[nx*ny],ymapr[nx*ny] : map ratios (xm/(2*hx),ym/(2*hy) !!!)
  //          fcoriolis[nx*ny] : coriolis parameter
  //  output: gvort[nx*ny] : geostrophic relative vorticity
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  const float g4 = g * 4.;

  int fsize = nx * ny;

  if (nx < 3 || ny < 3)
    return false;

  if (allDefined) {

    // loop extended, reset bad computations at boundaries later
    for (int i = nx; i < fsize - nx; i++)
      gvort[i] = (0.25 * xmapr[i] * xmapr[i] * (z[i - 1] - 2. * z[i] + z[i + 1])
          + 0.25 * ymapr[i] * ymapr[i] * (z[i - nx] - 2. * z[i] + z[i + nx])) * g4
          / fcoriolis[i];

  } else {

    for (int i = nx; i < fsize - nx; i++) {
      if (z[i - nx] != undef && z[i - 1] != undef && z[i] != undef && z[i + 1]
                                                                        != undef && z[i + nx] != undef)
        gvort[i] = (0.25 * xmapr[i] * xmapr[i] * (z[i - 1] - 2. * z[i] + z[i + 1])
            + 0.25 * ymapr[i] * ymapr[i] * (z[i - nx] - 2. * z[i] + z[i + nx])) * g4
            / fcoriolis[i];
      else
        gvort[i] = undef;
    }

  }

  // fill in edge values not computed (or badly computed) above
  fillEdges(nx, ny, gvort);

  return true;
}

// static
bool FieldFunctions::kIndex(int compute, int nx, int ny, const float *t500,
    const float *t700, const float *rh700, const float *t850, const float *rh850, float *kfield,
    float p500, float p700, float p850, bool& allDefined, float undef)
{
  // K-index: (t+td)850 - (t-td)700 - (t)500
  //
  // note: levels are not fixed
  //
  //  Pressure levels:
  //    compute=1 : tPPP arrays contain temperature (Kelvin)
  //    compute=2 : tPPP arrays contain pot. temp. (Kelvin)
  //
  //  input:
  //    nx,ny        : field dimensions
  //     tPPP[nx*ny] : temp. (Kelvin)  or  pot.temp. (Kelvin)
  //    rhPPP[nx*ny] : rel. humidity (%)
  //    pPPP         : pressure (hPa)
  //  output:
  //    kfield[nx*ny] : K-index

#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  if (p500 <= 0.0 || p500 >= p700 || p700 >= p850)
    return false;

  float cvt500, cvt700, cvt850;
  switch (compute) {
  case 1: // Temp(Kelvin) input
    cvt500 = 1.;
    cvt700 = 1.;
    cvt850 = 1.;
    break;
  case 2: // Pot.temp. (TH) input
    cvt500 = calculations::pidcp_from_p(p500);
    cvt700 = calculations::pidcp_from_p(p700);
    cvt850 = calculations::pidcp_from_p(p850);
    break;
  default:
    return false;
  }

  const int fsize = nx * ny;
  const bool inAllDefined = allDefined;

#ifdef HAVE_OPENMP
#pragma omp parallel for shared(allDefined)
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(inAllDefined, t500[i], t700[i], rh700[i], t850[i], rh850[i], undef)) {
      // 850 hPa: rh,T -> Td ... rh*e(T) = e(Td) => Td = ?
      const float rh = calculations::clamp_rh(0.01 * rh850[i]);
      const float tc850 = cvt850 * t850[i] - t0;
      const float tc700 = cvt700 * t700[i] - t0;
      const ewt_calculator ewt850(tc850), ewt700(tc700);
      if (not (ewt850.defined() and ewt700.defined())) {
        kfield[i] = undef;
        allDefined = false;
      } else {
        const float etd850 = ewt850.value() * rh;
        const float tdc850 = ewt850.inverse(etd850);
        // 700 hPa: rh,T -> Td ... rh*e(T) = e(Td) => Td = ?
        const float rh = calculations::clamp_rh(0.01 * rh700[i]);
        const float etd700 = ewt700.value() * rh;
        const float tdc700 = ewt700.inverse(etd700);
        const float tc500 = cvt500 * t500[i] - t0;
        kfield[i] = (tc850 + tdc850) - (tc700 - tdc700) - tc500;
      }
    } else
      kfield[i] = undef;
  }
  return true;
}

// static
bool FieldFunctions::ductingIndex(int compute, int nx, int ny, const float *t850,
    const float *rh850, float *duct, float p850, bool& allDefined, float undef)
{
  //  Pressure levels:
  //     compute=1 ; temp. (Kelvin) og rel. fukt. (%)  -> ducting
  //     compute=2 ; pot. temp. og  relativ fukt. (%)  -> ducting
  //
  // ducting = nw(T) - nw(Td),   nw(t) = b*ew(t)/(t*t)
  //
  // NB! nivaaet er ikke fast, men normalt 850 hPa.
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  const float bduct = 3.8e+5;

  if (p850 <= 0.0)
    return false;

  float tconvert;
  switch (compute) {
  case 1: // T(Kelvin),RH(%) input
    tconvert = 1.;
    break;
  case 2: // TH,RH(%) input
    tconvert = calculations::pidcp_from_p(p850);
    break;
  default:
    return false;
  }

  const int fsize = nx * ny;
  const bool inAllDefined = allDefined;

#ifdef HAVE_OPENMP
#pragma omp parallel for shared(allDefined)
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(inAllDefined, t850[i], rh850[i], undef)) {
      // 850 hPa: rh,T -> Td ... rh*e(T) = e(Td)
      const float rh = calculations::clamp_rh(0.01 * rh850[i]);
      const float tk = t850[i] * tconvert;
      const ewt_calculator ewt(tk- t0);
      if (not ewt.defined()) {
        duct[i] = undef;
        allDefined = false;
      } else {
        const float et = ewt.value();
        const float etd = et * rh;
        const float tdk = ewt.inverse(etd) + t0;
        duct[i] = bduct * (et / (tk * tk) - etd / (tdk * tdk));
      }
    } else
      duct[i] = undef;
  }
  return true;
}

// static
bool FieldFunctions::showalterIndex(int compute, int nx, int ny, const float *t500,
    const float *t850, const float *rh850, float *sfield, float p500, float p850,
    bool& allDefined, float undef)
{
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif
  // Showalter index:  t500 - tx500,
  //                   tx500 er "t850" hevet toerr-adiabatisk
  //                   til metning og videre langs
  //                   fuktigadiabat til 500 mb.
  //
  // metode: Benytter vanlig (modell-) metode for beregning av
  //         fuktigadiabat, dvs. hever foerst langs toerr-adiabat og
  //         justerer fuktighet og varme i oeverste nivaa ved noen
  //         iterasjoner. Fuktigheten som justeres er her ikke
  //         metnings-verdien nederst. (beregner ikke p,t for
  //         nivaa hvor metning inntreffer.)
  //
  //    compute=1 : tPPP arrays contain temperature (Kelvin)
  //    compute=2 : tPPP arrays contain pot. temp. (Kelvin)
  //
  //  input:
  //    nx,ny        : field dimensions
  //     tPPP[nx*ny] : temp. (Kelvin)  or  pot.temp. (Kelvin)
  //    rhPPP[nx*ny] : rel. humidity (%)
  //    pPPP         : pressure (hPa)
  //  output:
  //    sfield[nx*ny] : Showalter index
  //
  // note: levels are not fixed

  // antall iterasjoner for beregning av fuktigadiabat
  // (relativt mange p.g.a. 850 - 500 hPa uten "mellom-nivaa")

  if (p500 <= 0.0 || p500 >= p850)
    return false;

  const float pi500 = calculations::pi_from_p(p500);
  const float pi850 = calculations::pi_from_p(p850);

  float cvt500, cvt850, dryadiabat;
  switch (compute) {
  case 1:
    cvt500 = 1.;
    cvt850 = 1.;
    dryadiabat = cp * (cp / pi850) * (pi500 / cp);
    break;
  case 2:
    cvt500 = pi500 / cp;
    cvt850 = pi850 / cp;
    dryadiabat = cp * (pi500 / cp);
    break;
  default:
    return false;
  }

  const int niter = 7;
  const int fsize = nx * ny;

  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(allDefined, t500[i], t850[i],  rh850[i], undef)) {
      const float tk500 = cvt500 * t500[i];
      const float tk850 = cvt850 * t850[i];
      const float rh = calculations::clamp_rh(0.01 * rh850[i]);
      const ewt_calculator ewt(tk850 - t0);
      if (not ewt.defined()) {
        sfield[i] = undef;
        allDefined = false;
      } else {
        const float etd = ewt.value() * rh;
        
        // fuktigadiabat  (tcl: cp * grader kelvin)
        // lift preliminary along dry-adiabat (pot.temp. constant)
        float tcl = dryadiabat * t850[i];
        float qcl = eps * etd / p850;
        
        // adjust humidity and moisture in 'niter' iterations
        for (int n = 0; n < niter; n++) {
          const ewt_calculator ewt2(tcl / cp - t0);
          if (not ewt2.defined())
            break;
          const float esat = ewt2.value();
          const float qsat = eps * esat / p500;
          float dq = qcl - qsat;
          const float a1 = cplr * qcl / tcl;
          const float a2 = exl / tcl;
          dq = dq / (1. + a1 * a2);
          qcl = qcl - dq;
          tcl = tcl + dq * xlh;
        }
        
        const float tx500 = tcl / cp;
        sfield[i] = tk500 - tx500;
      }
    } else
      sfield[i] = undef;
  }
  return true;
}

// static
bool FieldFunctions::boydenIndex(int compute, int nx, int ny, const float *t700,
    const float *z700, const float *z1000, float *bfield, float p700, float p1000,
    bool& allDefined, float undef)
{
  // Boyden index:  (Z700-Z1000)/10 - Tc700 - 200.
  //
  //    compute=1 : tPPP arrays contain temperature (Kelvin)
  //    compute=2 : tPPP arrays contain pot. temp. (Kelvin)
  //
  //  input:
  //    nx,ny        : field dimensions
  //    tPPP[nx*ny]  : temp. (Kelvin)  or  pot.temp. (Kelvin)
  //    zPPP[nx*ny]  : height (m)
  //    pPPP         : pressure (hPa)
  //  output:
  //    bfield[nx*ny] : Boyden index
  //
  // note: levels are not fixed
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  if (compute <= 0 or compute >= 3)
    return false;

  if (p700 <= 0.0 || p700 >= p1000)
    return false;

  const float pi700 = cp * powf(p700 / p0, r / cp);
  const float tconv = (compute == 2) ? pi700 / cp : 1;
  const int fsize = nx * ny;

#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(allDefined, t700[i], z700[i], z1000[i], undef)) {
      const float tc700 = t700[i] * tconv - t0;
      bfield[i] = (z700[i] - z1000[i]) / 10. - tc700 - 200.;
    } else {
      bfield[i] = undef;
    }
  }
  return true;
}

//---------------------------------------------------
// hybrid model level (HLEVEL) functions
//---------------------------------------------------

// static
bool FieldFunctions::hleveltemp(int compute, int nx, int ny, const float *tinp,
    const float *ps, float *tout, float alevel, float blevel, bool& allDefined,
    float undef, const std::string& unit)
{
  //  Modell-flater, sigma/eta(hybrid):
  //    sigma: alevel=ptop*(1.-sigma)   blevel=sigma  (p=pt+(ps[]-pt)*sigma)
  //    eta:   alevel,blevel
  //    p = alevel + blevel * ps[]
  //
  //     compute=1 : potensiell temp. -> temp. (grader Celsius)
  //     compute=2 : potensiell temp. -> temp. (grader Kelvin)
  //     compute=3 : temp. (grader Kelvin)  -> potensiell temp. (Kelvin)
  //     compute=4 : temp. (grader Kelvin) -> saturated equivalent pot.temp.
  //     compute=5 : potensiell temp.      -> saturated equivalent pot.temp.
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  if (compute < 3) {
    if (unit == "celsius")
      compute = 1;
    else if (unit == "kelvin")
      compute = 2;
  }

  const int fsize = nx * ny;
  const bool inAllDefined = allDefined;

  if (calculations::bad_hlevel(alevel, blevel)) {
    METLIBS_LOG_ERROR("returning false, aHybrid or bHybrid not ok. aHybrid:"<<alevel<<" bHybrid:"<<blevel);
    return false;
  }

#ifdef HAVE_OPENMP
#pragma omp parallel for shared(allDefined)
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(inAllDefined, tinp[i], ps[i], undef)) {
      const float p = calculations::p_hlevel(ps[i], alevel, blevel);
      const float pidcp = calculations::pidcp_from_p(p);
      if (compute == 1) { // TH -> T(Celsius)
        tout[i] = tinp[i] * pidcp - t0;
      } else if (compute == 2) { // TH -> T(Kelvin)
        tout[i] = tinp[i] * pidcp;
      } else if (compute == 3) { // T(Kelvin) -> TH
        tout[i] = tinp[i] / pidcp;
      } else if (compute == 4) { // T(Kelvin) -> THESAT
        tout[i] = calculations::t_thesat(tinp[i], p, pidcp*cp, undef, allDefined);
      } else if (compute == 5) { // TH -> THESAT
        tout[i] = calculations::th_thesat(tinp[i], p, pidcp*cp, undef, allDefined);
      }
    } else
      tout[i] = undef;
  }
  return true;
}

// static
bool FieldFunctions::hlevelthe(int compute, int nx, int ny, const float *t, const float *q,
    const float *ps, float *the, float alevel, float blevel, bool& allDefined, float undef)
{

#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  //  Modell-flater, sigma/eta(hybrid):
  //    sigma: alevel=ptop*(1.-sigma)   blevel=sigma  (p=pt+(ps[]-pt)*sigma)
  //    eta:   alevel,blevel
  //       p = alevel + blevel * ps[]
  //
  //    compute=1 ; temp. (Kelvin)  og q     -> THE, ekvivalent pot.temp (Kelvin)
  //    compute=2 ; pot. temp. (Kelvin) og q -> THE, ekvivalent pot.temp (Kelvin)
  //
  //  input:
  //    nx,ny      : field dimensions
  //    t[nx*ny]   : temp. (Kelvin or Celsius)  or  pot.temp. (Kelvin)
  //    q[nx*ny]   : specific humidity (kg/kg)
  //    ps[nx*ny]  : surface pressure (hPa)
  //  output:
  //    the[nx*ny] : equivale pot. temp. (Kelvin)

  const int fsize = nx * ny;

  if (calculations::bad_hlevel(alevel, blevel)) {
    METLIBS_LOG_ERROR("returning false, aHybrid or bHybrid not ok. aHybrid:"<<alevel<<" bHybrid:"<<blevel);
    return false;
  }

#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(allDefined, t[i], q[i], ps[i], undef)) {
      const float p = calculations::p_hlevel(ps[i], alevel, blevel);
      const float pi = calculations::pi_from_p(p);
      if (compute == 1) // T(Kelvin), q -> THE
        the[i] = (t[i] * cp + q[i] * xlh) / pi;
      else if (compute == 2) // TH, q -> THE
        the[i] = t[i] + q[i] * xlh / pi;
    } else {
      the[i] = undef;
    }
  }
  return true;
}

// static
bool FieldFunctions::hlevelhum(int compute, int nx, int ny, const float *t,
    const float *huminp, const float *ps, float *humout, float alevel, float blevel,
    bool& allDefined, float undef, const std::string& unit)
{
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif
  //  Modell-flater, sigma/eta(hybrid):
  //    sigma: alevel=ptop*(1.-sigma)   blevel=sigma  (p=pt+(ps[]-pt)*sigma)
  //    eta:   alevel,blevel
  //       p = alevel + blevel * ps[]
  //
  //     compute=1  : temp. (Kelvin) og spes. fukt. -> rel. fuktighet (%)
  //     compute=2  : pot. temp. og spesifikk fukt. -> rel. fuktighet (%)
  //     compute=3  : temp. (Kelvin) og rel. fukt.  -> spesifikk fukt. (kg/kg)
  //     compute=4  : pot. temp. og  relativ fukt.  -> spesifikk fukt. (kg/kg)
  //     compute=5  : temp. (Kelvin) og spes. fukt. -> duggpunkt, Td (Celsius)
  //     compute=6  : pot. temp. og spesifikk fukt. -> duggpunkt, Td (Celsius)
  //     compute=7  : temp. (Kelvin) og rel. fukt.  -> duggpunkt, Td (Celsius)
  //     compute=8  : pot. temp. og  relativ fukt.  -> duggpunkt, Td (Celsius)
  //     compute=9  : temp. (Kelvin) og spes. fukt. -> duggpunkt, Td (Kelvin)
  //     compute=10 : pot. temp. og spesifikk fukt. -> duggpunkt, Td (Kelvin)
  //     compute=11 : temp. (Kelvin) og rel. fukt.  -> duggpunkt, Td (Kelvin)
  //     compute=12 : pot. temp. og  relativ fukt.  -> duggpunkt, Td (Kelvin)
  //
  //  Note: compute=7,11 are pressure independant (and not needed here)

  if (compute <= 0 or compute >= 13)
    return false;
  if (calculations::bad_hlevel(alevel, blevel)) {
    METLIBS_LOG_ERROR("returning false, aHybrid or bHybrid not ok. aHybrid:"<<alevel<<" bHybrid:"<<blevel);
    return false;
  }

  if (compute > 8 && unit == "celsius")
    compute -= 4;
  else if (compute > 4 && compute <= 8 && unit == "kelvin")
    compute += 4;

  const int fsize = nx * ny;
  const bool inAllDefined = allDefined;
  const float tdconv = (compute >= 9) ? t0 : 0;
  const bool need_p = not (compute == 7 or compute == 11);

#ifdef HAVE_OPENMP
#pragma omp parallel for shared(allDefined)
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(inAllDefined, t[i], huminp[i], undef)
        and ((not need_p) or inAllDefined or ps[i] != undef))
    {
      const float p = need_p ? calculations::p_hlevel(ps[i], alevel, blevel) : 0;
      if (compute == 1) { // T(Kelvin),q -> RH(%)
        humout[i] = calculations::tk_q_rh(t[i], huminp[i], p, undef, allDefined);
      } else if (compute == 2) { // TH,q -> RH(%)
        const float tk = t[i] * calculations::pidcp_from_p(p);
        humout[i] = calculations::tk_q_rh(tk, huminp[i], p, undef, allDefined);
      } else if (compute == 3) { // T(Kelvin),RH(%) -> q
        humout[i] = calculations::tk_rh_q(t[i], huminp[i], p, undef, allDefined);
      } else if (compute == 4) { // TH,RH(%) -> q
        const float tk = t[i] * calculations::pidcp_from_p(p);
        humout[i] = calculations::tk_rh_q(tk, huminp[i], p, undef, allDefined);
      } else if (compute == 5 or compute == 9) { // T(Kelvin),q -> Td(Celsius) or oT(Kelvin),q -> Td(Kelvin)
        humout[i] = calculations::tk_q_td(t[i], huminp[i], p, tdconv, undef, allDefined);
      } else if (compute == 6 or compute == 10) { // TH,q -> Td(Celsius)  or  TH,q -> Td(Kelvin)
        const float tk = t[i] * calculations::pidcp_from_p(p);
        humout[i] = calculations::tk_q_td(tk, huminp[i], p, tdconv, undef, allDefined);
      } else if (compute == 7 or compute == 11) { // T(Kelvin),RH(%) -> Td(Celsius)  or  T(Kelvin),RH(%) -> Td(Kelvin)
        humout[i] = calculations::tk_rh_td(t[i], huminp[i], tdconv, undef, allDefined);
      } else if (compute == 8 or compute == 12) { // TH,RH(%) -> Td(Celsius)  or  TH,RH(%) -> Td(Kelvin)
        const float tk = t[i] * calculations::pidcp_from_p(p);
        humout[i] = calculations::tk_rh_td(tk, huminp[i], tdconv, undef, allDefined);
      }
    } else {
      humout[i] = undef;
    }
  }
  return true;
}

// static
bool FieldFunctions::hlevelducting(int compute, int nx, int ny, const float *t,
    const float *h, const float *ps, float *duct, float alevel, float blevel,
    bool& allDefined, float undef)
{
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif
  //  Modell-flater, sigma/eta(hybrid):
  //    sigma: alevel=ptop*(1.-sigma)   blevel=sigma  (p=pt+(ps[]-pt)*sigma)
  //    eta:   alevel,blevel
  //       p = alevel + blevel * ps[]
  //
  //    compute=1 ; temp. (Kelvin)      and q (kg/kg)  -> ducting
  //    compute=2 ; pot. temp. (Kelvin) and q (kg/kg)  -> ducting
  //    compute=3 ; temp. (Kelvin)      and RH(%)      -> ducting
  //    compute=4 ; pot. temp. (Kelvin) and RH(%)      -> ducting
  //
  //  input:
  //    nx,ny      : field dimensions
  //    t[nx*ny]   : temp. (Kelvin)  or  pot.temp. (Kelvin)
  //    h[nx*ny]   : specific humidity (kg/kg) or rel.humidity (%)
  //    ps[nx*ny]  : surface pressure (hPa)
  //  output:
  //    duct[nx*ny]: ducting
  //...................................................................
  //       t in unit Kelvin, p in unit hPa ???
  //       duct=77.6*(p/t)+373000.*(q*p)/(eps*t*t)
  //       q*p/eps = rh*qsat*p/eps = rh*(eps*e(t)/p)*p/eps
  //               = rh*e(t) = (e(td)/e(t))*e(t) = e(td)
  //       => duct = 77.6*(p/t)+373000.*e(td)/(t*t)
  //          duct = 77.6*(p/t)+373000.*rh*e(t)/(t*t)
  //...................................................................

  if (calculations::bad_hlevel(alevel, blevel)) {
    METLIBS_LOG_ERROR("returning false, aHybrid or bHybrid not ok. aHybrid:"<<alevel<<" bHybrid:"<<blevel);
    return false;
  }

  const int fsize = nx * ny;
  const bool inAllDefined = allDefined;

#ifdef HAVE_OPENMP
#pragma omp parallel for shared(allDefined)
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(inAllDefined, t[i], h[i], ps[i], undef)) {
      const float p = calculations::p_hlevel(ps[i], alevel, blevel);
      float tk = t[i];
      if (compute % 2 == 0)
        tk *= calculations::pidcp_from_p(p);
      if (compute == 1 or compute == 2) { // T(Kelvin),q -> ducting  or  TH,q -> ducting
        duct[i] = calculations::tk_q_duct(t[i], h[i], p);
      } else if (compute == 3 or compute == 4) { // T(Kelvin),RH(%) -> ducting  or TH,RH(%) -> ducting
        duct[i] = calculations::tk_rh_duct(t[i], h[i], p, undef, allDefined);
      }
    } else
      duct[i] = undef;
  }
  return true;
}

// static
bool FieldFunctions::hlevelpressure(int nx, int ny, const float *ps, float *p,
    float alevel, float blevel, bool& allDefined, float undef)
{
  //  Model levels, eta(hybrid) or norlam_sigma:
  //    eta:   alevel,blevel    p = alevel + blevel * ps[]
  //    sigma: alevel=ptop*(1.-sigma)   blevel=sigma  (p=pt+(ps[]-pt)*sigma)
  //
  //  input:
  //    ps[nx*ny]  : surface pressure (hPa)
  //  output:
  //    p[nx*ny]: pressure (hPa)
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif
  if (calculations::bad_hlevel(alevel, blevel))
    return false;

  const int fsize = nx * ny;

#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(allDefined, ps[i], undef))
      p[i] = calculations::p_hlevel(ps[i], alevel, blevel);
    else
      p[i] = undef;
  }
  return true;
}

//---------------------------------------------------
// atmospheric model level (ALEVEL) functions
//---------------------------------------------------

// static
bool FieldFunctions::aleveltemp(int compute, int nx, int ny, const float *tinp,
    const float *p, float *tout, bool& allDefined, float undef, const std::string& unit)
{
  //  Ukjente modell-flater, gitt trykk (p):
  //
  //     compute=1 ; potensiell temp. -> temp. (grader Celsius)
  //     compute=2 ; potensiell temp. -> temp. (grader Kelvin)
  //     compute=3 ; temp. (grader Kelvin)  -> potensiell temp. (Kelvin)
  //     compute=4 ; temp. (grader Kelvin) -> saturated equivalent pot.temp.
  //     compute=5 ; potensiell temp.      -> saturated equivalent pot.temp.
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif
  if (compute <= 0 or compute >= 6)
    return false;

  if (compute < 3) {
    if (unit == "celsius")
      compute = 1;
    else if (unit == "kelvin")
      compute = 2;
  }

  const int fsize = nx * ny;
  const bool inAllDefined = allDefined;

#ifdef HAVE_OPENMP
#pragma omp parallel for shared(allDefined)
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(inAllDefined, tinp[i], p[i], undef)) {
      if (compute == 1) { // TH -> T(Celsius)
        tout[i] = tinp[i] * calculations::pidcp_from_p(p[i]) - t0;
      } else if (compute == 2) { // TH -> T(Kelvin)
        tout[i] = tinp[i] * calculations::pidcp_from_p(p[i]);
      } else if (compute == 3) { // T(Kelvin) -> TH
        tout[i] = tinp[i] / calculations::pidcp_from_p(p[i]);
      } else if (compute == 4) { // T(Kelvin) -> THESAT
        tout[i] = calculations::t_thesat(tinp[i], p[i], calculations::pi_from_p(p[i]), undef, allDefined);
      } else if (compute == 5) { // TH -> THESAT
        tout[i] = calculations::th_thesat(tinp[i], p[i], calculations::pi_from_p(p[i]), undef, allDefined);
      }
    } else {
      tout[i] = undef;
    }
  }
  return true;
}

// static
bool FieldFunctions::alevelthe(int compute, int nx, int ny, const float *t, const float *q,
    const float *p, float *the, bool& allDefined, float undef)
{
  //  Ukjente modell-flater, gitt trykk (p):
  //
  //    compute=1 ; temp. (Kelvin)  og q     -> THE, ekvivalent pot.temp (Kelvin)
  //    compute=2 ; pot. temp. (Kelvin) og q -> THE, ekvivalent pot.temp (Kelvin)
  //
  //  input:
  //    nx,ny      : field dimensions
  //    t[nx*ny]   : temp. (Kelvin)  or  pot.temp. (Kelvin)
  //    q[nx*ny]   : specific humidity (kg/kg)
  //    p[nx*ny]   : pressure (hPa)
  //  output:
  //    the[nx*ny] : equivalent pot. temp. (Kelvin)
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  if (compute != 1 and compute != 2)
    return false;

  const int fsize = nx * ny;

#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(allDefined, t[i], q[i], p[i], undef)) {
      const float pi = calculations::pi_from_p(p[i]);
      if (compute == 1) // T(Kelvin), q -> THE
        the[i] = (t[i] * cp + q[i] * xlh) / pi;
      else if (compute == 2) // TH, q -> THE
        the[i] = t[i] + q[i] * xlh / pi;
    } else {
      the[i] = undef;
    }
  }
  return true;
}

// static
bool FieldFunctions::alevelhum(int compute, int nx, int ny, const float *t,
    const float *huminp, const float *p, float *humout, bool& allDefined,
    float undef, const std::string& unit)
{
  //  Ukjente modell-flater, gitt trykk (p):
  //
  //     compute=1  : temp. (Kelvin) og spes. fukt. -> rel. fuktighet (%)
  //     compute=2  : pot. temp. og spesifikk fukt. -> rel. fuktighet (%)
  //     compute=3  : temp. (Kelvin) og rel. fukt.  -> spesifikk fukt. (kg/kg)
  //     compute=4  : pot. temp. og  relativ fukt.  -> spesifikk fukt. (kg/kg)
  //     compute=5  : temp. (Kelvin) og spes. fukt. -> duggpunkt, Td (Celsius)
  //     compute=6  : pot. temp. og spesifikk fukt. -> duggpunkt, Td (Celsius)
  //     compute=7  : temp. (Kelvin) og rel. fukt.  -> duggpunkt, Td (Celsius)
  //     compute=8  : pot. temp. og  relativ fukt.  -> duggpunkt, Td (Celsius)
  //     compute=9  : temp. (Kelvin) og spes. fukt. -> duggpunkt, Td (Kelvin)
  //     compute=10 : pot. temp. og spesifikk fukt. -> duggpunkt, Td (Kelvin)
  //     compute=11 : temp. (Kelvin) og rel. fukt.  -> duggpunkt, Td (Kelvin)
  //     compute=12 : pot. temp. og  relativ fukt.  -> duggpunkt, Td (Kelvin)
  //
  //  Note: compute=7,11 are pressure independant (and not needed here)
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif
  if (compute <= 0 or compute >= 13)
    return false;

  if (compute > 8 && unit == "celsius")
    compute -= 4;
  else if (compute > 4 && compute <= 8 && unit == "kelvin")
    compute += 4;

  const int fsize = nx * ny;
  const float tdconv = (compute >= 9) ? t0 : 0;
  const bool inAllDefined = allDefined;

#ifdef HAVE_OPENMP
#pragma omp parallel for shared(allDefined)
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(inAllDefined, t[i], huminp[i], undef)
        and ((compute != 7 and compute != 11) or inAllDefined or p[i] != undef))
    {
      if (compute == 1) { // T(Kelvin),q -> RH(%)
        humout[i] = calculations::tk_q_rh(t[i], huminp[i], p[i], undef, allDefined);
      } else if (compute == 2) { // TH,q -> RH(%)
        const float tk = t[i] * calculations::pidcp_from_p(p[i]);
        humout[i] = calculations::tk_q_rh(tk, huminp[i], p[i], undef, allDefined);
      } else if (compute == 3) { // T(Kelvin),RH(%) -> q
        humout[i] = calculations::tk_rh_q(t[i], huminp[i], p[i], undef, allDefined);
      } else if (compute == 4) { // TH,RH(%) -> q
        const float tk = t[i] * calculations::pidcp_from_p(p[i]);
        humout[i] = calculations::tk_rh_q(tk, huminp[i], p[i], undef, allDefined);
      } else if (compute == 5 or compute == 9) { // T(Kelvin),q -> Td(Celsius)  or  T(Kelvin),q -> Td(Kelvin)
        humout[i] = calculations::tk_q_td(t[i], huminp[i], p[i], tdconv, undef, allDefined);
      } else if (compute == 6 or compute == 10) { // TH,q -> Td(Celsius)  or  TH,q -> Td(Kelvin)
        const float tk = t[i] * calculations::pidcp_from_p(p[i]);
        humout[i] = calculations::tk_q_td(tk, huminp[i], p[i], tdconv, undef, allDefined);
      } else if (compute == 7 or compute == 11) { // T(Kelvin),RH(%) -> Td(Celsius)  or  T(Kelvin),RH(%) -> Td(Kelvin)
        humout[i] = calculations::tk_rh_td(t[i], huminp[i], tdconv, undef, allDefined);
      } else if (compute == 8 or compute == 12) { // TH,RH(%) -> Td(Celsius)  or  TH,RH(%) -> Td(Kelvin)
        const float tk = t[i] * calculations::pidcp_from_p(p[i]);
        humout[i] = calculations::tk_rh_td(tk, huminp[i], tdconv, undef, allDefined);
      }
    } else {
      humout[i] = undef;
    }
  }
  return true;
}

// static
bool FieldFunctions::alevelducting(int compute, int nx, int ny, const float *t,
    const float *h, const float *p, float *duct, bool& allDefined, float undef)
{
  //  Unknown atmospheric level, with input pressure field:
  //
  //    compute=1 ; temp. (Kelvin)      and q (kg/kg)  -> ducting
  //    compute=2 ; pot. temp. (Kelvin) and q (kg/kg)  -> ducting
  //    compute=3 ; temp. (Kelvin)      and RH(%)      -> ducting
  //    compute=4 ; pot. temp. (Kelvin) and RH(%)      -> ducting
  //
  //  input:
  //    nx,ny      : field dimensions
  //    t[nx*ny]   : temp. (Kelvin)  or  pot.temp. (Kelvin)
  //    h[nx*ny]   : specific humidity (kg/kg) or rel.humidity (%)
  //    p[nx*ny]   : pressure (hPa)
  //  output:
  //    duct[nx*ny]: ducting
  //...................................................................
  //       t in unit Kelvin, p in unit hPa, q in unit kg/kg
  //       duct=77.6*(p/t)+373000.*(q*p)/(eps*t*t)
  //       q*p/eps = rh*qsat*p/eps = rh*(eps*e(t)/p)*p/eps
  //               = rh*e(t) = (e(td)/e(t))*e(t) = e(td)
  //       => duct = 77.6*(p/t)+373000.*e(td)/(t*t)
  //          duct = 77.6*(p/t)+373000.*rh*e(t)/(t*t)
  //...................................................................
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  const int fsize = nx * ny;
  const bool inAllDefined = allDefined;

#ifdef HAVE_OPENMP
#pragma omp parallel for shared(allDefined)
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(inAllDefined, t[i], h[i], p[i], undef)) {
      float tk = t[i];
      if (compute % 2 == 0)
        tk *= calculations::pidcp_from_p(p[i]);
      if (compute == 1 or compute == 2) { // T(Kelvin),q -> ducting  or  TH,q -> ducting
        duct[i] = calculations::tk_q_duct(tk, h[i], p[i]);
      } else if (compute == 3 or compute == 4) { // T(Kelvin),RH(%) -> ducting  or TH,RH(%) -> ducting
        duct[i] = calculations::tk_rh_duct(tk, h[i], p[i], undef, allDefined);
      }
    } else {
      duct[i] = undef;
    }
  }
  return true;
}

//---------------------------------------------------
// isentropic level (ILEVEL) function
//---------------------------------------------------

// static
bool FieldFunctions::ilevelgwind(int nx, int ny, const float *mpot, float *ug,
    float *vg, const float *xmapr, const float *ymapr, const float *fcoriolis, bool& allDefined,
    float undef)
{
  //  Geostophic wind in isentropic level
  //  (centered differences)
  //
  //  isentropic level, z=Montgomery potential (M=cp*T+g*z)
  //
  //  input:  mpot[nx*ny] : Montgomery potential in isentropic level
  //          xmapr[nx*ny],ymapr[nx*ny] : map ratios (xm/(hx*2),ym/(hy*2))
  //          fcoriolis[nx*ny] : coriolis parameter
  //  output: ug[nx*ny] : x component of geostrophic wind (m/s)
  //          vg[nx*ny] : y component of geostrophic wind (m/s)
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  const int fsize = nx * ny;

  if (nx < 3 || ny < 3)
    return false;

  if (allDefined) {
    // loop extended, reset bad computations at boundaries later
    for (int i = nx; i < fsize - nx; i++) {
      ug[i] = -0.5 * ymapr[i] * (mpot[i + nx] - mpot[i - nx]) / fcoriolis[i];
      vg[i] = 0.5 * xmapr[i] * (mpot[i + 1] - mpot[i - 1]) / fcoriolis[i];
    }
  } else {
    for (int i = nx; i < fsize - nx; i++) {
      if (mpot[i - nx] != undef && mpot[i - 1] != undef && mpot[i + 1] != undef
          && mpot[i + nx] != undef) {
        ug[i] = -0.5 * ymapr[i] * (mpot[i + nx] - mpot[i - nx]) / fcoriolis[i];
        vg[i] = 0.5 * xmapr[i] * (mpot[i + 1] - mpot[i - 1]) / fcoriolis[i];
      } else {
        ug[i] = undef;
        vg[i] = undef;
      }
    }
  }

  // fill in edge values not computed (or badly computed) above
  fillEdges(nx, ny, ug);
  fillEdges(nx, ny, vg);
  return true;
}

//---------------------------------------------------
// ocean depth level (OZLEVEL) functions
//---------------------------------------------------

// static
bool FieldFunctions::seaSoundSpeed(int compute, int nx, int ny, const float *t,
    const float *s, float *soundspeed, float z_, bool& allDefined, float undef)
{
  //-------------------------------------------------------------
  //       D. Ross: "Revised simplified formulae
  //                 for calculating the speed of
  //                 sound in sea water"
  //       SACLANTCEN Memorandum SM-107 15 Mar 78
  //-------------------------------------------------------------
  //
  //    compute=1 ; temp. (Celsius) and salinity (ppt)
  //    compute=2 ; temp. (Kelvin)  and salinity (ppt)
  //
  //  input:
  //    nx,ny      : field dimensions
  //    t[nx*ny]   : temp. (Kelvin)  or  temp. (Celsius)
  //    s[nx*ny]   : salinity (ppt)
  //    z          : depth in meter (constant)
  //  output:
  //    soundspeed[nx*ny]: sea water sound speed in m/s
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  if (compute != 1 and compute != 2)
    return false;

  const int fsize = nx * ny;

  const float tconv = (compute == 1) ? 0 : t0;
  const double Z = fabsf(z_);
  const double Cz = 0.01635 * Z + 0.000000175 * Z * Z;

#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(allDefined, t[i], s[i], undef)) {
      const float T = t[i] - tconv;
      const float S = s[i];
      const double Ct = 4.565 * T - 0.0517 * T * T + 0.000221 * T * T * T;
      const double Cs = (1.338 - 0.013 * T + 0.0001 * T * T) * (S - 35.0);
      const double speed = 1449.1 + Ct + Cs + Cz;
      soundspeed[i] = float(speed);
    } else {
      soundspeed[i] = undef;
    }
  }
  return true;
}

//---------------------------------------------------
// level (vertical coordinate) independant functions
//---------------------------------------------------

// static
bool FieldFunctions::cvtemp(int compute, int nx, int ny, const float *tinp,
    float *tout, bool& allDefined, float undef)
{
  // compute=1 : Temperature from degrees Kelvin to degrees Celsius
  // compute=2 : Temperature from degrees Celsius to degrees Kelvin
  // compute=3 : Temperature from degrees Kelvin to degrees Celsius
  //             if input seams to be Kelvin
  // compute=4 : Temperature from degrees Celsius to degrees Kelvin
  //             if input seams to be Celsius
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  int fsize = nx * ny;
  float tconvert;

  switch (compute) {
  case 1:
    tconvert = -t0;
    break;
  case 2:
    tconvert = +t0;
    break;
  case 3:
    tconvert = -t0;
    break;
  case 4:
    tconvert = +t0;
    break;
  default:
    return false;
  }

  if (compute == 3 || compute == 4) {
    float tavg = 0.;
    if (allDefined) {
#ifdef HAVE_OPENMP
#pragma omp parallel
#endif
      { // paralell begins, note to part sum ltavg
        float ltavg = 0.;
#ifdef HAVE_OPENMP
#pragma omp for private(ltavg)
#endif
        for (int i = 0; i < fsize; i++)
          ltavg += tinp[i];
#ifdef HAVE_OPENMP
#pragma omp atomic
#endif
        tavg += ltavg;
      } // parallel ends here
      tavg /= float(fsize);
    } else {
      int navg = 0;
#ifdef HAVE_OPENMP
#pragma omp parallel
#endif
      {
        float ltavg = 0.;
        int lnavg = 0;
#ifdef HAVE_OPENMP
#pragma omp for private(ltavg,lnavg)
#endif
        for (int i = 0; i < fsize; i++) {
          if (tinp[i] != undef) {
            ltavg += tinp[i];
            lnavg++;
          }
        }
#ifdef HAVE_OPENMP
#pragma omp atomic
#endif
        tavg += ltavg;
        navg += lnavg;
      } // parallel end here
      if (navg > 0)
        tavg /= float(navg);
    }
    if ((compute == 3 && tavg < t0 / 2.) || (compute == 4 && tavg > t0 / 2.)) {
      if (&tout[0] != &tinp[0]) {
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
        for (int i = 0; i < fsize; i++)
          tout[i] = tinp[i];
      }
      return true;
    }
  }

  if (allDefined) {
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < fsize; i++)
      tout[i] = tinp[i] + tconvert;
  } else {
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < fsize; i++) {
      if (tinp[i] != undef)
        tout[i] = tinp[i] + tconvert;
      else
        tout[i] = undef;
    }
  }
  return true;
}

// static
bool FieldFunctions::cvhum(int compute, int nx, int ny, const float *t,
    const float *huminp, float *humout, bool& allDefined, float undef)
{
  //     compute=1 : temp. (Kelvin)  og rel. fukt.  -> duggpunkt, Td (Kelvin)
  //     compute=2 : temp. (Kelvin)  og rel. fukt.  -> duggpunkt, Td (Celsius)
  //     compute=3 : temp. (Celsius) og rel. fukt.  -> duggpunkt, Td (Celsius)
  //     compute=4 : temp. og duggpunkt (Kelvin)    -> rel.fuktighet (%)
  //     compute=5 : temp. og duggpunkt (Celsius)   -> rel.fuktighet (%)
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  const int fsize = nx * ny;
  const float tconv =  (compute == 1 || compute == 2 || compute == 4) ? t0 : 0;
  const float tdconv = (compute == 1) ? t0 : 0;
  const bool inAllDefined = allDefined;

  switch (compute) {

  case 1: // T(Kelvin),RH(%)  -> Td(Kelvin)
  case 2: // T(Kelvin),RH(%)  -> Td(Celsius)
  case 3: // T(Celsius),RH(%) -> Td(Celsius)
    for (int i = 0; i < fsize; i++) {
      if (calculations::is_defined(inAllDefined, t[i], huminp[i], undef)) {
        const ewt_calculator ewt(t[i] - tconv);
        if (not ewt.defined()) {
          humout[i] = undef;
          allDefined = false;
        } else {
          const float et = ewt.value();
          const float rh = calculations::clamp_rh(0.01 * huminp[i]);
          const float etd = rh * et;
          const float tdc = ewt.inverse(etd);
          humout[i] = tdc + tdconv;
        }
      } else {
        humout[i] = undef;
      }
    }
    break;

  case 4: // T(Kelvin),Td(Kelvin)   -> RH(%)
  case 5: // T(Celsius),Td(Celsius) -> RH(%)
    for (int i = 0; i < fsize; i++) {
      if (calculations::is_defined(inAllDefined, t[i], huminp[i], undef)) {
        const ewt_calculator ewt(t[i] - tconv), ewt2(huminp[i] - tconv);
        if (not (ewt.defined() and ewt2.defined())) {
          humout[i] = undef;
          allDefined = false;
        } else {
          const float et = ewt.value();
          const float etd = ewt2.value();
          const float rh = etd / et;
          humout[i] = rh * 100.;
        }
      } else
        humout[i] = undef;
    }
    break;

  default:
    return false;
  }
  return true;
}

// static
bool FieldFunctions::vectorabs(int nx, int ny, const float *u, const float *v,
    float *ff, bool& allDefined, float undef)
{
  //  ff = sqrt(u*u+v*v)
  //
  //  input:  u[nx*ny],v[nx*ny] : vector/wind components
  //  output: ff[nx*ny]
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif
  const int fsize = nx * ny;

#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(allDefined, u[i], v[i], undef))
      ff[i] = sqrtf(u[i] * u[i] + v[i] * v[i]);
    else
      ff[i] = undef;
  }
  return true;
}

// static
bool FieldFunctions::direction(int nx, int ny, float *u, float *v, const Area& area,
    float *dd, bool& allDefined, float undef)
{
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif
  // transform u,x to a geographic grid
  GridConverter gc;
  gc.xyv2geo(area, nx, ny, u, v);

  //  input:  u[nx*ny],v[nx*ny] : vector/wind components
  //  output: dd[nx*ny]
  const float deg = 180. / 3.141592654;

  const int npos = nx * ny;

#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
  for (int i = 0; i < npos; i++) {
    if (calculations::is_defined(allDefined, u[i], v[i], undef)) {
      const float ff = sqrtf(u[i] * u[i] + v[i] * v[i]);
      if (ff > 0.0001) {
        dd[i] = 270. - deg * atan2(v[i], u[i]);
        if (dd[i] > 360)
          dd[i] -= 360;
        if (dd[i] < 0)
          dd[i] += 360;
      } else
        dd[i] = 0;
    } else
      dd[i] = undef;
  }
  return true;
}

// static
bool FieldFunctions::relvort(int nx, int ny, const float *u, const float *v, float *rvort,
    const float *xmapr, const float *ymapr, bool& allDefined, float undef)
{
  //  Relative vorticity from u and v (centered differences)
  //
  //  input:  u[nx*ny],v[nx*ny] : wind components
  //          xmapr[nx*ny],ymapr[nx*ny] : map ratios (xm/(hx*2),ym/(hy*2))
  //  output: rvort[nx*ny]
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif
  if (nx < 3 || ny < 3)
    return false;

  const int fsize = nx * ny;

  // loop extended, reset bad computations at boundaries later
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
  for (int i = nx; i < fsize - nx; i++) {
    if (calculations::is_defined(allDefined, v[i-1], v[i+1], u[i-nx], u[i+nx], undef))
      rvort[i] = 0.5 * xmapr[i] * (v[i+1] - v[i-1]) - 0.5 * ymapr[i] * (u[i+nx] - u[i-nx]);
    else
      rvort[i] = undef;
  }

  // fill in edge values not computed (or badly computed) above
  fillEdges(nx, ny, rvort);
  return true;
}

// static
bool FieldFunctions::absvort(int nx, int ny, const float *u, const float *v, float *avort,
    const float *xmapr, const float *ymapr, const float *fcoriolis, bool& allDefined, float undef)
{
  //  Absolute vorticity from u and v (centered differences)
  //
  //  input:  u[nx*ny],v[nx*ny] : wind components
  //          xmapr[nx*ny],ymapr[nx*ny] : map ratios (xm/(hx*2),ym/(hy*2))
  //          fcoriolis[nx*ny] : coriolis parameter
  //  output: avort[nx*ny]
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  if (nx < 3 || ny < 3)
    return false;

  const int fsize = nx * ny;

  // loop extended, reset bad computations at boundaries later
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
  for (int i = nx; i < fsize - nx; i++) {
    if (calculations::is_defined(allDefined, v[i-1], v[i+1], u[i-nx], u[i+nx], undef))
      avort[i] = 0.5 * xmapr[i] * (v[i+1] - v[i-1]) - 0.5 * ymapr[i] * (u[i+nx] - u[i-nx]) + fcoriolis[i];
    else
      avort[i] = undef;
  }

  // fill in edge values not computed (or badly computed) above
  fillEdges(nx, ny, avort);

  return true;
}

// static
bool FieldFunctions::divergence(int nx, int ny, const float *u, const float *v,
    float *diverg, const float *xmapr, const float *ymapr, bool& allDefined, float undef)
{
  //  Divergence from u and v (centered differences)
  //
  //  input:  u[nx*ny],v[nx*ny] : wind components
  //          xmapr[nx*ny],ymapr[nx*ny] : map ratios (xm/(hx*2),ym/(hy*2))
  //  output: diverg[nx*ny]
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  if (nx < 3 || ny < 3)
    return false;

  const int fsize = nx * ny;
  // loop extended, reset bad computations at boundaries later
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
  for (int i = nx; i < fsize - nx; i++) {
    if (calculations::is_defined(allDefined, v[i-1], v[i+1], u[i-nx], u[i+nx], undef))
      diverg[i] = 0.5 * xmapr[i] * (u[i+1] - u[i-1]) + 0.5 * ymapr[i] * (v[i+nx] - v[i-nx]);
    else
      diverg[i] = undef;
  }

  // fill in edge values not computed (or badly computed) above
  fillEdges(nx, ny, diverg);

  return true;
}

// static
bool FieldFunctions::advection(int nx, int ny, const float *f, const float *u, const float *v,
    float *advec, const float *xmapr, const float *ymapr, float hours, bool& allDefined,
    float undef)
{
  // compute advection of a scalar field f (e.g. temperature):
  //     adection = -(u*dt/dx+v*dt/dy)*hours
  //
  //     dt/dx and dt/dy as centered differences
  //
  // input: f[nx*ny]: scalar field (e.g. temperature)
  //        u[nx*ny]: wind component in x-direction (m/s)
  //        v[nx*ny]: wind component in y-direction (m/s)
  //        xmapr[nx*ny],ymapr[nx*ny] : map ratios (xm/(hx*2),ym/(hy*2))
  //        hours: scaling to get a chosen time unit (delta_f/timeunit)
  //               e.g. hours=1./3600. => per second. ('SI')
  //                    hours=1.       => per hour
  //                    hours=24.      => per 24 hours
  // output: advec[nx*ny]
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  if (nx < 3 || ny < 3)
    return false;

  const float scale = -3600. * hours;
  const int fsize = nx * ny;

  // loop extended, reset bad computations at boundaries later
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
  for (int i = nx; i < fsize - nx; i++) {
    if (calculations::is_defined(allDefined, u[i], v[i], f[i-nx], f[i-1], f[i+1], f[i+nx], undef))
      advec[i] = (u[i] * 0.5 * xmapr[i] * (f[i+1] - f[i-1]) + v[i] * 0.5 * ymapr[i] * (f[i+nx] - f[i-nx])) * scale;
    else
      advec[i] = undef;
  }

  // fill in edge values not computed (or badly computed) above
  fillEdges(nx, ny, advec);
  return true;
}

// static
bool FieldFunctions::gradient(int compute, int nx, int ny, const float *field,
    float *fgrad, const float *xmapr, const float *ymapr, bool& allDefined, float undef)
{
  //  Gradienter beregnet med sentrerte differanser, kartfaktor
  //  og gridavstand i meter.
  //
  //  compute=1: df/dx
  //  compute=2: df/dy
  //  compute=3: abs(del(f))= sqrt((df/dx)**2 + (df/dy)**2)
  //  compute=4: del2(f)= del(del(f))
  //
  //  xmapr= xm/(hx*2)
  //  ymapr= ym/(hy*2)
  //
  //  NB! for compute=4 (del2(f)) behandles kartfaktorene ikke korrekt
  //      siden den deriverte i x- og y-retning burde inngaa,
  //      komplisert (mye regning) og antas aa ha liten effekt !!!
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  int fsize = nx * ny;
  float dfdx, dfdy, d2fdx, d2fdy;

  if (nx < 3 || ny < 3)
    return false;

  switch (compute) {

  case 1: // df/dx (loop extended, reset/complete later
    if (allDefined) {
      for (int i = 1; i < fsize - 1; i++)
        fgrad[i] = 0.5 * xmapr[i] * (field[i + 1] - field[i - 1]);
    } else {
      for (int i = 1; i < fsize - 1; i++) {
        if (field[i - 1] != undef && field[i + 1] != undef)
          fgrad[i] = 0.5 * xmapr[i] * (field[i + 1] - field[i - 1]);
        else
          fgrad[i] = undef;
      }
    }
    break;

  case 2: // df/dy (loop extended, reset/complete later
    if (allDefined) {
      for (int i = nx; i < fsize - nx; i++)
        fgrad[i] = 0.5 * ymapr[i] * (field[i + nx] - field[i - nx]);
    } else {
      for (int i = nx; i < fsize - nx; i++) {
        if (field[i - nx] != undef && field[i + nx] != undef)
          fgrad[i] = 0.5 * ymapr[i] * (field[i + nx] - field[i - nx]);
        else
          fgrad[i] = undef;
      }
    }
    break;

  case 3: // abs(del(f))= sqrt((df/dx)**2 + (df/dy)**2)
    if (allDefined) {
      for (int i = nx; i < fsize - nx; i++) {
        dfdx = 0.5 * xmapr[i] * (field[i + 1] - field[i - 1]);
        dfdy = 0.5 * ymapr[i] * (field[i + nx] - field[i - nx]);
        fgrad[i] = sqrtf(dfdx * dfdx + dfdy * dfdy);
      }
    } else {
      for (int i = nx; i < fsize - nx; i++) {
        if (field[i - nx] != undef && field[i - 1] != undef && field[i + 1]
            != undef && field[i + nx] != undef)
        {
          dfdx = 0.5 * xmapr[i] * (field[i + 1] - field[i - 1]);
          dfdy = 0.5 * ymapr[i] * (field[i + nx] - field[i - nx]);
          fgrad[i] = sqrtf(dfdx * dfdx + dfdy * dfdy);
        } else
          fgrad[i] = undef;
      }
    }
    break;

  case 4: // delsquare(f)= del(del(f))
    if (allDefined) {
      for (int i = nx; i < fsize - nx; i++) {
        d2fdx = field[i - 1] - 2.0 * field[i] + field[i + 1];
        d2fdy = field[i - nx] - 2.0 * field[i] + field[i + nx];
        fgrad[i] = 4.0f * (0.25 * xmapr[i] * xmapr[i] * d2fdx
            + 0.25 * ymapr[i] * ymapr[i] * d2fdy);
      }
    } else {
      for (int i = nx; i < fsize - nx; i++) {
        if (field[i - nx] != undef && field[i - 1] != undef && field[i]
            != undef && field[i + 1] != undef && field[i + nx] != undef)
        {
          d2fdx = field[i - 1] - 2.0 * field[i] + field[i + 1];
          d2fdy = field[i - nx] - 2.0 * field[i] + field[i + nx];
          fgrad[i] = 4.0 * (0.25 * xmapr[i] * xmapr[i] * d2fdx
              + 0.25 * ymapr[i] * ymapr[i] * d2fdy);
        } else
          fgrad[i] = undef;
      }
    }
    break;

  default:
    return false;
  }

  // fill in edge values not computed (or badly computed) above
  fillEdges(nx, ny, fgrad);

  return true;
}

// static
bool FieldFunctions::shapiro2_filter(int nx, int ny, float *field,
    float *fsmooth, bool& allDefined, float undef)
{
  //  2nd order Shapiro filter
  //
  //  G.J.Haltiner, Numerical Weather Prediction,
  //                   Objective Analysis,
  //                       Smoothing and filtering
  //
  //  input:   nx,ny          - field dimensions
  //           field[nx*ny]   - the field
  //  output:  fsmooth[nx*ny] - the smoothed field
  //
  //  Note: field and fsmooth may be the same array!
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  const int fsize = nx * ny;
  int i1, i2;
  float s;

  if (nx < 3 || ny < 3)
    return false;

  float *f1 = 0;
  float *f2 = new float[fsize];

  if (field == fsmooth) {
    f1 = field;
  } else {
    for (int i = 0; i < fsize; i++)
      fsmooth[i] = field[i];
    f1 = fsmooth;
  }

  if (allDefined) {

    s = 0.25;

    for (int n = 0; n < 2; n++) {

      for (int i = 1; i < fsize - 1; i++)
        f2[i] = f1[i] + s * (f1[i - 1] + f1[i + 1] - 2. * f1[i]);

      i1 = 0;
      i2 = nx - 1;
      for (int j = 0; j < ny; j++, i1 += nx, i2 += nx) {
        f2[i1] = f1[i1];
        f2[i2] = f1[i2];
      }

      for (int i = nx; i < fsize - nx; i++)
        f1[i] = f2[i] + s * (f2[i - nx] + f2[i + nx] - 2. * f2[i]);

      i2 = fsize - nx;
      for (i1 = 0; i1 < nx; i1++, i2++) {
        f1[i1] = f2[i1];
        f1[i2] = f2[i2];
      }

      s = -0.25;
    }

  } else {

    // s is uninitialised at this point!
    s = 0.25; // moved it here...

    float *s1 = new float[fsize];
    float *s2 = new float[fsize];

    for (int i = 1; i < fsize - 1; i++)
      s1[i] = (f1[i - 1] != undef && f1[i] != undef && f1[i + 1] != undef) ? s
          : 0.;

    for (int i = nx; i < fsize - nx; i++)
      s2[i]
         = (f1[i - nx] != undef && f1[i] != undef && f1[i + nx] != undef) ? s
             : 0.;

    // s = 0.25; // ... from here

    for (int n = 0; n < 2; n++) {

      for (int i = 1; i < fsize - 1; i++)
        f2[i] = f1[i] + s1[i] * (f1[i - 1] + f1[i + 1] - 2. * f1[i]);

      i1 = 0;
      i2 = nx - 1;
      for (int j = 0; j < ny; j++, i1 += nx, i2 += nx) {
        f2[i1] = f1[i1];
        f2[i2] = f1[i2];
      }

      for (int i = nx; i < fsize - nx; i++)
        f1[i] = f2[i] + s2[i] * (f2[i - nx] + f2[i + nx] - 2. * f2[i]);

      i2 = fsize - nx;
      for (i1 = 0; i1 < nx; i1++, i2++) {
        f1[i1] = f2[i1];
        f1[i2] = f2[i2];
      }

      s = -0.25;
    }

    delete[] s1;
    delete[] s2;
  }

  delete[] f2;

  return true;
}

// static
bool FieldFunctions::windCooling(int compute, int nx, int ny, const float *t,
    const float *u, const float *v, float *dtcool, bool& allDefined, float undef)
{
  // Compute wind cooling (the difference, not the sensed temperature)
  //
  //  compute=1 : temp. (Kelvin)
  //  compute=2 : temp. (Celsius)
  //
  //  input:   nx,ny         - field dimensions
  //           t[nx*ny]      - temperature (Kelvin or Celsius)
  //           u[nx*ny]      - wind component x-direction (m/s)
  //           v[nx*ny]      - wind component y-direction (m/s)
  //  output:  dtcool[nx*ny] - the cooling (temp. difference)
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  int fsize = nx * ny;
  float tconv, tc, ff, ffpow;

  tconv = 0.;
  if (compute == 1)
    tconv = t0;

  switch (compute) {

  case 1: // T(Kelvin)  -> dTcooling
  case 2: // T(Celsius) -> dTcooling
    if (allDefined) {
      for (int i = 0; i < fsize; i++) {
        tc = t[i] - tconv;
        ff = sqrtf(u[i] * u[i] + v[i] * v[i]) * 3.6; // m/s -> km/h
        ffpow = powf(ff, 0.16);
        dtcool[i] = 13.12 + 0.6215 * tc - 11.37 * ffpow + 0.3965 * tc * ffpow;
        if (dtcool[i] > 0.)
          dtcool[i] = 0.;
      }
    } else {
      for (int i = 0; i < fsize; i++) {
        if (t[i] != undef && u[i] != undef && v[i] != undef) {
          tc = t[i] - tconv;
          ff = sqrtf(u[i] * u[i] + v[i] * v[i]) * 3.6; // m/s -> km/h
          ffpow = powf(ff, 0.16);
          dtcool[i] = 13.12 + 0.6215 * tc - 11.37 * ffpow + 0.3965 * tc * ffpow;
          if (dtcool[i] > 0.)
            dtcool[i] = 0.;
        } else
          dtcool[i] = undef;
      }
    }
    break;

  default:
    return false;
  }
  return true;
}

// static
bool FieldFunctions::underCooledRain(int nx, int ny, const float *precip,
    const float *snow, const float *tk, float *undercooled, float precipMin,
    float snowRateMax, float tcMax, bool& allDefined, float undef)
{
  // compute possibility/danger for undercooled rain.....
  //
  // input:  precip[]  : total precip (rain+snow) in unit mm
  //         snow[]    : snow in unit mm (water)
  //         tk[]      : temperature im unit Kelvin (t0m)
  //         undef     : unde|fined value for all fields
  //         maxSnowRate: max rate of snow in total precip
  //         precLimit : min precipitation in unit mm
  //         tempLimit : max temperature in unit Celsius
  // output: undercooled[] : 0=no 1=yes
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  int fsize = nx * ny;
  float tkMax = tcMax + t0;

  if (allDefined) {
    for (int i = 0; i < fsize; i++) {
      if (precip[i] >= precipMin && tk[i] <= tkMax && snow[i] <= precip[i]
                                                                        * snowRateMax)
        undercooled[i] = 1.;
      else
        undercooled[i] = 0.;
    }
  } else {
    for (int i = 0; i < fsize; i++) {
      if (precip[i] != undef && snow[i] != undef && tk[i] != undef) {
        if (precip[i] >= precipMin && tk[i] <= tkMax && snow[i] <= precip[i]
                                                                          * snowRateMax)
          undercooled[i] = 1.;
        else
          undercooled[i] = 0.;
      } else {
        undercooled[i] = undef;
      }
    }
  }
  return true;
}

// static
bool FieldFunctions::thermalFrontParameter(int nx, int ny, const float *tx,
    float *tfp, const float *xmapr, const float *ymapr, bool& allDefined, float undef)
{
  // Thermal Front Parameter
  // TFP = -del(abs(del(T)) . del(t)/abs(del(T))
  //
  // input:   tx[nx*ny] : temperature, pot.temp. or some other temperature
  // output: tfp[nx*ny]
  //
  //  xmapr= xm/(hx*2)
  //  ymapr= ym/(hy*2)

#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  int fsize = nx * ny;
  float dabsdeltdx, dabsdeltdy, dtdxa, dtdya;

  float *absdelt = new float[fsize];

  if (!gradient(3, nx, ny, tx, absdelt, xmapr, ymapr, allDefined, undef)) {
    delete[] absdelt;
    return false;
  }

  // loop extended, reset bad computations at boundaries later
  if (allDefined) {
    for (int i = nx; i < fsize - nx; i++) {
      if (absdelt[i] != 0.0) {
        dabsdeltdx = 0.5 * xmapr[i] * (absdelt[i + 1] - absdelt[i - 1]);
        dabsdeltdy = 0.5 * ymapr[i] * (absdelt[i + nx] - absdelt[i - nx]);
        dtdxa = 0.5 * xmapr[i] * (tx[i + 1] - tx[i - 1]) / absdelt[i];
        dtdya = 0.5 * ymapr[i] * (tx[i + nx] - tx[i - nx]) / absdelt[i];
        tfp[i] = -(dabsdeltdx * dtdxa + dabsdeltdy * dtdya);
      } else {
        tfp[i] = 0.0;
      }
    }
  } else {
    for (int i = nx; i < fsize - nx; i++) {
      if (tx[i - nx] != undef && tx[i - 1] != undef && tx[i + 1] != undef
          && tx[i + nx] != undef && absdelt[i - nx] != undef && absdelt[i - 1]
                                                                        != undef && absdelt[i] != undef && absdelt[i] != 0.0
                                                                        && absdelt[i + 1] != undef && absdelt[i + nx] != undef) {
        dabsdeltdx = 0.5 * xmapr[i] * (absdelt[i + 1] - absdelt[i - 1]);
        dabsdeltdy = 0.5 * ymapr[i] * (absdelt[i + nx] - absdelt[i - nx]);
        dtdxa = 0.5 * xmapr[i] * (tx[i + 1] - tx[i - 1]) / absdelt[i];
        dtdya = 0.5 * ymapr[i] * (tx[i + nx] - tx[i - nx]) / absdelt[i];
        tfp[i] = -(dabsdeltdx * dtdxa + dabsdeltdy * dtdya);
      } else {
        tfp[i] = undef;
      }
    }
  }

  delete[] absdelt;

  // fill in edge values not computed (or badly computed) above
  fillEdges(nx, ny, tfp);

  return true;
}

// static
bool FieldFunctions::pressure2FlightLevel(int nx, int ny, const float *pressure,
    float *flightlevel, bool& allDefined, float undef)
{
  // compute standard FlightLevel from pressure,
  // using the same table as used when mapping standard pressure levels
  // to standard FlightLevels
  // (not very correct, but consistent...)

  // input:  pressure[] in unit hPa
  // output: flightlevel[] in unit 100 feet

  // tables in diMetConstants.h
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  const int nTab = nLevelTable - 1;

  int fsize = nx * ny;
  int k;
  float p, ratio;

  for (int i = 0; i < fsize; i++) {
    if (allDefined || pressure[i] != undef) {
      p = pressure[i];
      if (p > pLevelTable[0])
        p = pLevelTable[0];
      if (p < pLevelTable[nTab])
        p = pLevelTable[nTab];
      k = 1;
      while (k < nTab && pLevelTable[k] > p)
        k++;
      ratio = (p - pLevelTable[k - 1]) / (pLevelTable[k] - pLevelTable[k - 1]);
      flightlevel[i] = fLevelTable[k - 1] + (fLevelTable[k]
                                                         - fLevelTable[k - 1]) * ratio;
    } else {
      flightlevel[i] = undef;
    }
  }
  return true;
}

// static
bool FieldFunctions::momentumXcoordinate(int nx, int ny, const float *v, float *mxy,
    const float *xmapr, const float *fcoriolis, float fcoriolisMin, bool& allDefined,
    float undef)
{
  // Momentum coordinates, X component
  // Compute m(x,y) from v (centered differences)
  //
  // fcpriolisMin : minimum value of coriolis parameter used in computation
  //
  // m(x,y) = x + v*xm/(hx*fc) ..... xmapr = xm/(2*hx) !!!
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  int fsize = nx * ny;
  float fcormin, fcormax, fcor;

  if (nx < 3 || ny < 3)
    return false;

  fcormin = fabsf(fcoriolisMin);
  fcormax = -fcormin;

  if (allDefined) {
    for (int i = 0; i < fsize; i++) {
      fcor = fcoriolis[i];
      if (fcor >= 0. && fcor < fcormin)
        fcor = fcormin;
      else if (fcor <= 0. && fcor > fcormax)
        fcor = fcormax;
      mxy[i] = float(i % nx) + v[i] * xmapr[i] / fcor;
    }
  } else {
    for (int i = 0; i < fsize; i++) {
      if (v[i] != undef) {
        fcor = fcoriolis[i];
        if (fcor >= 0. && fcor < fcormin)
          fcor = fcormin;
        else if (fcor <= 0. && fcor > fcormax)
          fcor = fcormax;
        mxy[i] = float(i % nx) + v[i] * xmapr[i] / fcor;
      } else {
        mxy[i] = undef;
      }
    }
  }
  return true;
}

// static
bool FieldFunctions::momentumYcoordinate(int nx, int ny, const float *u, float *nxy,
    const float *ymapr, const float *fcoriolis, float fcoriolisMin, bool& allDefined,
    float undef)
{
  // Momentum coordinates, Y component
  // Compute n(x,y) from u (centered differences)
  //
  // fcpriolisMin : minimum value of coriolis parameter used in computation
  //
  // n(x,y) = y - u*ym/(hy*fc) ..... ymapr = ym/(2*hy) !!!
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  int fsize = nx * ny;
  float fcormin, fcormax, fcor;

  if (nx < 3 || ny < 3)
    return false;

  fcormin = fabsf(fcoriolisMin);
  fcormax = -fcormin;

  if (allDefined) {
    for (int i = 0; i < fsize; i++) {
      fcor = fcoriolis[i];
      if (fcor >= 0. && fcor < fcormin)
        fcor = fcormin;
      else if (fcor <= 0. && fcor > fcormax)
        fcor = fcormax;
      nxy[i] = float(i / nx) - u[i] * ymapr[i] / fcor;
    }
  } else {
    for (int i = 0; i < fsize; i++) {
      if (u[i] != undef) {
        fcor = fcoriolis[i];
        if (fcor >= 0. && fcor < fcormin)
          fcor = fcormin;
        else if (fcor <= 0. && fcor > fcormax)
          fcor = fcormax;
        nxy[i] = float(i / nx) - u[i] * ymapr[i] / fcor;
      } else {
        nxy[i] = undef;
      }
    }
  }
  return true;
}

// static
bool FieldFunctions::jacobian(int nx, int ny, const float *field1, const float *field2,
    float *fjacobian, const float *xmapr, const float *ymapr, bool& allDefined, float undef)
{
  //  Beregner den jacobiske av f1 og f2:
  //       Jacobian = df1/dx * df2/dy - df1/dy * df2/dx
  //
  //   xmapr = xm/(hx*2)
  //   ymapr = ym/(hy*2)

#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  int fsize = nx * ny;
  float df1dx, df1dy, df2dx, df2dy;

  if (nx < 3 || ny < 3)
    return false;

  if (allDefined) {

    // loop extended, reset bad computations at boundaries later
    for (int i = nx; i < fsize - nx; i++) {
      df1dx = 0.5 * xmapr[i] * (field1[i + 1] - field1[i - 1]);
      df1dy = 0.5 * ymapr[i] * (field1[i + nx] - field1[i - nx]);
      df2dx = 0.5 * xmapr[i] * (field2[i + 1] - field2[i - 1]);
      df2dy = 0.5 * ymapr[i] * (field2[i + nx] - field2[i - nx]);
      fjacobian[i] = df1dx * df2dy - df1dy * df2dx;
    }

  } else {

    for (int i = nx; i < fsize - nx; i++) {
      if (field1[i - nx] != undef && field1[i - 1] != undef && field1[i + 1]
                                                                      != undef && field1[i + nx] != undef && field2[i - nx] != undef
                                                                      && field2[i - 1] != undef && field2[i + 1] != undef && field2[i + nx]
                                                                                                                                    != undef) {
        df1dx = 0.5 * xmapr[i] * (field1[i + 1] - field1[i - 1]);
        df1dy = 0.5 * ymapr[i] * (field1[i + nx] - field1[i - nx]);
        df2dx = 0.5 * xmapr[i] * (field2[i + 1] - field2[i - 1]);
        df2dy = 0.5 * ymapr[i] * (field2[i + nx] - field2[i - nx]);
        fjacobian[i] = df1dx * df2dy - df1dy * df2dx;
      } else
        fjacobian[i] = undef;
    }

  }

  // fill in edge values not computed (or badly computed) above
  fillEdges(nx, ny, fjacobian);
  return true;
}

// static
bool FieldFunctions::vesselIcingOverland(int nx, int ny, const float *airtemp,
    const float *seatemp, const float *u, const float *v, float *icing, float freezingPoint,
    bool& allDefined, float undef)
{
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif
  int fsize = nx * ny;
  float tf = freezingPoint + t0; //freezing point in kelvin
  bool local_allDefined = allDefined;

  for (int i = 0; i < fsize; i++) {
    if (allDefined || (airtemp[i] != undef && seatemp[i] != undef && u[i]
                                                                       != undef && v[i] != undef)) {
      if (seatemp[i] < tf) {
        icing[i] = undef;
        local_allDefined = false;
      } else {
        float ff = sqrtf(u[i] * u[i] + v[i] * v[i]);
        icing[i] = ff * (tf - airtemp[i]) / (1 + 0.3 * (seatemp[i] - tf));
      }
    } else {
      icing[i] = undef;
    }
  }

  allDefined = local_allDefined;
  return true;

}

// static
bool FieldFunctions::vesselIcingMertins(int nx, int ny, const float *airtemp,
    const float *seatemp, const float *u, const float *v, float *icing,
    float freezingPoint, bool& allDefined, float undef)
{
  // Based on: H.O. Mertins : Icing on fishing vessels due to spray, Marine Observer No.221, 1968
  // All temperatures in degrees celsius
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  int fsize = nx * ny;
  float temp1; float temp2; float temp3;
  bool local_allDefined = allDefined;

  for (int i = 0; i < fsize; i++) {
    if (allDefined || (airtemp[i] != undef && seatemp[i] != undef && u[i]
                                                                       != undef && v[i] != undef)) {
      if (seatemp[i] < freezingPoint) {
        icing[i] = undef;
        local_allDefined = false;
      } 
      else {
        float ff = sqrtf(u[i] * u[i] + v[i] * v[i]);
        float temperature = airtemp[i];
        float sst=seatemp[i];
        if (ff>=10.8){
          if (ff<17.2) {
            temp1=-1.15*sst-4.3;
            temp2=-1.5*sst-10;
            temp3=-10000;
          }
          else if (ff<20.8){
            temp1=-0.6*sst-3.2;
            temp2=-1.05*sst-5.6;
            temp3=-1.75*sst-12.5;
          }
          else if (ff<28.5){
            temp1=-0.3*sst-2.6;
            temp2=-0.66*sst-3.32;
            temp3=-1.325*sst-7.651;
          }
          else {
            temp1=-0.14*sst-2.28;
            temp2=-0.3*sst-2.6;
            temp3=-1.16*sst-5.22;
          }

          if (temperature>-2){
            icing[i]=0;
          }
          else if (temperature>temp1){
            icing[i]=0.8333;
          }
          else if (temperature>temp2){
            icing[i]=2.0833;
          }
          else {
            if (temperature<=temp3 || ff<17.2) {
              icing[i]=4.375;
            }
            else {
              icing[i]=6.25;
            }
          }
        }
        else {
          icing[i]=0;
        }

      }
    } 
    else {
      icing[i] = undef;
    }
  }

  allDefined = local_allDefined;

  return true;

}

// static
bool FieldFunctions::values2classes(int nx, int ny, const float *fvalue,
    float *fclass, const vector<float>& values, bool& allDefined, float undef)
{
  // From value ranges to classes
  // values not covered for will be set to undefined
  //
  // Note: The classes will always be 0,1,2,3,...
  //       The values are minimum values except the last which is a maximim.
  //       If there are 10 values they can become classes 0-8 !
  //       Ranges are [min,max).
  //
  // Note: The argument 'allDefined' may be changed!
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  int fsize = nx * ny;
  int newundef = 0;

  if (values.size() < 2)
    return false;

  int nvalues = values.size() - 2;
  float fmin = values[0];
  float fmax = values[nvalues + 1];

  for (int i = 0; i < fsize; i++) {
    if (fvalue[i] != undef && fvalue[i] >= fmin && fvalue[i] < fmax) {
      int j = 1;
      while (j < nvalues && values[j] < fvalue[i])
        j++;
      fclass[i] = float(j - 1);
    } else {
      fclass[i] = undef;
      newundef++;
    }
  }

  allDefined = (newundef == 0);

  return true;
}

// static
bool FieldFunctions::fieldOPERfield(int compute, int nx, int ny, const float *field1,
    const float *field2, float *fres, bool& allDefined, float undef)
{
  // field1 <+-*/> field2
  //
  //  compute=1 : field1 + field2
  //  compute=2 : field1 - field2
  //  compute=3 : field1 * field2
  //  compute=4 : field1 / field2
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  if (compute <= 0 or compute >= 5)
    return false;

  const int fsize = nx * ny;
  const bool inAllDefined = allDefined;

#ifdef HAVE_OPENMP
#pragma omp parallel for shared(allDefined)
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(inAllDefined, field1[i], field2[i], undef)) {
      if (compute == 1)
        fres[i] = field1[i] + field2[i];
      else if (compute == 2)
        fres[i] = field1[i] - field2[i];
      else if (compute == 3)
        fres[i] = field1[i] * field2[i];
      else { /* compute == 4 */
        if (field2[i] != 0.0) {
          fres[i] = field1[i] / field2[i];
        } else {
          fres[i] = undef;
          allDefined = false;
        }
      }
    } else {
      fres[i] = undef;
    }
  }
  return true;
}

// static
bool FieldFunctions::fieldOPERconstant(int compute, int nx, int ny,
    const float *field, float constant, float *fres, bool& allDefined, float undef)
{
  // field <+-*/> constant
  //
  //  compute=1 : field + constant
  //  compute=2 : field - constant
  //  compute=3 : field * constant
  //  compute=4 : field / constant
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  if (compute <= 0 or compute >= 5)
    return false;

  const int fsize = nx * ny;

  if ((constant == undef) or (compute == 4 and constant == 0.0)) {  // field / constant
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < fsize; i++)
      fres[i] = undef;
    allDefined = false;
    return true;
  }

  const bool inAllDefined = allDefined;

#ifdef HAVE_OPENMP
#pragma omp parallel for shared(allDefined)
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(inAllDefined, field[i], undef)) {
      if (compute == 1)
        fres[i] = field[i] + constant;
      else if (compute == 2)
        fres[i] = field[i] - constant;
      else if (compute == 3)
        fres[i] = field[i] * constant;
      else /* compute == 4 */
        fres[i] = field[i] / constant;
    } else {
      fres[i] = undef;
    }
  }
  return true;
}

bool FieldFunctions::constantOPERfield(int compute, int nx, int ny,
    float constant, const float *field, float *fres, bool& allDefined, float undef)
{
  // constant <+-*/> field
  //
  //  compute=1 : constant + field
  //  compute=2 : constant - field
  //  compute=3 : constant * field
  //  compute=4 : constant / field
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  if (compute <= 0 or compute >= 5)
    return false;

  const int fsize = nx * ny;

  if (constant == undef) {
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < fsize; i++)
      fres[i] = undef;

    allDefined = false;
    return true;
  }

  const bool inAllDefined = allDefined;

#ifdef HAVE_OPENMP
#pragma omp parallel for shared(allDefined)
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(inAllDefined, field[i], undef)) {
      if (compute == 1)
        fres[i] = constant + field[i];
      else if (compute == 2)
        fres[i] = constant - field[i];
      else if (compute == 3)
        fres[i] = constant * field[i];
      else { /* compute == 4 */
        if (field[i] != 0.0) {
          fres[i] = constant / field[i];
        } else {
          fres[i] = undef;
          allDefined = false;
        }
      }
    } else {
      fres[i] = undef;
    }
  }
  return true;
}

// static
bool FieldFunctions::sumFields(int nx, int ny, const vector<float*>& fields,
    float *fres, bool& allDefined, float undef)
{
  // field + field + field +
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  const int fsize = nx * ny;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
  for (int i = 0; i < fsize; i++) {
    fres[i]  = 0;
  }

  size_t nFields = fields.size();
  if (allDefined) {
    size_t j;
    for (j = 0; j <nFields; j++) {
      // TODO, check if optimal, j must bee visible to all the threads
      //LB: 2014-02-14 - This does not work
      //#pragma omp parallel for private(j)
      for (int i = 0; i < fsize; i++) {
        fres[i]  += fields[j][i];
      }
    }
  } else {
    size_t j;
    for (j = 0; j <nFields; j++) {
      //#pragma omp parallel for private(j)
      for (int i = 0; i < fsize; i++) {
        if (fres[i] != undef && fields[j][i] != undef) {
          fres[i] += fields[j][i];
        } else {
          fres[i] = undef;
        }
      }
    }
  }
  return true;
}

// static
void FieldFunctions::fillEdges(int nx, int ny, float *field)
{
  // Fill in edge values not computed or badly computed.
  // Note that in some (d/dx,d/dy) cases even correct computed edge values will be changed,
  // possibly not so good when current computation is used in following computations
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  int i1 = nx;
  int i2 = nx * 2 - 1;

  for (int j = 1; j < ny - 1; j++, i1 += nx, i2 += nx) {
    field[i1] = field[i1 + 1];
    field[i2] = field[i2 - 1];
  }

  i2 = nx * ny - nx;
  for (i1 = 0; i1 < nx; i1++, i2++) {
    field[i1] = field[i1 + nx];
    field[i2] = field[i2 - nx];
  }
}

// static
bool FieldFunctions::meanValue(int nx, int ny, const vector< float*>& fields,
    float *fres, bool& allDefined, float undef)
{
  // returns mean value in each grid point
  // (field + field + field +) / nFields
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  int fsize = nx * ny;
  for (int i = 0; i < fsize; i++) {
    fres[i] = 0;
  }

  size_t nFields = fields.size();
  if (allDefined) {
    size_t j;
    for (j = 0; j <nFields; j++) {
      for (int i = 0; i < fsize; i++) {
        fres[i] += fields[j][i];
      }
    }
    for (int i = 0; i < fsize; i++) {
      fres[i] /= nFields;
    }
  } else {
    size_t j;
    for (j = 0; j <nFields; j++) {
      for (int i = 0; i < fsize; i++) {
        if (fres[i] != undef && fields[j][i] != undef) {
          fres[i] += fields[j][i];
        } else {
          fres[i] = undef;
        }
      }
      for (int i = 0; i < fsize; i++) {
        if (fres[i] != undef) {
          fres[i] /= nFields;
        }
      }
    }
  }

  return true;
}

// static
bool FieldFunctions::stddevValue(int nx, int ny, const vector<float*>& fields,
    float *fres, bool& allDefined, float undef)
{
  // returns mean value in each grid point
  // (field + field + field +) / nFields
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  int fsize = nx * ny;

  size_t nFields = fields.size();

  float *mean = new float[fsize];
  meanValue(nx,ny,fields,mean,allDefined,undef);

  for (int i = 0; i < fsize; i++) {
    fres[i] = 0;
  }


  if (allDefined) {
    size_t j;
    for (j = 0; j <nFields; j++) {
      for (int i = 0; i < fsize; i++) {
        fres[i] += pow(fields[j][i]-mean[i],2);
      }
    }
    for (int i = 0; i < fsize; i++) {
      fres[i] = sqrt(fres[i]/nFields);
    }
  } else {
    size_t j;
    for (j = 0; j <nFields; j++) {
      for (int i = 0; i < fsize; i++) {
        if (fres[i] != undef && fields[j][i] != undef) {
          fres[i] += pow(fields[j][i]-mean[i],2);
        }
      }
    }
    for (int i = 0; i < fsize; i++) {
      if (fres[i] != undef) {
        fres[i] = sqrt(fres[i]/nFields);
      }
    }
  }
  delete[] mean;

  return true;
}

// static
bool FieldFunctions::extremeValue(int compute, int nx, int ny, const vector<float*>& fields,
    float *fres, bool& allDefined, float undef)
{
  // returns min value in each grid point
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif
  METLIBS_LOG_INFO(LOGVAL(compute));
  //  compute=1 : max - value
  //  compute=2 : min - vale
  //  compute=3 : max -index
  //  compute=4 : min - index

  const int fsize = nx * ny;
  const size_t nFields = fields.size();
  float *tmp = new float[fsize];

  // init fres with undef values
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
  for (int i = 0; i < fsize; i++) {
    fres[i] = undef;
    tmp[i] = undef;
  }

  if ( compute == 1 ) { //max value
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (size_t j = 0; j <nFields; j++) {
      for (int i = 0; i < fsize; i++) {
        if (fres[i] == undef || (calculations::is_defined(allDefined, fields[j][i], undef) && fres[i] < fields[j][i])){
          fres[i] = fields[j][i];
        }
      }
    }

  } else if ( compute == 2 ) { //min value
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (size_t j = 0; j <nFields; j++) {
      for (int i = 0; i < fsize; i++) {
        if (fres[i] == undef || (calculations::is_defined(allDefined, fields[j][i], undef) && fres[i] > fields[j][i])) {
          fres[i] = fields[j][i];
        }
      }
    }

  } else if ( compute == 3 ) { // max index
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (size_t j = 0; j <nFields; j++) {
      for (int i = 0; i < fsize; i++) {
        if (tmp[i] == undef || (calculations::is_defined(allDefined, fields[j][i], undef) && tmp[i] < fields[j][i])) {
          tmp[i] = fields[j][i];
          fres[i] = j;
        }
      }
    }

  } else if ( compute == 4 ) { // min index
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (size_t j = 0; j <nFields; j++) {
      for (int i = 0; i < fsize; i++) {
        if (tmp[i] == undef || (calculations::is_defined(allDefined, fields[j][i], undef) && tmp[i] > fields[j][i])) {
          tmp[i] = fields[j][i];
          fres[i] = j;
        }
      }
    }
  }
  // cannot use omp parallel if we want to stop at the first undef value
  for (int i = 0; allDefined && i < fsize; i++)
    if (fres[i] == undef)
      allDefined = false;

  delete[] tmp;

  return true;
}

// static
bool FieldFunctions::probability(int compute, int nx, int ny, const vector<float*>& fields,
    const vector<float>& limits, float *fres, bool& allDefined, float undef)
{
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  // returns probability or number of  value above/below limit in each grid point
  //  compute=1 : above - probability
  //  compute=2 : below - probability
  //  compute=3 : between - probability
  //  compute=4 : above - number
  //  compute=5 : below - number
  //  compute=6 : betwwen - number

  const size_t fsize = nx * ny;
  size_t lsize = limits.size();
  size_t nFields = fields.size();

  //init fres with undef values
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
  for (size_t i = 0; i < fsize; i++) {
    fres[i] = 0;
  }

  if( allDefined ) {
    if ( lsize == 1 ) {
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
      for (size_t j = 0; j <nFields; j++) {
        for (size_t i = 0; i < fsize; i++) {
          if ( fields[j][i] > limits[0]){
            fres[i] ++;
          }
        }
      }
    } else if (lsize == 2 ) {
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
      for (size_t j = 0; j <nFields; j++) {
        for (size_t i = 0; i < fsize; i++) {
          if ( fields[j][i] > limits[0] && fields[j][i] < limits[1] ){
            fres[i] ++;
          }
        }
      }
    }

    if ( compute != 1 && compute != 4 ) {
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
      for (size_t i = 0; i < fsize; i++) {
        fres[i] = nFields - fres[i];
      }
    }

    if ( compute < 3 ) {
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
      for (size_t i = 0; i < fsize; i++) {
        fres[i]/=(nFields/100.);
      }
    }

  } else {
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (size_t j = 0; j <nFields; j++) {
      for (size_t i = 0; i < fsize; i++) {
        if ( fields[j][i] == undef ){
          fres[i] = undef;
        }
        if ( fres[i] != undef && fields[j][i] > limits[0] && (lsize == 2 && fields[j][i] < limits[1] )){
          fres[i] ++;
        }
      }
    }

    if ( compute != 1 && compute != 4 ) {
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
      for (size_t i = 0; i < fsize; i++) {
        if (fres[i] != undef) {
          fres[i] = nFields - fres[i];
        }
      }
    }

    if ( compute < 3 ) {
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
      for (size_t i = 0; i < fsize; i++) {
        if (fres[i] != undef) {
          fres[i]/=(nFields/100.);
        }
      }
    }
  }
  return true;
}

// static
bool FieldFunctions::percentile(int nx, int ny, const float* field,
    float percentile, int range, int step,
    float *fres, bool& allDefined, float undef)
{
#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  if (not allDefined) {
    METLIBS_LOG_ERROR("Field contains undefined values -> percentiles can not be calculated");
    return false;
  }

  // percentiles can not be calculated close to the border, set values undef
  allDefined = false;
  for (int l = 0; l < range; l++) {
    for( int i=0;i<nx;i++ ){
      int index = i+l*nx;
      fres[index] = undef;
    }
  }
  for (int l = range; l < ny-range; l++) {
    for( int i=0;i<range;i++ ){
      int index = i+l*nx;
      fres[index] = undef;
    }
    for( int i=nx-range;i<nx;i++ ){
      int index = i+l*nx;
      fres[index] = undef;
    }
  }
  for (int l = ny-range; l < ny; l++) {
    for( int i=0;i<nx;i++ ){
      int index = i+l*nx;
      fres[index] = undef;
    }
  }

  // calc. index corresponding to given percentile and range
  int ii = pow((2*range+1),2)*percentile/100;

  //loop through all gridpoints with given step
  vector<float> values;
  for (int l = range; l < ny-range; l+=step) {
    for( int i=range;i<nx-range;i+=step){

      //sort values
      values.clear();
      int index = i+l*nx;
      for(int k=l-range;k<l+range+1;k++){
        for ( int j = i-range; j<i+range+1;j++) {
          values.push_back(field[j+k*nx]);
        }
      }
      std::sort(values.begin(), values.end());

      //set value corresponding to given percentile
      fres[index]=values[ii];
      if (step > 1) {
        fres[index+1]=values[ii];
        fres[index+nx]=values[ii];
      }
      if (step > 2) {
        fres[index-1]=values[ii];
        fres[index-nx]=values[ii];
        fres[index+1+nx]=values[ii];
        fres[index-1+nx]=values[ii];
        fres[index-1-nx]=values[ii];
        fres[index+1-nx]=values[ii];
      }
    }
  }
  return true;
}



bool FieldFunctions::snow_in_cm(int nx, int ny, const float *snow_water, const float *tk2m, const float *td2m,
    float *snow_cm, bool& allDefined, float undef)
{
  /*
Function: snow_in_cm
Parameters: in - snow_water (kg/m2), tk2m (2 m air temperature in kelvin), td2m (2 m dew point temperature in kelvin).
            out - snow depth in cm.
Algorithm:
Computing snow in cm.
 t2m = 2 meter temperature in kelvin
 td = dew point temperature in kelvin

t=(t2m+td)/2

logit(t)=(1-e^(t-274.3)3.5)/(1+e^(t-274.3)3.5 )

mm2cm(t)=0.13/(0.02+0.1((t-252.0)/20)^2 )

fac=logit(t)*mm2cm(t)

snow_cm=smow_water*fac

Reference:
MESAN Mesoskalig analys, SMHI RMK Nr 75, Mars 1997.
   */


#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  int fsize = nx * ny;
  bool inAllDefined = allDefined;
#ifdef HAVE_OPENMP
#pragma omp parallel for shared(allDefined)
#endif
  for (int i = 0; i < fsize; i++) {
    if (calculations::is_defined(inAllDefined, snow_water[i], tk2m[i], td2m[i], undef)) {
      if (snow_water[i] <= 0.) {
        snow_cm[i] = 0.;
        continue;
      }

      float t=(tk2m[i]+td2m[i])/2.;

      float logit_t=(1-exp((t-274.3)*3.5))/(1+exp((t-274.3)*3.5) );

      float mm2cm_t=0.13/(0.02+0.1*((t-252.0)/20.0)*((t-252.0)/20.0));

      float fac=logit_t*mm2cm_t;
      // We must preserve model consistency
      if (fac <= 1.)
        snow_cm[i] = snow_water[i];
      else
        snow_cm[i]=snow_water[i]*fac;
    } else {
      snow_cm[i] = undef;
      allDefined = false;
    }
  }
  return true;
}
