// -*- c++ -*-
/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2015 met.no

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

#ifndef DIFIELD_FIELDFUNCTIONS_H
#define DIFIELD_FIELDFUNCTIONS_H

#include <map>
#include <string>
#include <vector>

class Area;
class Field;
class GridConverter;

/// field computation/conversion functions
///
/// Wrapper for a set of functions for calculations on fields and a set of keys
/// (in the form of an enumeration) signifying invocations of the functions.
/// Each function have one or more associated invocations.
///

/*
 How to add a new function:

 We will introduce a function performing calculation XXX on a single field, and add two modes or ways to call it

 1. Add the actual function in diFieldCalculations.h/.cc if it is
    independent of Area and GridConverter, else here. Something like:

    bool XXX(int compute, int nx, int ny, float *inp, float *out, bool& allDefined, float undef);

    the 'compute' variable has values in [1,2], and signifies the two different modes of the function

 2. Add two f_XXX definitions to the Function enum found below (in the correct section), each signifying a way of invoking XXX()
    ..
    f_XXX_1,
    f_XXX_2,
    ..
 3. In constructor: Add a text for the two Functions to the functionText map describing input/output arguments to XXX()

    functionText[f_XXX_1] = "XXX_1(f)";
    functionText[f_XXX_2] = "XXX_2(f)";

 4. In the fieldComputer() function: add code for your new function to the case () list

    case f_XXX_1:
         if (compute == 0)
           compute = 1;
    case f_XXX_2:
         if (compute == 0)
           compute = 2;
         res = XXX(compute, nx, ny, finp[0], fout[0], allDefined, undef);
         break;

 5. Your new calculations are now accessible from the setup file (section FIELD_COMPUTE) like this:

    tkXXX1=XXX_1(tk)
    tkXXX2=XXX_2(tk)

 NOTE:
 Parsing of the relevant section in the setup file is currently done in FieldSource TODO: fix this

 More information:

 ----------------------------------------------------------------------
 Function texts / descriptions in map functionText
 - The text is a declaration of the function name and call parameters.
   The actual use of the function is realized in the setup file.
 - A parameter like "const:name" means a float or integer number.
   Constants must always come after all input fields!
 - The last argument may signify a variable number of constants: "const:name,..."
   The key here is the use of ",..." as the last argument
 - If the function returns more than one result field, this
   is shown with "function(.....)2", a function that returns 2 fields.
   Use of such a function: "ug,vg=geostrophic.wind.plevel_z(z)".

 ----------------------------------------------------------------------
 Names used for vertical levels:
 plevel  = pressure levels
 hlevel  = model levels, sigma(norlam), eta/hybrid(hirlam,ecmwf,...)
 alevel  = model levels, any vertical coordinate with pressure field in each level
 ilevel  = isentrpoic levels (constant potential temperature levels)
 ozlevel = ocean depth levels

 The function name should include one of these to ensure correct level type.

 ----------------------------------------------------------------------
 Names used for parameters:
 th    = potential temperature (Kelvin)
 tk    = temperature (Kelvin)
 tc    = temperature (Celsius)
 tx    = any temperature
 tdk   = dew point temperature (Kelvin)
 tdc   = dew point temperature (Celsius)
 rh    = relative humidity (%)
 q     = specific humidity (kg/kg)
 psurf = surface pressure (hPa) needed for hlevel functions
 p     = pressure (hPa) needed in all levels for alevel functions
 z     = height (m)
 f     = any scalar field
 u,v   = wind components in grid x- and y-direction (m/s)
 salt  = water salt content in ppt

 */

class FieldFunctions {
public:
  /**
   * 'Function' enumerates the calculations that are supported
   * - The 'simple compute' section is special - these need no function texts
   * - The rest of the functions must be supported by the 'functionText' map to be usable
   */
  enum Function {
    f_undefined = -1, //!< f_undefined

    // simple compute functions (do not add function texts for these)
    f_add_f_f, //!< field + field
    f_subtract_f_f, //!< field - field
    f_multiply_f_f, //!< field * field
    f_divide_f_f, //!< field / field
    f_add_f_c, //!< field + constant
    f_subtract_f_c, //!< field - constant
    f_multiply_f_c, //!< field * constant
    f_divide_f_c, //!< field / constant
    f_add_c_f, //!< constant + field
    f_subtract_c_f, //!< constant - field
    f_multiply_c_f, //!< constant * field
    f_divide_c_f, //!< constant / field
    f_sum_f, //!<field+field+field ...

