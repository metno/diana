/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2014 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no
  
  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <diUtilities.h>
#include <diField/diRectangle.h>
#include <diField/TimeFilter.h>
#include <puCtools/puCglob.h> // for GLOB_BRACE
#include <gtest/gtest.h>
#include <cstring>

static const std::string SRC_TEST = TEST_SRCDIR "/";

TEST(TestUtilities, StartsWith)
{
  EXPECT_TRUE(diutil::startswith("fish", ""));
  EXPECT_TRUE(diutil::startswith("fish", "fi"));
  EXPECT_TRUE(diutil::startswith("fish", "fish"));

  EXPECT_FALSE(diutil::startswith("fish", "sh"));
  EXPECT_FALSE(diutil::startswith("fish", "is"));
  EXPECT_FALSE(diutil::startswith("fish", "fishy"));
}

TEST(TestUtilities, EndsWith)
{
  EXPECT_TRUE(diutil::endswith("fish", ""));
  EXPECT_TRUE(diutil::endswith("fish", "sh"));
  EXPECT_TRUE(diutil::endswith("fish", "fish"));

  EXPECT_FALSE(diutil::endswith("fish", "fi"));
  EXPECT_FALSE(diutil::endswith("fish", "is"));
  EXPECT_FALSE(diutil::endswith("fish", "fishy"));
}

TEST(TestUtilities, ReplacedChars)
{
  ASSERT_EQ("hei ho", diutil::replaced_chars("hei ho", "!\"#%", '_'));
  ASSERT_EQ("h_ei___ _h_o_", diutil::replaced_chars("h\"ei#%% \"h_o!", "!\"#%", '_'));
}

TEST(TestUtilities, AppendCharsSplitNewline)
{
  const char* input[] =  {
    "Hel", "lo", "\nWorl", "", "d!\nHello\nDian", "a!", 0
  };
  diutil::string_v lines(1);
  for (int i=0; input[i]; ++i)
    diutil::detail::append_chars_split_newline(lines, input[i], strlen(input[i]));

  ASSERT_EQ(4, lines.size());
  EXPECT_EQ("Hello",  lines[0]);
  EXPECT_EQ("World!", lines[1]);
  EXPECT_EQ("Hello",  lines[2]);
  EXPECT_EQ("Diana!", lines[3]);
}

TEST(TestUtilities, GlobTimeFilter)
{
  TimeFilter tf;
  const std::string pattern = SRC_TEST + "test_utilities_[yyyymmddHHMM]+???H??M";
  std::string filtered = pattern;
  tf.initFilter(filtered, true);
  EXPECT_EQ(SRC_TEST + "test_utilities_????????????+???H??M", filtered);

  const diutil::string_v matches = diutil::glob(filtered, GLOB_BRACE);

  EXPECT_EQ(1, matches.size());
  if (matches.size() >= 1) {
    const std::string ex = SRC_TEST + "test_utilities_201409121200+000H00M";
    EXPECT_EQ(ex, matches.front());
  }
}

TEST(TestUtilities, replace_reftime_with_offset)
{
  const miutil::miDate nowDate = miutil::miTime("2014-10-10 01:23:45").date();

  { const std::string pstr_in = "fish swims reftime=2014-10-09 00:00:00 fast";
    const std::string pstr_ex = "fish swims refhour=0 refoffset=-1 fast";
    std::string pstr = pstr_in;
    diutil::replace_reftime_with_offset(pstr, nowDate);
    EXPECT_EQ(pstr_ex, pstr);
  }

  { const std::string pstr_in = "fish swims reftime=2014-10-10 06:00:00 fast";
    const std::string pstr_ex = "fish swims refhour=6 fast";
    std::string pstr = pstr_in;
    diutil::replace_reftime_with_offset(pstr, nowDate);
    EXPECT_EQ(pstr_ex, pstr);
  }
}

