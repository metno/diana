/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2022 met.no

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
  EXPECT_NEAR(fx, 7.61581, 1e-4);
  EXPECT_NEAR(fy, 55.2456, 1e-4);

  EXPECT_EQ(fxy.x(), fx);
  EXPECT_EQ(fxy.y(), fy);

  EXPECT_TRUE(p_geo.convertPoints(p_utm32, 1, &dx, &dy));
  EXPECT_TRUE(p_geo.convertPoints(p_utm32, 1, &dxy));

  EXPECT_EQ(dxy.x(), dx);
  EXPECT_EQ(dxy.y(), dy);

  EXPECT_NEAR(fxy.x(), dxy.x(), 1e-4);
  EXPECT_NEAR(fxy.y(), dxy.y(), 1e-4);
}

TEST(Projection, ObTranPoints)
{
  const Projection src("+proj=ob_tran +o_proj=longlat +lon_0=0 +o_lat_p=90 +R=6.371e+06 +no_defs");
  const Projection dst = src;

  const float RAD_TO_DEG = 180 / M_PI, gdxy = 0.5;

  float px = (123 - gdxy) * 0.00436354 * RAD_TO_DEG; // 30.626
  float py = (0 - gdxy) * 0.00436342 * RAD_TO_DEG;   // -0.125
  EXPECT_TRUE(dst.convertPoints(src, 1, &px, &py));

#if 0 // fails due to differences between proj 4 and proj 8
  // result from proj 4; proj 8 does not modify
  EXPECT_NEAR(px, 210.6265, 1e-4);
  EXPECT_NEAR(py,  0.1254, 1e-4);
#endif

  px = (123 - gdxy) * 0.00436354 * RAD_TO_DEG; // 30.626
  py = (20 - gdxy) * 0.00436342 * RAD_TO_DEG;  //  4.875
  EXPECT_TRUE(dst.convertPoints(src, 1, &px, &py));
  // result from proj 4; proj 8 does not modify and test fails
  EXPECT_NEAR(px, 30.62652, 1e-4);
  EXPECT_NEAR(py, 4.87510, 1e-4);
}

TEST(Projection, Convert2Points)
{
  const Projection p_geo = Projection::geographic();
  const Projection p_utm32("+proj=utm +zone=32 +ellps=WGS84 +datum=WGS84 +units=m +no_defs");
  diutil::PointF fxy[] = {{412000, 6123000}, {422000, 6143000}};
  float fx0 = fxy[0].x(), fy0 = fxy[0].y();
  float fx[] = {fxy[0].x(), fxy[1].x()};
  float fy[] = {fxy[0].y(), fxy[1].y()};

  diutil::PointD dxy[] = {{fx[0], fy[0]}, {fx[1], fy[1]}};
  double dx0 = dxy[0].x(), dy0 = dxy[0].y();
  double dx1 = dxy[1].x(), dy1 = dxy[1].y();
  double dx[] = {dx0, dx1};
  double dy[] = {dy0, dy1};

  EXPECT_TRUE(p_geo.convertPoints(p_utm32, 1, &fx0, &fy0));
  EXPECT_NEAR(fx0, 7.61581, 1e-4);
  EXPECT_NEAR(fy0, 55.2456, 1e-4);

  EXPECT_TRUE(p_geo.convertPoints(p_utm32, 2, fx, fy));
  EXPECT_FLOAT_EQ(fx[0], fx0);
  EXPECT_FLOAT_EQ(fy[0], fy0);
  EXPECT_NEAR(fx[1], 7.7675, 1e-4);
  EXPECT_NEAR(fy[1], 55.427, 1e-4);

  EXPECT_TRUE(p_geo.convertPoints(p_utm32, 2, fxy));
  EXPECT_FLOAT_EQ(fxy[0].x(), fx[0]);
  EXPECT_FLOAT_EQ(fxy[0].y(), fy[0]);
  EXPECT_FLOAT_EQ(fxy[1].x(), fx[1]);
  EXPECT_FLOAT_EQ(fxy[1].y(), fy[1]);

  EXPECT_TRUE(p_geo.convertPoints(p_utm32, 2, dx, dy));
  EXPECT_FLOAT_EQ(dx[0], fx[0]);
  EXPECT_FLOAT_EQ(dy[0], fy[0]);
  EXPECT_FLOAT_EQ(dx[1], fx[1]);
  EXPECT_FLOAT_EQ(dy[1], fy[1]);

  EXPECT_TRUE(p_geo.convertPoints(p_utm32, 2, dxy));
  EXPECT_FLOAT_EQ(dxy[0].x(), fx[0]);
  EXPECT_FLOAT_EQ(dxy[0].y(), fy[0]);
  EXPECT_FLOAT_EQ(dxy[1].x(), fx[1]);
  EXPECT_FLOAT_EQ(dxy[1].y(), fy[1]);
}