    // pressure level (PLEVEL) functions
    f_tc_plevel_th, //!< f_tc_plevel_th
    f_tk_plevel_th, //!< f_tk_plevel_th
    f_th_plevel_tk, //!< f_th_plevel_tk
    f_thesat_plevel_tk, //!< f_thesat_plevel_tk
    f_thesat_plevel_th, //!< f_thesat_plevel_th
    f_the_plevel_tk_rh, //!< f_the_plevel_tk_rh
    f_the_plevel_th_rh, //!< f_the_plevel_th_rh
    f_rh_plevel_tk_q, //!< f_rh_plevel_tk_q
    f_rh_plevel_th_q, //!< f_rh_plevel_th_q
    f_q_plevel_tk_rh, //!< f_q_plevel_tk_rh
    f_q_plevel_th_rh, //!< f_q_plevel_th_rh
    f_tdc_plevel_tk_rh, //!< f_tdc_plevel_tk_rh
    f_tdc_plevel_th_rh, //!< f_tdc_plevel_th_rh
    f_tdc_plevel_tk_q, //!< f_tdc_plevel_tk_q
    f_tdc_plevel_th_q, //!< f_tdc_plevel_th_q
    f_tdk_plevel_tk_rh, //!< f_tdk_plevel_tk_rh
    f_tdk_plevel_th_rh, //!< f_tdk_plevel_th_rh
    f_tdk_plevel_tk_q, //!< f_tdk_plevel_tk_q
    f_tdk_plevel_th_q, //!< f_tdk_plevel_th_q
    f_tcmean_plevel_z1_z2, //!< f_tcmean_plevel_z1_z2
    f_tkmean_plevel_z1_z2, //!< f_tkmean_plevel_z1_z2
    f_thmean_plevel_z1_z2, //!< f_thmean_plevel_z1_z2
    f_qvector_plevel_z_tk_xcomp, //!< f_qvector_plevel_z_tk_xcomp; geographic
    f_qvector_plevel_z_th_xcomp, //!< f_qvector_plevel_z_th_xcomp; geographic
    f_qvector_plevel_z_tk_ycomp, //!< f_qvector_plevel_z_tk_ycomp; geographic
    f_qvector_plevel_z_th_ycomp, //!< f_qvector_plevel_z_th_ycomp; geographic
    f_ducting_plevel_tk_q, //!< f_ducting_plevel_tk_q
    f_ducting_plevel_th_q, //!< f_ducting_plevel_th_q
    f_ducting_plevel_tk_rh, //!< f_ducting_plevel_tk_rh
    f_ducting_plevel_th_rh, //!< f_ducting_plevel_th_rh
    f_geostrophic_wind_plevel_z_xcomp, //!< f_geostrophic_wind_plevel_z_xcomp; geographic
    f_geostrophic_wind_plevel_z_ycomp, //!< f_geostrophic_wind_plevel_z_ycomp; geographic
    f_geostrophic_vorticity_plevel_z,//!< f_geostrophic_vorticity_plevel_z; geographic
    f_kindex_plevel_tk_rh, //!< f_kindex_plevel_tk_rh
    f_kindex_plevel_th_rh, //!< f_kindex_plevel_th_rh
    f_ductingindex_plevel_tk_rh, //!< f_ductingindex_plevel_tk_rh
    f_ductingindex_plevel_th_rh, //!< f_ductingindex_plevel_th_rh
    f_showalterindex_plevel_tk_rh, //!< f_showalterindex_plevel_tk_rh
    f_showalterindex_plevel_th_rh, //!< f_showalterindex_plevel_th_rh
    f_boydenindex_plevel_tk_z, //!< f_boydenindex_plevel_tk_z
    f_boydenindex_plevel_th_z, //!< f_boydenindex_plevel_th_z
    f_sweatindex_plevel, //!< Severe Weather Threat Index

