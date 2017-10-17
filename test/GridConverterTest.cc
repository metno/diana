
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
#if 1
  const float* xmapr = 0, *ymapr = 0, *coriolis = 0;
  const float SCALE = 1;
  EXPECT_TRUE(gc.getMapFields(area, &xmapr, &ymapr, &coriolis));
#else
  float* xmapr = 0, *ymapr = 0, *coriolis = 0;
  const float SCALE = 0.5;
  float dxg = 0, dyg = 0;
  EXPECT_TRUE(gc.getMapFields(area, 3, 1, &xmapr, &ymapr, &coriolis, dxg, dyg));
#endif
  ASSERT_TRUE(xmapr);
  ASSERT_TRUE(ymapr);
  ASSERT_TRUE(coriolis);

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
    EXPECT_FLOAT_EQ(expect_x[i]*SCALE, xmapr[i]) << " i=" << i << " x";
    EXPECT_FLOAT_EQ(expect_y[i]*SCALE, ymapr[i]) << " i=" << i << " y";
    EXPECT_FLOAT_EQ(expect_c[i],    coriolis[i]) << " i=" << i << " c";
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

} // namespace

TEST(GridConverterTest, FindGridLimits)
{
  const GridArea fieldGridArea(Area(Projection("+proj=ob_tran +o_proj=longlat +lon_0=0 +o_lat_p=90 +R=6.371e+06 +no_defs +x_0=3.13723 +y_0=1.5708"),
          Rectangle(0,0,6.27913,3.14166)), 1440, 721, 0.00436354, 0.00436342);

  float *positionsX, *positionsY;
  const Area dummyMapArea(Projection("+proj=ob_tran +o_proj=longlat +lon_0=0 +o_lat_p=90 +R=6.371e+06 +no_defs +x_0=3.13723 +y_0=1.5708"),
      Rectangle(1.01817,1.81882,5.25533,2.42006));
  GridConverter gc;
  gc.getGridPoints(fieldGridArea, dummyMapArea, true, &positionsX, &positionsY);

  const diutil::Rect ex_rects[] = {
    diutil::Rect(0,152,422,609),
    diutil::Rect(0,0,1439,720),
    diutil::Rect(0,129,1026,720),
    diutil::Rect(0,0,1439,720),
    diutil::Rect(0,0,1439,720),
    diutil::Rect(0,0,1439,720)
  };
  const int N = sizeof(ex_rects)/sizeof(ex_rects[0]);
  const Rectangle map_rects[N] = {
    Rectangle(-0.448535,0.66958,1.83301,2.64988),
    Rectangle(-1.82378,-0.524082,3.20825,3.84354),
    Rectangle(-0.565771,0.567823,4.46626,4.93544),
    Rectangle(0,-1.1542,6.27913,4.29586),
    Rectangle(-1.56978,-1.1542,4.70935,4.29586),
    Rectangle(1.56978,-1.1542,7.84891,4.29586)
  };

  for (int i=0; i<N; ++i) {
    const diutil::Rect actual_rect = findGridLimits(fieldGridArea, map_rects[i], true, positionsX, positionsY);
    EXPECT_TRUE(ex_rects[i] == actual_rect) << "expected " << ex_rects[i] << " got " << actual_rect;
  }
}

#if 0
TEST(GridConverterTest, FindGridLimitsGeos)
{
  const GridArea fieldGridArea(Area(Projection("+proj=geos +lon_0=-135.000000 +a=6378169.0 +b=6356583.8 +h=35785831.0"
              " +units=km +towgs84=0,0,0 +x_0=5641264.640000 +y_0=5641264.640000"),
          Rectangle(0,0,11282.5,11282.5)), 2816, 2816, 4.00658, 4.00658);

  float *positionsX, *positionsY;
  const Area dummyMapArea(Projection("+proj=robin +lat_0=0.0 +lon_0=90 +x_0=0 +y_0=0 +ellps=WGS84 +towgs84=0,0,0 +no_defs"),
      Rectangle(-5.88293e+06,6.56044e+06,-4.25857e+06,7.25093e+06));

  GridConverter gc;
  gc.getGridPoints(fieldGridArea, dummyMapArea, true, &positionsX, &positionsY);

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
