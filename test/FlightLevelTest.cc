
#include <diField/VcrossUtil.h>
#include <diFlightLevel.h>

#include <gtest/gtest.h>

TEST(FlightLevelTest, FLFromPressure)
{
  // from table
  EXPECT_EQ("FL235", FlightLevel::getFlightLevel("400hPa"));
  EXPECT_EQ("FL005", FlightLevel::getFlightLevel("1000hPa"));

  // other
  EXPECT_EQ("FL140", FlightLevel::getFlightLevel("600hPa"));
  EXPECT_EQ("FL445", FlightLevel::getFlightLevel("150hPa"));
}

TEST(FlightLevelTest, PressureFromFL)
{
  // from table
  EXPECT_EQ("400hPa", FlightLevel::getPressureLevel("FL235"));
  EXPECT_EQ("1000hPa", FlightLevel::getPressureLevel("FL005"));

  // other
  EXPECT_EQ("600hPa", FlightLevel::getPressureLevel("FL140"));
  EXPECT_EQ("150hPa", FlightLevel::getPressureLevel("FL445"));
}
