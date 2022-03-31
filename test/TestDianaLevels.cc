/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018-2022 met.no

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

#include <diPlotOptions.h>
#include <diPolyContouring.h>

#include <boost/range/end.hpp>
#include <vector>
#include <cmath>

#include <gtest/gtest.h>

using namespace std;

TEST(TestDianaLevels, Log)
{
  const DianaLevelLog ll({1.0, 3.0});
  const int UNDEF = DianaLevels::UNDEF_LEVEL;

  EXPECT_EQ(-10, ll.level_for_value(0.9e-5));
  EXPECT_EQ( -5, ll.level_for_value(2.5e-3));
  EXPECT_EQ( -4, ll.level_for_value(3.1e-3));
  EXPECT_EQ(UNDEF, ll.level_for_value(0));

  EXPECT_FLOAT_EQ(1e-5, ll.value_for_level(-10));
  EXPECT_FLOAT_EQ(3e-3, ll.value_for_level( -5));
  EXPECT_FLOAT_EQ(1e-2, ll.value_for_level( -4));

  EXPECT_EQ(-71, ll.level_for_value(2.7e-36));
  EXPECT_FLOAT_EQ(3e-36, ll.value_for_level(-71));

  EXPECT_EQ(-72, ll.level_for_value(0.7e-36));
  EXPECT_FLOAT_EQ(1e-36, ll.value_for_level(-72));

  EXPECT_EQ(-85, ll.level_for_value(3.0e-43));
  EXPECT_EQ(-85, ll.level_for_value(2.9e-43));
  EXPECT_EQ(-85, ll.level_for_value(1.8e-43));
  EXPECT_EQ(-87, ll.level_for_value(2.5e-44));
  EXPECT_EQ(-85, ll.level_for_value(1.2e-43));
  EXPECT_EQ(-85, ll.level_for_value(1.3e-43));
  EXPECT_FLOAT_EQ(1e-42, ll.value_for_level(-84));
  EXPECT_FLOAT_EQ(3e-43, ll.value_for_level(-85));
  EXPECT_FLOAT_EQ(3e-44, ll.value_for_level(-87));
}

TEST(TestDianaLevels, List10)
{
  const DianaLevelList10 ll({1.0, 3.0}, 10);

  EXPECT_EQ(0, ll.level_for_value(0.1));
  EXPECT_EQ(1, ll.level_for_value(2));
  EXPECT_EQ(10, ll.level_for_value(1e11));

  EXPECT_FLOAT_EQ(1, ll.value_for_level(0));
  EXPECT_FLOAT_EQ(3, ll.value_for_level(1));
  EXPECT_FLOAT_EQ(10, ll.value_for_level(2));
  EXPECT_FLOAT_EQ(3e4, ll.value_for_level(9));
}

TEST(TestDianaLevels, List)
{
  const DianaLevelList ll({8, 10.8, 13.9, 17.2, 20.8, 24.5, 28.5, 32.7});

  EXPECT_EQ(0, ll.level_for_value(5));
  EXPECT_EQ(1, ll.level_for_value(9));
  EXPECT_EQ(2, ll.level_for_value(11));
}
