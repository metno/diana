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

#include <diField/diFieldManager.h>
#include <diField/diFieldFunctions.h>

#include "testinghelpers.h"

#include <mi_fieldcalc/MetConstants.h>

#include <fstream>
#include <string>

namespace {
const float epsilon = 1.0 / (1 << 6);

FieldRequest frqForParam(const FieldRequest& frq, const std::string& paramName)
{
  FieldRequest frqp = frq;
  frqp.paramName = paramName;
  return frqp;
}
} // namespace

TEST(FieldManager, TestRW)
{
    const std::string fileName(TEST_BUILDDIR "/testrun_fimexio_rw.nc");
    {
        const std::string origFileName(TEST_SRCDIR "/test_fimexio_rw.nc");
        std::ifstream orig(origFileName.c_str());
        ASSERT_TRUE(bool(orig)) << "no file '" << origFileName << "'";
        std::ofstream copy(fileName.c_str());
        copy << orig.rdbuf();
        ASSERT_TRUE(bool(orig));
    }

    const int NEWDATA = 17;

    FieldRequest fieldrequest;
    fieldrequest.paramName = "lwe_precipitation_rate";
    fieldrequest.ptime = miutil::miTime("2013-02-27 00:00:00");
    fieldrequest.refTime = "2013-02-27T00:00:00";
    fieldrequest.unit = "m/s";

    std::unique_ptr<FieldManager> fmanager(new FieldManager());
    {
        const std::string model_w = "fmtest_write";
        ASSERT_TRUE(fmanager->addModels({"filegroup=x", "model=" + model_w + " t=fimex sourcetype=netcdf file=" + fileName + " writeable=true"}));

        fieldrequest.modelName = model_w;
        Field_p fieldW = fmanager->makeField(fieldrequest);
        ASSERT_TRUE(fieldW != 0);
        ASSERT_EQ(1, fieldW->data[0]);
        fieldW->data[0] = NEWDATA;
        ASSERT_TRUE(fmanager->writeField(fieldrequest, fieldW));
    }
    {
        const std::string model_r = "fmtest_read";
        ASSERT_TRUE(fmanager->addModels({"filegroup=x", "model=" + model_r + " t=fimex sourcetype=netcdf file=" + fileName}));

        fieldrequest.modelName = model_r;
        Field_p fieldR = fmanager->makeField(fieldrequest);
        ASSERT_TRUE(fieldR != 0);
        ASSERT_EQ(NEWDATA, fieldR->data[0]);
    }
}

TEST(FieldManager, GetFieldTime)
{
  const std::string filename(TEST_SRCDIR "/test_fimexio_rw.nc");
  const std::string model = "fmtest";
  std::unique_ptr<FieldManager> fmanager(new FieldManager());
  ASSERT_TRUE(fmanager->addModels({ "model=" + model + " t=fimex sourcetype=netcdf file=" + filename }));

  FieldRequest frq0;
  frq0.modelName = model;
  frq0.paramName = "lwe_precipitation_rate";
  frq0.refTime = "2013-02-27T00:00:00";
  frq0.allTimeSteps = true;

  const miutil::miTime time0(frq0.refTime);

  {
    const auto times = fmanager->getFieldTime({frq0}, true);
    ASSERT_EQ(67, times.size());
    EXPECT_EQ(time0, *times.begin());
  }
  {
    FieldRequest frq = frq0;
    frq.hourOffset = 18;
    miutil::miTime time = time0;
    time.addHour(-frq.hourOffset);

    const auto times = fmanager->getFieldTime({frq}, true);
    ASSERT_FALSE(times.empty());
    EXPECT_EQ(time, *times.begin());
  }
  {
    FieldRequest frq1 = frq0;
    frq1.paramName = "sea_surface_temperature";

    const auto times = fmanager->getFieldTime({frq0, frq1}, true);
    ASSERT_FALSE(times.empty());
    EXPECT_EQ(time0, *times.begin());
  }
}