    // hybrid model level (HLEVEL) functions
    f_tc_hlevel_th_psurf, //!< f_tc_hlevel_th_psurf
    f_tk_hlevel_th_psurf, //!< f_tk_hlevel_th_psurf
    f_th_hlevel_tk_psurf, //!< f_th_hlevel_tk_psurf
    f_thesat_hlevel_tk_psurf, //!< f_thesat_hlevel_tk_psurf
    f_thesat_hlevel_th_psurf, //!< f_thesat_hlevel_th_psurf
    f_the_hlevel_tk_q_psurf, //!< f_the_hlevel_tk_q_psurf
    f_the_hlevel_th_q_psurf, //!< f_the_hlevel_th_q_psurf
    f_rh_hlevel_tk_q_psurf, //!< f_rh_hlevel_tk_q_psurf
    f_rh_hlevel_th_q_psurf, //!< f_rh_hlevel_th_q_psurf
    f_q_hlevel_tk_rh_psurf, //!< f_q_hlevel_tk_rh_psurf
    f_q_hlevel_th_rh_psurf, //!< f_q_hlevel_th_rh_psurf
    f_tdc_hlevel_tk_q_psurf, //!< f_tdc_hlevel_tk_q_psurf
    f_tdc_hlevel_th_q_psurf, //!< f_tdc_hlevel_th_q_psurf
    f_tdc_hlevel_tk_rh_psurf, //!< f_tdc_hlevel_tk_rh_psurf
    f_tdc_hlevel_th_rh_psurf, //!< f_tdc_hlevel_th_rh_psurf
    f_tdk_hlevel_tk_q_psurf, //!< f_tdk_hlevel_tk_q_psurf
    f_tdk_hlevel_th_q_psurf, //!< f_tdk_hlevel_th_q_psurf
    f_tdk_hlevel_tk_rh_psurf, //!< f_tdk_hlevel_tk_rh_psurf
    f_tdk_hlevel_th_rh_psurf, //!< f_tdk_hlevel_th_rh_psurf
    f_ducting_hlevel_tk_q_psurf, //!< f_ducting_hlevel_tk_q_psurf
    f_ducting_hlevel_th_q_psurf, //!< f_ducting_hlevel_th_q_psurf
    f_ducting_hlevel_tk_rh_psurf, //!< f_ducting_hlevel_tk_rh_psurf
    f_ducting_hlevel_th_rh_psurf, //!< f_ducting_hlevel_th_rh_psurf
    f_pressure_hlevel_xx_psurf, // just get eta.a and eta.b from field xx

    // misc atmospheric model level (ALEVEL) functions
    f_tc_alevel_th_p, //!< f_tc_alevel_th_p
    f_tk_alevel_th_p, //!< f_tk_alevel_th_p
    f_th_alevel_tk_p, //!< f_th_alevel_tk_p
    f_thesat_alevel_tk_p, //!< f_thesat_alevel_tk_p
    f_thesat_alevel_th_p, //!< f_thesat_alevel_th_p
    f_the_alevel_tk_q_p, //!< f_the_alevel_tk_q_p
    f_the_alevel_th_q_p, //!< f_the_alevel_th_q_p
    f_rh_alevel_tk_q_p, //!< f_rh_alevel_tk_q_p
    f_rh_alevel_th_q_p, //!< f_rh_alevel_th_q_p
    f_q_alevel_tk_rh_p, //!< f_q_alevel_tk_rh_p
    f_q_alevel_th_rh_p, //!< f_q_alevel_th_rh_p
    f_tdc_alevel_tk_q_p, //!< f_tdc_alevel_tk_q_p
    f_tdc_alevel_th_q_p, //!< f_tdc_alevel_th_q_p
    f_tdc_alevel_tk_rh_p, //!< f_tdc_alevel_tk_rh_p
    f_tdc_alevel_th_rh_p, //!< f_tdc_alevel_th_rh_p
    f_tdk_alevel_tk_q_p, //!< f_tdk_alevel_tk_q_p
    f_tdk_alevel_th_q_p, //!< f_tdk_alevel_th_q_p
    f_tdk_alevel_tk_rh_p, //!< f_tdk_alevel_tk_rh_p
    f_tdk_alevel_th_rh_p, //!< f_tdk_alevel_th_rh_p
    f_ducting_alevel_tk_q_p, //!< f_ducting_alevel_tk_q_p
    f_ducting_alevel_th_q_p, //!< f_ducting_alevel_th_q_p
    f_ducting_alevel_tk_rh_p, //!< f_ducting_alevel_tk_rh_p
    f_ducting_alevel_th_rh_p, //!< f_ducting_alevel_th_rh_p

