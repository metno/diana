/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2020 met.no

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

#include <diProjection.h>

#include "testinghelpers.h"

TEST(Projection, Convert1Point)
{
  const Projection p_geo = Projection::geographic();
  const Projection p_utm32("+proj=utm +zone=32 +ellps=WGS84 +datum=WGS84 +units=m +no_defs");

  diutil::PointF fxy(412000, 6123000);
  float fx = fxy.x(), fy = fxy.y();

  diutil::PointD dxy(fx, fy);
  double dx = dxy.x(), dy = dxy.y();

  EXPECT_TRUE(p_geo.convertPoints(p_utm32, 1, &fx, &fy));
  EXPECT_TRUE(p_geo.convertPoints(p_utm32, 1, &fxy));

  EXPECT_EQ(fxy.x(), fx);
  EXPECT_EQ(fxy.y(), fy);

  EXPECT_TRUE(p_geo.convertPoints(p_utm32, 1, &dx, &dy));
  EXPECT_TRUE(p_geo.convertPoints(p_utm32, 1, &dxy));

  EXPECT_EQ(dxy.x(), dx);
  EXPECT_EQ(dxy.y(), dy);

  EXPECT_NEAR(fxy.x(), dxy.x(), 1e-6);
  EXPECT_NEAR(fxy.y(), dxy.y(), 1e-6);
}

TEST(Projection, Empty)
{
  ditest::clearMemoryLog();
  Projection p("");
  EXPECT_EQ(0, ditest::getMemoryLogMessages().size());
}

TEST(Projection, BadProj4)
{
  ditest::clearMemoryLog();
  Projection p("+proj=fishy +x_0=123");
  const auto& msgs = ditest::getMemoryLogMessages();
  EXPECT_EQ(1, msgs.size());
  EXPECT_EQ(milogger::WARN, msgs[0].severity);
  EXPECT_EQ("diField.Projection", msgs[0].tag);
}