// this is more a test for GridCollection::getTimesFromCompute
TEST(FieldManager, GetFieldTimeComputed)
{
  const std::string tdk = "tdk", tk = "air_temperature";
  std::vector<std::string> errors;
  ASSERT_TRUE(FieldFunctions::parseComputeSetup({tdk+"=tdk.tk_rh("+tk+",relative_humidity)"}, errors) && errors.empty());

  const std::string filename(TEST_SRCDIR "/test_fimexio_rw.nc");
  const std::string model = "fmtest";
  std::unique_ptr<FieldManager> fmanager(new FieldManager());
  ASSERT_TRUE(fmanager->addModels({"filegroup=x", "model=" + model + " t=fimex sourcetype=netcdf file=" + filename }));

  FieldRequest frq0;
  frq0.modelName = model;
  frq0.refTime = "2013-02-27T00:00:00";
  const miutil::miTime time0(frq0.refTime);

  {
    const FieldRequest frq_tdk = frqForParam(frq0, tdk);
    const FieldRequest frq_tk = frqForParam(frq0, tk); // not computed

    const auto times_tdk = fmanager->getFieldTime({frq_tdk}, true);
    ASSERT_EQ(67, times_tdk.size());
    EXPECT_EQ(time0, *times_tdk.begin());

    const auto times_tk = fmanager->getFieldTime({frq_tk}, true);
    ASSERT_EQ(times_tdk, times_tk);
    EXPECT_EQ(time0, *times_tk.begin());
  }
  {
    const FieldRequest frq_dpt = frqForParam(frq0, "dew_point_temperature"); // not defined
    ASSERT_TRUE(fmanager->getFieldTime({frq_dpt}, true).empty());
  }
}

TEST(FieldManager, FieldComputeUnits)
{
  if (!ditest::hasTestExtra())
    return;

  std::unique_ptr<FieldManager> fmanager(new FieldManager());
  const std::vector<std::string> field_compute_setup{
      "dew_point_temperature_pl=tdc.plevel_tk_q(air_temperature_pl:unit=kelvin,specific_humidity_pl:units=kg/kg)",

      "total.totals.1=add(air_temperature_pl:unit=celsius,dew_point_temperature_pl)",
      "total.totals.2=multiply(air_temperature_pl:unit=celsius,2.0)",
      "total.totals=subtract(total.totals.1:level=850,total.totals.2:level=500)",
  };
  std::vector<std::string> field_compute_errors;
  FieldFunctions::parseComputeSetup(field_compute_setup, field_compute_errors);
  ASSERT_TRUE(field_compute_errors.empty());

  const std::string inputfile = ditest::pathTestExtra("ecmwf_2020050500.nc");
  const std::string model = "fctest";
  ASSERT_TRUE(fmanager->addModels({"filegroup=x", "model=" + model + " t=netcdf f=" + inputfile}));

  FieldRequest frq;
  frq.modelName = model;
  frq.refTime = "2020-05-05T00:00:00";
  frq.ptime = miutil::miTime("2020-05-05 06:00:00");

  FieldRequest frq_td = frq;
  frq_td.plevel = "1000"; // no "hPa" suffix becaus there is no vertical axis config
  frq_td.unit = "celsius";
  frq_td.paramName = "dew_point_temperature_pl";
  Field_p field_td = fmanager->makeField(frq_td);
  ASSERT_TRUE(field_td != 0);
  EXPECT_EQ(miutil::ALL_DEFINED, field_td->defined());

  FieldRequest frq_toto1 = frq;
  frq_toto1.paramName = "total.totals.1";
  frq_toto1.plevel = "850";
  frq_toto1.unit = "degC";
  Field_p field_toto1 = fmanager->makeField(frq_toto1);
  ASSERT_TRUE(field_toto1 != 0);
  EXPECT_EQ(miutil::ALL_DEFINED, field_toto1->defined());

  FieldRequest frq_toto2 = frq;
  frq_toto2.paramName = "total.totals.2";
  frq_toto2.plevel = "500";
  frq_toto2.unit = "celsius";
  Field_p field_toto2 = fmanager->makeField(frq_toto2);
  ASSERT_TRUE(field_toto2 != 0);
  EXPECT_EQ(miutil::ALL_DEFINED, field_toto2->defined());

  FieldRequest frq_toto = frq;
  frq_toto.paramName = "total.totals";
  frq_toto.unit = "";
  Field_p field_toto = fmanager->makeField(frq_toto);
  ASSERT_TRUE(field_toto != 0);
  EXPECT_EQ(miutil::ALL_DEFINED, field_toto->defined());

  Field_pv fv_toto1{field_toto1}, fv_toto2{field_toto2};
  ASSERT_TRUE(FieldManager::makeDifferenceFields(fv_toto1, fv_toto2) && fv_toto1.size() == 1);
  EXPECT_EQ(miutil::ALL_DEFINED, fv_toto1.front()->defined());
  EXPECT_NEAR(field_toto->data[0], fv_toto1.front()->data[0], 0.03125);
}