    // isentropic level (ILEVEL) function
    f_geostrophic_wind_ilevel_mpot, //!< f_geostrophic_wind_ilevel_mpot; geographic

    // ocean depth level (OZLEVEL) functions
    f_sea_soundspeed_ozlevel_tc_salt,//!< f_sea_soundspeed_ozlevel_tc_salt
    f_sea_soundspeed_ozlevel_tk_salt,//!< f_sea_soundspeed_ozlevel_tk_salt

    // level independent functions
    f_temp_k2c, //!< f_temp_k2c
    f_temp_c2k, //!< f_temp_c2k
    f_temp_k2c_possibly, //!< f_temp_k2c_possibly
    f_temp_c2k_possibly, //!< f_temp_c2k_possibly
    f_tdk_tk_rh, //!< f_tdk_tk_rh
    f_tdc_tk_rh, //!< f_tdc_tk_rh
    f_tdc_tc_rh, //!< f_tdc_tc_rh
    f_rh_tk_td, //!< f_rh_tk_td
    f_rh_tc_td, //!< f_rh_tc_td
    f_vector_abs, //!< f_vector_abs
    f_rel_vorticity, //!< f_rel_vorticity; geographic
    f_abs_vorticity, //!< f_abs_vorticity; geographic
    f_divergence, //!< f_divergence; geographic
    f_advection, //!< f_advection; geographic
    f_d_dx, //!< f_d_dx
    f_d_dy, //!< f_d_dy
    f_abs_del, //!< f_abs_del
    f_del_square, //!< f_del_square
    f_minvalue_fields, //!< f_minvalue_fields
    f_maxvalue_fields, //!< f_maxvalue_fields
    f_minvalue_field_const, //!< f_minvalue_field_const
    f_maxvalue_field_const, //!< f_maxvalue_field_const
    f_abs, //!< f_abs
    f_log10, //!< f_log10
    f_pow10, //!< f_pow10
    f_log, //!< f_log
    f_exp, //!< f_exp
    f_power, //!< f_power
    f_shapiro2_filter, //!< f_shapiro2_filter
    f_smooth, //!< f_smooth
    f_windcooling_tk_u_v, //!< f_windcooling_tk_u_v
    f_windcooling_tc_u_v, //!< f_windcooling_tc_u_v
    f_undercooled_rain, //!< f_undercooled_rain
    f_thermal_front_parameter_tx, //!< f_thermal_front_parameter_tx; geographic
    f_pressure2flightlevel, //!< f_pressure2flightlevel
    f_momentum_x_coordinate, //!< f_momentum_x_coordinate; geographic
    f_momentum_y_coordinate, //!< f_momentum_y_coordinate; geographic
    f_jacobian, //!< f_jacobian; geographic
    f_vessel_icing_overland, //!< f_vessel_icing_overland - obsolete
    f_vessel_icing_mertins, //!< f_vessel_icing_mertins - obsolete
    f_vessel_icing_overland2, //!< f_vessel_icing_overland
    f_vessel_icing_mertins2, //!< f_vessel_icing_mertins
    f_vessel_icing_modstall, //!< f_vessel_icing_modstall
    f_vessel_icing_testmod, //!< f_vessel_icing_testmod
    f_replace_undefined, //!< f_replace_undefined
    f_replace_defined, //!< f_replace_defined
    f_replace_all, //!< f_replace_all
    f_values2classes, //!< f_values2classes
    f_field_diff_forecast_hour, //!< f_field_diff_forecast_hour
    f_accum_diff_forecast_hour, //!< f_accum_diff_forecast_hour
    f_sum_of_forecast_hours,  //!< f_sum_of_forecast_hours
    f_sum_of_fields,
    f_max_of_fields,
    f_min_of_fields,
    f_no_of_fields_above,
    f_no_of_fields_below,
    f_index_of_fields_max,
    f_index_of_fields_min,
    f_sum,
    f_mean_value,  //!< f_mean_value
    f_stddev,  //!< f_stddev_value_of
    f_probability_above,  //!< f_probability_above
    f_probability_below,  //!< f_probability_below
    f_probability_between,  //!< f_probability_between
    f_number_above,
    f_number_below,
    f_number_between,
    f_equivalent_to,  //!< f_equivalent_to
    f_min_value,  //!< f_min_value
    f_max_value,  //!< f_max_value
    f_min_index,  //!< f_min_value
    f_max_index,  //!< f_max_value
    f_percentile,  //!< f_percentile
    f_neighbour_percentile,
    f_neighbour_mean,
    f_neighbour_probability_above,
    f_neighbour_probability_below,
    f_snow_cm_from_snow_water_tk_td //!< f_snow_cm_from_snow_water_tk_td
  };

