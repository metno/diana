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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diQuickMenues.h>

#include <gtest/gtest.h>

using namespace std;

TEST(TestQuickMenues, UpdateMapArea)
{
  {
    std::vector<std::string> lines;
    lines.push_back("WEBMAP hello");
    lines.push_back("MAP here=there area=Norge backcolour=white contour=on");
    lines.push_back("WEBMAP bye");

    std::vector<std::string> expect;
    expect.push_back("WEBMAP hello");
    expect.push_back("AREA name=Norge");
    expect.push_back("MAP here=there backcolour=white contour=on");
    expect.push_back("WEBMAP bye");

    EXPECT_TRUE(updateCommandSyntax(lines));
    EXPECT_EQ(expect, lines);
  }

  {
    std::vector<std::string> lines;
    lines.push_back("AREA name=Norge");
    lines.push_back("MAP here=there backcolour=white contour=on");
    std::vector<std::string> expect = lines;

    EXPECT_FALSE(updateCommandSyntax(lines));
    EXPECT_EQ(expect, lines);
  }
}

TEST(TestQuickMenues, UpdateField)
{
  {
    std::vector<std::string> lines;
    lines.push_back("FIELD @NORLAMEPS.@ECUTC Lameps_Probability_Wind>25m/s colour=green3");

    std::vector<std::string> expect;
    expect.push_back("FIELD model=@NORLAMEPS.@ECUTC plot=Lameps_Probability_Wind>25m/s colour=green3");

    EXPECT_TRUE(updateCommandSyntax(lines));
    EXPECT_EQ(expect, lines);
  }

  {
    std::vector<std::string> lines;
    lines.push_back("FIELD  model=AROME-MetCoOp.2.5KM refhour=6 plot=T.2M colour=red plottype=fill_cell ");
    std::vector<std::string> expect = lines;

    EXPECT_FALSE(updateCommandSyntax(lines));
    EXPECT_EQ(expect, lines);
  }

  {
    std::vector<std::string> lines;
    lines.push_back("FIELD ( AROME-MetCoOp.2.5KM T.2M - HIRLAM.8KM.00 T.2M ) level=500hPa colour=red");
    std::vector<std::string> expect;
    expect.push_back("FIELD ( model=AROME-MetCoOp.2.5KM plot=T.2M"
        " - model=HIRLAM.8KM.00 plot=T.2M ) vcoord=pressure vlevel=500hPa colour=red");

    EXPECT_TRUE(updateCommandSyntax(lines));
    EXPECT_EQ(expect, lines);
  }

  {
    std::vector<std::string> lines;
    lines.push_back("FIELD ( AROME T.2M level=500hPa - HIRLAM T.2M level=300hPa ) colour=red");
    std::vector<std::string> expect;
    expect.push_back("FIELD ( model=AROME plot=T.2M vcoord=pressure vlevel=500hPa"
        " - model=HIRLAM plot=T.2M vcoord=pressure vlevel=300hPa ) colour=red");

    EXPECT_TRUE(updateCommandSyntax(lines));
    EXPECT_EQ(expect, lines);
  }
}
