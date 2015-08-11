
#include <diFieldFunctions.h>
#include <diMetConstants.h>

#include <gtest/gtest.h>
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

namespace {
struct levelhum_params_t {
  int cah; // 'compute' for alevelhum and hlevelhum
  int cp;  // 'compute' for plevelhum
  float t; // temperature input
  float humin; // humidity input
  float p; // pressure input
  float expect; // expected output
  float near; // max deviation from expected
};
} // namespace

TEST(FieldFunctionsTest, XLevelHum)
{
  FieldFunctions ff;

  const levelhum_params_t levelhum_params[] = {
    // alevelhum/hlevelhum and plevelhum have compute numbers >= 5 switched
    {  1,  1, 30.68+T0, .025, 1013, 91.9, 0.1 },
    {  2,  2, 302.71,   .025, 1013, 91.9, 0.1  },
    {  3,  3, 30.68+T0, 55,   1013, 0.014963, .000001 },
    {  4,  4, 302.71,   55,   1013, 0.014963, .000001 },
    {  5,  7, 30.68+T0, .015, 1013, 20.6, 0.1 },
    {  6,  8, 302.71,   .015, 1013, 20.6, 0.1 },
    {  7,  5, 30.68+T0, 55,   1013, 20.6, 0.1 },
    {  8,  6, 302.71,   55,   1013, 20.6, 0.1 },
    { -1, -1, 0, 0, 0, 0, 0 }
  };
  const float alevel = 0, blevel = 1;

  for (int i=0; levelhum_params[i].cah >= 0; ++i) {
    const levelhum_params_t& p = levelhum_params[i];

    float humout = 2*UNDEF;
    bool allDefined = true;
    EXPECT_TRUE(ff.alevelhum(p.cah, 1, 1, &p.t, &p.humin, &p.p, &humout, allDefined, UNDEF, "celsius"));
    EXPECT_NEAR(p.expect, humout, p.near) << "alevelhum C i=" << i << " compute=" << p.cah;
    EXPECT_TRUE(allDefined);

    humout = 2*UNDEF;
    allDefined = true;
    EXPECT_TRUE(ff.hlevelhum(p.cah, 1, 1, &p.t, &p.humin, &p.p, &humout, alevel, blevel, allDefined, UNDEF, "celsius"));
    EXPECT_NEAR(p.expect, humout, p.near) << "hlevelhum C i=" << i << " compute=" << p.cah;
    EXPECT_TRUE(allDefined);

    humout = 2*UNDEF;
    allDefined = true;
    EXPECT_TRUE(ff.plevelhum(p.cp, 1, 1, &p.t, &p.humin, &humout, p.p, allDefined, UNDEF, "celsius"));
    EXPECT_NEAR(p.expect, humout, p.near) << "plevelhum C i=" << i << " compute=" << p.cp;
    EXPECT_TRUE(allDefined);

    humout = 2*UNDEF;
    allDefined = false;
    EXPECT_TRUE(ff.alevelhum(p.cah, 1, 1, &p.t, &p.humin, &p.p, &humout, allDefined, UNDEF, "celsius"));
    EXPECT_NEAR(p.expect, humout, p.near) << "alevelhum C i=" << i << " compute=" << p.cah;
    EXPECT_FALSE(allDefined);

    humout = 2*UNDEF;
    allDefined = false;
    EXPECT_TRUE(ff.hlevelhum(p.cah, 1, 1, &p.t, &p.humin, &p.p, &humout, alevel, blevel, allDefined, UNDEF, "celsius"));
    EXPECT_NEAR(p.expect, humout, p.near) << "hlevelhum C i=" << i << " compute=" << p.cah;
    EXPECT_FALSE(allDefined);

    humout = 2*UNDEF;
    allDefined = false;
    EXPECT_TRUE(ff.plevelhum(p.cp, 1, 1, &p.t, &p.humin, &humout, p.p, allDefined, UNDEF, "celsius"));
    EXPECT_NEAR(p.expect, humout, p.near) << "plevelhum C i=" << i << " compute=" << p.cp;
    EXPECT_FALSE(allDefined);

    if (p.cah < 5)
      continue;

    allDefined = true;
    EXPECT_TRUE(ff.alevelhum(p.cah, 1, 1, &p.t, &p.humin, &p.p, &humout, allDefined, UNDEF, "kelvin"));
    EXPECT_NEAR(p.expect+T0, humout, p.near) << "alevelhum K i=" << i << " compute=" << p.cah;
    EXPECT_TRUE(allDefined);

    allDefined = true;
    EXPECT_TRUE(ff.hlevelhum(p.cah, 1, 1, &p.t, &p.humin, &p.p, &humout, alevel, blevel, allDefined, UNDEF, "kelvin"));
    EXPECT_NEAR(p.expect+T0, humout, p.near) << "hlevelhum K i=" << i << " compute=" << p.cah;
    EXPECT_TRUE(allDefined);

    allDefined = true;
    EXPECT_TRUE(ff.plevelhum(p.cp, 1, 1, &p.t, &p.humin, &humout, p.p, allDefined, UNDEF, "kelvin"));
    EXPECT_NEAR(p.expect+T0, humout, p.near) << "plevelhum K i=" << i << " compute=" << p.cp;
    EXPECT_TRUE(allDefined);
  }
}

