/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2021 met.no

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

#include <diPlotStatus.h>

#include <gtest/gtest.h>


TEST(TestPlotStatus, Init)
{
  const PlotStatus pcs1;
  EXPECT_EQ(0, pcs1.count());

  const PlotStatus pcs2(P_OK_EMPTY, 3);
  EXPECT_EQ(3, pcs2.count());
  EXPECT_EQ(0, pcs2.get(P_WAITING));
  EXPECT_EQ(3, pcs2.get(P_OK_EMPTY));
}

TEST(TestPlotStatus, Add)
{
  PlotStatus pcs;

  pcs.add(PlotStatus());
  EXPECT_EQ(0, pcs.count());

  pcs.add(P_WAITING);
  EXPECT_EQ(0, pcs.get(P_ERROR));
  EXPECT_EQ(1, pcs.get(P_WAITING));
  EXPECT_EQ(1, pcs.count());
}
