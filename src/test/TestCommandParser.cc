/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013 met.no

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diCommandParser.h>

#include <gtest/gtest.h>

using namespace std;

TEST(TestCommandParser, IsInt)
{
  EXPECT_TRUE(CommandParser::isInt("0"));
  EXPECT_TRUE(CommandParser::isInt("17"));
  EXPECT_TRUE(CommandParser::isInt("-5"));
  EXPECT_TRUE(CommandParser::isInt("+123"));

  EXPECT_FALSE(CommandParser::isInt(" "));
  EXPECT_FALSE(CommandParser::isInt("0 1"));
  EXPECT_FALSE(CommandParser::isInt("0.1"));
  EXPECT_FALSE(CommandParser::isInt("+-1"));
  EXPECT_FALSE(CommandParser::isInt("1e5"));
  EXPECT_FALSE(CommandParser::isInt("1 hello"));
  EXPECT_FALSE(CommandParser::isInt("hello 2"));
}


TEST(TestCommandParser, IsFloat)
{
  EXPECT_TRUE(CommandParser::isFloat("0"));
  EXPECT_TRUE(CommandParser::isFloat("17"));
  EXPECT_TRUE(CommandParser::isFloat("-5"));
  EXPECT_TRUE(CommandParser::isFloat("+123"));

  EXPECT_TRUE(CommandParser::isFloat("0.0"));
  EXPECT_TRUE(CommandParser::isFloat("1.7"));
  EXPECT_TRUE(CommandParser::isFloat("-5.12345"));
  EXPECT_TRUE(CommandParser::isFloat("+1.23"));

  EXPECT_TRUE(CommandParser::isFloat("1e5"));
  EXPECT_TRUE(CommandParser::isFloat("1E5"));
  EXPECT_TRUE(CommandParser::isFloat("1.23E5"));
  EXPECT_TRUE(CommandParser::isFloat("-1.23e5"));

  EXPECT_FALSE(CommandParser::isFloat(" "));
  EXPECT_FALSE(CommandParser::isFloat("0.0 1"));
  EXPECT_FALSE(CommandParser::isFloat("+-1.0"));
  EXPECT_FALSE(CommandParser::isFloat("+-1.0e3"));
}

TEST(TestCommandParser, ParseString)
{
  const char input1[] = "alpha,beta";
  const int N1 = 2;
  const char* expected1_c[N1] = { "alpha", "beta" };
  const vector<string> expected1(expected1_c, expected1_c+N1);
  EXPECT_EQ(expected1, CommandParser::parseString(input1));

  const char input2[] = "\"alpha beta\",\"gamma\",\"delta\"";
  const int N2 = 3;
  const char* expected2_c[N2] = { "alpha beta", "gamma", "delta" };
  const vector<string> expected2(expected2_c, expected2_c+N2);
  EXPECT_EQ(expected2, CommandParser::parseString(input2));

  const char input3[] = "\"alpha,beta\"";
  const int N3 = 1;
  const char* expected3_c[N3] = { "alpha,beta" };
  const vector<string> expected3(expected3_c, expected3_c+N3);
  EXPECT_EQ(expected3, CommandParser::parseString(input3));
}