TEST(FieldFunctionsTest, ALevelTempPerformance)
{
  FieldFunctions ff;
  const float UNDEF = 1e30, T0 = 273.15;
  bool allDefined = true;

  { // compute == 3, T(Kelvin) -> TH
    const int N = 719*929; //1000000;
    const float F = 0.00001;
    float *tk = new float[N], *p = new float[N], *th = new float[N], *expected = new float[N];
    for (int i=0; i<N; ++i) {
      tk[i] = 20 + (i*F) + T0;
      p[i]  = 1005 + (i*F);
      th[i] = 2*UNDEF;
    }
    for (int i=0; i<1; ++i)
      EXPECT_TRUE(ff.aleveltemp(3, 1, N, tk, p, th, allDefined, UNDEF, "kelvin"));
    for (int i=0; i<N; ++i) {
      const float ex = tk[i] / powf(p[i] * MetNo::Constants::p0inv, MetNo::Constants::kappa);
      EXPECT_EQ(ex, th[i]);
    }
    delete[] tk;
    delete[] p;
    delete[] th;
    delete[] expected;
  }
}

namespace {
struct oper_params_t {
  int c; // 'compute'
  float a, b; // input
  bool input_defined;
  float expect; // expected output
  bool expect_defined;
};

enum { PLUS = 1, MINUS = 2, MUL = 3, DIV = 4 };

const oper_params_t oper_params[] = {
  { PLUS, 1, 3, true,   4, true },
  { PLUS, 1, 3, false,  4, false },
  { MINUS, 1, 3, true,  -2, true },
  { MINUS, 1, 3, false, -2, false },
  { MUL, 1.5, 3, true,  4.5, true },
  { MUL, 1.5, 3, false, 4.5, false },
  { DIV, 3, 1.5, true,  2, true },
  { DIV, 3, 1.5, false, 2, false },
  { DIV, 3, 0, true,  UNDEF, false },
  { DIV, 3, 0, false, UNDEF, false },
  { -1, 0, 0, false, 0, false }
};
} // namespace

TEST(FieldFunctionsTest, XOperX)
{
  float out;
  bool allDefined;
  
  for (int i=0; oper_params[i].c > 0; ++i) {
    const oper_params_t& op = oper_params[i];

    out = 2*UNDEF;
    allDefined = op.input_defined;
    EXPECT_TRUE(FieldFunctions::fieldOPERfield(op.c, 1, 1, &op.a, &op.b, &out, allDefined, UNDEF));
    EXPECT_EQ(op.expect_defined, allDefined);
    EXPECT_NEAR(op.expect, out, 1e-6) << " fOPf i=" << i << " c=" << op.c << " a=" << op.a << " b=" << op.b;
  
    out = 2*UNDEF;
    allDefined = op.input_defined;
    EXPECT_TRUE(FieldFunctions::fieldOPERconstant(op.c, 1, 1, &op.a, op.b, &out, allDefined, UNDEF));
    EXPECT_EQ(op.expect_defined, allDefined);
    EXPECT_NEAR(op.expect, out, 1e-6) << " fOPc i=" << i << " c=" << op.c << " a=" << op.a << " b=" << op.b;
  
    out = 2*UNDEF;
    allDefined = op.input_defined;
    EXPECT_TRUE(FieldFunctions::constantOPERfield(op.c, 1, 1, op.a, &op.b, &out, allDefined, UNDEF));
    EXPECT_EQ(op.expect_defined, allDefined);
    EXPECT_NEAR(op.expect, out, 1e-6) << " cOPf i=" << i << " c=" << op.c << " a=" << op.a << " b=" << op.b;
  }
}

