
#include <VcrossData.h>

#include <gtest/gtest.h>

typedef std::vector<size_t> size_v;
typedef std::vector<std::string> string_v;

namespace /* anonymous */ {
size_v operator<<(const size_v& container, size_t value)
{ size_v c(container); c.push_back(value); return c; }

string_v operator<<(const string_v& container, const std::string& value)
{ string_v c(container); c.push_back(value); return c; }
} // anonymous namespace

using namespace vcross;

TEST(ShapeTest, BasicShape)
{
  const size_v lengths = (size_v() << 2 << 3);
  const string_v names = (string_v() << "x" << Values::GEO_Z);
  const Values::Shape shape(names, lengths);

  EXPECT_EQ(0, shape.position("x"));
  EXPECT_EQ(1, shape.position(Values::GEO_Z));

  EXPECT_EQ(2, shape.length("x"));
  EXPECT_EQ(3, shape.length(1));

  EXPECT_EQ(2*3, shape.volume());
  EXPECT_EQ(2,   shape.rank());
}

TEST(ShapeTest, BasicIndex)
{
  const Values::Shape shape((string_v() << "x" << Values::GEO_Z), (size_v() << 2 << 3));
  
  Values::ShapeIndex idx(shape);
  EXPECT_EQ(0, idx.index());

  idx.set("x", 1);
  EXPECT_EQ(1, idx.index());

  idx.set(1, 2);
  EXPECT_EQ(5, idx.index());
}

TEST(ShapeTest, BasicSlice)
{
  const Values::Shape shape((string_v() << "x" << Values::GEO_Z), (size_v() << 2 << 3));

  Values::ShapeSlice slice(shape);
  slice.cut(1, 0, 2);
  EXPECT_EQ(2,   slice.length(Values::GEO_Z));
  EXPECT_EQ(2*2, slice.volume());
}
