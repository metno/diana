/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2020 met.no

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

#include <diField/diField.h>

#include <gtest/gtest.h>

TEST(Area, EqOperator)
{
  const Area a1(Projection("+proj=ob_tran +o_proj=longlat +lon_0=0 +o_lat_p=90 +R=6.371e+06 +no_defs +x_0=3.13723 +y_0=1.5708"),
                Rectangle(0, 0, 6.27913, 3.14166));
  ASSERT_TRUE(a1 == a1);
  ASSERT_FALSE(a1 != a1);

  const Area a2(Projection("+proj=utm +zone=33 +ellps=WGS84 +datum=WGS84 +units=m +no_defs"), Rectangle(0, 0, 10000, 110000));
  ASSERT_TRUE(a1 != a2);
  ASSERT_FALSE(a1 == a2);
}

TEST(GridArea, EqOperator)
{
  const GridArea ga1(Area(Projection("+proj=longlat +a=6367470 +e=0 +no_defs"), Rectangle(-1.0472, 0.5236, 1.5708, 1.5708)), 751, 301, 0.00349069, 0.00349069);
  ASSERT_TRUE(ga1 == ga1);
  ASSERT_FALSE(ga1 != ga1);

  // same Area as ga1, different grid size
  const GridArea ga2(ga1, 1501, 601, 0.00174534, 0.00174534);
  ASSERT_TRUE(ga1 != ga2);
  ASSERT_FALSE(ga1 == ga2);

// same P, nx, ny and resolution as ga1, but shifted rect
  const GridArea ga3(Area(Projection("+proj=longlat +a=6367470 +e=0 +no_defs"), Rectangle(0.0472, -0.4764, 0.5708, 0.5708)), 751, 301, 0.00349069, 0.00349069);
  ASSERT_FALSE(ga1 == ga3);
  ASSERT_TRUE(ga1 != ga3);
}
