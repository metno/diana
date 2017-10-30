
#include <diArea.h>
#include <diFieldCalculations.h>
#include <diFieldFunctions.h>
#include <diGridConverter.h>
#include <diMetConstants.h>

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
  const levelhum_params_t levelhum_params[] = {
    // alevelhum/hlevelhum and plevelhum have compute numbers >= 5 switched
    {  1,  1, 30.68f+T0, .025f, 1013, 91.9f, 0.1f },
    {  2,  2, 302.71f,   .025f, 1013, 91.9f, 0.1f },
    {  3,  3, 30.68f+T0, 55,    1013, 0.014963f, .000001f },
    {  4,  4, 302.71f,   55,    1013, 0.014963f, .000001f },
    {  5,  7, 30.68f+T0, .015f, 1013, 20.6f, 0.1f },
    {  6,  8, 302.71f,   .015f, 1013, 20.6f, 0.1f },
    {  7,  5, 30.68f+T0, 55,    1013, 20.6f, 0.1f },
    {  8,  6, 302.71f,   55,    1013, 20.6f, 0.1f },
    { -1, -1, 0, 0, 0, 0, 0 }
  };
  const float alevel = 0, blevel = 1;

  for (int i=0; levelhum_params[i].cah >= 0; ++i) {
    const levelhum_params_t& p = levelhum_params[i];

    float humout = 2*UNDEF;
    difield::ValuesDefined fDefined = difield::ALL_DEFINED;
    EXPECT_TRUE(FieldCalculations::alevelhum(p.cah, 1, 1, &p.t, &p.humin, &p.p, &humout, fDefined, UNDEF, "celsius"));
    EXPECT_NEAR(p.expect, humout, p.near) << "alevelhum C i=" << i << " compute=" << p.cah;
    EXPECT_EQ(difield::ALL_DEFINED, fDefined);

    humout = 2*UNDEF;
    fDefined = difield::ALL_DEFINED;
    EXPECT_TRUE(FieldCalculations::hlevelhum(p.cah, 1, 1, &p.t, &p.humin, &p.p, &humout, alevel, blevel, fDefined, UNDEF, "celsius"));
    EXPECT_NEAR(p.expect, humout, p.near) << "hlevelhum C i=" << i << " compute=" << p.cah;
    EXPECT_EQ(difield::ALL_DEFINED, fDefined);

    humout = 2*UNDEF;
    fDefined = difield::ALL_DEFINED;
    EXPECT_TRUE(FieldCalculations::plevelhum(p.cp, 1, 1, &p.t, &p.humin, &humout, p.p, fDefined, UNDEF, "celsius"));
    EXPECT_NEAR(p.expect, humout, p.near) << "plevelhum C i=" << i << " compute=" << p.cp;
    EXPECT_EQ(difield::ALL_DEFINED, fDefined);

    humout = 2*UNDEF;
    fDefined = difield::SOME_DEFINED;
    EXPECT_TRUE(FieldCalculations::alevelhum(p.cah, 1, 1, &p.t, &p.humin, &p.p, &humout, fDefined, UNDEF, "celsius"));
    EXPECT_NEAR(p.expect, humout, p.near) << "alevelhum C i=" << i << " compute=" << p.cah;
    EXPECT_EQ(difield::ALL_DEFINED, fDefined);

    humout = 2*UNDEF;
    fDefined = difield::SOME_DEFINED;
    EXPECT_TRUE(FieldCalculations::hlevelhum(p.cah, 1, 1, &p.t, &p.humin, &p.p, &humout, alevel, blevel, fDefined, UNDEF, "celsius"));
    EXPECT_NEAR(p.expect, humout, p.near) << "hlevelhum C i=" << i << " compute=" << p.cah;
    EXPECT_EQ(difield::ALL_DEFINED, fDefined);

    humout = 2*UNDEF;
    fDefined = difield::SOME_DEFINED;
    EXPECT_TRUE(FieldCalculations::plevelhum(p.cp, 1, 1, &p.t, &p.humin, &humout, p.p, fDefined, UNDEF, "celsius"));
    EXPECT_NEAR(p.expect, humout, p.near) << "plevelhum C i=" << i << " compute=" << p.cp;
    EXPECT_EQ(difield::ALL_DEFINED, fDefined);

    if (p.cah < 5)
      continue;

    fDefined = difield::ALL_DEFINED;
    EXPECT_TRUE(FieldCalculations::alevelhum(p.cah, 1, 1, &p.t, &p.humin, &p.p, &humout, fDefined, UNDEF, "kelvin"));
    EXPECT_NEAR(p.expect+T0, humout, p.near) << "alevelhum K i=" << i << " compute=" << p.cah;
    EXPECT_EQ(difield::ALL_DEFINED, fDefined);

    fDefined = difield::ALL_DEFINED;
    EXPECT_TRUE(FieldCalculations::hlevelhum(p.cah, 1, 1, &p.t, &p.humin, &p.p, &humout, alevel, blevel, fDefined, UNDEF, "kelvin"));
    EXPECT_NEAR(p.expect+T0, humout, p.near) << "hlevelhum K i=" << i << " compute=" << p.cah;
    EXPECT_EQ(difield::ALL_DEFINED, fDefined);

    fDefined = difield::ALL_DEFINED;
    EXPECT_TRUE(FieldCalculations::plevelhum(p.cp, 1, 1, &p.t, &p.humin, &humout, p.p, fDefined, UNDEF, "kelvin"));
    EXPECT_NEAR(p.expect+T0, humout, p.near) << "plevelhum K i=" << i << " compute=" << p.cp;
    EXPECT_EQ(difield::ALL_DEFINED, fDefined);
  }
}

