/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2015-2020 met.no

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

#include "diana_config.h"

#include "diFieldFunctions.h"

#include "diArea.h"
#include "diGridConverter.h"
#include "diField.h"
#include "diFlightLevel.h"

#include <mi_fieldcalc/FieldCalculations.h>

#include <puTools/miStringFunctions.h>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#include <cmath>
#include <iomanip>
#include <cfloat>

#include <memory.h>

//#define DEBUGPRINT 1
#define ENABLE_FIELDFUNCTIONS_TIMING 1

using namespace miutil;

#define MILOGGER_CATEGORY "diField.FieldFunctions"
#include "miLogger/miLogging.h"

// static data (setup)
std::vector<FieldFunctions::FieldCompute> FieldFunctions::vFieldCompute;

FieldFunctions::FieldFunctions()
{
}

// static
std::string FieldFunctions::FIELD_COMPUTE_SECTION()
{
  return "FIELD_COMPUTE";
}

static const std::string u_K("K");
static const std::string u_C("celsius");
static const std::string u_hPa("hPa");
static const std::string u_kg_kg("kg/kg");
static const std::string u_pc("%");
static const std::string u_m_s("m/s");
static const std::string u_kg_m2("kg/m2");
static const std::string u_m("m");

static const std::string u_as0("=0");

static const FieldFunctions::Arg a_th("th", u_K);
static const FieldFunctions::Arg a_tk("tk", u_K);
static const FieldFunctions::Arg a_tc("tc", u_C);
static const FieldFunctions::Arg a_q("q", u_kg_kg);
static const FieldFunctions::Arg a_rh("rh", u_pc);
static const FieldFunctions::Arg a_rh1("rh", "1");
static const FieldFunctions::Arg a_tdk("tdk", u_K);
static const FieldFunctions::Arg a_tdc("tdc", u_C);
static const FieldFunctions::Arg a_psurf("psurf", u_hPa);
static const FieldFunctions::Arg a_p("p", u_hPa);
static const FieldFunctions::Arg a_z("z", u_m);

