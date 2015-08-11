
#include <diGridConverter.h>

#include <gtest/gtest.h>
#include <iomanip>

TEST(GridConverterTest, GetMapFields)
{
  const int nx = 3, ny = 3;
  const GridArea area(Area(Projection("+proj=utm +zone=33 +ellps=WGS84 +datum=WGS84 +units=m +no_defs"),
          Rectangle(0, 100000, 10000, 110000)), nx, ny, 1000, 1000);
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
