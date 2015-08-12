
#include <VcrossUtil.h>

#include <gtest/gtest.h>

TEST(VcrossUtilTest, Stepped)
{
  EXPECT_EQ(2, vcross::util::stepped_index( 1, 1, 14));
  EXPECT_EQ(0, vcross::util::stepped_index(13, 1, 14));

  EXPECT_EQ(3, vcross::util::stepped_index(13, 5, 14));
}
