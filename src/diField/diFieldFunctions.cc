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

#include "diFieldFunctions.h"

#include "../util/openmp_tools.h"

#include "diArea.h"
#include "diGridConverter.h"
#include "diField.h"
#include "diFieldCalculations.h"
#include "diFlightLevel.h"

#include <puTools/miStringFunctions.h>

#include <cmath>
#include <iomanip>
#include <cfloat>

#include <memory.h>

//#define DEBUGPRINT 1
#define ENABLE_FIELDFUNCTIONS_TIMING 1

using namespace miutil;
using namespace std;

#define MILOGGER_CATEGORY "diField.FieldFunctions"
#include "miLogger/miLogging.h"

// static data (setup)
vector<FieldFunctions::FieldCompute> FieldFunctions::vFieldCompute;

vector<std::string> FieldFunctions::vFieldName;
map<std::string,int> FieldFunctions::mFieldName;

map< std::string, FieldFunctions::Zaxis_info > FieldFunctions::Zaxis_info_map;

FieldFunctions::FieldFunctions()
{
}

// static
std::string FieldFunctions::FIELD_COMPUTE_SECTION()
{
  return "FIELD_COMPUTE";
}

// static
std::string FieldFunctions::FIELD_VERTICAL_COORDINATES_SECTION()
{
  return "FIELD_VERTICAL_COORDINATES";
}

// static
const FieldFunctions::Zaxis_info* FieldFunctions::findZaxisInfo(const std::string& name)
{
  const std::map< std::string, Zaxis_info>::const_iterator it = Zaxis_info_map.find(name);
  if (it != Zaxis_info_map.end())
    return &(it->second);
  else
    return 0;
}

// static
const FieldFunctions::VerticalType FieldFunctions::getVerticalType(const std::string& vctype)
{
  if ( vctype == "none" )
    return FieldFunctions::vctype_none;
  if ( vctype == "pressure")
    return FieldFunctions::vctype_pressure;
  if ( vctype == "hybrid")
    return FieldFunctions::vctype_hybrid;
  if ( vctype == "atmospheric")
    return FieldFunctions::vctype_atmospheric;
  if ( vctype == "isentropic")
    return FieldFunctions::vctype_isentropic;
  if ( vctype == "oceandepth")
    return FieldFunctions::vctype_oceandepth;
  if ( vctype == "other")
    return FieldFunctions::vctype_other;

  return FieldFunctions::vctype_none;

}

// static member
bool FieldFunctions::registerFunction(functions_t& functions, Function f, const std::string& funcText)
{
  // The function text should contain a variable definition inside a '(' ')' pair
  const size_t p1 = funcText.find('('), p2 = funcText.find(')');
  if (p1 == string::npos || p2 == string::npos || p1 == 0 || p1 >= p2)
    return false;

  // the name of the function (must be matched in setup)
  std::string functionName = funcText.substr(0, p1);
  FunctionHelper fh;
  fh.func = f;

  // parse the arguments
  const std::string str = funcText.substr(p1 + 1, p2 - p1 - 1);
  const vector<std::string> vstr = miutil::split(str, 0, ",");

  // set the vertical component type
  fh.vertcoord = vctype_none;
  if (miutil::contains(functionName, ".plevel_"))
    fh.vertcoord = vctype_pressure;
  else if (miutil::contains(functionName, ".hlevel_"))
    fh.vertcoord = vctype_hybrid;
  else if (miutil::contains(functionName, ".alevel_"))
    fh.vertcoord = vctype_atmospheric;
  else if (miutil::contains(functionName, ".ilevel_"))
    fh.vertcoord = vctype_isentropic;
  else if (miutil::contains(functionName, ".ozlevel_"))
    fh.vertcoord = vctype_oceandepth;

  fh.numfields = 0;
  fh.numconsts = 0;
  fh.numresult = 1;

  bool err = true;
  unsigned int i = 0;
  while (i < vstr.size() && (vstr[i].length() < 6 || vstr[i].substr(0, 6) != "const:"))
    i++;
  if (i == vstr.size()) {
    fh.numfields = vstr.size();
    if (fh.numfields > 0 && vstr[i-1].find_first_not_of('.') == string::npos) {
      fh.numfields = -1;
    }
  } else {
    fh.numfields = i;
    if (fh.numfields > 0 && vstr[i-1].find_first_not_of('.') == string::npos) {
      METLIBS_LOG_ERROR("Error while parsing functions defined in FieldFunctions:"
          "Functions with both fields and contants cannot have a variable numder of fields: " << funcText);
      return false;
    }
    fh.numconsts = vstr.size() - fh.numfields;
    i = vstr.size() - 1;
    if (vstr[i].find_first_not_of('.') == string::npos)
      fh.numconsts = -(fh.numconsts - 1);
  }
  if (p2 + 1 < funcText.length()) {
    const std::string nr = funcText.substr(p2 + 1);
    if (miutil::is_int(nr)) {
      fh.numresult = miutil::to_int(nr);
      if (fh.numresult > 0)
        err = false;
    }
  } else {
    err = false;
  }
  if (!err) {
    functions[functionName] = fh;
    return true;
  } else {
    METLIBS_LOG_ERROR("programming error: bad function definition '" << funcText << "'");
    return false;
  }
}

