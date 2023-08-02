/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2022 met.no

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

#include <VcrossData.h>

#include <gtest/gtest.h>

typedef std::vector<size_t> size_v;
typedef std::vector<std::string> string_v;

using diutil::Values;
using namespace vcross;

TEST(ShapeTest, BasicShape)
{
  const size_v lengths = {2, 3};
  const string_v names = {"x", Values::GEO_Z};
  const Values::Shape shape(names, lengths);

  EXPECT_EQ(0, shape.position("x"));
  EXPECT_EQ(1, shape.position(Values::GEO_Z));

  EXPECT_EQ(2, shape.length("x"));
  EXPECT_EQ(3, shape.length(1));

  EXPECT_EQ(2*3, shape.volume());
  EXPECT_EQ(2,   shape.rank());
}

TEST(ShapeTest, BasicIndex)
{
  const Values::Shape shape({"x", Values::GEO_Z}, {2, 3});

  Values::ShapeIndex idx(shape);
  EXPECT_EQ(0, idx.index());

  idx.set("x", 1);
  EXPECT_EQ(1, idx.index());

  idx.set(1, 2);
  EXPECT_EQ(5, idx.index());
}

TEST(ShapeTest, BasicSlice)
{
  const Values::Shape shape({"x", Values::GEO_Z}, {2, 3});

  Values::ShapeSlice slice(shape);
  slice.cut(1, 0, 2);
  EXPECT_EQ(2,   slice.length(Values::GEO_Z));
  EXPECT_EQ(2*2, slice.volume());
}
