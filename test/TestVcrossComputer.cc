/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2022 met.no

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

#include <vcross_v2/VcrossComputer.h>
#include <vcross_v2/VcrossQtManager.h>
#include <vcross_v2/VcrossSetup.h>

#include <diField/FimexSource.h>
#include <diField/VcrossUtil.h>

#include <fstream>

#include <gtest/gtest.h>

#define MILOGGER_CATEGORY "diana.test.VcrossComputer"
#include "miLogger/miLogging.h"

using diutil::Values;
using namespace vcross;

namespace /* anonymous */ {

const char BANGLADESH_FILE[] = "bangladesh_vc.nc";
const int BANGLADESH_N_CS = 4;
const int BANGLADESH_CS_LEN[BANGLADESH_N_CS] = { 1, 1, 96, 124 };
const int BANGLADESH_N_TIME = 5;
const int BANGLADESH_N_Z = 15;
//const vcross::Time BANGLADESH_RT = util::from_miTime(miutil::miTime("2014-11-24 12:00:00"));
const char BANGLADESH_RTT[] = "2014-11-24 12:00:00";

Source_p openFimexSource(const std::string& file)
{
  const std::string fileName = std::string(TEST_SRCDIR) + "/" + file;
  std::ifstream inputfile(fileName.c_str());
  if (not inputfile)
    return FimexSource_p();

  diutil::CharsetConverter_p csc = diutil::findConverter(diutil::CHARSET_READ(), diutil::CHARSET_INTERNAL());
  return FimexSource_p(new FimexSource(fileName, "netcdf", "", csc));
}

} /* anonymous namespace */

TEST(TestVcrossComputer, BangladeshTH)
{
  NameItem_v computations;
  computations.push_back(parseComputationLine("th = th_from_tk(air_temperature_pl)"));

  Source_p fs = openFimexSource(BANGLADESH_FILE);
  if (not fs)
    return;

  fs->update();
  const vcross::Time BANGLADESH_RT = util::from_miTime(miutil::miTime(BANGLADESH_RTT));
  Inventory_cp inv = fs->getInventory(BANGLADESH_RT);
  ASSERT_TRUE(bool(inv));
  ASSERT_EQ(BANGLADESH_N_TIME, inv->times.npoint());
  ASSERT_EQ(BANGLADESH_N_CS,   inv->crossections.size());

  Crossection_cp cs3 = inv->crossections.at(3);
  ASSERT_TRUE(bool(cs3));
  EXPECT_EQ(BANGLADESH_CS_LEN[3], cs3->length());

  InventoryBase_cps fields(inv->fields.begin(), inv->fields.end());
  vcross::resolve(fields, computations);

  FunctionData_cp th = std::dynamic_pointer_cast<const FunctionData>(findItemById(fields, "th"));
  ASSERT_TRUE(bool(th));

  collectRequired(fields, th);

  ZAxisData_cp vertical = th->zaxis();
  InventoryBase_cp vertical_pressure = th->zaxis()->pressureField();
  ASSERT_TRUE(vertical and vertical_pressure);

  const Time& time = inv->times.at(1);

  InventoryBase_cps request;
  collectRequired(request, th);

  name2value_t n2v;
  fs->getCrossectionValues(BANGLADESH_RT, cs3, time, request, n2v, 0);
  vc_evaluate_field(th, n2v);

  auto th_values = n2v[th->id()];
  ASSERT_TRUE(bool(th_values));
  auto vertical_values = n2v[vertical->id()];
  if (!vertical_values)
    vertical_values = n2v[vertical_pressure->id()];
  ASSERT_TRUE(bool(vertical_values));

  const Values::Shape& shape(th_values->shape());
  ASSERT_EQ(2, shape.rank());
  EXPECT_EQ(Values::GEO_X, shape.name(0));
  EXPECT_EQ(BANGLADESH_CS_LEN[3], shape.length(0));
  EXPECT_EQ(Values::GEO_Z, shape.name(1));
  EXPECT_EQ(BANGLADESH_N_Z, shape.length(1));

  Values::ShapeIndex idx(th_values->shape());
  idx.set(Values::GEO_X, 43);
  idx.set(Values::GEO_Z, 13);
  EXPECT_FLOAT_EQ(301.51855, th_values->value(idx));

  idx.set(Values::GEO_Z, 3);
  EXPECT_FLOAT_EQ(414.29428, th_values->value(idx));

  idx.set(Values::GEO_X, 2);
  EXPECT_FLOAT_EQ(422.00815, th_values->value(idx));
}