TEST(FieldFunctionsTest, ALevelTempPerformance)
{
  const float UNDEF = 1e30, T0 = 273.15;
  difield::ValuesDefined fDefined = difield::ALL_DEFINED;

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
      EXPECT_TRUE(FieldCalculations::aleveltemp(3, 1, N, tk, p, th, fDefined, UNDEF, "kelvin"));
    for (int i=0; i<N; ++i) {
      const float ex = tk[i] / powf(p[i] * MetNo::Constants::p0inv, MetNo::Constants::kappa);
      EXPECT_FLOAT_EQ(ex, th[i]);
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
  difield::ValuesDefined input_defined;
  float expect; // expected output
};

enum { PLUS = 1, MINUS = 2, MUL = 3, DIV = 4 };

const oper_params_t oper_params[] = {
  { PLUS,  1, 3, difield::ALL_DEFINED,    4 },
  { PLUS,  1, 3, difield::SOME_DEFINED, 4,},
  { MINUS, 1, 3, difield::ALL_DEFINED,    -2 },
  { MINUS, 1, 3, difield::SOME_DEFINED, -2 },
  { MUL, 1.5, 3, difield::ALL_DEFINED,    4.5 },
  { MUL, 1.5, 3, difield::SOME_DEFINED, 4.5 },
  { DIV, 3, 1.5, difield::ALL_DEFINED,    2 },
  { DIV, 3, 1.5, difield::SOME_DEFINED, 2 },
  { DIV, 3, 0,   difield::ALL_DEFINED,    UNDEF },
  { DIV, 3, 0,   difield::SOME_DEFINED, UNDEF },
  { -1,  0, 0,   difield::SOME_DEFINED, 0 }
};
} // namespace