// static member
bool FieldFunctions::registerFunctions(functions_t& f)
{
  bool ok = true;

#ifdef PURELY_EXPERIMENTAL_DO_NOT_ADD
  // simple computations - the functionTexts here are purely experimental - do not add!
  ok &= registerFunction(f, f_add_f_f, "add(f1,f2)"); // field - field
  ok &= registerFunction(f, f_subtract_f_f, "subtract(f1,f2)"); // field - field
  ok &= registerFunction(f, f_multiply_f_f, "multiply(f1,f2)"); // field * field
  ok &= registerFunction(f, f_divide_f_f, "divide(f1,f2)"); // field / field
  ok &= registerFunction(f, f_add_f_c, "add(f,const:value)"); // field + constant
  ok &= registerFunction(f, f_subtract_f_c, "subtract(f,const:value)"); // field - constant
  ok &= registerFunction(f, f_multiply_f_c, "multiply(f,const:value)"); // field * constant
  ok &= registerFunction(f, f_divide_f_c, "divide(f,const:value)"); // field / constant
  ok &= registerFunction(f, f_add_c_f, "add(const:value,f)"); // constant + field
  ok &= registerFunction(f, f_subtract_c_f, "subtract(const:value,f)"); // constant - field
  ok &= registerFunction(f, f_multiply_c_f, "multiply(const:value,f)"); // constant * field
  ok &= registerFunction(f, f_divide_c_f, "divide(const:value,f)"); // constant / field
#endif

  ok &= registerFunction(f, f_sum_f, "sum(field,...)");
  // pressure level (PLEVEL) functions
  ok &= registerFunction(f, f_tc_plevel_th, "tc.plevel_th(th)");
  ok &= registerFunction(f, f_tk_plevel_th, "tk.plevel_th(th)");
  ok &= registerFunction(f, f_th_plevel_tk, "th.plevel_tk(tk)");
  ok &= registerFunction(f, f_thesat_plevel_tk, "thesat.plevel_tk(tk)");
  ok &= registerFunction(f, f_thesat_plevel_th, "thesat.plevel_th(th)");
  ok &= registerFunction(f, f_the_plevel_tk_rh, "the.plevel_tk_rh(tk,rh)");
  ok &= registerFunction(f, f_the_plevel_th_rh, "the.plevel_th_rh(th,rh)");
  ok &= registerFunction(f, f_rh_plevel_tk_q, "rh.plevel_tk_q(tk,q)");
  ok &= registerFunction(f, f_rh_plevel_th_q, "rh.plevel_th_q(th,q)");
  ok &= registerFunction(f, f_q_plevel_tk_rh, "q.plevel_tk_rh(tk,rh)");
  ok &= registerFunction(f, f_q_plevel_th_rh, "q.plevel_th_rh(th,rh)");
  ok &= registerFunction(f, f_tdc_plevel_tk_rh, "tdc.plevel_tk_rh(tk,rh)");
  ok &= registerFunction(f, f_tdc_plevel_th_rh, "tdc.plevel_th_rh(th,rh)");
  ok &= registerFunction(f, f_tdc_plevel_tk_q, "tdc.plevel_tk_q(tk,q)");
  ok &= registerFunction(f, f_tdc_plevel_th_q, "tdc.plevel_th_q(th,q)");
  ok &= registerFunction(f, f_tdk_plevel_tk_rh, "tdk.plevel_tk_rh(tk,rh)");
  ok &= registerFunction(f, f_tdk_plevel_th_rh, "tdk.plevel_th_rh(th,rh)");
  ok &= registerFunction(f, f_tdk_plevel_tk_q, "tdk.plevel_tk_q(tk,q)");
  ok &= registerFunction(f, f_tdk_plevel_th_q, "tdk.plevel_th_q(th,q)");
  ok &= registerFunction(f, f_tcmean_plevel_z1_z2, "tcmean.plevel_z1_z2(z1,z2)");
  ok &= registerFunction(f, f_tkmean_plevel_z1_z2, "tkmean.plevel_z1_z2(z1,z2)");
  ok &= registerFunction(f, f_thmean_plevel_z1_z2, "thmean.plevel_z1_z2(z1,z2)");
  ok &= registerFunction(f, f_ducting_plevel_tk_q, "ducting.plevel_tk_q(tk,q)");
  ok &= registerFunction(f, f_ducting_plevel_th_q, "ducting.plevel_th_q(th,q)");
  ok &= registerFunction(f, f_ducting_plevel_tk_rh, "ducting.plevel_tk_rh(tk,rh)");
  ok &= registerFunction(f, f_ducting_plevel_th_rh, "ducting.plevel_th_rh(th,rh)");
  ok &= registerFunction(f, f_kindex_plevel_tk_rh, "kindex.plevel_tk_rh(tk500,tk700,rh700,tk850,rh850)");
  ok &= registerFunction(f, f_kindex_plevel_th_rh, "kindex.plevel_th_rh(th500,th700,rh700,th850,rh850)");
  ok &= registerFunction(f, f_ductingindex_plevel_tk_rh, "ductingindex.plevel_tk_rh(tk850,rh850)");
  ok &= registerFunction(f, f_ductingindex_plevel_th_rh, "ductingindex.plevel_th_rh(th850,rh850)");
  ok &= registerFunction(f, f_showalterindex_plevel_tk_rh, "showalterindex.plevel_tk_rh(tk500,tk850,rh850)");
  ok &= registerFunction(f, f_showalterindex_plevel_th_rh, "showalterindex.plevel_th_rh(th500,th850,rh850)");
  ok &= registerFunction(f, f_boydenindex_plevel_tk_z, "boydenindex.plevel_tk_z(tk700,z700,z1000)");
  ok &= registerFunction(f, f_boydenindex_plevel_th_z, "boydenindex.plevel_th_z(th700,z700,z1000)");
  ok &= registerFunction(f, f_sweatindex_plevel, "sweatindex.plevel(t850,t500,td850,td500,u850,v850,u500,v500)");

  // hybrid model level (HLEVEL) functions
  ok &= registerFunction(f, f_tc_hlevel_th_psurf, "tc.hlevel_th_psurf(th,psurf)");
  ok &= registerFunction(f, f_tk_hlevel_th_psurf, "tk.hlevel_th_psurf(th,psurf)");
  ok &= registerFunction(f, f_th_hlevel_tk_psurf, "th.hlevel_tk_psurf(tk,psurf)");
  ok &= registerFunction(f, f_thesat_hlevel_tk_psurf, "thesat.hlevel_tk_psurf(tk,psurf)");
  ok &= registerFunction(f, f_thesat_hlevel_th_psurf, "thesat.hlevel_th_psurf(th,psurf)");
  ok &= registerFunction(f, f_the_hlevel_tk_q_psurf, "the.hlevel_tk_q_psurf(tk,q,psurf)");
  ok &= registerFunction(f, f_the_hlevel_th_q_psurf, "the.hlevel_th_q_psurf(th,q,psurf)");
  ok &= registerFunction(f, f_rh_hlevel_tk_q_psurf, "rh.hlevel_tk_q_psurf(tk,q,psurf)");
  ok &= registerFunction(f, f_rh_hlevel_th_q_psurf, "rh.hlevel_th_q_psurf(th,q,psurf)");
  ok &= registerFunction(f, f_q_hlevel_tk_rh_psurf, "q.hlevel_tk_rh_psurf(tk,rh,psurf)");
  ok &= registerFunction(f, f_q_hlevel_th_rh_psurf, "q.hlevel_th_rh_psurf(th,rh,psurf)");
  ok &= registerFunction(f, f_tdc_hlevel_tk_q_psurf, "tdc.hlevel_tk_q_psurf(tk,q,psurf)");
  ok &= registerFunction(f, f_tdc_hlevel_th_q_psurf, "tdc.hlevel_th_q_psurf(th,q,psurf)");
  ok &= registerFunction(f, f_tdc_hlevel_tk_rh_psurf, "tdc.hlevel_tk_rh_psurf(tk,rh,psurf)");
  ok &= registerFunction(f, f_tdc_hlevel_th_rh_psurf, "tdc.hlevel_th_rh_psurf(th,rh,psurf)");
  ok &= registerFunction(f, f_tdk_hlevel_tk_q_psurf, "tdk.hlevel_tk_q_psurf(tk,q,psurf)");
  ok &= registerFunction(f, f_tdk_hlevel_th_q_psurf, "tdk.hlevel_th_q_psurf(th,q,psurf)");
  ok &= registerFunction(f, f_tdk_hlevel_tk_rh_psurf, "tdk.hlevel_tk_rh_psurf(tk,rh,psurf)");
  ok &= registerFunction(f, f_tdk_hlevel_th_rh_psurf, "tdk.hlevel_th_rh_psurf(th,rh,psurf)");
  ok &= registerFunction(f, f_ducting_hlevel_tk_q_psurf, "ducting.hlevel_tk_q_psurf(tk,q,psurf)");
  ok &= registerFunction(f, f_ducting_hlevel_th_q_psurf, "ducting.hlevel_th_q_psurf(th,q,psurf)");
  ok &= registerFunction(f, f_ducting_hlevel_tk_rh_psurf, "ducting.hlevel_tk_rh_psurf(tk,rh,psurf)");
  ok &= registerFunction(f, f_ducting_hlevel_th_rh_psurf, "ducting.hlevel_th_rh_psurf(th,rh,psurf)");
  ok &= registerFunction(f, f_pressure_hlevel_xx_psurf, "pressure.hlevel_xx_psurf(xx,psurf)"); // just get eta.a and eta.b from field xx

  // misc atmospheric model level (ALEVEL) functions
  ok &= registerFunction(f, f_tc_alevel_th_p, "tc.alevel_th_p(th,p)");
  ok &= registerFunction(f, f_tk_alevel_th_p, "tk.alevel_th_p(th,p)");
  ok &= registerFunction(f, f_th_alevel_tk_p, "th.alevel_tk_p(tk,p)");
  ok &= registerFunction(f, f_thesat_alevel_tk_p, "thesat.alevel_tk_p(tk,p)");
  ok &= registerFunction(f, f_thesat_alevel_th_p, "thesat.alevel_th_p(th,p)");
  ok &= registerFunction(f, f_the_alevel_tk_q_p, "the.alevel_tk_q_p(tk,q,p)");
  ok &= registerFunction(f, f_the_alevel_th_q_p, "the.alevel_th_q_p(th,q,p)");
  ok &= registerFunction(f, f_rh_alevel_tk_q_p, "rh.alevel_tk_q_p(tk,q,p)");
  ok &= registerFunction(f, f_rh_alevel_th_q_p, "rh.alevel_th_q_p(th,q,p)");
  ok &= registerFunction(f, f_q_alevel_tk_rh_p, "q.alevel_tk_rh_p(tk,rh,p)");
  ok &= registerFunction(f, f_q_alevel_th_rh_p, "q.alevel_th_rh_p(th,rh,p)");
  ok &= registerFunction(f, f_tdc_alevel_tk_q_p, "tdc.alevel_tk_q_p(tk,q,p)");
  ok &= registerFunction(f, f_tdc_alevel_th_q_p, "tdc.alevel_th_q_p(th,q,p)");
  ok &= registerFunction(f, f_tdc_alevel_tk_rh_p, "tdc.alevel_tk_rh_p(tk,rh,p)");
  ok &= registerFunction(f, f_tdc_alevel_th_rh_p, "tdc.alevel_th_rh_p(th,rh,p)");
  ok &= registerFunction(f, f_tdk_alevel_tk_q_p, "tdk.alevel_tk_q_p(tk,q,p)");
  ok &= registerFunction(f, f_tdk_alevel_th_q_p, "tdk.alevel_th_q_p(th,q,p)");
  ok &= registerFunction(f, f_tdk_alevel_tk_rh_p, "tdk.alevel_tk_rh_p(tk,rh,p)");
  ok &= registerFunction(f, f_tdk_alevel_th_rh_p, "tdk.alevel_th_rh_p(th,rh,p)");
  ok &= registerFunction(f, f_ducting_alevel_tk_q_p, "ducting.alevel_tk_q_p(tk,q,p)");
  ok &= registerFunction(f, f_ducting_alevel_th_q_p, "ducting.alevel_th_q_p(th,q,p)");
  ok &= registerFunction(f, f_ducting_alevel_tk_rh_p, "ducting.alevel_tk_rh_p(tk,rh,p)");
  ok &= registerFunction(f, f_ducting_alevel_th_rh_p, "ducting.alevel_th_rh_p(th,rh,p)");

  // ocean depth level (OZLEVEL) functions
  ok &= registerFunction(f, f_sea_soundspeed_ozlevel_tc_salt, "sea.soundspeed.ozlevel_tc_salt(seatemp.c,salt)");
  ok &= registerFunction(f, f_sea_soundspeed_ozlevel_tk_salt, "sea.soundspeed.ozlevel_tk_salt(seatemp.k,salt)");

  // level independent functions
  ok &= registerFunction(f, f_temp_k2c, "temp_k2c(tk)");
  ok &= registerFunction(f, f_temp_c2k, "temp_c2k(tc)");
  ok &= registerFunction(f, f_temp_k2c_possibly, "temp_k2c_possibly(tk)");
  ok &= registerFunction(f, f_temp_c2k_possibly, "temp_c2k_possibly(tc)");
  ok &= registerFunction(f, f_tdk_tk_rh, "tdk.tk_rh(tk,rh)");
  ok &= registerFunction(f, f_tdc_tk_rh, "tdc.tk_rh(tk,rh)");
  ok &= registerFunction(f, f_tdc_tc_rh, "tdc.tc_rh(tc,rh)");
  ok &= registerFunction(f, f_rh_tk_td, "rh.tk_tdk(tk,tdk)");
  ok &= registerFunction(f, f_rh_tc_td, "rh.tc_tdc(tc,tdc)");
  ok &= registerFunction(f, f_vector_abs, "vector.abs(u,v)");
  ok &= registerFunction(f, f_d_dx, "d/dx(f)");
  ok &= registerFunction(f, f_d_dy, "d/dy(f)");
  ok &= registerFunction(f, f_abs_del, "abs.del(f)");
  ok &= registerFunction(f, f_del_square, "del.square(f)");
  ok &= registerFunction(f, f_minvalue_fields, "minvalue.fields(f1,f2)");
  ok &= registerFunction(f, f_maxvalue_fields, "maxvalue.fields(f1,f2)");
  ok &= registerFunction(f, f_minvalue_field_const, "minvalue.field.const(f,const:value)");
  ok &= registerFunction(f, f_maxvalue_field_const, "maxvalue.field.const(f,const:value)");
  ok &= registerFunction(f, f_abs, "abs(f)");
  ok &= registerFunction(f, f_log10, "log10(f)");
  ok &= registerFunction(f, f_pow10, "pow10(f)");
  ok &= registerFunction(f, f_log, "log(f)");
  ok &= registerFunction(f, f_exp, "exp(f)");
  ok &= registerFunction(f, f_power, "power(f,const:exponent)");
  ok &= registerFunction(f, f_shapiro2_filter, "shapiro2.filter(f)");
  ok &= registerFunction(f, f_smooth, "smooth(f,const:numsmooth)");
  ok &= registerFunction(f, f_windcooling_tk_u_v, "windcooling_tk_u_v(tk2m,u10m,v10m)");
  ok &= registerFunction(f, f_windcooling_tc_u_v, "windcooling_tc_u_v(tc2m,u10m,v10m)");
  ok &= registerFunction(f, f_undercooled_rain, "undercooled.rain(precip,snow,tk,const:precipMin,const:snowRateMax,const:tcMax)");
  ok &= registerFunction(f, f_pressure2flightlevel, "pressure2flightlevel(p)");
  ok &= registerFunction(f, f_vessel_icing_overland, "vessel.icing.overland(airtemp,seatemp,u10m,v10m,const:freezingPoint)");
  ok &= registerFunction(f, f_vessel_icing_mertins, "vessel.icing.mertins(airtemp,seatemp,u10m,v10m,const:freezingPoint)");
  ok &= registerFunction(f, f_vessel_icing_overland2, "vessel.icing.overland2(airtemp,seatemp,u10m,v10m,salinity0m,aice)");
  ok &= registerFunction(f, f_vessel_icing_mertins2, "vessel.icing.mertins2(airtemp,seatemp,u10m,v10m,salinity0m,aice)");
  ok &= registerFunction(f, f_vessel_icing_modstall, "vessel.icing.modstall(salinity0m,significant_wave_height,u10m,v10m,tc2m,relative_humidity_2m,temperature0m,air_pressure_at_sea_level,significant_wave_period,aice,depth,const:vs,const:alpha,const:zmin,const:zmax)");
  ok &= registerFunction(f, f_vessel_icing_testmod, "vessel.icing.testmod(salinity0m,significant_wave_height,u10m,v10m,tc2m,relative_humidity_2m,sst,air_pressure_at_sea_level,significant_wave_period,aice,depth,const:vs,const:alpha,const:zmin,const:zmax)");
  ok &= registerFunction(f, f_replace_undefined, "replace.undefined(f,const:value)");
  ok &= registerFunction(f, f_replace_defined, "replace.defined(f,const:value)");
  ok &= registerFunction(f, f_replace_all, "replace.all(f,const:value)");
  ok &= registerFunction(f, f_values2classes, "values2classes(f,const:limits_low_to_high,...)");
  ok &= registerFunction(f, f_field_diff_forecast_hour, "field.diff.forecast.hour(field,const:relHourFirst,const:relHourLast)");
  ok &= registerFunction(f, f_accum_diff_forecast_hour, "accum.diff.forecast.hour(accumfield,const:relHourFirst,const:relHourLast)");
  ok &= registerFunction(f, f_sum_of_forecast_hours, "sum_of_forecast_hours(field,const:forecastHours,...)");
  ok &= registerFunction(f, f_sum_of_fields, "sum_of_fields(field)");
  ok &= registerFunction(f, f_max_of_fields, "max_of_fields(field)");
  ok &= registerFunction(f, f_min_of_fields, "min_of_fields(field)");
  ok &= registerFunction(f, f_no_of_fields_above, "no_of_fields_above(field,const:limit)");
  ok &= registerFunction(f, f_no_of_fields_below, "no_of_fields_below(field,const:limit)");
  ok &= registerFunction(f, f_index_of_fields_max, "index_of_fields_max(field)");
  ok &= registerFunction(f, f_index_of_fields_min, "index_of_fields_min(field)");
  ok &= registerFunction(f, f_sum, "sum(field,...)");
  ok &= registerFunction(f, f_mean_value, "mean_value(field,...)");
  ok &= registerFunction(f, f_stddev, "stddev(field)");
  ok &= registerFunction(f, f_probability_above, "probability_above(field,const:limit)");
  ok &= registerFunction(f, f_probability_below, "probability_below(field,const:limit)");
  ok &= registerFunction(f, f_probability_between, "probability_between(field,const:limit,const:limit)");
  ok &= registerFunction(f, f_number_above, "number_above(field,const:limit)");
  ok &= registerFunction(f, f_number_below, "number_below(field,const:limit)");
  ok &= registerFunction(f, f_number_between, "number_between(field,const:limit,const:limit)");
  ok &= registerFunction(f, f_equivalent_to, "equivalent_to(field)");
  ok &= registerFunction(f, f_min_value, "min_value(field,...)");
  ok &= registerFunction(f, f_max_value, "max_value(field,...)");
  ok &= registerFunction(f, f_min_index, "min_index(field,...)");
  ok &= registerFunction(f, f_max_index, "max_index(field,...)");
  ok &= registerFunction(f, f_percentile, "percentile(field,const:value,...)");
  ok &= registerFunction(f, f_neighbour_percentile, "neighbour_percentile(field,const:value,...)");
  ok &= registerFunction(f, f_neighbour_mean, "neighbour_mean(field,const:value,...)");
  ok &= registerFunction(f, f_neighbour_probability_above, "neighbour_probability_above(field,const:value,...)");
  ok &= registerFunction(f, f_neighbour_probability_below, "neighbour_probability_below(field,const:value,...)");
  ok &= registerFunction(f, f_snow_cm_from_snow_water_tk_td, "snow.cm.from.snow.water(snow,tk,td)");

  // geographic functions

  // initialize function texts
  ok &= registerFunction(f, f_qvector_plevel_z_tk_xcomp, "qvector.plevel_z_tk_xcomp(z,tk)");
  ok &= registerFunction(f, f_qvector_plevel_z_th_xcomp, "qvector.plevel_z_th_xcomp(z,th)");
  ok &= registerFunction(f, f_qvector_plevel_z_tk_ycomp, "qvector.plevel_z_tk_ycomp(z,tk)");
  ok &= registerFunction(f, f_qvector_plevel_z_th_ycomp, "qvector.plevel_z_th_ycomp(z,th)");
  ok &= registerFunction(f, f_geostrophic_wind_plevel_z_xcomp, "geostrophic.wind.plevel_z_xcomp(z)");
  ok &= registerFunction(f, f_geostrophic_wind_plevel_z_ycomp, "geostrophic.wind.plevel_z_ycomp(z)");
  ok &= registerFunction(f, f_geostrophic_vorticity_plevel_z, "geostrophic.vorticity.plevel_z(z)");

  // isentropic level (ILEVEL) function NB! functions with two output fields do not work (TODO)
  ok &= registerFunction(f, f_geostrophic_wind_ilevel_mpot, "geostrophic_wind.ilevel_mpot(mpot)2");

  // level independent functions
  ok &= registerFunction(f, f_rel_vorticity, "rel.vorticity(u,v)");
  ok &= registerFunction(f, f_abs_vorticity, "abs.vorticity(u,v)");
  ok &= registerFunction(f, f_divergence, "divergence(u,v)");
  ok &= registerFunction(f, f_advection, "advection(f,u,v,const:hours)");
  ok &= registerFunction(f, f_thermal_front_parameter_tx, "thermal.front.parameter_tx(tx)");
  ok &= registerFunction(f, f_momentum_x_coordinate, "momentum.x.coordinate(v,const:coriolisMin)");
  ok &= registerFunction(f, f_momentum_y_coordinate, "momentum.y.coordinate(u,const:coriolisMin)");
  ok &= registerFunction(f, f_jacobian, "jacobian(fx,fy)");

  return ok;
}

