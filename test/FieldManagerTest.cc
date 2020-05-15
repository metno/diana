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

#include <gtest/gtest.h>

#include <fstream>
#include <string>

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