TEST(FieldManager, FieldComputeUnitsSimpleFunctions)
{
  if (!ditest::hasTestExtra())
    return;

  std::unique_ptr<FieldManager> fmanager(new FieldManager());
  const std::string funcs[4]{"add", "subtract", "multiply", "divide"};
  std::vector<std::string> field_compute_setup{
      "tc=temp_k2c(air_temperature_pl)",
      "tdk=tdk.plevel_tk_q(air_temperature_pl,specific_humidity_pl)",
      "t_plus_q=add(air_temperature_pl,specific_humidity_pl)",
      "tctc=multiply(air_temperature_pl:units=celsius,air_temperature_pl:units=celsius)",
  };
  for (int i = 0; i < 4; ++i) {
    field_compute_setup.push_back("f_" + funcs[i] + "=" + funcs[i] + "(tc,tdk)");
  }
  std::vector<std::string> field_compute_errors;
  FieldFunctions::parseComputeSetup(field_compute_setup, field_compute_errors);
  ASSERT_TRUE(field_compute_errors.empty());

  const std::string inputfile = ditest::pathTestExtra("ecmwf_2020050500.nc");
  const std::string model = "fctest";
  ASSERT_TRUE(fmanager->addModels({"filegroup=x", "model=" + model + " t=netcdf f=" + inputfile}));

  FieldRequest frq;
  frq.modelName = model;
  frq.refTime = "2020-05-05T00:00:00";
  frq.ptime = miutil::miTime("2020-05-05 06:00:00");
  frq.plevel = "1000"; // no "hPa" suffix becaus there is no vertical axis config

  const std::string degC = "celsius";
  const std::string units[4]{degC, degC, "", ""};

  Field_p field_tc = fmanager->makeField(frqForParam(frq, "tc"));
  ASSERT_TRUE(field_tc != 0);
  EXPECT_EQ(degC, field_tc->unit);

  Field_p field_tdk = fmanager->makeField(frqForParam(frq, "tdk"));
  ASSERT_TRUE(field_tdk != 0);
  EXPECT_EQ("K", field_tdk->unit);

  const float tc0 = field_tc->data[0], tdk0 = field_tdk->data[0];

  {
    const float values[4]{tc0 + tdk0, tc0 - tdk0, tc0 * tdk0, tc0 / tdk0};
    for (int i = 0; i < 4; ++i) {
      Field_p field_i = fmanager->makeField(frqForParam(frq, "f_" + funcs[i]));
      ASSERT_TRUE(field_i != 0);
      EXPECT_EQ(miutil::ALL_DEFINED, field_i->defined());
      EXPECT_TRUE(field_i->unit.empty()) << i << ' ' << funcs[i];
      EXPECT_NEAR(values[i], field_i->data[0], epsilon) << i << ' ' << funcs[i];
    }
  }
  {
    Field_p field_t_plus_q = fmanager->makeField(frqForParam(frq, "t_plus_q"));
    ASSERT_TRUE(field_t_plus_q != 0);
    EXPECT_TRUE(field_t_plus_q->unit.empty());
  }
  {
    Field_p field_tctc = fmanager->makeField(frqForParam(frq, "tctc"));
    ASSERT_TRUE(field_tctc != 0);
    EXPECT_TRUE(field_tctc->unit.empty());
    EXPECT_NEAR(tc0*tc0, field_tctc->data[0], epsilon);
  }
}

