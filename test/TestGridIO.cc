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

#include <diField/FimexIO.h>

#include "testinghelpers.h"

namespace {
const std::string inputformat("netcdf"), inputreftime = "", inputconfig = "";
const std::string nosuchname("no-such-name");

void TestGetRefTimeFindZ(const std::string& inputfile, const std::string& zaxisname)
{
  std::unique_ptr<FimexIOsetup> setup(new FimexIOsetup);
  std::unique_ptr<FimexIO> io(new FimexIO("test", inputfile, inputreftime, inputformat, inputconfig, {}, true, setup.get()));
  io->makeInventory(io->getReferenceTime());

  const auto reftimes = io->getReferenceTimes();
  ASSERT_EQ(1, reftimes.size());

  EXPECT_FALSE(reftimes.begin()->empty());

  const std::string reftime = *reftimes.begin();
  EXPECT_FALSE(reftime.empty());
  EXPECT_EQ(zaxisname, io->getZaxis(reftime, zaxisname).getName());

  EXPECT_TRUE(io->getZaxis(reftime, nosuchname).getName().empty());
}
}

TEST(GridIO, GetRefTimeFindZArome)
{
  TestGetRefTimeFindZ(ditest::pathTest("arome.nc"), "pressure0");
}

TEST(GridIO, GetRefTimeFindZEC)
{
  if (!ditest::hasTestExtra())
    return;
  TestGetRefTimeFindZ(ditest::pathTestExtra("ecmwf_2020050500.nc"), "pressure0");
}

TEST(FimexIO, NoSuchTime)
{
  const std::string fileName(TEST_SRCDIR "/test_fimexio_rw.nc");

  const std::string refTime = "2013-02-27T00:00:00";
  const std::string paramName = "lwe_precipitation_rate";

  std::unique_ptr<FimexIOsetup> fios(new FimexIOsetup);
  std::unique_ptr<FimexIO> fio(new FimexIO("mdl", fileName, refTime, "", "", {}, true, fios.get()));
  EXPECT_TRUE(fio->makeInventory(refTime));

  const auto& rti = fio->getReftimeInventory(refTime);
  const auto it = std::find_if(rti.parameters.begin(), rti.parameters.end(), [&](const gridinventory::GridParameter& gp) { return gp.key.name == paramName; });
  ASSERT_NE(it, rti.parameters.end());

  {
    ditest::clearMemoryLog();

    const miutil::miTime ptime = miutil::miTime("2023-02-27 00:00:00");
    EXPECT_TRUE(fio->getData(refTime, *it, "", ptime, "") == nullptr); // no such time

    const auto& lm = ditest::getMemoryLogMessages();
    EXPECT_EQ(0, std::count_if(lm.begin(), lm.end(), [](milogger::memory::MemorySystem::Message m) { return m.severity == milogger::WARN; }));
  }

  {
    ditest::clearMemoryLog();

    const miutil::miTime ptime = miutil::miTime("2013-02-27 00:00:00");
    ASSERT_TRUE(fio->getData(refTime, *it, "", ptime, "") != nullptr); // time ok

    const auto& lm = ditest::getMemoryLogMessages();
    EXPECT_EQ(0, std::count_if(lm.begin(), lm.end(), [](milogger::memory::MemorySystem::Message m) { return m.severity == milogger::WARN; }));
  }
}