  /// Vertical coordinate types
  enum VerticalType {
    vctype_none, ///< surface and other single level fields
    vctype_pressure, ///< pressure levels
    vctype_hybrid, ///< model levels, eta(Hirlam,ECMWF,...) and norlam_sigma
    vctype_atmospheric, ///< other model levels (needing pressure in each level)
    vctype_isentropic, ///< isentropic (constant pot.temp.) levels
    vctype_oceandepth, ///< ocean model depth levels
    vctype_other ///< other multilevel fields (without dedicated compute functions)
  };

  /// One field computation from setup
  struct FieldCompute {
    std::string name; ///< result field (possibly only one ot the results)
    std::vector<std::string> results; ///< all result fields
    std::string functionName; ///< function name, for debugging only
    Function function; ///< function identifier
    std::vector<std::string> input; ///< input field(s)
    std::vector<float> constants; ///< input constants ("const:.." in functions)
    VerticalType vctype; ///
  };

  struct Zaxis_info {
    std::string name;
    VerticalType vctype;
    std::string levelprefix;
    std::string levelsuffix;
    bool index;

    Zaxis_info()
      : vctype(vctype_none), index(false) {}
  };

  struct FieldSpec {
    std::string paramName;
    std::string vcoordName;
    std::string ecoordName;
    std::string levelName;
    bool use_standard_name;
    std::string unit;
    std::string elevel;
    std::string fcHour;
    std::string option;

    FieldSpec()
      : use_standard_name(false) { }
  };

  // Build operation types to read and compute fields
  enum BuildOper {
    bop_none, bop_read, bop_function, bop_result, bop_delete
  };

  static std::map< std::string, Zaxis_info> Zaxis_info_map;

private:
  struct FunctionHelper {
    Function func;
    int numfields;
    int numconsts;
    unsigned int numresult;
    FieldFunctions::VerticalType vertcoord;
    FunctionHelper() :
      func(FieldFunctions::f_undefined), numfields(0), numconsts(0),
      numresult(1), vertcoord(FieldFunctions::vctype_none)
      { }
  };

  typedef std::map<std::string, FunctionHelper> functions_t;

public:
  FieldFunctions();

  static const std::vector<FieldCompute>& fieldComputes()
    { return vFieldCompute; }

  static const FieldCompute& fieldCompute(int idx)
    { return vFieldCompute.at(idx); }

  static std::string FIELD_COMPUTE_SECTION();
  static std::string FIELD_VERTICAL_COORDINATES_SECTION();

  static const Zaxis_info* findZaxisInfo(const std::string& name);

  static const FieldFunctions::VerticalType getVerticalType(const std::string& vctype);

  /// read setup section for field computations
  static bool parseComputeSetup(const std::vector<std::string>& lines,
      std::vector<std::string>& errors);
  static bool parseVerticalSetup(const std::vector<std::string>& lines,
      std::vector<std::string>& errors);

  static bool splitFieldSpecs(const std::string& paramName, FieldSpec& fs);

  static void setFieldNames(const std::vector<std::string>& vfieldname);

  /// return true if funcion includes several times steps
  static bool isTimeStepFunction(Function f);

  /// call function and return results
  static bool fieldComputer(Function function, const std::vector<float>& constants,
      const std::vector<Field*>& vfinput, const std::vector<Field*>& vfres, class GridConverter& gc);

private:
  static bool registerFunctions(functions_t& functions);
  static bool registerFunction(functions_t& functions, Function f, const std::string& funcText);
  static bool mapTimeStepFunction(Function& f);

private:
  static std::vector<std::string> vFieldName;
  static std::map<std::string, int> mFieldName;

  static std::vector<FieldCompute> vFieldCompute;

};

#endif // DIFIELD_FIELDFUNCTIONS_H