TEST(FieldManager, FieldComputeUnitsVarargsF)
{
  if (!ditest::hasTestExtra())
    return;

  std::unique_ptr<FieldManager> fmanager(new FieldManager());
  const std::vector<std::string> field_compute_setup {
      "tc=temp_k2c(air_temperature_pl)",
      "tdk=tdk.plevel_tk_q(air_temperature_pl,specific_humidity_pl)",
      "tsum=sum(tc,air_temperature_pl,tdk)",
      "tmean=mean_value(tc,air_temperature_pl)",
      "tmin=min_value(tdk,air_temperature_pl)",
  };
  std::vector<std::string> field_compute_errors;
  FieldFunctions::parseComputeSetup(field_compute_setup, field_compute_errors);
  ASSERT_TRUE(field_compute_errors.empty());

  const std::string inputfile = ditest::pathTestExtra("ecmwf_2020050500.nc");
  const std::string model = "fctest";
  ASSERT_TRUE(fmanager->addModels({"filegroup=x", "model=" + model + " t=netcdf f=" + inputfile}));

  FieldRequest frq;
  frq.modelName = model;
  frq.refTime = "2020-05-05T00:00:00";
  frq.ptime = miutil::miTime("2020-05-05 06:00:00");
  frq.plevel = "1000"; // no "hPa" suffix becaus there is no vertical axis config

  const std::string degC = "celsius";

  Field_p field_tc = fmanager->makeField(frqForParam(frq, "tc"));
  ASSERT_TRUE(field_tc != 0);
  EXPECT_EQ(degC, field_tc->unit);

  Field_p field_tdk = fmanager->makeField(frqForParam(frq, "tdk"));
  ASSERT_TRUE(field_tdk != 0);
  EXPECT_EQ("K", field_tdk->unit);

  const float tc0 = field_tc->data[0], tk0 = tc0 + miutil::constants::t0, tdk0 = field_tdk->data[0], tdc0 = tdk0 - miutil::constants::t0;

  {
    Field_p field_sum = fmanager->makeField(frqForParam(frq, "tsum"));
    ASSERT_TRUE(field_sum != 0);
    EXPECT_EQ(degC, field_sum->unit);
    EXPECT_NEAR(2 * tc0 + tdc0, field_sum->data[0], epsilon);
  }
  {
    Field_p field_mean = fmanager->makeField(frqForParam(frq, "tmean"));
    ASSERT_TRUE(field_mean != 0);
    EXPECT_EQ(degC, field_mean->unit);
    EXPECT_NEAR(tc0, field_mean->data[0], epsilon);
  }
  {
    Field_p field_mean = fmanager->makeField(frqForParam(frq, "tmin"));
    ASSERT_TRUE(field_mean != 0);
    EXPECT_EQ("K", field_mean->unit);
    EXPECT_NEAR(std::min(tdk0, tk0), field_mean->data[0], epsilon);
  }
}

TEST(FieldManager, FieldComputeUnitsFC)
{
  if (!ditest::hasTestExtra())
    return;

  std::unique_ptr<FieldManager> fmanager(new FieldManager());
  const std::string funcs[4]{"add", "subtract", "multiply", "divide"};
  const std::vector<std::string> field_compute_setup {
      "tc=temp_k2c(air_temperature_pl)",
      "tc2a=add(tc,2.0)",
      "tc2b=add(2.0,tc)",
      "tcm=multiply(0.5,tc)",
      "tcd=divide(tc,2.0)",
      };
  std::vector<std::string> field_compute_errors;
  FieldFunctions::parseComputeSetup(field_compute_setup, field_compute_errors);
  ASSERT_TRUE(field_compute_errors.empty());

  const std::string inputfile = ditest::pathTestExtra("ecmwf_2020050500.nc");
  const std::string model = "fctest";
  ASSERT_TRUE(fmanager->addModels({"filegroup=x", "model=" + model + " t=netcdf f=" + inputfile}));

  FieldRequest frq;
  frq.modelName = model;
  frq.refTime = "2020-05-05T00:00:00";
  frq.ptime = miutil::miTime("2020-05-05 06:00:00");
  frq.plevel = "1000"; // no "hPa" suffix becaus there is no vertical axis config

  const std::string degC = "celsius";

  Field_p field_tc = fmanager->makeField(frqForParam(frq, "tc"));
  ASSERT_TRUE(field_tc != 0);
  EXPECT_EQ(degC, field_tc->unit);

  const float tc0 = field_tc->data[0];

  {
    Field_p f = fmanager->makeField(frqForParam(frq, "tc2a"));
    ASSERT_TRUE(f != 0);
    EXPECT_TRUE(f->unit.empty()) << "tc2a";
    EXPECT_NEAR(2 + tc0, f->data[0], epsilon);
  }
  {
    Field_p f = fmanager->makeField(frqForParam(frq, "tc2b"));
    ASSERT_TRUE(f != 0);
    EXPECT_TRUE(f->unit.empty()) << "tc2b";
    EXPECT_NEAR(2 + tc0, f->data[0], epsilon);
  }
  {
    Field_p f = fmanager->makeField(frqForParam(frq, "tcm"));
    ASSERT_TRUE(f != 0);
    EXPECT_TRUE(f->unit.empty()) << "tcm";
    EXPECT_NEAR(tc0 / 2, f->data[0], epsilon);
  }
  {
    Field_p f = fmanager->makeField(frqForParam(frq, "tcd"));
    ASSERT_TRUE(f != 0);
    EXPECT_TRUE(f->unit.empty()) << "tcd";
    EXPECT_NEAR(tc0 / 2, f->data[0], epsilon);
  }
}
