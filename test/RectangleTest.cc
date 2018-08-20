/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2018 met.no

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
#include <diRectangle.h>

#include <gtest/gtest.h>

TEST(RectangleTest, Intersection)
{
  const Rectangle square2(0, 0, 2, 2);

  EXPECT_TRUE(square2.intersects(Rectangle(1, 1, 3, 3)));
  EXPECT_TRUE(square2.intersects(Rectangle(0, 1, 2, 3)));
  EXPECT_TRUE(square2.intersects(Rectangle(0, -1, 2, 3)));
  EXPECT_TRUE(square2.intersects(Rectangle(1, 0, 3, 2)));

  EXPECT_TRUE(square2.intersects(Rectangle(2, 0, 4, 2)));
  EXPECT_TRUE(square2.intersects(Rectangle(0, -2, 2, 0)));

  EXPECT_FALSE(square2.intersects(Rectangle(1, 3, 3, 5)));
}

#if 0
TEST(RectangleTest, Extension)
{
  Rectangle ex(0, 0, 2, 2);
  ex.setExtension(1);

  EXPECT_TRUE(ex.isnear(-0.99, -0.99));
  EXPECT_TRUE(ex.isnear(2.99, 2.99));

  EXPECT_FALSE(ex.isnear(-1.01, -1.01));
  EXPECT_FALSE(ex.isnear(3.01, 3.01));
}
#endif

TEST(RectangleTest, Extended)
{
  const Rectangle ex = diutil::extendedRectangle(Rectangle(0, 0, 2, 2), 1);

  EXPECT_TRUE(ex.isinside(-0.99, -0.99));
  EXPECT_TRUE(ex.isinside(2.99, 2.99));

  EXPECT_FALSE(ex.isinside(-1.01, -1.01));
  EXPECT_FALSE(ex.isinside(3.01, 3.01));
}

TEST(RectangleTest, Equal)
{
  const Rectangle r1(0, 0, 2, 2);

  const Rectangle r_different = diutil::extendedRectangle(r1, Rectangle::EQUAL_TOLERANCE * 2);
  EXPECT_FALSE(r1 == r_different);
  EXPECT_TRUE(r1 != r_different);

  const Rectangle r_equal = diutil::extendedRectangle(r1, Rectangle::EQUAL_TOLERANCE / 2);
  EXPECT_TRUE(r1 == r_equal);
  EXPECT_FALSE(r1 != r_equal);
}

TEST(RectangleTest, FromString)
{
  Rectangle r;
  r.setRectangle("0:2:1:3");

  EXPECT_EQ(r, Rectangle(0, 1, 2, 3));
}

TEST(RectangleTest, ToString)
{
  const Rectangle r(0, 1, 2, 3);
  EXPECT_EQ("0:2:1:3", r.toString());
}
