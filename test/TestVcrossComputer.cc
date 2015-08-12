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

#include <vcross_v2/VcrossComputer.h>
#include <vcross_v2/VcrossQtManager.h>
#include <vcross_v2/VcrossSetup.h>

#include <diField/FimexSource.h>
#include <diField/VcrossUtil.h>

#include <fstream>

#include <gtest/gtest.h>

#define MILOGGER_CATEGORY "diana.test.VcrossComputer"
#include "miLogger/miLogging.h"

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

  return FimexSource_p(new FimexSource(fileName, "netcdf"));
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
  ASSERT_TRUE(inv);
  ASSERT_EQ(BANGLADESH_N_TIME, inv->times.npoint());
  ASSERT_EQ(BANGLADESH_N_CS,   inv->crossections.size());

  Crossection_cp cs3 = inv->crossections.at(3);
  ASSERT_TRUE(cs3);
  EXPECT_EQ(BANGLADESH_CS_LEN[3], cs3->length());

  InventoryBase_cps fields(inv->fields.begin(), inv->fields.end());
  vcross::resolve(fields, computations);

  FunctionData_cp th = boost::dynamic_pointer_cast<const FunctionData>(findItemById(fields, "th"));
  ASSERT_TRUE(th);

  collectRequired(fields, th);

  ZAxisData_cp vertical = th->zaxis();
  InventoryBase_cp vertical_pressure = th->zaxis()->pressureField();
  ASSERT_TRUE(vertical and vertical_pressure);

  const Time& time = inv->times.at(1);

  InventoryBase_cps request;
  collectRequired(request, th);

  name2value_t n2v;
  fs->getCrossectionValues(BANGLADESH_RT, cs3, time, request, n2v);
  vc_evaluate_field(th, n2v);

  Values_cp th_values = n2v[th->id()];
  ASSERT_TRUE(th_values);
  Values_cp vertical_values = n2v[vertical->id()];
  if (not vertical_values)
    vertical_values = n2v[vertical_pressure->id()];
  ASSERT_TRUE(vertical_values);

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