TEST(TestVcrossComputer, FunctionsWithConstants)
{
  const std::string name_twotk = "twice_tk", name_tktwo = "tk_twice", name_tktk = "tktk";
  NameItem_v computations;
  computations.push_back(parseComputationLine(name_twotk + " = multiply(const:2,air_temperature_pl)"));
  computations.push_back(parseComputationLine(name_tktwo + " = multiply(air_temperature_pl,const:2)"));
  computations.push_back(parseComputationLine(name_tktk  + " = multiply(air_temperature_pl,air_temperature_pl)"));

  Source_p fs = openFimexSource(BANGLADESH_FILE);
  if (not fs)
    return;

  fs->update();
  const vcross::Time BANGLADESH_RT = util::from_miTime(miutil::miTime(BANGLADESH_RTT));
  Inventory_cp inv = fs->getInventory(BANGLADESH_RT);

  Crossection_cp cs3 = inv->crossections.at(3);

  InventoryBase_cps fields(inv->fields.begin(), inv->fields.end());
  vcross::resolve(fields, computations);

  FunctionData_cp twotk = std::dynamic_pointer_cast<const FunctionData>(findItemById(fields, name_twotk));
  FunctionData_cp tktwo = std::dynamic_pointer_cast<const FunctionData>(findItemById(fields, name_tktwo));
  FunctionData_cp tktk  = std::dynamic_pointer_cast<const FunctionData>(findItemById(fields, name_tktk));
  ASSERT_TRUE(bool(twotk) && bool(tktwo) && bool(tktk));

  const Time& time = inv->times.at(1);

  InventoryBase_cps request;
  collectRequired(request, twotk);
  collectRequired(request, tktwo);
  collectRequired(request, tktk);

  name2value_t n2v;
  fs->getCrossectionValues(BANGLADESH_RT, cs3, time, request, n2v, 0);
  vc_evaluate_field(twotk, n2v);
  vc_evaluate_field(tktwo, n2v);
  vc_evaluate_field(tktk,  n2v);

  auto twotk_values = n2v[twotk->id()];
  auto tktwo_values = n2v[tktwo->id()];
  auto tktk_values = n2v[tktk->id()];
  ASSERT_TRUE(bool(twotk_values) && bool(tktwo_values) && bool(tktk_values));

  Values::ShapeIndex idx(tktwo_values->shape());
  idx.set(Values::GEO_X, 43);
  idx.set(Values::GEO_Z, 13);

  const float v_twotk = twotk_values->value(idx);
  const float v_tktwo = tktwo_values->value(idx);
  const float v_tktk  = tktk_values->value(idx);
  EXPECT_FLOAT_EQ(v_twotk, v_tktwo);
  EXPECT_FLOAT_EQ(v_twotk*v_twotk / 4, v_tktk);
}

