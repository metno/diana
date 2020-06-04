/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2020 met.no

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

#include <diArea.h>
#include <diFieldFunctions.h>
#include <diGridConverter.h>

#include <mi_fieldcalc/FieldCalculations.h>
#include <mi_fieldcalc/MetConstants.h>

#include <gtest/gtest.h>

#include <cmath>
#include <iostream>
#include <iomanip>

namespace /* anonymous */ {
const float UNDEF = 12356789, T0 = 273.15;
} // anonymous namespace

TEST(FieldFunctionsTest, ParseCompute)
{
  std::vector<std::string> lines, errors;

  lines.push_back("the=the.plevel_tk_rh(air_temperature:unit=kelvin,relative_humidity:unit=0.01)\n");
  lines.push_back("precipitation_percentile_1h_70=percentile(accumprecip.1h,70,10)\n");
  EXPECT_TRUE(FieldFunctions::parseComputeSetup(lines, errors));
  EXPECT_TRUE(errors.empty());
}

TEST(FieldFunctionsTest, ParseComputeBogus)
{
  std::vector<std::string> lines, errors;

  lines.push_back("fish=different(from,wood)\n");
  EXPECT_TRUE(FieldFunctions::parseComputeSetup(lines, errors));
  EXPECT_FALSE(errors.empty());
}

