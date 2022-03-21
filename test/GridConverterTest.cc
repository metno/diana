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

#include <diGridConverter.h>
#include <diPoint.h>

#include <gtest/gtest.h>
#include <iomanip>

TEST(GridConverterTest, GetMapFields)
{
  const int nx = 3, ny = 3;
  const GridArea area(Area(Projection("+proj=utm +zone=33 +ellps=WGS84 +datum=WGS84 +units=m +no_defs"),
          Rectangle(0, 0, 10000, 110000)), nx, ny, 1000, 1000);
  GridConverter gc;
  MapFields_cp mf;
#if 1
  const float SCALE = 1;
  mf = gc.getMapFields(area);
#else
  const float SCALE = 0.5;
  float dxg = 0, dyg = 0;
  mf = gc.getMapFields(area, 3, 1, dxg, dyg);
#endif
  ASSERT_TRUE(mf != nullptr);
  ASSERT_TRUE(mf->xmapr);
  ASSERT_TRUE(mf->ymapr);
  ASSERT_TRUE(mf->coriolis);

  const float expect_x[nx*ny] = {
    0.0010038398,
    0.0010037329,
    0.0010037329,
    0.0010038398,
    0.0010037329,
    0.0010037329,
    0.0010037329,
    0.0010038398,
    0.0010038398
  };
  const float expect_y[nx*ny] = {
    0.00099709968,
    0.00099708722,
    0.000997075,
    0.00099709968,
    0.00099708722,
    0.000997075,
    0.00099709968,
    0.00099708722,
    0.000997075,
  };
  const float expect_c[nx*ny] = {
    0, 0, 0,
    2.2957812e-08, 2.2958098e-08, 2.2958378e-08,
    4.5915623e-08, 4.5916192e-08, 4.5916757e-08
  };
  for (int i=0; i<nx*ny; ++i) {
    EXPECT_NEAR(expect_x[i] * SCALE, mf->xmapr[i], 5e-7) << " i=" << i << " x";
    EXPECT_NEAR(expect_y[i] * SCALE, mf->ymapr[i], 5e-7) << " i=" << i << " y";
    EXPECT_NEAR(expect_c[i], mf->coriolis[i], 5e-7) << " i=" << i << " c";
  }
}

namespace {

diutil::Rect findGridLimits(const GridArea& area, const Rectangle& mapRect,
    bool center_on_gridpoint, const float* positionsX, const float* positionsY)
{
  diutil::Rect r;
  GridConverter::findGridLimits(area, mapRect, center_on_gridpoint,
      positionsX, positionsY, r.x1, r.x2, r.y1, r.y2);
  return r;
}

bool operator==(const diutil::Rect& a, const diutil::Rect& b)
{
  return a.x1 == b.x1 && a.x2 == b.x2 && a.y1 == b.y1 && a.y2 == b.y2;
}

const float RAD_TO_DEG = 180 / M_PI;
Rectangle rectangleR2D(float x1, float y1, float x2, float y2)
{
  return Rectangle(x1 * RAD_TO_DEG, y1 * RAD_TO_DEG, x2 * RAD_TO_DEG, y2 * RAD_TO_DEG);
}
} // namespace

#if 0 // fails due to differences between proj 4 and proj 8 for ob_tran
TEST(GridConverterTest, FindGridLimits)
{
  const GridArea fieldGridArea(Area(Projection("+proj=ob_tran +o_proj=longlat +lon_0=0 +o_lat_p=90 +R=6.371e+06 +no_defs +x_0=3.13723 +y_0=1.5708"),
                                    rectangleR2D(0, 0, 6.27913, 3.14166)),
                               1440, 721, 0.00436354*RAD_TO_DEG, 0.00436342*RAD_TO_DEG);

  const Area dummyMapArea(Projection("+proj=ob_tran +o_proj=longlat +lon_0=0 +o_lat_p=90 +R=6.371e+06 +no_defs +x_0=3.13723 +y_0=1.5708"),
                          rectangleR2D(1.01817, 1.81882, 5.25533, 2.42006));
  GridConverter gc;
  Points_p p = gc.getGridPoints(fieldGridArea, dummyMapArea, true);
  const float *positionsX = p->x, *positionsY = p->y;

  EXPECT_NEAR(positionsX[123], 210.6265, 1e-4);
  EXPECT_NEAR(positionsY[123], 0.1254, 1e-4);

  EXPECT_NEAR(positionsX[7200], 358.89285278320312, 1e-4);
  EXPECT_NEAR(positionsY[7200], 0.8750, 1e-4);

  const diutil::Rect ex_rects[] = {
    diutil::Rect(0,152,422,609),
    diutil::Rect(0,0,1439,720),
    diutil::Rect(0,129,1026,720),
    diutil::Rect(0,0,1439,720),
    diutil::Rect(0,0,1439,720),
    diutil::Rect(0,0,1439,720)
  };
  const int N = sizeof(ex_rects)/sizeof(ex_rects[0]);
  const Rectangle map_rects[N] = {rectangleR2D(-0.448535, 0.66958, 1.83301, 2.64988),  rectangleR2D(-1.82378, -0.524082, 3.20825, 3.84354),
                                  rectangleR2D(-0.565771, 0.567823, 4.46626, 4.93544), rectangleR2D(0, -1.1542, 6.27913, 4.29586),
                                  rectangleR2D(-1.56978, -1.1542, 4.70935, 4.29586),   rectangleR2D(1.56978, -1.1542, 7.84891, 4.29586)};

  for (int i=0; i<N; ++i) {
    const diutil::Rect actual_rect = findGridLimits(fieldGridArea, map_rects[i], true, positionsX, positionsY);
    EXPECT_TRUE(ex_rects[i] == actual_rect) << i << " expected " << ex_rects[i] << " got " << actual_rect;
  }
}
#endif

#if 0
TEST(GridConverterTest, FindGridLimitsGeos)
{
  const GridArea fieldGridArea(Area(Projection("+proj=geos +lon_0=-135.000000 +a=6378169.0 +b=6356583.8 +h=35785831.0"
              " +units=km +towgs84=0,0,0 +x_0=5641264.640000 +y_0=5641264.640000"),
          Rectangle(0,0,11282.5,11282.5)), 2816, 2816, 4.00658, 4.00658);

  const Area dummyMapArea(Projection("+proj=robin +lat_0=0.0 +lon_0=90 +x_0=0 +y_0=0 +ellps=WGS84 +towgs84=0,0,0 +no_defs"),
      Rectangle(-5.88293e+06,6.56044e+06,-4.25857e+06,7.25093e+06));

  GridConverter gc;
  Points_cp p = gc.getGridPoints(fieldGridArea, dummyMapArea, true);
  const float *positionsX = p->x, *positionsY = p->y;

  const diutil::Rect ex_rects[] = {
    diutil::Rect(2815,2815,1,1) // invalid
  };
  const int N = sizeof(ex_rects)/sizeof(ex_rects[0]);
  const Rectangle map_rects[N] = {
    Rectangle(-5.88293e+06,6.56044e+06,-4.25857e+06,7.25093e+06)
  };

  for (int i=0; i<N; ++i) {
    const diutil::Rect actual_rect = findGridLimits(fieldGridArea, map_rects[i], true, positionsX, positionsY);
    EXPECT_TRUE(ex_rects[i] == actual_rect) << "expected " << ex_rects[i] << " got " << actual_rect;
  }
}
#endif