TEST(TestUtilities, numberList)
{
  const char* expected_c[13] = { "0.4", "0.5", "0.6", "0.7", "0.8", "0.9", "1", "2", "2.5", "3", "4", "5", "6" };
  const std::vector<std::string> expected(expected_c, expected_c + 13);

  const float enormal[] = { 1., 2., 2.5, 3., 4., 5., 6., 7., 8., 9., -1 };
  const std::vector<std::string> actual = diutil::numberList(1, enormal);
  EXPECT_EQ(expected, actual);
}

// tell googletest how to print QPolygonF objects
// see https://code.google.com/p/googletest/wiki/AdvancedGuide#Teaching_Google_Test_How_to_Print_Your_Values
static void PrintTo(const QPolygonF& polygon, ::std::ostream* out)
{
  *out << '[';
  for (int i=0; i<polygon.size(); ++i)
    *out << '(' << polygon.at(i).x() << ',' << polygon.at(i).y() << ')';
  *out << ']';
}

TEST(TestUtilities, trimToRectangle)
{
  const Rectangle rect(0, 0, 2.5, 2.5);
  const Rectangle rect4(0, 0, 4, 4);

  { QPolygonF polyin1, expect1;
    polyin1 << QPointF(1, 1) // inside
            << QPointF(2, 1) // inside
            << QPointF(3, 1) // right
            << QPointF(4, 1) // right
            << QPointF(5, 1) // right
            << QPointF(5, 3) // right + above
            << QPointF(5, 4) // right + above
            << QPointF(5, 5) // right + above
            << QPointF(4, 5) // right + above
            << QPointF(3, 5) // right + above
            << QPointF(1, 5) // above
            << QPointF(1, 4) // above
            << QPointF(1, 3) // above
            << QPointF(1, 2) // inside
            << QPointF(1, 1);// inside
    expect1 << QPointF(1, 1) // inside
            << QPointF(2, 1) // inside
            << QPointF(3, 1) // right
            << QPointF(5, 1) // right
            << QPointF(5, 3) // right + above
            << QPointF(3, 5) // right + above
            << QPointF(1, 5) // above
            << QPointF(1, 3) // above
            << QPointF(1, 2) // inside
            << QPointF(1, 1);// inside
    const QPolygonF actual1 = diutil::trimToRectangle(rect, polyin1);
    EXPECT_EQ(expect1, actual1);
  }

  { QPolygonF polyin2, expect2;
    polyin2 << QPointF(1, 1) // inside
            << QPointF(5, 1) // right
            << QPointF(5, 3) // right + above
            << QPointF(1, 1);// inside
    expect2 = polyin2;
    const QPolygonF actual2 = diutil::trimToRectangle(rect, polyin2);
    EXPECT_EQ(expect2, actual2);
  }

  { QPolygonF polyin3, expect3;
    polyin3 << QPointF(1, 1) // inside
            << QPointF(5, 1) // right
            << QPointF(1, 3) // inside
            << QPointF(1, 1);// inside
    expect3 = polyin3;
    const QPolygonF actual3 = diutil::trimToRectangle(rect4, polyin3);
    EXPECT_EQ(expect3, actual3);
  }

  { QPolygonF polyin4, expect4;
    polyin4 << QPointF(1, 1) // inside
            << QPointF(3, 1) // inside
            << QPointF(1, 3) // inside
            << QPointF(1, 1);// inside
    expect4 = polyin4;
    const QPolygonF actual4 = diutil::trimToRectangle(rect4, polyin4);
    EXPECT_EQ(expect4, actual4);
  }

  { QPolygonF polyin5, expect5;
    polyin5 << QPointF(1, 1) // inside
            << QPointF(3, 1) // right
            << QPointF(1, 3) // above
            << QPointF(1, 1);// inside
    expect5 = polyin5;
    const QPolygonF actual5 = diutil::trimToRectangle(rect, polyin5);
    EXPECT_EQ(expect5, actual5);
  }
}
