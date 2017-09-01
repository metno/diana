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
  connect(qm, &VcrossQuickmenues::quickmenuUpdate, this, &QuickmenuSlots::onQuickmenuesUpdate);
}

void QuickmenuSlots::reset()
{
  titles.clear();
  qmenues.clear();
}

void QuickmenuSlots::onQuickmenuesUpdate(const std::string& t, const PlotCommand_cpv& qm)
{
  titles.push_back(t);
  qmenues.push_back(qm);
}

} // namespace test
} // namespace vcross

namespace {
typedef std::vector<std::string> string_v;
static const char AROME_FILE[] = "arome_vprof.nc";
} // namespace

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
  qm.parse(makeCommands(qmlines, true));

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
  manager->updateField(0, miutil::KeyValue_v(1, miutil::KeyValue("colour", "red")));
  EXPECT_EQ(1, qmslots.titles.size());
  EXPECT_EQ(1, qmslots.qmenues.size());
  qmslots.reset();

  // add field => expect qm update
  manager->addField(vcross::QtManager::PlotSpec("MODEL1", vcross::QtManager::vctime_t("2014-10-20 00:00:00"), "Temp(K)"),
      miutil::KeyValue_v(1, miutil::KeyValue("colour", "black")), -1);
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

TEST(TestVcrossQuickmenues, ChangeTime)
{
  string_v sources;
  sources.push_back("m=AROME f=" TEST_SRCDIR "/arome_smhi.nc t=netcdf"
      // " predefined_cs=" TEST_SRCDIR "/approach_ENHF.kml" // FIXME reading kml files requires a PlotModule!
    );

  string_v computations;

  string_v plots;
  plots.push_back("name=temperature plot=CONTOUR(air_temperature_ml) colour=red  line.interval=1.");

  vcross::QtManager_p manager(new vcross::QtManager);
  manager->parseSetup(sources, computations, plots);

  vcross::VcrossQuickmenues qm(manager);

  string_v qmlines;
  qmlines.push_back("VCROSS model=AROME field=temperature colour=blue");
  qmlines.push_back("CROSSECTION=ENVA");
  qmlines.push_back("CROSSECTION_LONLAT_DEG=10.7,63.4 10.9,63.5 10.9,63.4 11.1,63.5");
  qm.parse(makeCommands(qmlines, true));
  ASSERT_EQ("ENVA", manager->getCrossectionLabel().toStdString());

  ASSERT_EQ(4, manager->getTimeCount());

  manager->setTimeIndex(0);
  ASSERT_EQ("ENVA", manager->getCrossectionLabel().toStdString());

  manager->setTimeIndex(1);
  ASSERT_EQ("ENVA", manager->getCrossectionLabel().toStdString());
}

TEST(TestVcrossQuickmenues, MakeCommandsVcross)
{
  std::vector<std::string> lines;
  lines.push_back("VCROSS");
  lines.push_back("text=on textColour=black PositionNames=on positionNamesColour=red");
  lines.push_back("LevelNumbers=on");
  lines.push_back("VCROSS model=EC_bfs refhour=0 field=Temp(K) colour=green linewidth=1 line.interval=1");
  lines.push_back("CROSSECTION=(58N;0E)-(58N;20E)");

  const PlotCommand_cpv pcs = makeCommands(lines);
  EXPECT_EQ(5, pcs.size());
  for (PlotCommand_cp pc : pcs) {
    EXPECT_EQ("VCROSS", pc->commandKey());
  }
}