TEST(TestVcrossComputer, AddMultiplyDivide)
{
  const std::string name_xmuly = "x_times_y", name_xdivy = "x_divided_by_y", name_xaddy = "x_plus_y";
  NameItem_v computations;
  computations.push_back(parseComputationLine(name_xmuly + " = multiply(x_wind_pl,y_wind_pl)"));
  computations.push_back(parseComputationLine(name_xdivy + " = divide(x_wind_pl,y_wind_pl)"));
  computations.push_back(parseComputationLine(name_xaddy  + " = add(x_wind_pl,y_wind_pl)"));

  Source_p fs = openFimexSource(BANGLADESH_FILE);
  if (not fs)
    return;

  fs->update();
  const vcross::Time BANGLADESH_RT = util::from_miTime(miutil::miTime(BANGLADESH_RTT));
  Inventory_cp inv = fs->getInventory(BANGLADESH_RT);

  Crossection_cp cs3 = inv->crossections.at(0);

  InventoryBase_cps fields(inv->fields.begin(), inv->fields.end());
  vcross::resolve(fields, computations);

  FunctionData_cp xmy = std::dynamic_pointer_cast<const FunctionData>(findItemById(fields, name_xmuly));
  FunctionData_cp xdy = std::dynamic_pointer_cast<const FunctionData>(findItemById(fields, name_xdivy));
  FunctionData_cp xpy = std::dynamic_pointer_cast<const FunctionData>(findItemById(fields, name_xaddy));
  ASSERT_TRUE(bool(xmy) && bool(xdy) && bool(xpy));

  const Time& time = inv->times.at(0);

  InventoryBase_cps request;
  collectRequired(request, xmy);
  collectRequired(request, xdy);
  collectRequired(request, xpy);

  name2value_t n2v;
  fs->getCrossectionValues(BANGLADESH_RT, cs3, time, request, n2v, 0);
  vc_evaluate_field(xmy, n2v);
  vc_evaluate_field(xdy, n2v);
  vc_evaluate_field(xpy, n2v);

  auto xmy_values = n2v[xmy->id()];
  auto xdy_values = n2v[xdy->id()];
  auto xpy_values = n2v[xpy->id()];
  ASSERT_TRUE(bool(xmy_values) && bool(xdy_values) && bool(xpy_values));

  Values::ShapeIndex idx(xdy_values->shape());
  idx.set(Values::GEO_X, 0);
  idx.set(Values::GEO_Z, 0);

  const float x_0 = 5.961807, y_0 = -3.595242;

  const float v_xmy = xmy_values->value(idx);
  const float v_xdy = xdy_values->value(idx);
  const float v_xpy = xpy_values->value(idx);
  EXPECT_FLOAT_EQ(x_0*y_0, v_xmy);
  EXPECT_FLOAT_EQ(x_0/y_0, v_xdy);
  EXPECT_FLOAT_EQ(x_0+y_0, v_xpy);
}

TEST(TestVcrossComputer, Quadratic)
{
  const std::string name_xw = "x_wind_pl", name_xwu = "xw_u", name_xw_2l = "xw_2l",
      name_xw_sq = "xw_sq", name_xw_3sq = "xw_3sq", name_xwq = "xw_quadratic";
  NameItem_v computations;
  computations.push_back(parseComputationLine(name_xwu + " = impose_unit(" + name_xw + ",1)"));
  computations.push_back(parseComputationLine(name_xw_2l + " = multiply(const:1.5," + name_xwu + ")"));
  computations.push_back(parseComputationLine(name_xw_sq + " = multiply(" + name_xwu + "," + name_xwu + ")"));
  computations.push_back(parseComputationLine(name_xw_3sq + " = multiply(const:3," + name_xw_sq + ")"));
  computations.push_back(parseComputationLine(name_xwq + " = add(xw_2l," + name_xw_3sq + ")"));

  Source_p fs = openFimexSource(BANGLADESH_FILE);
  if (not fs)
    return;

  fs->update();
  const vcross::Time BANGLADESH_RT = util::from_miTime(miutil::miTime(BANGLADESH_RTT));
  Inventory_cp inv = fs->getInventory(BANGLADESH_RT);

  Crossection_cp cs3 = inv->crossections.at(0);

  InventoryBase_cps fields(inv->fields.begin(), inv->fields.end());
  vcross::resolve(fields, computations);

  FunctionData_cp xwq = std::dynamic_pointer_cast<const FunctionData>(findItemById(fields, name_xwq));
  ASSERT_TRUE(bool(xwq));

  const Time& time = inv->times.at(0);

  InventoryBase_cps request;
  collectRequired(request, xwq);

  name2value_t n2v;
  fs->getCrossectionValues(BANGLADESH_RT, cs3, time, request, n2v, 0);
  vc_evaluate_field(xwq, n2v);

  auto xwq_values = n2v[xwq->id()];
  ASSERT_TRUE(bool(xwq_values));

  Values::ShapeIndex idx(xwq_values->shape());
  idx.set(Values::GEO_X, 0);
  idx.set(Values::GEO_Z, 0);

  const float x_0 = 5.961807;
  const float v_xwq = xwq_values->value(idx);
  EXPECT_FLOAT_EQ(x_0*1.5 + 3*x_0*x_0, v_xwq);
}
