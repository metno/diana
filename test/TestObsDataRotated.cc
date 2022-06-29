/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2022 met.no

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

#include "diObsDataRotated.h"
#include "diObsDataVector.h"

#include <gtest/gtest.h>

TEST(TestObsDataRotated, Basic)
{
  static const std::string XWIND = "x_wind", YWIND = "y_wind";

  auto obsv = std::make_shared<ObsDataVector>();
  obsv->basic(0).xpos = 10;
  obsv->basic(0).ypos = 60;
  obsv->put_float(0, obsv->add_key(XWIND), 4);
  obsv->put_float(0, obsv->add_key(YWIND), 3);

  auto obsr = std::make_shared<ObsDataRotated>(obsv);

  {
    auto fp = obsr->get_float(0, XWIND);
    ASSERT_TRUE(fp);
    EXPECT_FLOAT_EQ(4, *fp);
  }

  obsr->put_rotated_float(0, XWIND, 5);
  obsr->put_rotated_float(0, YWIND, 0);

  {
    auto fp = obsr->get_float(0, XWIND);
    ASSERT_TRUE(fp);
    EXPECT_FLOAT_EQ(5, *fp);
  }
  {
    auto fp = obsr->get_unrotated_float(0, XWIND);
    ASSERT_TRUE(fp);
    EXPECT_FLOAT_EQ(4, *fp);
  }

  obsr->put_float(0, XWIND, -4);

  {
    auto fp = obsr->get_float(0, XWIND);
    ASSERT_TRUE(fp);
    EXPECT_FLOAT_EQ(5, *fp);
  }
  {
    auto fp = obsr->get_unrotated_float(0, XWIND);
    ASSERT_TRUE(fp);
    EXPECT_FLOAT_EQ(-4, *fp);
  }
  {
    auto fp = obsr->get_unrotated_float(0, YWIND);
    ASSERT_TRUE(fp);
    EXPECT_FLOAT_EQ(3, *fp);
  }

  {
    const auto od = obsr->at(0);
    auto fp = od.get_float(XWIND);
    ASSERT_TRUE(fp);
    EXPECT_FLOAT_EQ(5, *fp);
  }
}