TEST(FieldFunctionsTest, MapRatios)
{
  const int nx = 3, ny = 3;
  const GridArea area(Area(Projection("+proj=utm +zone=33 +ellps=WGS84 +datum=WGS84 +units=m +no_defs"),
          Rectangle(0, 0, 10000, 110000)), nx, ny, 1000, 1000);
  GridConverter gc;
  MapFields_cp mf;
#if 1
  EXPECT_TRUE(mf = gc.getMapFields(area));
#else
  float dxg = 0, dyg = 0;
  EXPECT_TRUE(mf = gc.getMapFields(area, 3, 1, dxg, dyg));
#endif
  ASSERT_TRUE(mf->xmapr);
  ASSERT_TRUE(mf->ymapr);
  ASSERT_TRUE(mf->coriolis);

  const float finp0[nx*ny] = { 1, 1, 1, 2, 2, 2, 3, 3, 3 };
  const float finp1[nx*ny] = { 1, 2, 3, 2, 3, 1, 3, 1, 2 };
  const float finp2[nx*ny] = { 3, 3, 3, 2, 2, 2, 1, 1, 1 };
  float fout0[nx*ny], fout1[nx*ny];

  miutil::ValuesDefined fDefined = miutil::ALL_DEFINED;
  EXPECT_TRUE(miutil::fieldcalc::plevelgwind_xcomp(nx, ny, finp0, mf->xmapr, mf->ymapr, mf->coriolis, fout0, fDefined, UNDEF));
  const float ex_plevelgwind_xcomp[nx*ny] = {
    -425621.28,
    -425621.28,
    -425621.28,
    -425621.28,
    -425621.28,
    -425621.28,
    -425621.28,
    -425621.28,
    -425621.28
  };
  for (int i=0; i<nx*ny; ++i)
    EXPECT_FLOAT_EQ(ex_plevelgwind_xcomp[i], fout0[i]) << "i=" << i;

  EXPECT_TRUE(miutil::fieldcalc::plevelgwind_ycomp(nx, ny, finp0, mf->xmapr, mf->ymapr, mf->coriolis, fout0, fDefined, UNDEF));
  const float ex_plevelgwind_ycomp[nx*ny] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
  };
  for (int i=0; i<nx*ny; ++i)
    EXPECT_FLOAT_EQ(ex_plevelgwind_ycomp[i], fout0[i]) << "i=" << i;

  EXPECT_TRUE(miutil::fieldcalc::plevelgvort(nx, ny, finp0, mf->xmapr, mf->ymapr, mf->coriolis, fout0, fDefined, UNDEF));
  const float ex_plevelgvort[nx*ny] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
  };
  for (int i=0; i<nx*ny; ++i)
    EXPECT_FLOAT_EQ(ex_plevelgvort[i], fout0[i]) << "i=" << i;

  EXPECT_TRUE(miutil::fieldcalc::ilevelgwind(nx, ny, finp0, mf->xmapr, mf->ymapr, mf->coriolis, fout0, fout1, fDefined, UNDEF));
  const float ex_ilevelgwind[nx*ny] = {
    -43430.742,
    -43430.742,
    -43430.742,
    -43430.742,
    -43430.742,
    -43430.742,
    -43430.742,
    -43430.742,
    -43430.742
  };
  for (int i=0; i<nx*ny; ++i)
    EXPECT_FLOAT_EQ(ex_ilevelgwind[i], fout0[i]) << "i=" << i;

  EXPECT_TRUE(miutil::fieldcalc::relvort(nx, ny, finp0, finp1, mf->xmapr, mf->ymapr, fout0, fDefined, UNDEF));
  const float ex_relvort[nx*ny] = {
    -0.0014989537,
    -0.0014989537,
    -0.0014989537,
    -0.0014989537,
    -0.0014989537,
    -0.0014989537,
    -0.0014989537,
    -0.0014989537,
    -0.0014989537
  };
  for (int i=0; i<nx*ny; ++i)
    EXPECT_FLOAT_EQ(ex_relvort[i], fout0[i]) << "i=" << i;

  EXPECT_TRUE(miutil::fieldcalc::absvort(nx, ny, finp0, finp1, mf->xmapr, mf->ymapr, mf->coriolis, fout0, fDefined, UNDEF));
  const float ex_absvort[nx*ny] = {
    -0.0014989307,
    -0.0014989307,
    -0.0014989307,
    -0.0014989307,
    -0.0014989307,
    -0.0014989307,
    -0.0014989307,
    -0.0014989307,
    -0.0014989307
  };
  for (int i=0; i<nx*ny; ++i)
    EXPECT_FLOAT_EQ(ex_absvort[i], fout0[i]) << "i=" << i;

  EXPECT_TRUE(miutil::fieldcalc::divergence(nx, ny, finp0, finp1, mf->xmapr, mf->ymapr, fout0, fDefined, UNDEF));
  const float ex_divergence[nx*ny] = {
    -0.00049854361,
    -0.00049854361,
    -0.00049854361,
    -0.00049854361,
    -0.00049854361,
    -0.00049854361,
    -0.00049854361,
    -0.00049854361,
    -0.00049854361
  };
  for (int i=0; i<nx*ny; ++i)
    EXPECT_FLOAT_EQ(ex_divergence[i], fout0[i]) << "i=" << i;

  EXPECT_TRUE(miutil::fieldcalc::advection(nx, ny, finp0, finp1, finp2, mf->xmapr, mf->ymapr, 2, fout0, fDefined, UNDEF));
  const float ex_advection[nx*ny] = {
    -14.358056,
    -14.358056,
    -14.358056,
    -14.358056,
    -14.358056,
    -14.358056,
    -14.358056,
    -14.358056,
    -14.358056
  };
  for (int i=0; i<nx*ny; ++i)
    EXPECT_FLOAT_EQ(ex_advection[i], fout0[i]) << "i=" << i;

  EXPECT_TRUE(miutil::fieldcalc::gradient(nx, ny, finp0, mf->xmapr, mf->ymapr, 1, fout0, fDefined, UNDEF));
  const float ex_gradient_1[nx*ny] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
  };
  for (int i=0; i<nx*ny; ++i)
    EXPECT_FLOAT_EQ(ex_gradient_1[i], fout0[i]) << "i=" << i;

  EXPECT_TRUE(miutil::fieldcalc::gradient(nx, ny, finp0, mf->xmapr, mf->ymapr, 2, fout0, fDefined, UNDEF));
  const float ex_gradient_2[nx*ny] = {
    0.00099708722,
    0.00099708722,
    0.00099708722,
    0.00099708722,
    0.00099708722,
    0.00099708722,
    0.00099708722,
    0.00099708722,
    0.00099708722,
  };
  for (int i=0; i<nx*ny; ++i)
    EXPECT_FLOAT_EQ(ex_gradient_2[i], fout0[i]) << "i=" << i;

  EXPECT_TRUE(miutil::fieldcalc::thermalFrontParameter(nx, ny, finp0, mf->xmapr, mf->ymapr, fout0, fDefined, UNDEF));
  const float ex_thermalFrontParameter[nx*ny] = {
    -0,
    -0,
    -0,
    -0,
    -0,
    -0,
    -0,
    -0,
    -0
  };
  for (int i=0; i<nx*ny; ++i)
    EXPECT_FLOAT_EQ(ex_thermalFrontParameter[i], fout0[i]) << "i=" << i;

  EXPECT_TRUE(miutil::fieldcalc::momentumXcoordinate(nx, ny, finp0, mf->xmapr, mf->coriolis, 1e-4, fout0, fDefined, UNDEF));
  const float ex_momentumXcoordinate[nx*ny] = {
    10.038398,
    11.03733,
    12.03733,
    20.076796,
    21.074659,
    22.074659,
    30.111988,
    31.115194,
    32.115192
  };
  for (int i=0; i<nx*ny; ++i)
    EXPECT_FLOAT_EQ(ex_momentumXcoordinate[i], fout0[i]) << "i=" << i;

  EXPECT_TRUE(miutil::fieldcalc::jacobian(nx, ny, finp0, finp1, mf->xmapr, mf->ymapr, fout0, fDefined, UNDEF));
  const float ex_jacobian[nx*ny] = {
    5.0040461e-07,
    5.0040461e-07,
    5.0040461e-07,
    5.0040461e-07,
    5.0040461e-07,
    5.0040461e-07,
    5.0040461e-07,
    5.0040461e-07,
    5.0040461e-07
  };
  for (int i=0; i<nx*ny; ++i)
    EXPECT_FLOAT_EQ(ex_jacobian[i], fout0[i]) << "i=" << i;
}