TEST(FieldFunctionsTest, XOperX)
{
  float out;
  difield::ValuesDefined fDefined;

  for (int i=0; oper_params[i].c > 0; ++i) {
    const oper_params_t& op = oper_params[i];

    out = 2*UNDEF;
    fDefined = op.input_defined;
    EXPECT_TRUE(FieldCalculations::fieldOPERfield(op.c, 1, 1, &op.a, &op.b, &out, fDefined, UNDEF));
    EXPECT_EQ(op.expect == UNDEF, difield::NONE_DEFINED == fDefined) << "fOPf i=" << i;
    EXPECT_NEAR(op.expect, out, 1e-6) << " fOPf i=" << i << " c=" << op.c << " a=" << op.a << " b=" << op.b;

    out = 2*UNDEF;
    fDefined = op.input_defined;
    EXPECT_TRUE(FieldCalculations::fieldOPERconstant(op.c, 1, 1, &op.a, op.b, &out, fDefined, UNDEF));
    EXPECT_EQ(op.expect == UNDEF, difield::NONE_DEFINED == fDefined) << "fOPc i=" << i;
    EXPECT_NEAR(op.expect, out, 1e-6) << " fOPc i=" << i << " c=" << op.c << " a=" << op.a << " b=" << op.b;

    out = 2*UNDEF;
    fDefined = op.input_defined;
    EXPECT_TRUE(FieldCalculations::constantOPERfield(op.c, 1, 1, op.a, &op.b, &out, fDefined, UNDEF));
    EXPECT_EQ(op.expect == UNDEF, difield::NONE_DEFINED == fDefined) << "cOPf i=" << i;
    EXPECT_NEAR(op.expect, out, 1e-6) << " cOPf i=" << i << " c=" << op.c << " a=" << op.a << " b=" << op.b;
  }
}

