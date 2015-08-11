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

#ifndef _Field_Functions_h
#define _Field_Functions_h

#include "diArea.h"
#include "diGridConverter.h"

#include <map>
#include <vector>

/// field computation/conversion functions
///
/// Wrapper for a set of functions for calculations on fields and a set of keys
/// (in the form of an enumeration) signifying invocations of the functions.
/// Each function have one or more associated invocations.
///

/*
 How to add a new function:

 We will introduce a function performing calculation XXX on a single field, and add two modes or ways to call it

 1. Add the actual function as member of this class:
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

class Field;

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
    f_qvector_plevel_z_tk_xcomp, //!< f_qvector_plevel_z_tk_xcomp
    f_qvector_plevel_z_th_xcomp, //!< f_qvector_plevel_z_th_xcomp
    f_qvector_plevel_z_tk_ycomp, //!< f_qvector_plevel_z_tk_ycomp
    f_qvector_plevel_z_th_ycomp, //!< f_qvector_plevel_z_th_ycomp
    f_ducting_plevel_tk_q, //!< f_ducting_plevel_tk_q
    f_ducting_plevel_th_q, //!< f_ducting_plevel_th_q
    f_ducting_plevel_tk_rh, //!< f_ducting_plevel_tk_rh
    f_ducting_plevel_th_rh, //!< f_ducting_plevel_th_rh
    f_geostrophic_wind_plevel_z_xcomp, //!< f_geostrophic_wind_plevel_z_xcomp
    f_geostrophic_wind_plevel_z_ycomp, //!< f_geostrophic_wind_plevel_z_ycomp
    f_geostrophic_vorticity_plevel_z,//!< f_geostrophic_vorticity_plevel_z
    f_kindex_plevel_tk_rh, //!< f_kindex_plevel_tk_rh
    f_kindex_plevel_th_rh, //!< f_kindex_plevel_th_rh
    f_ductingindex_plevel_tk_rh, //!< f_ductingindex_plevel_tk_rh
    f_ductingindex_plevel_th_rh, //!< f_ductingindex_plevel_th_rh
    f_showalterindex_plevel_tk_rh, //!< f_showalterindex_plevel_tk_rh
    f_showalterindex_plevel_th_rh, //!< f_showalterindex_plevel_th_rh
    f_boydenindex_plevel_tk_z, //!< f_boydenindex_plevel_tk_z
    f_boydenindex_plevel_th_z, //!< f_boydenindex_plevel_th_z

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
    f_geostrophic_wind_ilevel_mpot, //!< f_geostrophic_wind_ilevel_mpot

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
    f_direction, //!< f_direction
    f_rel_vorticity, //!< f_rel_vorticity
    f_abs_vorticity, //!< f_abs_vorticity
    f_divergence, //!< f_divergence
    f_advection, //!< f_advection
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
    f_thermal_front_parameter_tx, //!< f_thermal_front_parameter_tx
    f_pressure2flightlevel, //!< f_pressure2flightlevel
    f_momentum_x_coordinate, //!< f_momentum_x_coordinate
    f_momentum_y_coordinate, //!< f_momentum_y_coordinate
    f_jacobian, //!< f_jacobian
    f_vessel_icing_overland, //!< f_vessel_icing_overland
    f_vessel_icing_mertins, //!< f_vessel_icing_mertins
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
    vctype_other
  ///< other multilevel fields (without dedicated compute functions)
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
   Zaxis_info() : vctype(vctype_none), index(false)
  {
  }
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
    FieldSpec() : use_standard_name(false)
    {
    }
  };

  // Build operation types to read and compute fields
  enum BuildOper {
    bop_none, bop_read, bop_function, bop_result, bop_delete
  };


private:
  /// maps a Function enum to a descriptive text
  std::map<Function, std::string> functionText;

public:
  FieldFunctions();

  static std::map<Function, Function> functionMap;

  static std::vector<std::string> vFieldName;
  static std::map<std::string, int> mFieldName;

  static std::vector<FieldCompute> vFieldCompute;
  static std::map<VerticalType, std::map<std::string, std::vector<int> > > mFieldCompute;

  static std::map<std::string, std::string> pLevel2flightLevel;
  static std::map<std::string, std::string> flightLevel2pLevel;

  static std::map< std::string, Zaxis_info > Zaxis_info_map;

  static std::string FIELD_COMPUTE_SECTION();
  static std::string FIELD_VERTICAL_COORDINATES_SECTION();

  /// read setup section for field computations
  static bool parseComputeSetup(const std::vector<std::string>& lines,
      std::vector<std::string>& errors);
  static bool parseVerticalSetup(const std::vector<std::string>& lines,
      std::vector<std::string>& errors);

  static bool splitFieldSpecs(const std::string& paramName, FieldSpec& fs);


  /**
   * builds a conversion table between pressurelevels and flightlevels.
   */
  static void buildPLevelsToFlightLevelsTable();
  static std::string getPressureLevel(const std::string& flightlevel);
  static std::string getFlightLevel(const std::string& presurelevel);

  static void setFieldNames(const std::vector<std::string>& vfieldname);

  /// get function text (name/description) for a given Function
  const std::string getFunctionText(Function f) const;

  /// get all defined functions with associated texts
  const std::map<Function, std::string> & getFunctionTexts() const
  {
    return functionText;
  }

  /// return true if funcion includes several times steps
  bool isTimeStepFunction(Function f) const
  {
    return (f>=f_field_diff_forecast_hour && f<=f_index_of_fields_min);
  }

  /// call function and return results
  bool fieldComputer(Function function, const std::vector<float>& constants,
      const std::vector<Field*>& vfinput, const std::vector<Field*>& vfres,
      GridConverter& gc);

  /*
   * The actual functions follows
   */

  //---------------------------------------------------
  // pressure level (PLEVEL) functions
  //---------------------------------------------------

  static bool pleveltemp(int compute, int nx, int ny, const float *tinp, float *tout,
      float p, bool& allDefined, float undef, const std::string& unit);

  static bool plevelthe(int compute, int nx, int ny, const float *t, const float *rh, float *the,
      float p, bool& allDefined, float undef);

  static bool plevelhum(int compute, int nx, int ny, const float *t, const float *huminp,
      float *humout, float p, bool& allDefined, float undef, const std::string& unit);

  static bool pleveldz2tmean(int compute, int nx, int ny, const float *z1, const float *z2,
      float *tmean, float p1, float p2, bool& allDefined, float undef);

  static bool plevelqvector(int compute, int nx, int ny, const float *z, const float *t,
      float *qcomp, const float *xmapr, const float *ymapr, const float *fcoriolis,
      float p, bool& allDefined, float undef);

  static bool plevelducting(int compute, int nx, int ny, const float *t, const float *h,
      float *duct, float p, bool& allDefined, float undef);

  static bool plevelgwind_xcomp(int nx, int ny, const float *z, float *ug,
      const float *xmapr, const float *ymapr, const float *fcoriolis, bool& allDefined,
      float undef);

  static bool plevelgwind_ycomp(int nx, int ny, const float *z, float *vg,
      const float *xmapr, const float *ymapr, const float *fcoriolis, bool& allDefined,
      float undef);

  static bool plevelgvort(int nx, int ny, const float *z, float *gvort, const float *xmapr,
      const float *ymapr, const float *fcoriolis, bool& allDefined, float undef);

  static bool kIndex(int compute, int nx, int ny, const float *t500, const float *t700,
      const float *rh700, const float *t850, const float *rh850, float *kfield, float p500,
      float p700, float p850, bool& allDefined, float undef);

  static bool ductingIndex(int compute, int nx, int ny, const float *t850, const float *rh850,
      float *duct, float p850, bool& allDefined, float undef);

  static bool showalterIndex(int compute, int nx, int ny, const float *t500, const float *t850,
      const float *rh850, float *sfield, float p500, float p850, bool& allDefined,
      float undef);

  static bool boydenIndex(int compute, int nx, int ny, const float *t700, const float *z700,
      const float *z1000, float *bfield, float p700, float p1000, bool& allDefined,
      float undef);

  //---------------------------------------------------
  // hybrid model level (HLEVEL) functions
  //---------------------------------------------------

  static bool hleveltemp(int compute, int nx, int ny, const float *tinp, const float *ps,
      float *tout, float alevel, float blevel, bool& allDefined, float undef,
      const std::string& unit);

  static bool hlevelthe(int compute, int nx, int ny, const float *t, const float *q, const float *ps,
      float *the, float alevel, float blevel, bool& allDefined, float undef);

  static bool hlevelhum(int compute, int nx, int ny, const float *t, const float *huminp,
      const float *ps, float *humout, float alevel, float blevel, bool& allDefined,
      float undef, const std::string& unit);

  static bool hlevelducting(int compute, int nx, int ny, const float *t, const float *h,
      const float *ps, float *duct, float alevel, float blevel, bool& allDefined,
      float undef);

  static bool hlevelpressure(int nx, int ny, const float *ps, float *p, float alevel,
      float blevel, bool& allDefined, float undef);

  //---------------------------------------------------
  // atmospheric model level (ALEVEL) functions
  //---------------------------------------------------

  static bool aleveltemp(int compute, int nx, int ny, const float *tinp, const float *p,
      float *tout, bool& allDefined, float undef, const std::string& unit);

  static bool alevelthe(int compute, int nx, int ny, const float *t, const float *q, const float *p,
      float *the, bool& allDefined, float undef);

  static bool alevelhum(int compute, int nx, int ny, const float *t, const float *huminp,
      const float *p, float *humout, bool& allDefined, float undef, const std::string& unit);

  static bool alevelducting(int compute, int nx, int ny, const float *t, const float *h, const float *p,
      float *duct, bool& allDefined, float undef);

  //---------------------------------------------------
  // isentropic level (ILEVEL) function
  //---------------------------------------------------

  static bool ilevelgwind(int nx, int ny, const float *mpot, float *ug, float *vg,
      const float *xmapr, const float *ymapr, const float *fcoriolis, bool& allDefined,
      float undef);

  //---------------------------------------------------
  // ocean depth level (OZLEVEL) functions
  //---------------------------------------------------

  static bool seaSoundSpeed(int compute, int nx, int ny, const float *t, const float *s,
      float *soundspeed, float z, bool& allDefined, float undef);

  //---------------------------------------------------
  // level (pressure) independant functions
  //---------------------------------------------------

  static bool cvtemp(int compute, int nx, int ny, const float *tinp, float *tout,
      bool& allDefined, float undef);

  static bool cvhum(int compute, int nx, int ny, const float *t, const float *huminp,
      float *humout, bool& allDefined, float undef);

  static bool vectorabs(int nx, int ny, const float *u, const float *v, float *ff,
      bool& allDefined, float undef);

  static bool direction(int nx, int ny, float *u, float *v, const Area& area, float *dd,
      bool& allDefined, float undef);

  static bool relvort(int nx, int ny, const float *u, const float *v, float *rvort, const float *xmapr,
      const float *ymapr, bool& allDefined, float undef);

  static bool absvort(int nx, int ny, const float *u, const float *v, float *avort, const float *xmapr,
      const float *ymapr, const float *fcoriolis, bool& allDefined, float undef);

  static bool divergence(int nx, int ny, const float *u, const float *v, float *diverg,
      const float *xmapr, const float *ymapr, bool& allDefined, float undef);

  static bool advection(int nx, int ny, const float *f, const float *u, const float *v, float *advec,
      const float *xmapr, const float *ymapr, float hours, bool& allDefined, float undef);

  static bool gradient(int compute, int nx, int ny, const float *field, float *fgrad,
      const float *xmapr, const float *ymapr, bool& allDefined, float undef);

  static bool shapiro2_filter(int nx, int ny, float *field, float *fsmooth,
      bool& allDefined, float undef);

  static bool windCooling(int compute, int nx, int ny, const float *t, const float *u, const float *v,
      float *dtcool, bool& allDefined, float undef);

  static bool underCooledRain(int nx, int ny, const float *precip, const float *snow, const float *tk,
      float *undercooled, float precipMin, float snowRateMax, float tcMax,
      bool& allDefined, float undef);

  static bool thermalFrontParameter(int nx, int ny, const float *t, float *tfp,
      const float *xmapr, const float *ymapr, bool& allDefined, float undef);

  static bool pressure2FlightLevel(int nx, int ny, const float *pressure,
      float *flightlevel, bool& allDefined, float undef);

  static bool momentumXcoordinate(int nx, int ny, const float *v, float *mxy, const float *xmapr,
      const float *fcoriolis, float fcoriolisMin, bool& allDefined, float undef);

  static bool momentumYcoordinate(int nx, int ny, const float *u, float *nxy, const float *ymapr,
      const float *fcoriolis, float fcoriolisMin, bool& allDefined, float undef);

  static bool jacobian(int nx, int ny, const float *field1, const float *field2, float *fjacobian,
      const float *xmapr, const float *ymapr, bool& allDefined, float undef);

  static bool vesselIcingOverland(int nx, int ny, const float *airtemp, const float *seatemp, const float *u,
      const float *v, float *icing, float freezingpoint, bool& allDefined, float undef);

  static bool vesselIcingMertins(int nx, int ny, const float *airtemp, const float *seatemp, const float *u,
      const float *v, float *icing, float freezingpoint, bool& allDefined, float undef);

  static bool values2classes(int nx, int ny, const float *fvalue, float *fclass,
      const std::vector<float>& values, bool& allDefined, float undef);

  static bool fieldOPERfield(int compute, int nx, int ny, const float *field1,
      const float *field2, float *fres, bool& allDefined, float undef);

  static bool fieldOPERconstant(int compute, int nx, int ny, const float *field,
      float constant, float *fres, bool& allDefined, float undef);

  static bool constantOPERfield(int compute, int nx, int ny, float constant,
      const float *field, float *fres, bool& allDefined, float undef);

  static bool sumFields(int nx, int ny, const std::vector<float*>& fields,
      float *fres, bool& allDefined, float undef);

  static void fillEdges(int nx, int ny, float *field);

  static bool meanValue(int nx, int ny, const std::vector<float*>& fields,
      float *fres, bool& allDefined, float undef);

  static bool stddevValue(int nx, int ny, const std::vector<float*>& fields,
      float *fres, bool& allDefined, float undef);

  static bool extremeValue(int compute, int nx, int ny, const std::vector<float*>& fields,
      float *fres, bool& allDefined, float undef);

  static bool probability(int compute, int nx, int ny, const std::vector<float*>& fields,
      const std::vector<float>& limits,
      float *fres, bool& allDefined, float undef);

  static bool percentile(int nx, int ny, const float* field, float percentile, int range, int step,
      float *fres, bool& allDefined, float undef);

  static bool snow_in_cm(int nx, int ny, const float *snow_water, const float *tk2m, const float *td2m,
      float *snow_cm, bool& allDefined, float undef);
};

#endif
