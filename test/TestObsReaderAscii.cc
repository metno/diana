/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2021 met.no

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

#include <diObsAscii.h>
#include <diObsReaderAscii.h>

#include <gtest/gtest.h>

namespace {
const static std::string LYN_LX(TEST_SRCDIR "/testdata/lyndata/");
const miutil::miTime t_11_00("2017-12-10 19:00");
const miutil::miTime t_19_10("2017-12-10 19:10");
}

TEST(TestObsReaderAscii, Configure)
{
  ObsReader_p ascii = std::make_shared<ObsReaderAscii>();
  EXPECT_TRUE(ascii->configure("ascii", LYN_LX + "lx-opendata.[yyyymmddTHHMMSS]Z"));
  EXPECT_TRUE(ascii->configure("headerfile", LYN_LX + "lx.header"));

  const std::set<miutil::miTime> times = ascii->getTimes(false, true);
  EXPECT_EQ(24, times.size());
  EXPECT_TRUE(times.count(t_11_00));
  EXPECT_TRUE(times.count(t_19_10));

  EXPECT_FALSE(ascii->checkForUpdates(false));
}

TEST(TestObsReaderAscii, GetData)
{
  ObsReader_p ascii = std::make_shared<ObsReaderAscii>();
  EXPECT_TRUE(ascii->configure("ascii", LYN_LX + "lx-opendata.[yyyymmddTHHMMSS]Z"));
  EXPECT_TRUE(ascii->configure("headerfile", LYN_LX + "lx.header"));
  EXPECT_TRUE(ascii->configure("timerange", "-5,0"));

  ObsDataRequest_p req = std::make_shared<ObsDataRequest>();
  req->obstime = t_19_10;
  req->timeDiff = 10;
  ObsDataResult_p res = std::make_shared<ObsDataResult>();
  ascii->getData(req, res);
  EXPECT_EQ(7, res->data().size());
}

TEST(TestObsReaderAscii, GetParameters)
{
  ObsReader_p ascii = std::make_shared<ObsReaderAscii>();
  EXPECT_TRUE(ascii->configure("ascii", LYN_LX + "lx-opendata.[yyyymmddTHHMMSS]Z"));
  EXPECT_TRUE(ascii->configure("headerfile", LYN_LX + "lx.header"));

  const std::vector<std::string> ex_params = {"Ver", "Year", "Month", "Day",   "Hour", "Min",  "Sec", "Ns",  "Lat", "Lon", "Pk",    "Fd", "No_sens",
                                              "Df",  "Ea",   "Major", "Minor", "Chi",  "Rise", "Pz",  "Mrr", "Ci",  "Ai",  "Sig_i", "Ti"};
  const std::vector<ObsDialogInfo::Par> ac_params = ascii->getParameters();
  ASSERT_EQ(ex_params.size(), ac_params.size());
  for (size_t i = 0; i < ex_params.size(); ++i)
    EXPECT_EQ(ex_params[i], ac_params[i].name);
}

TEST(TestObsAscii, BracketContentsOk)
{
  std::vector<std::string> inout{"[NAME UALF_Lyn]", "[COLUMNS", "Ver:f:\"UALF versjons nummer\"", "Year:year:\"År\"", "]"};
  EXPECT_TRUE(ObsAscii::bracketContents(inout));
  const std::vector<std::string> exp{"NAME UALF_Lyn", "COLUMNS Ver:f:\"UALF versjons nummer\" Year:year:\"År\" "};
  EXPECT_EQ(inout, exp);
}

TEST(TestObsAscii, BracketContentsBad)
{
  std::vector<std::string> inout{"[NAME UALF_Lyn]", "[COLUMNS"};
  EXPECT_FALSE(ObsAscii::bracketContents(inout));
}
