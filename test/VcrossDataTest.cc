
#include <VcrossData.h>
#include <VcrossUtil.h>

#include <gtest/gtest.h>

using namespace vcross;

static const char SECONDS_SINCE_1970[] = "seconds since 1970-01-01 00:00:00";

TEST(VcrossTimeTest, StrictWeakOrdering)
{
  const Time invalid1, invalid2,
      valid1(SECONDS_SINCE_1970,  60),
      valid2(SECONDS_SINCE_1970, 120),
      valid3(SECONDS_SINCE_1970, 180);

  EXPECT_FALSE(invalid1 < invalid1);
  EXPECT_FALSE(invalid1 < invalid2);

  EXPECT_TRUE(!(invalid1 < invalid2) && !(invalid2 < invalid1));

  EXPECT_TRUE(invalid1 < valid1);
  EXPECT_FALSE(valid1 < invalid1);

  EXPECT_TRUE(valid1 < valid2);
  EXPECT_TRUE(valid2 < valid3);
  EXPECT_TRUE(valid1 < valid3);

  EXPECT_FALSE(valid2 < valid1);
  EXPECT_FALSE(valid3 < valid1);
}

TEST(VcrossTimeTest, OpEqual)
{
  const Time invalid1, invalid2,
      valid1(SECONDS_SINCE_1970,  60),
      valid2(SECONDS_SINCE_1970, 120),
      valid3(SECONDS_SINCE_1970, 180);

  EXPECT_TRUE(invalid1 == invalid1);
  EXPECT_TRUE(invalid1 == invalid2);

  EXPECT_FALSE(valid1 == invalid1);

  EXPECT_FALSE(valid1 == valid2);
  EXPECT_FALSE(valid2 == valid3);
  EXPECT_FALSE(valid1 == valid3);
}
