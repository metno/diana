
#include <DataReshape.h>

#include <gtest/gtest.h>

typedef std::vector<size_t> size_v;
typedef std::vector<std::string> string_v;

namespace /* anonymous */ {
size_v operator<<(const size_v& container, size_t value)
{ size_v c(container); c.push_back(value); return c; }

string_v operator<<(const string_v& container, const std::string& value)
{ string_v c(container); c.push_back(value); return c; }
} // anonymous namespace

TEST(DataReshapeTest, Volume)
{
  const size_v size_a = (size_v() << 2 << 2);
  EXPECT_EQ(4, calculate_volume(size_a));

  const size_v size_b = (size_v() << 3 << 4 << 5);
  EXPECT_EQ(60, calculate_volume(size_b));
}

TEST(DataReshapeTest, IdenticalDimensions1)
{
  const size_v size = (size_v() << 2 << 2);
  const string_v name_a = (string_v() << "x" << "z");
  const string_v name_b = (string_v() << "z" << "x");

  size_t sameIn, sameOut, sameSize;
  EXPECT_FALSE(count_identical_dimensions(name_a, size, name_b, size, sameIn, sameOut, sameSize));
  EXPECT_EQ(0, sameIn);
  EXPECT_EQ(0, sameOut);
  EXPECT_EQ(1, sameSize);
}

TEST(DataReshapeTest, IdenticalDimensions2)
{
  const size_v size_a = (size_v() << 2 << 1 << 5);
  const string_v name_a = (string_v() << "x" << "time" << "z");

  const size_v size_b = (size_v() << 1 << 2 << 5 << 1);
  const string_v name_b = (string_v() << "hei" << "x" << "z" << "ho");

  size_t sameIn, sameOut, sameSize;
  EXPECT_TRUE(count_identical_dimensions(name_a, size_a, name_b, size_b, sameIn, sameOut, sameSize));
  EXPECT_EQ(3, sameIn);
  EXPECT_EQ(4, sameOut);
  EXPECT_EQ(2*5, sameSize);
}

TEST(DataReshapeTest, ReshapeArray1)
{
  const size_v size_both = (size_v() << 2 << 2);
  const string_v name_1 = (string_v() << "x" << "z");
  const string_v name_2 = (string_v() << "z" << "x");

  const float floats_in[4] = { 11, 12, 21, 22 };
  float floats_out[4] = { 0, 0, 0, 0 };

  reshape(name_1, size_both, name_2, size_both, floats_in, floats_out);

  const float floats_ex[4] = { 11, 21, 12, 22 };
  for (int i=0; i<4; ++i)
    EXPECT_EQ(floats_ex[i], floats_out[i]) << "i=" << i;
}

TEST(DataReshapeTest, ReshapeArray2)
{
  const size_v size_a = (size_v() << 3 << 1 << 2);
  const string_v name_a = (string_v() << "z" << "time" << "x");

  const size_v size_b = (size_v() << 1 << 2 << 3 << 1);
  const string_v name_b = (string_v() << "ho" << "x" << "z" << "yo");

  const float floats_in[6] = { 11, 21, 31, 12, 22, 32 };
  float floats_out[6];

  reshape(name_a, size_a, name_b, size_b, floats_in, floats_out);

  const float floats_ex[6] = { 11, 12, 21, 22, 31, 32 };
  for (int i=0; i<6; ++i)
    EXPECT_EQ(floats_ex[i], floats_out[i]) << "i=" << i;
}

TEST(DataReshapeTest, ReshapeArray3)
{
  const size_v size_a = (size_v() << 3 << 2 << 2);
  const string_v name_a = (string_v() << "z" << "y" << "x");
  const float floats_in[12] = {
    111, 112, 113, 121, 122, 123,
    211, 212, 213, 221, 222, 223
  };

  const size_v size_b = (size_v() << 2 << 2 << 3);
  const string_v name_b = (string_v() << "y" << "x" << "z");
  float floats_out[12];

  reshape(name_a, size_a, name_b, size_b, floats_in, floats_out);

  const float floats_ex[12] = {
    111, 121, 211, 221,
    112, 122, 212, 222,
    113, 123, 213, 223
  };
  for (int i=0; i<12; ++i)
    EXPECT_EQ(floats_ex[i], floats_out[i]) << "i=" << i;
}

TEST(DataReshapeTest, ReshapeArrayIdentical)
{
  const size_v size_a = (size_v() << 3 << 1 << 2);
  const string_v name_a = (string_v() << "z" << "hei" << "x");

  const size_v size_b = (size_v() << 3 << 2);
  const string_v name_b = (string_v() << "z" << "x");

  boost::shared_array<float> array_in(new float[6]);
  const float floats_in[6] = { 11, 21, 31, 12, 22, 32 };
  for (int i=0; i<6; ++i)
    array_in[i] = floats_in[i];

  boost::shared_array<float> array_out = reshape(name_a, size_a, name_b, size_b, array_in);
  EXPECT_EQ(array_in, array_out);
}

TEST(DataReshapeTest, Expand1)
{
  const size_v size_a = (size_v() << 3);
  const string_v name_a = (string_v() << "z");
  const float floats_in[3] = { 1, 2, 3 };

  const size_v size_b = (size_v() << 2 << 3);
  const string_v name_b = (string_v() << "x" << "z");
  float floats_out[6];

  reshape(name_a, size_a, name_b, size_b, floats_in, floats_out);

  const float floats_ex[6] = { 1, 1, 2, 2, 3, 3 };
  for (int i=0; i<6; ++i)
    EXPECT_EQ(floats_ex[i], floats_out[i]) << "i=" << i;
}

TEST(DataReshapeTest, Expand2)
{
  const size_v size_a = (size_v() << 3);
  const string_v name_a = (string_v() << "z");
  const float floats_in[3] = { 1, 2, 3 };

  const size_v size_b = (size_v() << 3 << 2);
  const string_v name_b = (string_v() << "z" << "x");
  float floats_out[6];

  reshape(name_a, size_a, name_b, size_b, floats_in, floats_out);

  const float floats_ex[6] = { 1, 2, 3, 1, 2, 3 };
  for (int i=0; i<6; ++i)
    EXPECT_EQ(floats_ex[i], floats_out[i]) << "i=" << i;
}