TEST(FieldFunctionsTest, MapRatios)
{
  const int nx = 3, ny = 3;
  const GridArea area(Area(Projection("+proj=utm +zone=33 +ellps=WGS84 +datum=WGS84 +units=m +no_defs"),
          Rectangle(0, 100000, 10000, 110000)), nx, ny, 1000, 1000);
  GridConverter gc;
#if 1
  const float* xmapr = 0, *ymapr = 0, *coriolis = 0;
  EXPECT_TRUE(gc.getMapFields(area, &xmapr, &ymapr, &coriolis));
#else
  float* xmapr = 0, *ymapr = 0, *coriolis = 0;
  float dxg = 0, dyg = 0;
  EXPECT_TRUE(gc.getMapFields(area, 3, 1, &xmapr, &ymapr, &coriolis, dxg, dyg));
#endif
  ASSERT_TRUE(xmapr);
  ASSERT_TRUE(ymapr);
  ASSERT_TRUE(coriolis);

  const float finp0[nx*ny] = { 1, 1, 1, 2, 2, 2, 3, 3, 3 };
  const float finp1[nx*ny] = { 1, 2, 3, 2, 3, 1, 3, 1, 2 };
  const float finp2[nx*ny] = { 3, 3, 3, 2, 2, 2, 1, 1, 1 };
  float fout0[nx*ny], fout1[nx*ny];

  bool allDefined = true;
  EXPECT_TRUE(FieldFunctions::plevelgwind_xcomp(nx, ny, finp0, fout0, xmapr, ymapr, coriolis, allDefined, UNDEF));
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

  EXPECT_TRUE(FieldFunctions::plevelgwind_ycomp(nx, ny, finp0, fout0, xmapr, ymapr, coriolis, allDefined, UNDEF));
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

  EXPECT_TRUE(FieldFunctions::plevelgvort(nx, ny, finp0, fout0, xmapr, ymapr, coriolis, allDefined, UNDEF));
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

  EXPECT_TRUE(FieldFunctions::ilevelgwind(nx, ny, finp0, fout0, fout1, xmapr, ymapr, coriolis, allDefined, UNDEF));
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

  EXPECT_TRUE(FieldFunctions::relvort(nx, ny, finp0, finp1, fout0, xmapr, ymapr, allDefined, UNDEF));
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

  EXPECT_TRUE(FieldFunctions::absvort(nx, ny, finp0, finp1, fout0, xmapr, ymapr, coriolis, allDefined, UNDEF));
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

  EXPECT_TRUE(FieldFunctions::divergence(nx, ny, finp0, finp1, fout0, xmapr, ymapr, allDefined, UNDEF));
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

  EXPECT_TRUE(FieldFunctions::advection(nx, ny, finp0, finp1, finp2, fout0, xmapr, ymapr, 2, allDefined, UNDEF));
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

  EXPECT_TRUE(FieldFunctions::gradient(1, nx, ny, finp0, fout0, xmapr, ymapr, allDefined, UNDEF));
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

  EXPECT_TRUE(FieldFunctions::gradient(2, nx, ny, finp0, fout0, xmapr, ymapr, allDefined, UNDEF));
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

  EXPECT_TRUE(FieldFunctions::thermalFrontParameter(nx, ny, finp0, fout0, xmapr, ymapr, allDefined, UNDEF));
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

  EXPECT_TRUE(FieldFunctions::momentumXcoordinate(nx, ny, finp0, fout0, xmapr, coriolis, 1e-4, allDefined, UNDEF));
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

  EXPECT_TRUE(FieldFunctions::jacobian(nx, ny, finp0, finp1, fout0, xmapr, ymapr, allDefined, UNDEF));
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
