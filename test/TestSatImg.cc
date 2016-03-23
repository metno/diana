/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2016 met.no

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

#include <miRaster/satimg.h>

#include <gtest/gtest.h>

using namespace std;

TEST(TestSatImg, JulianDay)
{
  EXPECT_EQ(31 + 7, satimg::JulianDay(2000, 2, 7));
  EXPECT_EQ(31 + 7, satimg::JulianDay(2001, 2, 7));
  EXPECT_EQ(31 + 7, satimg::JulianDay(2004, 2, 7));

  EXPECT_EQ(31 + 29 + 7, satimg::JulianDay(2000, 3, 7));
  EXPECT_EQ(31 + 28 + 7, satimg::JulianDay(2001, 3, 7));
  EXPECT_EQ(31 + 29 + 7, satimg::JulianDay(2004, 3, 7));
}
