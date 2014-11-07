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
#include <diField/TimeFilter.h>
#include <puCtools/puCglob.h> // for GLOB_BRACE
#include <gtest/gtest.h>
#include <cstring>

static const std::string SRC_TEST = std::string(TOP_SRCDIR) + "/src/test/";

TEST(TestUtilities, StartsWith)
{
  EXPECT_TRUE(diutil::startswith("fish", ""));
  EXPECT_TRUE(diutil::startswith("fish", "fi"));

  EXPECT_FALSE(diutil::startswith("fish", "sh"));
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