static const FieldFunctions::FunctionHelper functions[] {
    // pressure level (PLEVEL) functions

    // FieldCalculations pleveltemp
    {FieldFunctions::f_tc_plevel_th, FieldVerticalAxes::vctype_pressure, "tc.plevel_th", u_C, {a_th}, {}},
    {FieldFunctions::f_tk_plevel_th, FieldVerticalAxes::vctype_pressure, "tk.plevel_th", u_K, {a_th}, {}},
    {FieldFunctions::f_th_plevel_tk, FieldVerticalAxes::vctype_pressure, "th.plevel_tk", u_K, {a_tk}, {}},
    {FieldFunctions::f_thesat_plevel_tk, FieldVerticalAxes::vctype_pressure, "thesat.plevel_tk", u_K, {a_tk}, {}},
    {FieldFunctions::f_thesat_plevel_th, FieldVerticalAxes::vctype_pressure, "thesat.plevel_th", u_K, {a_th}, {}},

    // FieldCalculations plevelthe
    {FieldFunctions::f_the_plevel_tk_rh, FieldVerticalAxes::vctype_pressure, "the.plevel_tk_rh", u_K, {a_tk, a_rh}, {}},
    {FieldFunctions::f_the_plevel_th_rh, FieldVerticalAxes::vctype_pressure, "the.plevel_th_rh", u_K, {a_th, a_rh}, {}},

    // FieldCalculations plevelhum
    {FieldFunctions::f_rh_plevel_tk_q, FieldVerticalAxes::vctype_pressure, "rh.plevel_tk_q", u_pc, {a_tk, a_q}, {}},
    {FieldFunctions::f_rh_plevel_th_q, FieldVerticalAxes::vctype_pressure, "rh.plevel_th_q", u_pc, {a_th, a_q}, {}},
    {FieldFunctions::f_q_plevel_tk_rh, FieldVerticalAxes::vctype_pressure, "q.plevel_tk_rh", u_kg_kg, {a_tk, a_rh}, {}},
    {FieldFunctions::f_q_plevel_th_rh, FieldVerticalAxes::vctype_pressure, "q.plevel_th_rh", u_kg_kg, {a_th, a_rh}, {}},
    {FieldFunctions::f_tdc_plevel_tk_rh, FieldVerticalAxes::vctype_pressure, "tdc.plevel_tk_rh", u_C, {a_tk, a_rh}, {}},
    {FieldFunctions::f_tdc_plevel_th_rh, FieldVerticalAxes::vctype_pressure, "tdc.plevel_th_rh", u_C, {a_th, a_rh}, {}},
    {FieldFunctions::f_tdc_plevel_tk_q, FieldVerticalAxes::vctype_pressure, "tdc.plevel_tk_q", u_C, {a_tk, a_q}, {}},
    {FieldFunctions::f_tdc_plevel_th_q, FieldVerticalAxes::vctype_pressure, "tdc.plevel_th_q", u_C, {a_th, a_q}, {}},
    {FieldFunctions::f_tdk_plevel_tk_rh, FieldVerticalAxes::vctype_pressure, "tdk.plevel_tk_rh", u_K, {a_tk, a_rh}, {}},
    {FieldFunctions::f_tdk_plevel_th_rh, FieldVerticalAxes::vctype_pressure, "tdk.plevel_th_rh", u_K, {a_th, a_rh}, {}},
    {FieldFunctions::f_tdk_plevel_tk_q, FieldVerticalAxes::vctype_pressure, "tdk.plevel_tk_q", u_K, {a_tk, a_q}, {}},
    {FieldFunctions::f_tdk_plevel_th_q, FieldVerticalAxes::vctype_pressure, "tdk.plevel_th_q", u_K, {a_th, a_q}, {}},

    // FieldCalculations pleveldz2tmean
    {FieldFunctions::f_tcmean_plevel_z1_z2, FieldVerticalAxes::vctype_pressure, "tcmean.plevel_z1_z2", u_C, {{"z1"}, {"z2"}}, {}},
    {FieldFunctions::f_tkmean_plevel_z1_z2, FieldVerticalAxes::vctype_pressure, "tkmean.plevel_z1_z2", u_K, {{"z1"}, {"z2"}}, {}},
    {FieldFunctions::f_thmean_plevel_z1_z2, FieldVerticalAxes::vctype_pressure, "thmean.plevel_z1_z2", u_K, {{"z1"}, {"z2"}}, {}},

    // FieldCalculations plevelducting
    {FieldFunctions::f_ducting_plevel_tk_q, FieldVerticalAxes::vctype_pressure, "ducting.plevel_tk_q", {a_tk, a_q}, {}},
    {FieldFunctions::f_ducting_plevel_th_q, FieldVerticalAxes::vctype_pressure, "ducting.plevel_th_q", {a_th, a_q}, {}},
    {FieldFunctions::f_ducting_plevel_tk_rh, FieldVerticalAxes::vctype_pressure, "ducting.plevel_tk_rh", {a_tk, a_rh}, {}},
    {FieldFunctions::f_ducting_plevel_th_rh, FieldVerticalAxes::vctype_pressure, "ducting.plevel_th_rh", {a_th, a_rh}, {}},

    // FieldFunctions kIndex
    {FieldFunctions::f_kindex_plevel_tk_rh, FieldVerticalAxes::vctype_pressure, "kindex.plevel_tk_rh", "1", {{"tk500", u_K}, {"tk700", u_K}, {"rh700", u_pc}, {"tk850", u_K}, {"rh850", u_pc}}, {}},
    {FieldFunctions::f_kindex_plevel_th_rh, FieldVerticalAxes::vctype_pressure, "kindex.plevel_th_rh", "1", {{"th500", u_K}, {"th700", u_K}, {"rh700", u_pc}, {"th850", u_K}, {"rh850", u_pc}}, {}},

    // FieldFunctions ductingIndex
    {FieldFunctions::f_ductingindex_plevel_tk_rh, FieldVerticalAxes::vctype_pressure, "ductingindex.plevel_tk_rh", "1", {{"tk850", u_K}, {"rh850", u_pc}}, {}},
    {FieldFunctions::f_ductingindex_plevel_th_rh, FieldVerticalAxes::vctype_pressure, "ductingindex.plevel_th_rh", "1", {{"th850", u_K}, {"rh850", u_pc}}, {}},

    // FieldFunctions showalterIndex
    {FieldFunctions::f_showalterindex_plevel_tk_rh, FieldVerticalAxes::vctype_pressure, "showalterindex.plevel_tk_rh", "1", {{"tk500", u_K}, {"tk850", u_K}, {"rh850", u_pc}}, {}},
    {FieldFunctions::f_showalterindex_plevel_th_rh, FieldVerticalAxes::vctype_pressure, "showalterindex.plevel_th_rh", "1", {{"th500", u_K}, {"th850", u_K}, {"rh850", u_pc}}, {}},

    // FieldFunctions boydenIndex
    {FieldFunctions::f_boydenindex_plevel_tk_z, FieldVerticalAxes::vctype_pressure, "boydenindex.plevel_tk_z", "1", {{"tk700", u_K}, {"z700", u_m}, {"z1000", u_m}}, {}},
    {FieldFunctions::f_boydenindex_plevel_th_z, FieldVerticalAxes::vctype_pressure, "boydenindex.plevel_th_z", "1", {{"th700", u_K}, {"z700", u_m}, {"z1000", u_m}}, {}},

    // FieldFunctions sweatIndex
    {FieldFunctions::f_sweatindex_plevel, FieldVerticalAxes::vctype_none, "sweatindex.plevel", {{"t850", u_C}, {"t500", u_C}, {"td850", u_C}, {"td500", u_C}, {"u850", u_m_s}, {"v850", u_m_s}, {"u500", u_m_s}, {"v500", u_m_s}}, {}},


    // hybrid model level (HLEVEL) functions
    {FieldFunctions::f_tc_hlevel_th_psurf, FieldVerticalAxes::vctype_hybrid, "tc.hlevel_th_psurf", u_C, {a_th, a_psurf}, {}},
    {FieldFunctions::f_tk_hlevel_th_psurf, FieldVerticalAxes::vctype_hybrid, "tk.hlevel_th_psurf", u_K, {a_th, a_psurf}, {}},
    {FieldFunctions::f_th_hlevel_tk_psurf, FieldVerticalAxes::vctype_hybrid, "th.hlevel_tk_psurf", u_K, {a_tk, a_psurf}, {}},
    {FieldFunctions::f_thesat_hlevel_tk_psurf, FieldVerticalAxes::vctype_hybrid, "thesat.hlevel_tk_psurf", u_K, {a_tk, a_psurf}, {}},
    {FieldFunctions::f_thesat_hlevel_th_psurf, FieldVerticalAxes::vctype_hybrid, "thesat.hlevel_th_psurf", u_K, {a_th, a_psurf}, {}},
    {FieldFunctions::f_the_hlevel_tk_q_psurf, FieldVerticalAxes::vctype_hybrid, "the.hlevel_tk_q_psurf", u_K, {a_tk, a_q, a_psurf}, {}},
    {FieldFunctions::f_the_hlevel_th_q_psurf, FieldVerticalAxes::vctype_hybrid, "the.hlevel_th_q_psurf", u_K, {a_th, a_q, a_psurf}, {}},
    {FieldFunctions::f_rh_hlevel_tk_q_psurf, FieldVerticalAxes::vctype_hybrid, "rh.hlevel_tk_q_psurf", u_pc, {a_tk, a_q, a_psurf}, {}},
    {FieldFunctions::f_rh_hlevel_th_q_psurf, FieldVerticalAxes::vctype_hybrid, "rh.hlevel_th_q_psurf", u_pc, {a_th, a_q, a_psurf}, {}},
    {FieldFunctions::f_q_hlevel_tk_rh_psurf, FieldVerticalAxes::vctype_hybrid, "q.hlevel_tk_rh_psurf", u_kg_kg, {a_tk, a_rh, a_psurf}, {}},
    {FieldFunctions::f_q_hlevel_th_rh_psurf, FieldVerticalAxes::vctype_hybrid, "q.hlevel_th_rh_psurf", u_kg_kg, {a_th, a_rh, a_psurf}, {}},
    {FieldFunctions::f_tdc_hlevel_tk_q_psurf, FieldVerticalAxes::vctype_hybrid, "tdc.hlevel_tk_q_psurf", u_C, {a_tk, a_q, a_psurf}, {}},
    {FieldFunctions::f_tdc_hlevel_th_q_psurf, FieldVerticalAxes::vctype_hybrid, "tdc.hlevel_th_q_psurf", u_C, {a_th, a_q, a_psurf}, {}},
    {FieldFunctions::f_tdc_hlevel_tk_rh_psurf, FieldVerticalAxes::vctype_hybrid, "tdc.hlevel_tk_rh_psurf", u_C, {a_tk, a_rh, a_psurf}, {}},
    {FieldFunctions::f_tdc_hlevel_th_rh_psurf, FieldVerticalAxes::vctype_hybrid, "tdc.hlevel_th_rh_psurf", u_C, {a_th, a_rh, a_psurf}, {}},
    {FieldFunctions::f_tdk_hlevel_tk_q_psurf, FieldVerticalAxes::vctype_hybrid, "tdk.hlevel_tk_q_psurf", u_K, {a_tk, a_q, a_psurf}, {}},
    {FieldFunctions::f_tdk_hlevel_th_q_psurf, FieldVerticalAxes::vctype_hybrid, "tdk.hlevel_th_q_psurf", u_K, {a_th, a_q, a_psurf}, {}},
    {FieldFunctions::f_tdk_hlevel_tk_rh_psurf, FieldVerticalAxes::vctype_hybrid, "tdk.hlevel_tk_rh_psurf", u_K, {a_tk, a_rh, a_psurf}, {}},
    {FieldFunctions::f_tdk_hlevel_th_rh_psurf, FieldVerticalAxes::vctype_hybrid, "tdk.hlevel_th_rh_psurf", u_K, {a_th, a_rh, a_psurf}, {}},
    {FieldFunctions::f_ducting_hlevel_tk_q_psurf, FieldVerticalAxes::vctype_hybrid, "ducting.hlevel_tk_q_psurf", {a_tk, a_q, a_psurf}, {}},
    {FieldFunctions::f_ducting_hlevel_th_q_psurf, FieldVerticalAxes::vctype_hybrid, "ducting.hlevel_th_q_psurf", {a_th, a_q, a_psurf}, {}},
    {FieldFunctions::f_ducting_hlevel_tk_rh_psurf, FieldVerticalAxes::vctype_hybrid, "ducting.hlevel_tk_rh_psurf", {a_tk, a_rh, a_psurf}, {}},
    {FieldFunctions::f_ducting_hlevel_th_rh_psurf, FieldVerticalAxes::vctype_hybrid, "ducting.hlevel_th_rh_psurf", {a_th, a_rh, a_psurf}, {}},
    {FieldFunctions::f_pressure_hlevel_xx_psurf, FieldVerticalAxes::vctype_hybrid, "pressure.hlevel_xx_psurf", u_hPa, {{"xx"}, a_psurf}, {}}, // just get eta.a and eta.b from field xx

    // misc atmospheric model level (ALEVEL) functions
    {FieldFunctions::f_tc_alevel_th_p, FieldVerticalAxes::vctype_atmospheric, "tc.alevel_th_p", u_C, {a_th, a_p}, {}},
    {FieldFunctions::f_tk_alevel_th_p, FieldVerticalAxes::vctype_atmospheric, "tk.alevel_th_p", u_K, {a_th, a_p}, {}},
    {FieldFunctions::f_th_alevel_tk_p, FieldVerticalAxes::vctype_atmospheric, "th.alevel_tk_p", u_K, {a_tk, a_p}, {}},
    {FieldFunctions::f_thesat_alevel_tk_p, FieldVerticalAxes::vctype_atmospheric, "thesat.alevel_tk_p", u_K, {a_tk, a_p}, {}},
    {FieldFunctions::f_thesat_alevel_th_p, FieldVerticalAxes::vctype_atmospheric, "thesat.alevel_th_p", u_K, {a_th, a_p}, {}},
    {FieldFunctions::f_the_alevel_tk_q_p, FieldVerticalAxes::vctype_atmospheric, "the.alevel_tk_q_p", u_K, {a_tk, a_q, a_p}, {}},
    {FieldFunctions::f_the_alevel_th_q_p, FieldVerticalAxes::vctype_atmospheric, "the.alevel_th_q_p", u_K, {a_th, a_q, a_p}, {}},
    {FieldFunctions::f_rh_alevel_tk_q_p, FieldVerticalAxes::vctype_atmospheric, "rh.alevel_tk_q_p", u_pc, {a_tk, a_q, a_p}, {}},
    {FieldFunctions::f_rh_alevel_th_q_p, FieldVerticalAxes::vctype_atmospheric, "rh.alevel_th_q_p", u_pc, {a_th, a_q, a_p}, {}},
    {FieldFunctions::f_q_alevel_tk_rh_p, FieldVerticalAxes::vctype_atmospheric, "q.alevel_tk_rh_p", u_kg_kg, {a_tk, a_rh, a_p}, {}},
    {FieldFunctions::f_q_alevel_th_rh_p, FieldVerticalAxes::vctype_atmospheric, "q.alevel_th_rh_p", u_kg_kg, {a_th, a_rh, a_p}, {}},
    {FieldFunctions::f_tdc_alevel_tk_q_p, FieldVerticalAxes::vctype_atmospheric, "tdc.alevel_tk_q_p", u_C, {a_tk, a_q, a_p}, {}},
    {FieldFunctions::f_tdc_alevel_th_q_p, FieldVerticalAxes::vctype_atmospheric, "tdc.alevel_th_q_p", u_C, {a_th, a_q, a_p}, {}},
    {FieldFunctions::f_tdc_alevel_tk_rh_p, FieldVerticalAxes::vctype_atmospheric, "tdc.alevel_tk_rh_p", u_C, {a_tk, a_rh, a_p}, {}},
    {FieldFunctions::f_tdc_alevel_th_rh_p, FieldVerticalAxes::vctype_atmospheric, "tdc.alevel_th_rh_p", u_C, {a_th, a_rh, a_p}, {}},
    {FieldFunctions::f_tdk_alevel_tk_q_p, FieldVerticalAxes::vctype_atmospheric, "tdk.alevel_tk_q_p", u_K, {a_tk, a_q, a_p}, {}},
    {FieldFunctions::f_tdk_alevel_th_q_p, FieldVerticalAxes::vctype_atmospheric, "tdk.alevel_th_q_p", u_K, {a_th, a_q, a_p}, {}},
    {FieldFunctions::f_tdk_alevel_tk_rh_p, FieldVerticalAxes::vctype_atmospheric, "tdk.alevel_tk_rh_p", u_K, {a_tk, a_rh, a_p}, {}},
    {FieldFunctions::f_tdk_alevel_th_rh_p, FieldVerticalAxes::vctype_atmospheric, "tdk.alevel_th_rh_p", u_K, {a_th, a_rh, a_p}, {}},
    {FieldFunctions::f_ducting_alevel_tk_q_p, FieldVerticalAxes::vctype_atmospheric, "ducting.alevel_tk_q_p", {a_tk, a_q, a_p}, {}},
    {FieldFunctions::f_ducting_alevel_th_q_p, FieldVerticalAxes::vctype_atmospheric, "ducting.alevel_th_q_p", {a_th, a_q, a_p}, {}},
    {FieldFunctions::f_ducting_alevel_tk_rh_p, FieldVerticalAxes::vctype_atmospheric, "ducting.alevel_tk_rh_p", {a_tk, a_rh, a_p}, {}},
    {FieldFunctions::f_ducting_alevel_th_rh_p, FieldVerticalAxes::vctype_atmospheric, "ducting.alevel_th_rh_p", {a_th, a_rh, a_p}, {}},

    // ocean depth level (OZLEVEL) functions
    {FieldFunctions::f_sea_soundspeed_ozlevel_tc_salt, FieldVerticalAxes::vctype_oceandepth, "sea.soundspeed.ozlevel_tc_salt", u_m_s, {{"seatemp.c", u_C}, {"salt"}}, {}},
    {FieldFunctions::f_sea_soundspeed_ozlevel_tk_salt, FieldVerticalAxes::vctype_oceandepth, "sea.soundspeed.ozlevel_tk_salt", u_m_s, {{"seatemp.k", u_K}, {"salt"}}, {}},

    // level independent functions
    {FieldFunctions::f_tdk_tk_rh, FieldVerticalAxes::vctype_none, "tdk.tk_rh", u_K, {a_tk, a_rh}, {}},
    {FieldFunctions::f_tdc_tk_rh, FieldVerticalAxes::vctype_none, "tdc.tk_rh", u_C, {a_tk, a_rh}, {}},
    {FieldFunctions::f_abshum_tk_rh, FieldVerticalAxes::vctype_none, "abshum.tk_rh", {a_tk, a_rh1}, {}},
    {FieldFunctions::f_tdc_tc_rh, FieldVerticalAxes::vctype_none, "tdc.tc_rh", u_C, {a_tc, a_rh}, {}},
    {FieldFunctions::f_rh_tk_td, FieldVerticalAxes::vctype_none, "rh.tk_tdk", u_pc, {a_tk, a_tdk}, {}},
    {FieldFunctions::f_rh_tc_td, FieldVerticalAxes::vctype_none, "rh.tc_tdc", u_pc, {a_tc, a_tdc}, {}},
    {FieldFunctions::f_vector_abs, FieldVerticalAxes::vctype_none, "vector.abs", u_as0, {{"u"}, {"v"}}, {}},
    {FieldFunctions::f_d_dx, FieldVerticalAxes::vctype_none, "d/dx", {{"f"}}, {}},
    {FieldFunctions::f_d_dy, FieldVerticalAxes::vctype_none, "d/dy", {{"f"}}, {}},
    {FieldFunctions::f_abs_del, FieldVerticalAxes::vctype_none, "abs.del", {{"f"}}, {}},
    {FieldFunctions::f_del_square, FieldVerticalAxes::vctype_none, "del.square", {{"f"}}, {}},
    {FieldFunctions::f_minvalue_fields, FieldVerticalAxes::vctype_none, "minvalue.fields", u_as0, {{"f1"}, {"f2"}}, {}},
    {FieldFunctions::f_maxvalue_fields, FieldVerticalAxes::vctype_none, "maxvalue.fields", u_as0, {{"f1"}, {"f2"}}, {}},
    {FieldFunctions::f_minvalue_field_const, FieldVerticalAxes::vctype_none, "minvalue.field.const", u_as0, {{"f"}}, {{"value"}}},
    {FieldFunctions::f_maxvalue_field_const, FieldVerticalAxes::vctype_none, "maxvalue.field.const", u_as0, {{"f"}}, {{"value"}}},
    {FieldFunctions::f_abs, FieldVerticalAxes::vctype_none, "abs", u_as0, {{"f"}}, {}},
    {FieldFunctions::f_log10, FieldVerticalAxes::vctype_none, "log10", {{"f"}}, {}},
    {FieldFunctions::f_pow10, FieldVerticalAxes::vctype_none, "pow10", {{"f"}}, {}},
    {FieldFunctions::f_log, FieldVerticalAxes::vctype_none, "log", {{"f"}}, {}},
    {FieldFunctions::f_exp, FieldVerticalAxes::vctype_none, "exp", {{"f"}}, {}},
    {FieldFunctions::f_power, FieldVerticalAxes::vctype_none, "power", {{"f"}}, {{"exponent"}}},
    {FieldFunctions::f_shapiro2_filter, FieldVerticalAxes::vctype_none, "shapiro2.filter", u_as0, {{"f"}}, {}},
    {FieldFunctions::f_smooth, FieldVerticalAxes::vctype_none, "smooth", u_as0, {{"f"}}, {{"numsmooth"}}},
    {FieldFunctions::f_windcooling_tk_u_v, FieldVerticalAxes::vctype_none, "windcooling_tk_u_v", u_K, {{"tk2m",u_K}, {"u10m", u_m_s}, {"v10m", u_m_s}}, {}},
    {FieldFunctions::f_windcooling_tc_u_v, FieldVerticalAxes::vctype_none, "windcooling_tc_u_v", u_K, {{"tc2m",u_C}, {"u10m", u_m_s}, {"v10m", u_m_s}}, {}},
    {FieldFunctions::f_undercooled_rain, FieldVerticalAxes::vctype_none, "undercooled.rain", "1", {{"precip", u_kg_m2}, {"snow", u_kg_m2}, a_tk}, {{"precipMin"}, {"snowRateMax"}, {"tcMax"}}},
    {FieldFunctions::f_pressure2flightlevel, FieldVerticalAxes::vctype_none, "pressure2flightlevel", "100ft", {{"p", u_hPa}}, {}},
    {FieldFunctions::f_vessel_icing_overland, FieldVerticalAxes::vctype_none, "vessel.icing.overland", {{"airtemp", u_C}, {"seatemp", u_C}, {"u10m", u_m_s}, {"v10m", u_m_s}, {"salinity0m"}, {"aice"}}, {}},
    {FieldFunctions::f_vessel_icing_mertins, FieldVerticalAxes::vctype_none, "vessel.icing.mertins", {{"airtemp"}, {"seatemp"}, {"u10m"}, {"v10m"}, {"salinity0m"}, {"aice"}}, {}},
    {FieldFunctions::f_vessel_icing_modstall, FieldVerticalAxes::vctype_none, "vessel.icing.modstall", {{"salinity0m"}, {"significant_wave_height"},{"u10m", u_m_s},{"v10m", u_m_s},{"tc2m", u_C},{"relative_humidity_2m"},{"temperature0m", u_C},{"air_pressure_at_sea_level","hPa"},{"significant_wave_period"},{"aice"},{"depth"}},{{"vs"}, {"alpha"}, {"zmin"}, {"zmax"}}},
    {FieldFunctions::f_vessel_icing_mincog, FieldVerticalAxes::vctype_none, "vessel.icing.mincog", {{"salinity0m"},{"significant_wave_height"},{"u10m", u_m_s},{"v10m", u_m_s},{"tc2m", u_C},{"relative_humidity_2m", "1"},{"sst", u_C},{"air_pressure_at_sea_level","hPa"},{"significant_wave_period"},{"aice"},{"depth"}},{{"vs"}, {"alpha"}, {"zmin"}, {"zmax"}, {"alt"}}},
    {FieldFunctions::f_replace_undefined, FieldVerticalAxes::vctype_none, "replace.undefined", u_as0, {{"f"}}, {{"value"}}},
    {FieldFunctions::f_replace_defined, FieldVerticalAxes::vctype_none, "replace.defined", u_as0, {{"f"}}, {{"value"}}},
    {FieldFunctions::f_replace_all, FieldVerticalAxes::vctype_none, "replace.all", u_as0, {{"f"}}, {{"value"}}},
    {FieldFunctions::f_values2classes, FieldVerticalAxes::vctype_none, "values2classes", {{"f"}}, {{"limits_low_to_high"}}, FieldFunctions::varargs_const},
    {FieldFunctions::f_field_diff_forecast_hour, FieldVerticalAxes::vctype_none, "field.diff.forecast.hour", u_as0, {{"field"}}, {{"relHourFirst"}, {"relHourLast"}}},
    {FieldFunctions::f_accum_diff_forecast_hour, FieldVerticalAxes::vctype_none, "accum.diff.forecast.hour", u_as0, {{"accumfield"}}, {{"relHourFirst"}, {"relHourLast"}}},
    {FieldFunctions::f_sum_of_forecast_hours, FieldVerticalAxes::vctype_none, "sum_of_forecast_hours", u_as0, {{"field"}},{{"forecastHours"}}, FieldFunctions::varargs_const},
    {FieldFunctions::f_sum_of_fields, FieldVerticalAxes::vctype_none, "sum_of_fields", u_as0, {{"field"}}, {}},
    {FieldFunctions::f_mean_of_fields, FieldVerticalAxes::vctype_none, "mean_of_fields", u_as0, {{"field"}}, {}},
    {FieldFunctions::f_max_of_fields, FieldVerticalAxes::vctype_none, "max_of_fields", u_as0, {{"field"}}, {}},
    {FieldFunctions::f_min_of_fields, FieldVerticalAxes::vctype_none, "min_of_fields", u_as0, {{"field"}}, {}},
    {FieldFunctions::f_no_of_fields_above, FieldVerticalAxes::vctype_none, "no_of_fields_above", "1", {{"field"}}, {{"limit"}}},
    {FieldFunctions::f_no_of_fields_below, FieldVerticalAxes::vctype_none, "no_of_fields_below", "1", {{"field"}}, {{"limit"}}},
    {FieldFunctions::f_index_of_fields_max, FieldVerticalAxes::vctype_none, "index_of_fields_max", "1", {{"field"}}, {}},
    {FieldFunctions::f_index_of_fields_min, FieldVerticalAxes::vctype_none, "index_of_fields_min", "1", {{"field"}}, {}},
    {FieldFunctions::f_sum, FieldVerticalAxes::vctype_none, "sum", u_as0, {{"field"}}, {}, FieldFunctions::varargs_field},
    {FieldFunctions::f_mean_value, FieldVerticalAxes::vctype_none, "mean_value", u_as0, {{"field"}}, {}, FieldFunctions::varargs_field},
    {FieldFunctions::f_stddev, FieldVerticalAxes::vctype_none, "stddev", u_as0, {{"field"}}, {}},
    {FieldFunctions::f_probability_above, FieldVerticalAxes::vctype_none, "probability_above", u_pc, {{"field"}}, {{"limit"}}},
    {FieldFunctions::f_probability_below, FieldVerticalAxes::vctype_none, "probability_below", u_pc, {{"field"}}, {{"limit"}}},
    {FieldFunctions::f_probability_between, FieldVerticalAxes::vctype_none, "probability_between", u_pc, {{"field"}}, {{"limit"}, {"limit"}}},
    {FieldFunctions::f_number_above, FieldVerticalAxes::vctype_none, "number_above", "1", {{"field"}}, {{"limit"}}},
    {FieldFunctions::f_number_below, FieldVerticalAxes::vctype_none, "number_below", "1", {{"field"}}, {{"limit"}}},
    {FieldFunctions::f_number_between, FieldVerticalAxes::vctype_none, "number_between", "1", {{"field"}}, {{"limit"}, {"limit"}}},
    {FieldFunctions::f_equivalent_to, FieldVerticalAxes::vctype_none, "equivalent_to", u_as0, {{"field"}}, {}},
    {FieldFunctions::f_min_value, FieldVerticalAxes::vctype_none, "min_value", u_as0, {{"field"}}, {}, FieldFunctions::varargs_field},
    {FieldFunctions::f_max_value, FieldVerticalAxes::vctype_none, "max_value", u_as0, {{"field"}}, {}, FieldFunctions::varargs_field},
    {FieldFunctions::f_min_index, FieldVerticalAxes::vctype_none, "min_index", u_as0, {{"field"}}, {}, FieldFunctions::varargs_field},
    {FieldFunctions::f_max_index, FieldVerticalAxes::vctype_none, "max_index", u_as0, {{"field"}}, {}, FieldFunctions::varargs_field},
    {FieldFunctions::f_percentile, FieldVerticalAxes::vctype_none, "percentile", u_as0, {{"field"}}, {{"value"}}, FieldFunctions::varargs_const},
    {FieldFunctions::f_neighbour_percentile, FieldVerticalAxes::vctype_none, "neighbour_percentile", u_as0, {{"field"}}, {{"value"}}, FieldFunctions::varargs_const},
    {FieldFunctions::f_neighbour_mean, FieldVerticalAxes::vctype_none, "neighbour_mean", u_as0, {{"field"}}, {{"value"}}, FieldFunctions::varargs_const},
    {FieldFunctions::f_neighbour_probability_above, FieldVerticalAxes::vctype_none, "neighbour_probability_above", "1", {{"field"}}, {{"value"}}, FieldFunctions::varargs_const},
    {FieldFunctions::f_neighbour_probability_above2, FieldVerticalAxes::vctype_none, "neighbour_probability_above2", "1", {{"field"}}, {{"value"}}, FieldFunctions::varargs_const},
    {FieldFunctions::f_neighbour_probability_below, FieldVerticalAxes::vctype_none, "neighbour_probability_below", "1", {{"field"}}, {{"value"}}, FieldFunctions::varargs_const},
    {FieldFunctions::f_neighbour_probability_below2, FieldVerticalAxes::vctype_none, "neighbour_probability_below2", "1", {{"field"}}, {{"value"}}, FieldFunctions::varargs_const},
    {FieldFunctions::f_neighbour_max, FieldVerticalAxes::vctype_none, "neighbour_max", u_as0, {{"field"}}, {{"value"}}, FieldFunctions::varargs_const},
    {FieldFunctions::f_neighbour_min, FieldVerticalAxes::vctype_none, "neighbour_min", u_as0, {{"field"}}, {{"value"}}, FieldFunctions::varargs_const},
    {FieldFunctions::f_snow_cm_from_snow_water_tk_td, FieldVerticalAxes::vctype_none, "snow.cm.from.snow.water", "cm", {{"snow", u_kg_m2}, a_tk, a_tdk}, {}},

    // geographic functions

    {FieldFunctions::f_qvector_plevel_z_tk_xcomp, FieldVerticalAxes::vctype_pressure, "qvector.plevel_z_tk_xcomp", {a_z, a_tk}, {}},
    {FieldFunctions::f_qvector_plevel_z_th_xcomp, FieldVerticalAxes::vctype_pressure, "qvector.plevel_z_th_xcomp", {a_z, a_th}, {}},
    {FieldFunctions::f_qvector_plevel_z_tk_ycomp, FieldVerticalAxes::vctype_pressure, "qvector.plevel_z_tk_ycomp", {a_z, a_tk}, {}},
    {FieldFunctions::f_qvector_plevel_z_th_ycomp, FieldVerticalAxes::vctype_pressure, "qvector.plevel_z_th_ycomp", {a_z, a_th}, {}},
    {FieldFunctions::f_geostrophic_wind_plevel_z_xcomp, FieldVerticalAxes::vctype_pressure, "geostrophic.wind.plevel_z_xcomp", u_m_s, {a_z}, {}},
    {FieldFunctions::f_geostrophic_wind_plevel_z_ycomp, FieldVerticalAxes::vctype_pressure, "geostrophic.wind.plevel_z_ycomp", u_m_s, {a_z}, {}},
    {FieldFunctions::f_geostrophic_vorticity_plevel_z, FieldVerticalAxes::vctype_pressure, "geostrophic.vorticity.plevel_z", {a_z}, {}},

    // isentropic level (ILEVEL) function NB! functions with two output fields do not work (TODO)
    {FieldFunctions::f_geostrophic_wind_ilevel_mpot, FieldVerticalAxes::vctype_isentropic, "geostrophic_wind.ilevel_mpot", {{"mpot"}}, {}, FieldFunctions::varargs_none, 2},

    // level independent functions
    {FieldFunctions::f_rel_vorticity, FieldVerticalAxes::vctype_none, "rel.vorticity", {{"u"}, {"v"}}, {}},
    {FieldFunctions::f_abs_vorticity, FieldVerticalAxes::vctype_none, "abs.vorticity", {{"u"}, {"v"}}, {}},
    {FieldFunctions::f_divergence, FieldVerticalAxes::vctype_none, "divergence", {{"u"}, {"v"}}, {}},
    {FieldFunctions::f_advection, FieldVerticalAxes::vctype_none, "advection", {{"f"}, {"u"}, {"v"}}, {{"hours"}}},
    {FieldFunctions::f_thermal_front_parameter_tx, FieldVerticalAxes::vctype_none, "thermal.front.parameter_tx", {{"tx"}}, {}},
    {FieldFunctions::f_momentum_x_coordinate, FieldVerticalAxes::vctype_none, "momentum.x.coordinate", {{"v"}}, {{"coriolisMin"}}},
    {FieldFunctions::f_momentum_y_coordinate, FieldVerticalAxes::vctype_none, "momentum.y.coordinate", {{"u"}}, {{"coriolisMin"}}},
    {FieldFunctions::f_jacobian, FieldVerticalAxes::vctype_none, "jacobian", {{"fx"}, {"fy"}}, {}}
};

