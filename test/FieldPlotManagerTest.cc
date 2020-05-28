/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2020 met.no

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

#include <diField/diFieldManager.h>
#include <diFieldPlotCommand.h>
#include <diFieldPlotManager.h>

#include "testinghelpers.h"

#include <mi_fieldcalc/MetConstants.h>

#include <string>

TEST(FieldPlotManager, ParameterMinus)
{
  const std::string tkc = "air_temp2m_auto_corr", tk = "air_temperature";
  const float ex_tkc = 263.81, ex_tk = 265.48;

  const std::string filename(TEST_SRCDIR "/test_fimexio_rw.nc");
  const std::string model = "fmtest";
  FieldManager_p fmanager(new FieldManager());
  ASSERT_TRUE(fmanager->addModels({"filegroup=x", "model=" + model + " t=fimex sourcetype=netcdf file=" + filename}));

  std::unique_ptr<FieldPlotManager> fpm(new FieldPlotManager(fmanager));

  const std::string model_param = "model=" + model + " parameter=", tail = " plottype=fill_cell palette=blue";
  const miutil::miTime ptime("2013-02-27 00:00:00");
  {
    FieldPlotCommand_cp cmd = FieldPlotCommand::fromString(model_param + tkc + tail, false);
    Field_pv out;
    ASSERT_TRUE(fpm->makeFields(cmd, ptime, out));
    ASSERT_TRUE(!out.empty() && out.front() && out.front()->data);
    EXPECT_NEAR(ex_tkc, out.front()->data[0], 0.125);
  }

  {
    FieldPlotCommand_cp cmd = FieldPlotCommand::fromString("( " + model_param + tkc + " - " + model_param + tk + " )" + tail, false);
    Field_pv out;
    ASSERT_TRUE(fpm->makeFields(cmd, ptime, out));
    ASSERT_TRUE(!out.empty() && out.front() && out.front()->data);
    EXPECT_NEAR(ex_tkc - ex_tk, out.front()->data[0], 0.125);
  }

  {
    FieldPlotCommand_cp cmd = FieldPlotCommand::fromString(model_param + tkc + tail + " units=m", false);
    Field_pv out;
    ASSERT_FALSE(fpm->makeFields(cmd, ptime, out)); // cannot convert temperature to meter
  }
}

TEST(FieldPlotManager, ParameterMinusComputed)
{
  if (!ditest::hasTestExtra())
    return;

  const std::string tdk = "dew_point_temperature_pl", tk = "air_temperature_pl";
#if 0
  const float t0 = miutil::constants::t0;
  const std::string units = "K";
#else
  const float t0 = 0;
  const std::string units = "degC";
#endif
  const float ex_tdk = -0.718 + t0, ex_tk = 6.26 + t0;

  std::vector<std::string> errors;
  ASSERT_TRUE(FieldFunctions::parseComputeSetup({tdk + "=tdk.plevel_tk_q(" + tk + ",specific_humidity_pl)"}, errors) && errors.empty());

  const std::string filename = ditest::pathTestExtra("ecmwf_2020050500.nc");
  const std::string model = "fmtest";
  FieldManager_p fmanager(new FieldManager());
  ASSERT_TRUE(fmanager->addModels({"filegroup=x", "model=" + model + " t=fimex sourcetype=netcdf file=" + filename}));

  std::unique_ptr<FieldPlotManager> fpm(new FieldPlotManager(fmanager));

  const std::string model_param = "model=" + model + " parameter=", tail = " vlevel=1000 plottype=fill_cell palette=blue units=" + units;
  const miutil::miTime ptime("2020-05-05 00:00:00");

  const float eps = 1.0 / (1 << 10);

  FieldPlotCommand_cp cmd_tk = FieldPlotCommand::fromString(model_param + tk + tail, false);
  Field_pv out_tk;
  ASSERT_TRUE(fpm->makeFields(cmd_tk, ptime, out_tk));
  ASSERT_TRUE(!out_tk.empty() && out_tk.front() && out_tk.front()->data);
  EXPECT_EQ(miutil::ALL_DEFINED, out_tk.front()->defined());
  EXPECT_NEAR(ex_tk, out_tk.front()->data[0], eps);

  FieldPlotCommand_cp cmd_tdk = FieldPlotCommand::fromString(model_param + tdk + tail, false);
  Field_pv out_tdk;
  ASSERT_TRUE(fpm->makeFields(cmd_tdk, ptime, out_tdk));
  ASSERT_TRUE(!out_tdk.empty() && out_tdk.front() && out_tdk.front()->data);
  EXPECT_EQ(miutil::ALL_DEFINED, out_tdk.front()->defined());
  EXPECT_NEAR(ex_tdk, out_tdk.front()->data[0], eps);

  EXPECT_NEAR(ex_tk - ex_tdk, out_tk.front()->data[0] - out_tdk.front()->data[0], eps);

  FieldPlotCommand_cp cmd_diff = FieldPlotCommand::fromString("( " + model_param + tk + " - " + model_param + tdk + " )" + tail, false);
  Field_pv out_diff;
  ASSERT_TRUE(fpm->makeFields(cmd_diff, ptime, out_diff));
  ASSERT_TRUE(!out_diff.empty() && out_diff.front() && out_diff.front()->data);
  EXPECT_EQ(miutil::ALL_DEFINED, out_diff.front()->defined());
  EXPECT_NEAR(ex_tk - ex_tdk, out_diff.front()->data[0], eps);
}
