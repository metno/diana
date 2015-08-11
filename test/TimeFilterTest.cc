
#include <TimeFilter.h>

#include <gtest/gtest.h>

TEST(TimeFilterTest, Basic)
{
  TimeFilter tf;
  const std::string pattern = "C11/C11_[yyyymmddHHMM]+???H??M";
  std::string filtered = pattern;
  tf.initFilter(filtered, true);

  EXPECT_EQ("C11/C11_????????????+???H??M", filtered);
}