// static member
bool FieldFunctions::parseComputeSetup(const std::vector<std::string>& lines, std::vector<std::string>& errors)
{
  METLIBS_LOG_SCOPE();

  vFieldCompute.clear();

  // parse setup
  std::map<std::string, int> compute;
  compute["add"] = 0;
  compute["subtract"] = 1;
  compute["multiply"] = 2;
  compute["divide"] = 3;
  std::map<std::string, int>::const_iterator pc, pcend = compute.end();

  const FunctionHelper* pf = nullptr;

  const int nlines = lines.size();
  for (int l = 0; l < nlines; l++) {
    const std::string& oneline = lines[l];
    const std::vector<std::string> vstr = miutil::split(oneline, 0, " ", true);
    int n = vstr.size();
    for (int i = 0; i < n; i++) {
      bool err = true;
      const std::vector<std::string> vspec = miutil::split(vstr[i], 1, "=", false);
      if (vspec.size() == 2) {
        const size_t p1 = vspec[1].find('('), p2 = vspec[1].find(')');
        if (p1 != std::string::npos && p2 != std::string::npos && p1 > 0 && p1 < p2 - 1) {
          const std::string functionName = vspec[1].substr(0, p1);
          const std::string functionName_lc = miutil::to_lower(functionName);
          const std::string str = vspec[1].substr(p1 + 1, p2 - p1 - 1);
          FieldCompute fcomp;
          fcomp.name = vspec[0];
          fcomp.functionName = miutil::to_upper(functionName);
          FieldVerticalAxes::VerticalType vctype = FieldVerticalAxes::vctype_none;
          fcomp.func = nullptr;

          // First check if function is a simple calculation
          if ((pc = compute.find(functionName)) != pcend) {
            fcomp.results.push_back(fcomp.name);
            const std::vector<std::string> vpart = miutil::split(str, 1, ",");
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
          } else if ((pf = std::find_if(boost::begin(functions), boost::end(functions),
                                        [functionName_lc](const FunctionHelper& fh) { return functionName_lc == fh.name; })) != boost::end(functions)) {
            const FunctionHelper& fh = *pf;
            fcomp.function = fh.func;
            fcomp.func = &fh;
            // check function arguments
            std::vector<std::string> vpart = miutil::split(str, 0, ",", false);
            int npart = vpart.size();
            int numf = fh.numfields();
            int numc = fh.numconsts();
            if (numf < 0 ) {
              numf = npart;
            }
            unsigned int numr = fh.numresult();
            vctype = fh.vertcoord;
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

  return true;
}

// static member
bool FieldFunctions::splitFieldSpecs(const std::string& paramName,FieldSpec& fs)
{
  fs.use_standard_name = false;
  fs.paramName = paramName;
  if (paramName.find(':')==std::string::npos)
    return false;

  bool levelSpecified= false;
  std::vector<std::string> vs= miutil::split(paramName, 0, ":");
  fs.paramName= vs[0];
  std::vector<std::string> vp;
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
      fs.vcoord = true;
    } else if (vp[0] == "ecoord") {
      fs.ecoord = true;
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
    f = f_sum;
  else if (f == f_sum_of_fields)
    f = f_sum;
  else if (f == f_mean_of_fields)
    f = f_mean_value;
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

bool FieldFunctions::fieldComputer(Function function, const std::vector<float>& constants, const Field_pv& vfinput, const Field_pv& vfres, GridConverter& gc)
{
  using namespace miutil;
  using namespace miutil::fieldcalc;

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

  std::vector<float*> finp;
  std::vector<miutil::ValuesDefined> fdefin;
  std::vector<float*> fout;

  bool ok = true;

  int ninp = vfinput.size();
  int nout = vfres.size();
  int nconst = constants.size();

  miutil::ValuesDefined fDefined = (ninp > 0) ? vfinput.front()->defined() : miutil::NONE_DEFINED;

  std::string unit;

  for (Field_cp fi : vfinput) {
    finp.push_back(fi->data);
    fdefin.push_back(fi->defined());
    if (fi->defined() == miutil::NONE_DEFINED)
      fDefined = miutil::NONE_DEFINED;
    if (fi->defined() == miutil::SOME_DEFINED)
      fDefined = miutil::SOME_DEFINED; // not perfectly correct
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

  const float undef = miutil::UNDEF;

  int nsmooth;
  float plevel, plevel1, plevel2, plevel3, alevel, blevel, odepth;
  float hours, precipMin, snowRateMax, tcMax, fcoriolisMin;

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
    res = pleveltemp(nx, ny, finp[0], plevel, unit, compute, fout[0], fDefined, undef);
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
    res = plevelthe(nx, ny, finp[0], finp[1], plevel, compute, fout[0], fDefined, undef);
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
    res = plevelhum(nx, ny, finp[0], finp[1], plevel, unit, compute, fout[0], fDefined, undef);
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
    res = pleveldz2tmean(nx, ny, finp[0], finp[1], plevel1, plevel2, compute, fout[0], fDefined, undef);
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
    res = plevelducting(nx, ny, finp[0], finp[1], plevel, compute, fout[0], fDefined, undef);
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
    res = kIndex(nx, ny, finp[0], finp[1], finp[2], finp[3], finp[4], plevel1, plevel2, plevel3, compute, fout[0], fDefined, undef);
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
    res = ductingIndex(nx, ny, finp[0], finp[1], plevel, compute, fout[0], fDefined, undef);
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
    res = showalterIndex(nx, ny, finp[0], finp[1], finp[2], plevel1, plevel2, compute, fout[0], fDefined, undef);
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
    res = boydenIndex(nx, ny, finp[0], finp[1], finp[2], plevel1, plevel2, compute, fout[0], fDefined, undef);
    break;

  case f_sweatindex_plevel:
    if (ninp != 8 || nout != 1)
      break;
    res = sweatIndex(nx, ny, finp[0], finp[1], finp[2], finp[3], finp[4], finp[5], finp[6], finp[7], fout[0], fDefined, undef);
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
    res = hleveltemp(nx, ny, finp[0], finp[1], alevel, blevel, unit, compute, fout[0], fDefined, undef);
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
    res = hlevelthe(nx, ny, finp[0], finp[1], finp[2], alevel, blevel, compute, fout[0], fDefined, undef);
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
    res = hlevelhum(nx, ny, finp[0], finp[1], finp[2], alevel, blevel, unit, compute, fout[0], fDefined, undef);
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
    res = hlevelducting(nx, ny, finp[0], finp[1], finp[2], alevel, blevel, compute, fout[0], fDefined, undef);
    break;

  case f_pressure_hlevel_xx_psurf:
    if (ninp != 2 || nout != 1)
      break;
    alevel = vfinput[0]->aHybrid;
    blevel = vfinput[0]->bHybrid;
    res = hlevelpressure(nx, ny, finp[1], alevel, blevel, fout[0], fDefined, undef);
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
    res = aleveltemp(nx, ny, finp[0], finp[1], unit, compute, fout[0], fDefined, undef);
    break;

  case f_the_alevel_tk_q_p:
    if (compute == 0)
      compute = 1;
  case f_the_alevel_th_q_p:
    if (compute == 0)
      compute = 2;
    if (ninp != 3 || nout != 1)
      break;
    res = alevelthe(nx, ny, finp[0], finp[1], finp[2], compute, fout[0], fDefined, undef);
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
    res = alevelhum(nx, ny, finp[0], finp[1], finp[2], unit, compute, fout[0], fDefined, undef);
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
    res = alevelducting(nx, ny, finp[0], finp[1], finp[2], compute, fout[0], fDefined, undef);
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
    res = seaSoundSpeed(nx, ny, finp[0], finp[1], odepth, compute, fout[0], fDefined, undef);
    break;

    //---------------------------------------------------
    // level (pressure) independent functions
    //---------------------------------------------------

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
    res = cvhum(nx, ny, finp[0], finp[1], unit, compute, fout[0], fDefined, undef);
    break;

  case f_abshum_tk_rh:
    if (ninp != 2 || nout != 1)
      break;
    res = abshum(nx, ny, finp[0], finp[1], fout[0], fDefined, undef);
    break;

  case f_vector_abs:
    if (ninp != 2 || nout != 1)
      break;
    res = vectorabs(nx, ny, finp[0], finp[1], fout[0], fDefined, undef);
    break;

  case f_minvalue_fields:
    if (ninp != 2 || nout != 1)
      break;
    minvalueFields(nx, ny, finp[0], finp[1], fout[0], fDefined, UNDEF);
    res = true;
    break;

  case f_maxvalue_fields:
    if (ninp != 2 || nout != 1)
      break;
    maxvalueFields(nx, ny, finp[0], finp[1], fout[0], fDefined, UNDEF);
    res = true;
    break;

  case f_minvalue_field_const:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    minvalueFieldConst(nx, ny, finp[0], constants[0], fout[0], fDefined, UNDEF);
    res = true;
    break;

  case f_maxvalue_field_const:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    maxvalueFieldConst(nx, ny, finp[0], constants[0], fout[0], fDefined, UNDEF);
    res = true;
    break;

  case f_abs:
    if (ninp != 1 || nout != 1)
      break;
    absvalueField(nx, ny, finp[0], fout[0], fDefined, miutil::UNDEF);
    res = true;
    break;

  case f_log10:
    if (ninp != 1 || nout != 1)
      break;
    log10Field(nx, ny, finp[0], fout[0], fDefined, miutil::UNDEF);
    res = true;
    break;

  case f_pow10:
    if (ninp != 1 || nout != 1)
      break;
    pow10Field(nx, ny, finp[0], fout[0], fDefined, miutil::UNDEF);
    res = true;
    break;

  case f_log:
    if (ninp != 1 || nout != 1)
      break;
    logField(nx, ny, finp[0], fout[0], fDefined, miutil::UNDEF);
    res = true;
    break;

  case f_exp:
    if (ninp != 1 || nout != 1)
      break;
    expField(nx, ny, finp[0], fout[0], fDefined, miutil::UNDEF);
    res = true;
    break;

  case f_power:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;

    powerField(nx, ny, finp[0], constants[0], fout[0], fDefined, miutil::UNDEF);
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
    fieldcalc::copy_field(fout[0], finp[0], fsize);
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
    res = windCooling(nx, ny, finp[0], finp[1], finp[2], compute, fout[0], fDefined, undef);
    break;

  case f_undercooled_rain:
    if (ninp != 3 || nout != 1 || nconst != 3)
      break;
    precipMin = constants[0];
    snowRateMax = constants[1];
    tcMax = constants[2];
    res = underCooledRain(nx, ny, finp[0], finp[1], finp[2], precipMin, snowRateMax, tcMax, fout[0], fDefined, undef);
    break;

  case f_pressure2flightlevel:
    if (ninp != 1 || nout != 1)
      break;
    res = pressure2FlightLevel(nx, ny, finp[0], fout[0], fDefined, undef);
    break;

  case f_vessel_icing_overland:
    if (ninp != 6 || nout != 1)
      break;
    res = vesselIcingOverland(nx, ny, finp[0], finp[1], finp[2], finp[3], finp[4], finp[5], fout[0], fDefined, undef);
    break;

  case f_vessel_icing_mertins:
    if (ninp != 6 || nout != 1)
      break;
    res = vesselIcingMertins(nx, ny, finp[0], finp[1], finp[2], finp[3], finp[4], finp[5], fout[0], fDefined, undef);
    break;

  case f_vessel_icing_modstall:
    if (ninp != 11 || nout != 1 || nconst != 4)
      break;
    res = vesselIcingModStall(nx, ny, finp[0], finp[1], finp[2], finp[3], finp[4], finp[5], finp[6], finp[7], finp[8], finp[9], finp[10], constants[0],
                              constants[1], constants[2], constants[3], fout[0], fDefined, undef);
    break;

  case f_vessel_icing_mincog:
    if (ninp != 11 || nout != 1 || nconst != 5)
      break;
    res = vesselIcingMincog(nx, ny, finp[0], finp[1], finp[2], finp[3], finp[4], finp[5], finp[6], finp[7], finp[8], finp[9], finp[10], constants[0],
                            constants[1], constants[2], constants[3], constants[4], fout[0], fDefined, undef);
    break;

  case f_replace_undefined:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    replaceUndefined(nx, ny, finp[0], constants[0], fout[0], fDefined, miutil::UNDEF);
    res = true;
    break;

  case f_replace_defined:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    replaceDefined(nx, ny, finp[0], constants[0], fout[0], fDefined, miutil::UNDEF);
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
    res = values2classes(nx, ny, finp[0], fout[0], constants, fDefined, undef);
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
    res = fieldOPERconstant(compute, nx, ny, finp[0], constants[0], fout[0], fDefined, undef);
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
    res = constantOPERfield(compute, nx, ny, constants[0], finp[0], fout[0], fDefined, undef);
    break;

  case f_equivalent_to :
    if (ninp != 1 || nout != 1)
      break;
    fieldcalc::copy_field(fout[0], finp[0], fsize);
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
    res = extremeValue(compute, nx, ny, finp, fout[0], fDefined, undef);
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
    res = probability(compute, nx, ny, finp, fdefin, constants, fout[0], fDefined, undef);
    break;

  case f_neighbour_probability_above:
    if (compute == 0)
      compute = 6;
  case f_neighbour_probability_below:
    if (compute == 0)
      compute = 5;
  case f_percentile:
  case f_neighbour_percentile:
    if (compute == 0)
      compute = 4;
  case f_neighbour_min:
    if (compute == 0)
      compute = 3;
  case f_neighbour_max:
    if (compute == 0)
      compute = 2;
  case f_neighbour_mean:
    if (compute == 0)
      compute = 1;
    if (ninp != 1 || nout != 1 || nconst < 1)
      break;
    res = neighbourFunctions(nx, ny, finp[0], constants, compute, fout[0], fDefined, undef);
    break;

  case f_neighbour_probability_above2:
    if (compute == 0)
      compute = 6;
  case f_neighbour_probability_below2:
    if (compute == 0)
      compute = 5;
    if (ninp != 1 || nout != 1 || nconst < 2)
      break;
    res = neighbourProbFunctions(nx, ny, finp[0], constants, compute, fout[0], fDefined, undef);
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
    if (MapFields_cp mf = gc.getMapFields(vfinput[0]->area))
      res = plevelqvector(nx, ny, finp[0], finp[1], mf->xmapr, mf->ymapr, mf->coriolis, plevel, compute, fout[0], fDefined, undef);
    break;

  case f_geostrophic_wind_plevel_z_xcomp:
    if (ninp != 1 || nout != 1)
      break;
    if (MapFields_cp mf = gc.getMapFields(vfinput[0]->area))
      res = plevelgwind_xcomp(nx, ny, finp[0], mf->xmapr, mf->ymapr, mf->coriolis, fout[0], fDefined, undef);
    break;

  case f_geostrophic_wind_plevel_z_ycomp:
    if (ninp != 1 || nout != 1)
      break;
    if (MapFields_cp mf = gc.getMapFields(vfinput[0]->area))
      res = plevelgwind_ycomp(nx, ny, finp[0], mf->xmapr, mf->ymapr, mf->coriolis, fout[0], fDefined, undef);
    break;

  case f_geostrophic_vorticity_plevel_z:
    if (ninp != 1 || nout != 1)
      break;
    if (MapFields_cp mf = gc.getMapFields(vfinput[0]->area))
      res = plevelgvort(nx, ny, finp[0], mf->xmapr, mf->ymapr, mf->coriolis, fout[0], fDefined, undef);
    break;

  case f_geostrophic_wind_ilevel_mpot:
    if (ninp != 1 || nout != 2)
      break;
    if (MapFields_cp mf = gc.getMapFields(vfinput[0]->area))
      res = ilevelgwind(nx, ny, finp[0], mf->xmapr, mf->ymapr, mf->coriolis, fout[0], fout[1], fDefined, undef);
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
    if (MapFields_cp mf = gc.getMapFields(vfinput[0]->area))
      res = gradient(nx, ny, finp[0], mf->xmapr, mf->ymapr, compute, fout[0], fDefined, undef);
    break;

  case f_rel_vorticity:
    if (ninp != 2 || nout != 1)
      break;
    if (MapFields_cp mf = gc.getMapFields(vfinput[0]->area))
      res = relvort(nx, ny, finp[0], finp[1], mf->xmapr, mf->ymapr, fout[0], fDefined, undef);
    break;

  case f_abs_vorticity:
    if (ninp != 2 || nout != 1)
      break;
    if (MapFields_cp mf = gc.getMapFields(vfinput[0]->area))
      res = absvort(nx, ny, finp[0], finp[1], mf->xmapr, mf->ymapr, mf->coriolis, fout[0], fDefined, undef);
    break;

  case f_divergence:
    if (ninp != 2 || nout != 1)
      break;
    if (MapFields_cp mf = gc.getMapFields(vfinput[0]->area))
      res = divergence(nx, ny, finp[0], finp[1], mf->xmapr, mf->ymapr, fout[0], fDefined, undef);
    break;

  case f_advection:
    if (ninp != 3 || nout != 1 || nconst != 1)
      break;
    hours = constants[0];
    if (MapFields_cp mf = gc.getMapFields(vfinput[0]->area))
      res = advection(nx, ny, finp[0], finp[1], finp[2], mf->xmapr, mf->ymapr, hours, fout[0], fDefined, undef);
    break;

  case f_thermal_front_parameter_tx:
    if (ninp != 1 || nout != 1)
      break;
    if (MapFields_cp mf = gc.getMapFields(vfinput[0]->area))
      res = thermalFrontParameter(nx, ny, finp[0], mf->xmapr, mf->ymapr, fout[0], fDefined, undef);
    break;

  case f_momentum_x_coordinate:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    fcoriolisMin = constants[0];
    if (MapFields_cp mf = gc.getMapFields(vfinput[0]->area))
      res = momentumXcoordinate(nx, ny, finp[0], mf->xmapr, mf->coriolis, fcoriolisMin, fout[0], fDefined, undef);
    break;

  case f_momentum_y_coordinate:
    if (ninp != 1 || nout != 1 || nconst != 1)
      break;
    fcoriolisMin = constants[0];
    if (MapFields_cp mf = gc.getMapFields(vfinput[0]->area))
      res = momentumYcoordinate(nx, ny, finp[0], mf->ymapr, mf->coriolis, fcoriolisMin, fout[0], fDefined, undef);
    break;

  case f_jacobian:
    if (ninp != 2 || nout != 1)
      break;
    if (MapFields_cp mf = gc.getMapFields(vfinput[0]->area))
      res = jacobian(nx, ny, finp[0], finp[1], mf->xmapr, mf->ymapr, fout[0], fDefined, undef);
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
