/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2020-2022 met.no

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

#include <util/diUnitsConverter.h>

#include <gtest/gtest.h>

TEST(UnitsConverter, Undef)
{
  const float pressure_hPa[] = {-1, 1001, 1013, -1, -1, 1012, -1};
  const size_t N = sizeof(pressure_hPa) / sizeof(pressure_hPa[0]);
  float pressure_Pa[N];
  EXPECT_TRUE(diutil::unitConversion("hPa", "Pa", N, -1, pressure_hPa, pressure_Pa));
  for (auto i = 0; i < N; ++i) {
    if (pressure_hPa[i] == -1) {
      EXPECT_EQ(pressure_Pa[i], -1) << "undef " << i;
    } else {
      EXPECT_EQ(pressure_Pa[i], pressure_hPa[i] * 100) << "def " << i;
    }
  }
}

TEST(UnitsConverter, Identical)
{
  auto uconv = diutil::unitConverter("Kelvin", "K");
  ASSERT_TRUE(uconv);

  EXPECT_EQ(diutil::UNITS_IDENTICAL, uconv->convertibility());
}

TEST(UnitsConverter, Linear)
{
  auto uconv = diutil::unitConverter("degreeC", "K");
  ASSERT_TRUE(uconv);

  EXPECT_EQ(diutil::UNITS_LINEAR, uconv->convertibility());
}