TEST(FieldFunctionsTest, MapRatios)
{
  const int nx = 3, ny = 3;
  const GridArea area(Area(Projection("+proj=utm +zone=33 +ellps=WGS84 +datum=WGS84 +units=m +no_defs"),
          Rectangle(0, 0, 10000, 110000)), nx, ny, 1000, 1000);
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

  difield::ValuesDefined fDefined = difield::ALL_DEFINED;
  EXPECT_TRUE(FieldCalculations::plevelgwind_xcomp(nx, ny, finp0, fout0, xmapr, ymapr, coriolis, fDefined, UNDEF));
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

  EXPECT_TRUE(FieldCalculations::plevelgwind_ycomp(nx, ny, finp0, fout0, xmapr, ymapr, coriolis, fDefined, UNDEF));
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

  EXPECT_TRUE(FieldCalculations::plevelgvort(nx, ny, finp0, fout0, xmapr, ymapr, coriolis, fDefined, UNDEF));
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

  EXPECT_TRUE(FieldCalculations::ilevelgwind(nx, ny, finp0, fout0, fout1, xmapr, ymapr, coriolis, fDefined, UNDEF));
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

  EXPECT_TRUE(FieldCalculations::relvort(nx, ny, finp0, finp1, fout0, xmapr, ymapr, fDefined, UNDEF));
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

  EXPECT_TRUE(FieldCalculations::absvort(nx, ny, finp0, finp1, fout0, xmapr, ymapr, coriolis, fDefined, UNDEF));
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

  EXPECT_TRUE(FieldCalculations::divergence(nx, ny, finp0, finp1, fout0, xmapr, ymapr, fDefined, UNDEF));
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

  EXPECT_TRUE(FieldCalculations::advection(nx, ny, finp0, finp1, finp2, fout0, xmapr, ymapr, 2, fDefined, UNDEF));
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

  EXPECT_TRUE(FieldCalculations::gradient(1, nx, ny, finp0, fout0, xmapr, ymapr, fDefined, UNDEF));
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

  EXPECT_TRUE(FieldCalculations::gradient(2, nx, ny, finp0, fout0, xmapr, ymapr, fDefined, UNDEF));
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

  EXPECT_TRUE(FieldCalculations::thermalFrontParameter(nx, ny, finp0, fout0, xmapr, ymapr, fDefined, UNDEF));
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

  EXPECT_TRUE(FieldCalculations::momentumXcoordinate(nx, ny, finp0, fout0, xmapr, coriolis, 1e-4, fDefined, UNDEF));
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

  EXPECT_TRUE(FieldCalculations::jacobian(nx, ny, finp0, finp1, fout0, xmapr, ymapr, fDefined, UNDEF));
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

TEST(FieldCalculationsTest, Probability)
{
  const float UNDEF = 123456;

  const int N_ENS = 10;
  float fields[N_ENS];
  std::vector<float*> infields;
  for (size_t i=0; i<N_ENS; ++i)
    infields.push_back(&fields[i]);

  std::fill(fields, fields+N_ENS, UNDEF);
  fields[2] = 940;
  fields[4] = 3500;

  std::vector<difield::ValuesDefined> defined(N_ENS, difield::SOME_DEFINED);
  defined[0] = difield::NONE_DEFINED;
  defined[8] = difield::NONE_DEFINED;

  std::vector<float> limits(2, 3000);
  float field_out = UNDEF;
  difield::ValuesDefined defined_out = difield::NONE_DEFINED;
  EXPECT_TRUE(FieldCalculations::probability(/*compute=*/2/*probability_below*/,
                                             /*nx=*/1, /*ny=*/1, infields, defined,
                                             limits, &field_out, defined_out, UNDEF));
  EXPECT_NEAR(100.0*1/8, field_out, 1e-6);
  EXPECT_EQ(difield::ALL_DEFINED, defined_out);

  field_out = UNDEF;
  defined_out = difield::NONE_DEFINED;
  EXPECT_TRUE(FieldCalculations::probability(/*compute=*/1/*probability_above*/,
                                             /*nx=*/1, /*ny=*/1, infields, defined,
                                             limits, &field_out, defined_out, UNDEF));
  EXPECT_NEAR(100.0*1/8, field_out, 1e-6);
  EXPECT_EQ(difield::ALL_DEFINED, defined_out);

  field_out = UNDEF;
  defined_out = difield::NONE_DEFINED;
  limits[0] = 4000;
  EXPECT_TRUE(FieldCalculations::probability(/*compute=*/2/*probability_below*/,
                                             /*nx=*/1, /*ny=*/1, infields, defined,
                                             limits, &field_out, defined_out, UNDEF));
  EXPECT_NEAR(100.0*2/8, field_out, 1e-6);
  EXPECT_EQ(difield::ALL_DEFINED, defined_out);

  field_out = UNDEF;
  defined_out = difield::NONE_DEFINED;
  limits[0] =  500;
  limits[1] = 4000;
  EXPECT_TRUE(FieldCalculations::probability(/*compute=*/3/*probability_between*/,
                                             /*nx=*/1, /*ny=*/1, infields, defined,
                                             limits, &field_out, defined_out, UNDEF));
  EXPECT_NEAR(100.0*2/8, field_out, 1e-6);
  EXPECT_EQ(difield::ALL_DEFINED, defined_out);
}

TEST(FieldCalculationsTest, Probability12)
{
  const float UNDEF = 123456;

  const int N_ENS = 10;
  float fields[N_ENS];
  std::vector<float*> infields;
  for (size_t i=0; i<N_ENS; ++i)
    infields.push_back(&fields[i]);

  std::fill(fields, fields+N_ENS, 12);
  fields[3] = fields[5] = UNDEF;

  std::vector<difield::ValuesDefined> defined(N_ENS, difield::SOME_DEFINED);

  std::vector<float> limits(1, 3000);
  float field_out = UNDEF;
  difield::ValuesDefined defined_out = difield::NONE_DEFINED;
  EXPECT_TRUE(FieldCalculations::probability(/*compute=*/2/*probability_below*/,
                                             /*nx=*/1, /*ny=*/1, infields, defined,
                                             limits, &field_out, defined_out, UNDEF));
  EXPECT_NEAR(80, field_out, 1e-6);
  EXPECT_EQ(difield::ALL_DEFINED, defined_out);

  field_out = UNDEF;
  defined_out = difield::NONE_DEFINED;
  EXPECT_TRUE(FieldCalculations::probability(/*compute=*/1/*probability_above*/,
                                             /*nx=*/1, /*ny=*/1, infields, defined,
                                             limits, &field_out, defined_out, UNDEF));
  EXPECT_NEAR(0, field_out, 1e-6);
  EXPECT_EQ(difield::ALL_DEFINED, defined_out);
}