// static member
bool FieldFunctions::parseComputeSetup(const vector<std::string>& lines, vector<std::string>& errors)
{
  METLIBS_LOG_SCOPE();

  functions_t functions;
  if (!registerFunctions(functions))
    return false;

  // parse setup
  map<std::string, int> compute;
  compute["add"] = 0;
  compute["subtract"] = 1;
  compute["multiply"] = 2;
  compute["divide"] = 3;
  map<std::string, int>::const_iterator pc, pcend = compute.end();

  functions_t::const_iterator pf;

  const int nlines = lines.size();
  for (int l = 0; l < nlines; l++) {
    const std::string& oneline = lines[l];
    const vector<std::string> vstr = miutil::split(oneline, 0, " ", true);
    int n = vstr.size();
    for (int i = 0; i < n; i++) {
      bool err = true;
      const vector<std::string> vspec = miutil::split(vstr[i], 1, "=", false);
      if (vspec.size() == 2) {
        const size_t p1 = vspec[1].find('('), p2 = vspec[1].find(')');
        if (p1 != string::npos && p2 != string::npos && p1 > 0 && p1 < p2 - 1) {
          const std::string functionName = vspec[1].substr(0, p1);
          const std::string str = vspec[1].substr(p1 + 1, p2 - p1 - 1);
          FieldCompute fcomp;
          fcomp.name = vspec[0];
          fcomp.functionName = miutil::to_upper(functionName);
          VerticalType vctype = vctype_none;

          // First check if function is a simple calculation
          if ((pc = compute.find(functionName)) != pcend) {
            fcomp.results.push_back(fcomp.name);
            const vector<std::string> vpart = miutil::split(str, 1, ",");
            if (vpart.size() == 2) {
              bool b0 = miutil::is_number(vpart[0]);
              bool b1 = miutil::is_number(vpart[1]);
              if (!b0 && !b1) {
                fcomp.function = Function(f_add_f_f + pc->second);
                fcomp.input.push_back(vpart[0]);
                fcomp.input.push_back(vpart[1]);
                err = false;
              } else if (!b0 && b1) {
                fcomp.function = Function(f_add_f_c + pc->second);
                fcomp.input.push_back(vpart[0]);
                fcomp.constants.push_back(atof(vpart[1].c_str()));
                err = false;
              } else if (b0 && !b1) {
                fcomp.function = Function(f_add_c_f + pc->second);
                fcomp.input.push_back(vpart[1]);
                fcomp.constants.push_back(atof(vpart[0].c_str()));
                err = false;
              }
            }

            // then check if function is defined in the FunctionHelper container
          } else if ((pf = functions.find(miutil::to_lower(functionName))) != functions.end()) {
            fcomp.function = pf->second.func;
            // check function arguments
            vector<std::string> vpart = miutil::split(str, 0, ",", false);
            int npart = vpart.size();
            int numf = pf->second.numfields;
            int numc = pf->second.numconsts;
            if (numf < 0 ) {
              numf = npart;
            }
            unsigned int numr = pf->second.numresult;
            vctype = pf->second.vertcoord;
            // if any fields or constants as input...
            if ((numc >= 0 && npart == numf + numc)
                || (numc < 0 && npart >= numf - numc))
            {
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
              vFieldCompute.push_back(fcomp);
            }
          }
        }
      }
      if (err) {
        std::string errm = FIELD_COMPUTE_SECTION()
            + "|" + miutil::from_number(l)
            + "|Error in field compute : " + vstr[i];
        errors.push_back(errm);
        // show a warning, but do not fail
      }
    }
  }

  for (size_t n = 0; n < vFieldCompute.size(); n++) {
    if (!mFieldName.count(vFieldCompute[n].name)) {
      mFieldName[vFieldCompute[n].name] = vFieldName.size();
      vFieldName.push_back(vFieldCompute[n].name);
    }
  }

  return true;
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
          zaxis_info.vctype= getVerticalType(stokens[1]);
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

void FieldFunctions::setFieldNames(const vector<std::string>& vfieldname)
{
  vFieldName = vfieldname;
  for(size_t i=0;i<vfieldname.size();i++) {
    mFieldName[vfieldname[i]]=i;
  }
}

// static
bool FieldFunctions::isTimeStepFunction(Function f)
{
  return mapTimeStepFunction(f);
}

// static
bool FieldFunctions::mapTimeStepFunction(Function& f)
{
  if (f == f_field_diff_forecast_hour || f == f_accum_diff_forecast_hour)
    f = f_subtract_f_f;
  else if (f == f_sum_of_forecast_hours)
    f = f_sum_f;
  else if (f == f_sum_of_fields)
    f = f_sum_f;
  else if (f == f_max_of_fields)
    f = f_max_value;
  else if (f == f_min_of_fields)
    f = f_min_value;
  else if (f == f_no_of_fields_above)
    f = f_number_above;
  else if (f == f_no_of_fields_below)
    f = f_number_below;
  else if (f == f_index_of_fields_max)
    f = f_max_index;
  else if (f == f_index_of_fields_min)
    f = f_min_index;
  else
    return false;

  return true;
}

bool FieldFunctions::fieldComputer(Function function,
    const std::vector<float>& constants, const std::vector<Field*>& vfinput,
    const std::vector<Field*>& vfres, GridConverter& gc)
{
  using namespace FieldCalculations;

#ifdef ENABLE_FIELDFUNCTIONS_TIMING
  METLIBS_LOG_TIME();
#endif

  // Perform field computation according to 'function'.
  // All fields must have the same dimensions (nx and ny).
  // All "horizontal" computations assuming nonstaggered fields (A-grid).

  if (vfinput.empty() || vfres.empty()) {
    return false;
  }

  mapTimeStepFunction(function);

  int nx = vfinput[0]->area.nx;
  int ny = vfinput[0]->area.ny;
  int fsize = nx * ny;

  vector<float*> finp;
  vector<difield::ValuesDefined> fdefin;
  vector<float*> fout;

  bool ok = true;

  int ninp = vfinput.size();
  int nout = vfres.size();
  int nconst = constants.size();

  difield::ValuesDefined fDefined = (ninp > 0) ? vfinput.front()->defined() : difield::NONE_DEFINED;

  std::string unit;

  for (Field* fi : vfinput) {
    finp.push_back(fi->data);
    fdefin.push_back(fi->defined());
    if (fi->defined() == difield::NONE_DEFINED)
      fDefined = difield::NONE_DEFINED;
    if (fi->defined() == difield::SOME_DEFINED)
      fDefined = difield::SOME_DEFINED; // not perfectly correct
    if (fi->area.nx != nx || fi->area.ny != ny)
      ok = false;
  }

  for (int i = 0; i < nout; i++) {
    fout.push_back(vfres[i]->data);
    unit = vfres[i]->unit;
    if (vfres[i]->area.nx != nx || vfres[i]->area.ny != ny)
      ok = false;
  }

  if (!ok) {
    METLIBS_LOG_ERROR("Field dimensions not equal");
    return false;
  }

  const float undef = difield::UNDEF;

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
    res = pleveltemp(compute, nx, ny, finp[0], fout[0], plevel, fDefined,
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
        fDefined, undef);
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
        fDefined, undef, unit);
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
        plevel2, fDefined, undef);
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
        fDefined, undef);
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
        fout[0], plevel1, plevel2, plevel3, fDefined, undef);
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
        fDefined, undef);
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
        plevel1, plevel2, fDefined, undef);
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
        plevel1, plevel2, fDefined, undef);
    break;

  case f_sweatindex_plevel:
    if (ninp != 8 || nout != 1)
      break;
    res = sweatIndex(compute, nx, ny, finp[0], finp[1], finp[2], finp[3],
        finp[4], finp[5], finp[6], finp[7], fout[0], fDefined, undef);
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
        blevel, fDefined, undef,unit);
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
        alevel, blevel, fDefined, undef);
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
        alevel, blevel, fDefined, undef, unit);
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
        alevel, blevel, fDefined, undef);
    break;

  case f_pressure_hlevel_xx_psurf:
    if (ninp != 2 || nout != 1)
      break;
    alevel = vfinput[0]->aHybrid;
    blevel = vfinput[0]->bHybrid;
    res = hlevelpressure(nx, ny, finp[1], fout[0], alevel, blevel, fDefined,
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
    res = aleveltemp(compute, nx, ny, finp[0], finp[1], fout[0], fDefined,
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
        fDefined, undef);
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
        fDefined, undef, unit);
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
        fDefined, undef);
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
        fDefined, undef);
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
    res = cvtemp(compute, nx, ny, finp[0], fout[0], fDefined, undef);
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
    res = cvhum(compute, nx, ny, finp[0], finp[1], fout[0], fDefined, undef, unit);
    break;

  case f_vector_abs:
    if (ninp != 2 || nout != 1)
      break;
    res = vectorabs(nx, ny, finp[0], finp[1], fout[0], fDefined, undef);
    break;

  case f_minvalue_fields:
    if (ninp != 2 || nout != 1)
      break;
    DIUTIL_OPENMP_PARALLEL(fsize, for)
    for (int i = 0; i < fsize; i++) {
      if (calculations::is_defined(fDefined, finp[0][i], finp[1][i], difield::UNDEF))
        fout[0][i] = std::min(finp[0][i], finp[1][i]);
      else
        fout[0][i] = difield::UNDEF;
    }
    res = true;
    break;

  case f_maxvalue_fields:
    if (ninp != 2 || nout != 1)
      break;
    DIUTIL_OPENMP_PARALLEL(fsize, for)
    for (int i = 0; i < fsize; i++) {
      if (calculations::is_defined(fDefined, finp[0][i], finp[1][i], difield::UNDEF))
        fout[0][i] = std::max(finp[0][i], finp[1][i]);
      else
        fout[0][i] = difield::UNDEF;
    }
    res = true;
    break;

  case f_minvalue_field_const:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    constant = constants[0];
    if (constant != difield::UNDEF) {
      DIUTIL_OPENMP_PARALLEL(fsize, for)
      for (int i = 0; i < fsize; i++) {
        if (calculations::is_defined(fDefined, finp[0][i], difield::UNDEF))
          fout[0][i] = std::min(finp[0][i], constant);
        else
          fout[0][i] = difield::UNDEF;
      }
    } else {
      fDefined = difield::NONE_DEFINED;
      DIUTIL_OPENMP_PARALLEL(fsize, for)
      for (int i = 0; i < fsize; i++)
        fout[0][i] = difield::UNDEF;
    }
    res = true;
    break;

  case f_maxvalue_field_const:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    constant = constants[0];
    if (constant != difield::UNDEF) {
      DIUTIL_OPENMP_PARALLEL(fsize, for)
      for (int i = 0; i < fsize; i++) {
        if (calculations::is_defined(fDefined, finp[0][i], difield::UNDEF))
          fout[0][i] = std::max(finp[0][i], constant);
        else
          fout[0][i] = difield::UNDEF;
      }
    } else {
      fDefined = difield::NONE_DEFINED;
      DIUTIL_OPENMP_PARALLEL(fsize, for)
      for (int i = 0; i < fsize; i++)
        fout[0][i] = difield::UNDEF;
    }
    res = true;
    break;

  case f_abs:
    if (ninp != 1 || nout != 1)
      break;
    DIUTIL_OPENMP_PARALLEL(fsize, for)
    for (int i = 0; i < fsize; i++) {
      if (calculations::is_defined(fDefined, finp[0][i], difield::UNDEF))
        fout[0][i] = fabs(finp[0][i]);
      else
        fout[0][i] = difield::UNDEF;
    }
    res = true;
    break;

  case f_log10:
    if (ninp != 1 || nout != 1)
      break;
    DIUTIL_OPENMP_PARALLEL(fsize, for)
    for (int i = 0; i < fsize; i++) {
      if (calculations::is_defined(fDefined, finp[0][i], difield::UNDEF))
        fout[0][i] = log10(finp[0][i]);
      else
        fout[0][i] = difield::UNDEF;
    }
    res = true;
    break;

  case f_pow10:
    if (ninp != 1 || nout != 1)
      break;
    DIUTIL_OPENMP_PARALLEL(fsize, for)
    for (int i = 0; i < fsize; i++) {
      if (calculations::is_defined(fDefined, finp[0][i], difield::UNDEF))
        fout[0][i] = pow10(finp[0][i]);
      else
        fout[0][i] = difield::UNDEF;
    }
    res = true;
    break;

  case f_log:
    if (ninp != 1 || nout != 1)
      break;
    DIUTIL_OPENMP_PARALLEL(fsize, for)
    for (int i = 0; i < fsize; i++) {
      if (calculations::is_defined(fDefined, finp[0][i], difield::UNDEF))
        fout[0][i] = log(finp[0][i]);
      else
        fout[0][i] = difield::UNDEF;
    }
    res = true;
    break;

  case f_exp:
    if (ninp != 1 || nout != 1)
      break;
    DIUTIL_OPENMP_PARALLEL(fsize, for)
    for (int i = 0; i < fsize; i++) {
      if (calculations::is_defined(fDefined, finp[0][i], difield::UNDEF))
        fout[0][i] = exp(finp[0][i]);
      else
        fout[0][i] = difield::UNDEF;
    }
    res = true;
    break;

  case f_power:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    constant = constants[0];
    if (constant != difield::UNDEF) {
      DIUTIL_OPENMP_PARALLEL(fsize, for)
      for (int i = 0; i < fsize; i++) {
        if (calculations::is_defined(fDefined, finp[0][i], difield::UNDEF))
          fout[0][i] = powf(finp[0][i], constant);
        else
          fout[0][i] = difield::UNDEF;
      }
    } else {
      fDefined = difield::NONE_DEFINED;
      DIUTIL_OPENMP_PARALLEL(fsize, for)
      for (int i = 0; i < fsize; i++)
        fout[0][i] = difield::UNDEF;
    }
    res = true;
    break;

  case f_shapiro2_filter:
    if (ninp != 1 || nout != 1)
      break;
    res = shapiro2_filter(nx, ny, finp[0], fout[0], fDefined, undef);
    break;

  case f_smooth:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    nsmooth = int(constants[0] + 0.5);
    calculations::copy_field(fout[0], finp[0], fsize);
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
        fDefined, undef);
    break;

  case f_undercooled_rain:
    if (ninp != 3 || nout != 1 || nconst != 3)
      break;
    precipMin = constants[0];
    snowRateMax = constants[1];
    tcMax = constants[2];
    res = underCooledRain(nx, ny, finp[0], finp[1], finp[2], fout[0],
        precipMin, snowRateMax, tcMax, fDefined, undef);
    break;

  case f_pressure2flightlevel:
    if (ninp != 1 || nout != 1)
      break;
    res = pressure2FlightLevel(nx, ny, finp[0], fout[0], fDefined, undef);
    break;

  case f_vessel_icing_overland:
    if (ninp != 4 || nout != 1 || nconst != 1)
      break;
    res = vesselIcingOverland(nx, ny, finp[0], finp[1], finp[2], finp[3], fout[0],
        constants[0], fDefined, undef);
    break;

  case f_vessel_icing_mertins:
    if (ninp != 4 || nout != 1 || nconst != 1)
      break;
    res = vesselIcingMertins(nx, ny, finp[0], finp[1], finp[2], finp[3], fout[0],
        constants[0], fDefined, undef);
    break;


  case f_vessel_icing_overland2:
    if (ninp != 6 || nout != 1)
      break;
    res = vesselIcingOverland2(nx, ny, finp[0], finp[1], finp[2], finp[3], finp[4], finp[5], fout[0],
        fDefined, undef);
    break;

  case f_vessel_icing_mertins2:
    if (ninp != 6 || nout != 1)
      break;
    res = vesselIcingMertins2(nx, ny, finp[0], finp[1], finp[2], finp[3], finp[4], finp[5], fout[0],
        fDefined, undef);
    break;

  case f_vessel_icing_modstall:
    if (ninp != 11 || nout != 1 || nconst != 4)
      break;
    res = vesselIcingModStall(nx, ny, finp[0], finp[1], finp[2], finp[3], finp[4],
        finp[5], finp[6], finp[7], finp[8], finp[9], finp[10], fout[0],
        constants[0], constants[1], constants[2], constants[3], fDefined, undef);
    break;

  case f_vessel_icing_testmod:
    if (ninp != 11 || nout != 1 || nconst != 4)
      break;
    res = vesselIcingTestMod(nx, ny, finp[0], finp[1], finp[2], finp[3], finp[4],
        finp[5], finp[6], finp[7], finp[8], finp[9], finp[10], fout[0],
        constants[0], constants[1], constants[2], constants[3], fDefined, undef);
    break;

  case f_replace_undefined:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    constant = constants[0];
    DIUTIL_OPENMP_PARALLEL(fsize, for)
    for (int i = 0; i < fsize; i++)
      if (finp[0][i] != difield::UNDEF)
        fout[0][i] = finp[0][i];
      else
        fout[0][i] = constant;
    vfres[0]->checkDefined();
    res = true;
    break;

  case f_replace_defined:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    constant = constants[0];
    DIUTIL_OPENMP_PARALLEL(fsize, for)
    for (int i = 0; i < fsize; i++)
      if (finp[0][i] != difield::UNDEF)
        fout[0][i] = constant;
      else
        fout[0][i] = difield::UNDEF;
    res = true;
    break;

  case f_replace_all:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    vfres[0]->fill(constants[0]);
    res = true;
    break;

  case f_values2classes:
    if (ninp != 1 || nout != 1 || nconst < 2)
      break;
    // constants has at least low_value and high_value
    res = values2classes(nx, ny, finp[0], fout[0], constants,
        fDefined, undef);
    break;

  // time step functions
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
        fDefined, undef);
    break;

  case f_sum_f:
    if (compute == 0)
      compute = 5; // field + field + field +
    res = sumFields(nx, ny, finp, fout[0],
        fDefined, undef);
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
        fDefined, undef);
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
        fDefined, undef);
    break;

  case f_equivalent_to :
    if (ninp != 1 || nout != 1)
      break;
    calculations::copy_field(fout[0], finp[0], fsize);
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
        fDefined, undef);
    break;


  case f_mean_value:
    res = meanValue(nx, ny, finp, fdefin, fout[0], fDefined, undef);
    break;

  case f_sum:
    res = sumFields(nx, ny, finp, fout[0], fDefined, undef);
    break;

  case f_stddev:
    res = stddevValue(nx, ny, finp, fdefin, fout[0], fDefined, undef);
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
    res = probability(compute, nx, ny, finp, fdefin, constants,  fout[0],
        fDefined, undef);
    break;

  case f_percentile:
  case f_neighbour_percentile:
    if (compute == 0)
      compute = 4;
  case f_neighbour_probability_above:
    if (compute == 0)
      compute = 2;
  case f_neighbour_probability_below:
    if (compute == 0)
      compute = 3;
  case f_neighbour_mean:
    if (compute == 0)
      compute = 1;
    if (ninp != 1 || nout != 1 || nconst < 1)
      break;
    res = neighbourFunctions(nx, ny, finp[0], constants, compute, fout[0],
        fDefined, undef);
    break;

  case f_snow_cm_from_snow_water_tk_td:
    if (ninp != 3 || nout != 1)
      break;
    res = snow_in_cm(nx, ny, finp[0], finp[1], finp[2], fout[0], fDefined, undef);
    break;

    // ==================== begin geographic functions

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
      res = FieldCalculations::plevelqvector(compute, nx, ny, finp[0], finp[1], fout[0],
          xmapr, ymapr, fcoriolis, plevel, fDefined, undef);
    break;

  case f_geostrophic_wind_plevel_z_xcomp:
    if (ninp != 1 || nout != 1)
      break;
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, &fcoriolis))
      res = FieldCalculations::plevelgwind_xcomp(nx, ny, finp[0], fout[0], xmapr, ymapr,
          fcoriolis, fDefined, undef);
    break;

  case f_geostrophic_wind_plevel_z_ycomp:
    if (ninp != 1 || nout != 1)
      break;
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, &fcoriolis))
      res = FieldCalculations::plevelgwind_ycomp(nx, ny, finp[0], fout[0], xmapr, ymapr,
          fcoriolis, fDefined, undef);
    break;

  case f_geostrophic_vorticity_plevel_z:
    if (ninp != 1 || nout != 1)
      break;
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, &fcoriolis))
      res = FieldCalculations::plevelgvort(nx, ny, finp[0], fout[0], xmapr, ymapr, fcoriolis,
          fDefined, undef);
    break;

  case f_geostrophic_wind_ilevel_mpot:
    if (ninp != 1 || nout != 2)
      break;
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, &fcoriolis))
      res = FieldCalculations::ilevelgwind(nx, ny, finp[0], fout[0], fout[1], xmapr, ymapr,
          fcoriolis, fDefined, undef);
    break;

    //---------------------------------------------------
    // level (pressure) independent functions
    //---------------------------------------------------

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
      res = gradient(compute, nx, ny, finp[0], fout[0], xmapr, ymapr, fDefined, undef);
    break;

  case f_rel_vorticity:
    if (ninp != 2 || nout != 1)
      break;
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, 0))
      res = FieldCalculations::relvort(nx, ny, finp[0], finp[1], fout[0], xmapr, ymapr,
          fDefined, undef);
    break;

  case f_abs_vorticity:
    if (ninp != 2 || nout != 1)
      break;
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, &fcoriolis))
      res = FieldCalculations::absvort(nx, ny, finp[0], finp[1], fout[0], xmapr, ymapr, fcoriolis,
          fDefined, undef);
    break;

  case f_divergence:
    if (ninp != 2 || nout != 1)
      break;
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, 0))
      res = FieldCalculations::divergence(nx, ny, finp[0], finp[1], fout[0], xmapr, ymapr,
          fDefined, undef);
    break;

  case f_advection:
    if (ninp != 3 || nout != 1 || nconst != 1)
      break;
    hours = constants[0];
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, 0))
      res = FieldCalculations::advection(nx, ny, finp[0], finp[1], finp[2], fout[0], xmapr, ymapr,
          hours, fDefined, undef);
    break;

  case f_thermal_front_parameter_tx:
    if (ninp != 1 || nout != 1)
      break;
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, 0))
      res = FieldCalculations::thermalFrontParameter(nx, ny, finp[0], fout[0], xmapr, ymapr,
          fDefined, undef);
    break;

  case f_momentum_x_coordinate:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    fcoriolisMin = constants[0];
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, &fcoriolis))
      res = FieldCalculations::momentumXcoordinate(nx, ny, finp[0], fout[0], xmapr, fcoriolis,
          fcoriolisMin, fDefined, undef);
    break;

  case f_momentum_y_coordinate:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    fcoriolisMin = constants[0];
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, &fcoriolis))
      res = FieldCalculations::momentumYcoordinate(nx, ny, finp[0], fout[0], ymapr, fcoriolis,
          fcoriolisMin, fDefined, undef);
    break;

  case f_jacobian:
    if (ninp != 2 || nout != 1)
      break;
    if (gc.getMapFields(vfinput[0]->area, &xmapr, &ymapr, 0))
      res = FieldCalculations::jacobian(nx, ny, finp[0], finp[1], fout[0], xmapr, ymapr,
          fDefined, undef);
    break;

    // ==================== end geographic functions

  default:
    METLIBS_LOG_ERROR("Illegal function='" << function << "'");
    break;
  }

  // fDefined might have changed during computation
  for (int i = 0; i < nout; i++) {
    vfres[i]->forceDefined(fDefined);
  }
  return res;
}
