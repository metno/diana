
#include <diProjection.h>

#include <gtest/gtest.h>
#include <iomanip>

TEST(ProjectionTest, Convert1Point)
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
