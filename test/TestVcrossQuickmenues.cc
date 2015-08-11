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

#include "TestVcrossQuickmenues.h"
#include <vcross_v2/VcrossQtManager.h>
#include <vcross_v2/VcrossQuickmenues.h>

#include <gtest/gtest.h>

#define MILOGGER_CATEGORY "diana.test.VcrossQuickmenues"
#include "miLogger/miLogging.h"

namespace vcross {
namespace test {

QuickmenuSlots::QuickmenuSlots(VcrossQuickmenues* qm)
{
  connect(qm, SIGNAL(quickmenuUpdate(const std::string&, const std::vector<std::string>&)),
      this, SLOT(onQuickmenuesUpdate(const std::string&, const std::vector<std::string>&)));
}

void QuickmenuSlots::reset()
{
  titles.clear();
  qmenues.clear();
}

void QuickmenuSlots::onQuickmenuesUpdate(const std::string& t, const std::vector<std::string>& qm)
{
  titles.push_back(t);
  qmenues.push_back(qm);
}

} // namespace test
} // namespace vcross

namespace {
typedef std::vector<std::string> string_v;
static const char AROME_FILE[] = "arome_vprof.nc";
}

TEST(TestVcrossQuickmenues, Script)
{
  string_v sources;
  sources.push_back("m=MODEL1 f=" TEST_SRCDIR "/" + std::string(AROME_FILE) + " t=netcdf");

  string_v computations;
  computations.push_back("vc_surface_altitude  = convert_unit(altitude,m)");
  computations.push_back("vc_surface_altitude  = height_above_msl_from_surface_geopotential(surface_geopotential)");
  computations.push_back("tk = identity(air_temperature_ml)");
  computations.push_back("tk = tk_from_th(air_potential_temperature_ml)");
  computations.push_back("ff_normal     = normal(x_wind_ml, y_wind_ml)");
  computations.push_back("ff_tangential = tangential(x_wind_ml, y_wind_ml)");

  string_v plots;
  plots.push_back("name=Temp(K) plot=CONTOUR(tk)  colour=red  line.interval=1.");
  plots.push_back("name=Vind    plot=WIND(ff_tangential,ff_normal) colour=blue");

  vcross::QtManager_p manager(new vcross::QtManager);
  manager->parseSetup(sources, computations, plots);

  vcross::VcrossQuickmenues qm(manager);

  vcross::test::QuickmenuSlots qmslots(&qm);

  string_v qmlines;
  qmlines.push_back("VCROSS model=MODEL1 field=Vind colour=blue");
  qmlines.push_back("CROSSECTION=Nesbyen 6");
  qm.parse(qmlines);

  // no update here, we ran a script
  EXPECT_EQ(0, qmslots.titles.size());
  EXPECT_EQ(0, qmslots.qmenues.size());
  qmslots.reset();

  // select a different crossection => expect qm update
  manager->setCrossectionIndex(manager->findCrossectionIndex("Nesbyen 7"));
  EXPECT_EQ(1, qmslots.titles.size());
  EXPECT_EQ(1, qmslots.qmenues.size());
  qmslots.reset();

  // set a different style => expect qm update
  manager->updateField(0, "colour=red");
  EXPECT_EQ(1, qmslots.titles.size());
  EXPECT_EQ(1, qmslots.qmenues.size());
  qmslots.reset();

  // add field => expect qm update
  manager->addField(vcross::QtManager::PlotSpec("MODEL1", vcross::QtManager::vctime_t("2014-10-20 00:00:00"), "Temp(K)"),
      "colour=black", -1);
  EXPECT_EQ(1, qmslots.titles.size());
  EXPECT_EQ(1, qmslots.qmenues.size());
  qmslots.reset();

  // set visibility => expect no qm update (as we have two fields)
  manager->setFieldVisible(0, false);
  manager->setFieldVisible(0, true);
  EXPECT_EQ(0, qmslots.titles.size());
  EXPECT_EQ(0, qmslots.qmenues.size());
  qmslots.reset();

  // remove all => expect no qm update
  manager->removeAllFields();
  EXPECT_EQ(0, qmslots.titles.size());
  EXPECT_EQ(0, qmslots.qmenues.size());
  qmslots.reset();
}