TEST(Projection, ConvertExamplePoints)
{
  const Projection p_mywave("+proj=ob_tran +o_proj=longlat +lon_0=-40 +o_lat_p=22 +R=6.371e+06 +no_defs");
  const Projection p_lonlat("+proj=longlat +R=6.371e+06 +no_defs");
  {
    const int N = 2;
    float x[N] = {5.53, 5.93};
    float y[N] = {-14.35, -13.95};
    EXPECT_TRUE(p_mywave.convertPoints(p_mywave, N, x, y));
    EXPECT_NEAR(x[0], 5.53, 1e-3);
    EXPECT_NEAR(x[1], 5.93, 1e-3);
    EXPECT_NEAR(y[0], -14.35, 1e-3);
    EXPECT_NEAR(y[1], -13.95, 1e-3);
  }
  {
    const int N = 2;
    float x[N] = {-220, +220};
    float y[N] = {+60, -60};
    EXPECT_TRUE(p_lonlat.convertPoints(p_lonlat, N, x, y));
    EXPECT_NEAR(x[0], -220, 1e-3);
    EXPECT_NEAR(x[1], +220, 1e-3);
    EXPECT_NEAR(y[0], +60, 1e-3);
    EXPECT_NEAR(y[1], -60, 1e-3);
  }
  {
    const int N = 2;
    float x[N] = {-220, +220};
    float y[N] = {+60, -60};
    EXPECT_TRUE(p_mywave.convertPoints(p_lonlat, N, x, y));
    EXPECT_NEAR(x[0], 0, 1e-3);
    EXPECT_NEAR(x[1], -149.486, 1e-3);
    EXPECT_NEAR(y[0], 52, 1e-3);
    EXPECT_NEAR(y[1], -14.1178, 1e-3);
  }
}

TEST(Projection, ConvertGeographic)
{
  const Projection p_utm32("+proj=utm +zone=32 +ellps=WGS84 +datum=WGS84 +units=m +no_defs");

  float lon_x = 10, lat_y = 60;
  p_utm32.convertFromGeographic(1, &lon_x, &lat_y);
  EXPECT_NEAR(555776.25, lon_x, 1);
  EXPECT_NEAR(6651832.5, lat_y, 1);

  p_utm32.convertToGeographic(1, &lon_x, &lat_y);
  EXPECT_NEAR(10, lon_x, 1e-2);
  EXPECT_NEAR(60, lat_y, 1e-2);
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
  ASSERT_GE(msgs.size(), 1);
  EXPECT_EQ(milogger::WARN, msgs.back().severity);
  EXPECT_EQ("diField.Projection", msgs.back().tag);
}

TEST(Projection, CopyAssign)
{
  const Projection p_utm32("+proj=utm +zone=32 +ellps=WGS84 +datum=WGS84 +units=m +no_defs");

  const Projection p_copy(p_utm32);
  EXPECT_TRUE(p_copy.isDefined());

  Projection p_assign;
  p_assign = p_utm32;
  p_assign = p_copy;
  EXPECT_TRUE(p_assign.isDefined());
}

TEST(Projection, GeosFromWKT)
{
  Projection p;
  EXPECT_TRUE(p.setFromWKT("PROJCS[\"Geostationary_Satellite\",GEOGCS[\"GCS_WGS_1984\",DATUM[\"D_unknown\",SPHEROID[\"WGS84\",6378137,298.257223563]],"
                           "PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.017453292519943295]],PROJECTION[\"Geostationary_Satellite\"],"
                           "PARAMETER[\"central_meridian\",0],PARAMETER[\"satellite_height\",35785831],"
                           "PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],UNIT[\"Meter\",1]]"));
}

TEST(Projection, ProjFromEPSG)
{
  Projection p("EPSG:3857");
}

TEST(Projection, IsGeographic)
{
  EXPECT_TRUE(Projection::geographic().isGeographic());
  EXPECT_TRUE(Projection("+proj=ob_tran +o_proj=longlat +lon_0=0 +o_lat_p=90 +R=6.371e+06 +no_defs").isGeographic());

  EXPECT_FALSE(Projection("EPSG:3857").isGeographic());
  EXPECT_FALSE(Projection("+proj=geos +lon_0=0 +h=35785831 +x_0=0 +y_0=0 +ellps=WGS84 +units=m +no_defs").isGeographic());
  EXPECT_FALSE(Projection("+proj=utm +zone=33 +ellps=WGS84 +datum=WGS84 +units=m +no_defs").isGeographic());
}

TEST(Projection, IsDegree)
{
  {
    const auto p_mywave = Projection("+proj=ob_tran +o_proj=longlat +lon_0=-40 +o_lat_p=22 +R=6.371e+06 +no_defs");
    EXPECT_TRUE(p_mywave.isDegree());
    EXPECT_FALSE(p_mywave.isGeographic());
  }
  {
    const auto p = Projection("EPSG:4326");
    EXPECT_TRUE(p.isDefined());
    EXPECT_TRUE(p.isDegree());
    EXPECT_TRUE(p.isGeographic());
  }
  {
    const auto p = Projection(" +proj=longlat +datum=WGS84 +no_defs");
    EXPECT_TRUE(p.isDefined());
    EXPECT_TRUE(p.isDegree());
    EXPECT_TRUE(p.isGeographic());
  }
  {
    const auto p = Projection("+proj=longlat +R=6.371e+06 +no_defs");
    EXPECT_TRUE(p.isDefined());
    EXPECT_TRUE(p.isDegree());
    EXPECT_TRUE(p.isGeographic());
  }
}

TEST(Projection, EPSG3575)
{
  const Projection src("EPSG:3575");
  const Projection dst("+proj=longlat +R=6.371e+06 +no_defs");

  EXPECT_FALSE(src.isDegree());

  float px = 0;          // 10;
  float py = -3309819.5; // 60;
  EXPECT_TRUE(dst.transformationFrom(src)->forward(1, &px, &py));

  EXPECT_NEAR(px, 10, 1e-4);
  EXPECT_NEAR(py, 60, 1e-4);
}

TEST(Projection, InitEPSG3575)
{
  const Projection src("+init=epsg:3575");
  const Projection dst("  +proj=longlat +R=6.371e+06 +no_defs"); // note spaces

  EXPECT_FALSE(src.isDegree());

  float px = 0;          // 10;
  float py = -3309819.5; // 60;
  EXPECT_TRUE(dst.transformationFrom(src)->forward(1, &px, &py));

  EXPECT_NEAR(px, 10, 1e-4);
  EXPECT_NEAR(py, 60, 1e-4);
}
