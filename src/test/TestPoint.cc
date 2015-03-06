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

#include <diPoint.h>
#include <gtest/gtest.h>

TEST(TestPoint, AspectRatio)
{
  const Rectangle r(0, 0, 100, 100);

  const Rectangle r1e = diutil::fixedAspectRatio(r, 1, true);
  EXPECT_EQ(r, r1e);

  const Rectangle r1s = diutil::fixedAspectRatio(r, 1, false);
  EXPECT_EQ(r, r1s);

  const Rectangle r10e = diutil::fixedAspectRatio(r, 10, true);
  EXPECT_FLOAT_EQ(10, r10e.width() / r10e.height());
  EXPECT_EQ(Rectangle(-450, 0, 550, 100), r10e);

  const Rectangle r10s = diutil::fixedAspectRatio(r, 10, false);
  EXPECT_FLOAT_EQ(10, r10s.width() / r10s.height());
  EXPECT_EQ(Rectangle(0, 45, 100, 55), r10s);

  const Rectangle r01e = diutil::fixedAspectRatio(r, 0.1, true);
  EXPECT_FLOAT_EQ(0.1, r01e.width() / r01e.height());
  EXPECT_EQ(Rectangle(0, -450, 100, 550), r01e);

  const Rectangle r01s = diutil::fixedAspectRatio(r, 0.1, false);
  EXPECT_FLOAT_EQ(0.1, r01s.width() / r01s.height());
  EXPECT_EQ(Rectangle(45, 0, 55, 100), r01s);
}
