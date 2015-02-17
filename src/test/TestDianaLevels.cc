
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diPlotOptions.h>
#include <diPolyContouring.h>

#include <boost/range/end.hpp>
#include <vector>
#include <cmath>

#include <gtest/gtest.h>

using namespace std;

TEST(TestDianaLevels, Log)
{
  const float a_loglinevalues[] = { 1, 3 };
  const std::vector<float> v_loglinevalues(a_loglinevalues, boost::end(a_loglinevalues));
  const DianaLevelLog ll(v_loglinevalues);
  const int UNDEF = DianaLevels::UNDEF_LEVEL;

  EXPECT_EQ(-10, ll.level_for_value(0.9e-5));
  EXPECT_EQ( -5, ll.level_for_value(2.5e-3));
  EXPECT_EQ( -4, ll.level_for_value(3.1e-3));
  EXPECT_EQ(UNDEF, ll.level_for_value(0));

  EXPECT_FLOAT_EQ(1e-5, ll.value_for_level(-10));
  EXPECT_FLOAT_EQ(3e-3, ll.value_for_level( -5));
  EXPECT_FLOAT_EQ(1e-2, ll.value_for_level( -4));

  EXPECT_EQ(-71, ll.level_for_value(2.7e-36));
  EXPECT_FLOAT_EQ(3e-36, ll.value_for_level(-71));

  EXPECT_EQ(-72, ll.level_for_value(0.7e-36));
  EXPECT_FLOAT_EQ(1e-36, ll.value_for_level(-72));

  EXPECT_EQ(-85, ll.level_for_value(3.0e-43));
  EXPECT_EQ(-85, ll.level_for_value(2.9e-43));
  EXPECT_EQ(-85, ll.level_for_value(1.8e-43));
  EXPECT_EQ(-87, ll.level_for_value(2.5e-44));
  EXPECT_EQ(-85, ll.level_for_value(1.2e-43));
  EXPECT_EQ(-85, ll.level_for_value(1.3e-43));
  EXPECT_FLOAT_EQ(1e-42, ll.value_for_level(-84));
  EXPECT_FLOAT_EQ(3e-43, ll.value_for_level(-85));
  EXPECT_FLOAT_EQ(3e-44, ll.value_for_level(-87));
}

TEST(TestDianaLevels, List10)
{
  const float a_loglinevalues[] = { 1, 3 };
  const std::vector<float> v_loglinevalues(a_loglinevalues, boost::end(a_loglinevalues));
  const DianaLevelList10 ll(v_loglinevalues, 10);

  EXPECT_EQ(0, ll.level_for_value(0.1));
  EXPECT_EQ(1, ll.level_for_value(2));
  EXPECT_EQ(10, ll.level_for_value(1e11));

  EXPECT_FLOAT_EQ(1, ll.value_for_level(0));
  EXPECT_FLOAT_EQ(3, ll.value_for_level(1));
  EXPECT_FLOAT_EQ(10, ll.value_for_level(2));
  EXPECT_FLOAT_EQ(3e4, ll.value_for_level(9));
}
