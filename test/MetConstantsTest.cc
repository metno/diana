
#include <diMetConstants.h>

#include <gtest/gtest.h>

using namespace MetNo::Constants;

namespace {

// from ICAO doc 7488
const double p_h_doc7488[][2] =  {
  {    8.7, 31985 },
  {   10.0, 31055 },
  {   11.1, 30360 },
  {   19.4, 26680 },
  {   97.3, 16353 },
  {  139.5, 14069 },
  {  244.1, 10517 },
  {  354.2,  8035 },
  {  459.7,  6189 },
  {  590.8,  4324 },
  {  739.7,  2576 },
  {  840.7,  1547 },
  {  936.8,   657 },
  { 1010.0,    27 },
  { 1020.0,   -56 },
  { 1050.0,  -302 },
  { 1130.0,  -929 },
  { -1, -1 }
};

const int p_h_examples[][2] =  {
  {  600, 140 },
  {  500, 185 },
  {  400, 235 },
  {  300, 300 },
  {  250, 340 },
  {  200, 385 },
  {  150, 445 },
  { -1, -1 }
};

}

TEST(MetConstantsTest, ICAOGeoAltitudeFromPressure1)
{
  for (int i=0; p_h_doc7488[i][0] > 0; ++i) {
    const double p = p_h_doc7488[i][0];
    const double h_actual = ICAO_geo_altitude_from_pressure(p);
    const double h_expected = p_h_doc7488[i][1];
    // allow a deviation of 1.55m due to rounding
    EXPECT_NEAR(h_expected, h_actual, 1.55) << " p=" << p;
  }
}

TEST(MetConstantsTest, ICAOGeoAltitudeFromPressure2)
{
  for (int i=0; p_h_examples[i][0] > 0; ++i) {
    const double p = p_h_examples[i][0];
    const int FL_actual = FL_from_geo_altitude(ICAO_geo_altitude_from_pressure(p));
    const int FL_expected = p_h_examples[i][1];
    EXPECT_EQ(FL_expected, FL_actual) << " p=" << p;
  }
}

TEST(MetConstantsTest, ICAOGeoAltitudeFromPressure3)
{
  for (int i=0; i < nLevelTable; ++i) {
    const double p = pLevelTable[i];
    const int FL_actual = FL_from_geo_altitude(ICAO_geo_altitude_from_pressure(p));
    const int FL_expected = fLevelTable[i];
    EXPECT_EQ(FL_expected, FL_actual) << " p=" << p;
  }
}

TEST(MetConstantsTest, ICAOPressureFromGeoAltitude1)
{
  for (int i=0; p_h_doc7488[i][0] > 0; ++i) {
    const double h          = p_h_doc7488[i][1];
    const double p_expected = p_h_doc7488[i][0];
    const double p_actual = ICAO_pressure_from_geo_altitude(h);
    // allow some deviation due to rounding
    EXPECT_NEAR(p_expected, p_actual, 0.01*p_expected) << " h=" << h;
  }
}
