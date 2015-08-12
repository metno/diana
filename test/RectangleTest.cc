
#include <diRectangle.h>

#include <gtest/gtest.h>

TEST(RectangleTest, Intersection)
{
  const Rectangle square2(0, 0, 2, 2);

  EXPECT_TRUE(square2.intersects(Rectangle(1, 1, 3, 3)));
  EXPECT_TRUE(square2.intersects(Rectangle(0, 1, 2, 3)));
  EXPECT_TRUE(square2.intersects(Rectangle(0, -1, 2, 3)));
  EXPECT_TRUE(square2.intersects(Rectangle(1, 0, 3, 2)));

  EXPECT_TRUE(square2.intersects(Rectangle(2, 0, 4, 2)));
  EXPECT_TRUE(square2.intersects(Rectangle(0, -2, 2, 0)));

  EXPECT_FALSE(square2.intersects(Rectangle(1, 3, 3, 5)));
}
