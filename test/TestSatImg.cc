/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2016-2018 met.no

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

#include <miRaster/satimg.h>

#include <gtest/gtest.h>


TEST(TestSatImg, JulianDay)
{
  EXPECT_EQ(31 + 7, satimg::JulianDay(2000, 2, 7));
  EXPECT_EQ(31 + 7, satimg::JulianDay(2001, 2, 7));
  EXPECT_EQ(31 + 7, satimg::JulianDay(2004, 2, 7));

  EXPECT_EQ(31 + 29 + 7, satimg::JulianDay(2000, 3, 7));
  EXPECT_EQ(31 + 28 + 7, satimg::JulianDay(2001, 3, 7));
  EXPECT_EQ(31 + 29 + 7, satimg::JulianDay(2004, 3, 7));
}

TEST(TestSatImg, MiTiffProj4FalseNE)
{
  const std::string proj4_nooffset = "+proj=stere +lon_0=0 +lat_0=90 +lat_ts=60 +ellps=WGS84 +towgs84=0,0,0 +units=km "; // space at end
  std::string proj4 = proj4_nooffset + "+x_0=1936001.860000 +y_0=4564248.000000";
  double value = 0;
  EXPECT_TRUE(satimg::proj4_value(proj4, "+x_0=", value, true));
  EXPECT_NEAR(1936001.86, value, 1);
  EXPECT_TRUE(satimg::proj4_value(proj4, "+y_0=", value, true));
  EXPECT_NEAR(4564248.0, value, 1);
  EXPECT_EQ(proj4_nooffset, proj4);
}
