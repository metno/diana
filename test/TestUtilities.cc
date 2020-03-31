/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2014-2018 met.no

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
#include <util/charsets.h>
#include <util/diLineMerger.h>
#include <util/format_int.h>
#include <util/nearest_element.h>
#include <util/polygon_util.h>
#include <util/string_util.h>
#include <util/subprocess.h>
#include <util/time_util.h>
#include <diField/diRectangle.h>

#include "util/geo_util.h"

#include <puCtools/puCglob.h> // for GLOB_BRACE

#include <gtest/gtest.h>

#include <cstring>
#include <map>
#include <set>

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
  const std::string filtered = SRC_TEST + "test_utilities_????????????+???H??M";
  const diutil::string_v matches = diutil::glob(filtered, GLOB_BRACE);

  EXPECT_EQ(1, matches.size());
  if (matches.size() >= 1) {
    const std::string ex = SRC_TEST + "test_utilities_201409121200+000H00M";
    EXPECT_EQ(ex, matches.front());
  }
}

TEST(TestUtilities, format_int)
{
  int month = 7, day = 24;
  std::string out = "%m-%d";
  diutil::format_int(month, out, 0, 2, '0');
  diutil::format_int(day,   out, 3, 2, '0');
  diutil::format_int(0,     out, 8, 3, 'x');
  ASSERT_EQ("07-24   xx0", out);
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

TEST(TestUtilities, Latin1ToUtf8)
{
  ASSERT_TRUE(bool(diutil::findConverter(diutil::ISO_8859_1, diutil::UTF_8)));
}

TEST(TestUtilities, Utf8ToLatin1)
{
  diutil::CharsetConverter_p c = diutil::findConverter(diutil::UTF_8, diutil::ISO_8859_1);
  ASSERT_TRUE(bool(c));

  EXPECT_EQ("bl\xE5" "b\xE6" "r", c->convert("blåbær"));
  EXPECT_EQ("\xC5r\xF8", c->convert("Årø"));
}

TEST(TestUtilities, GetLineConverterUTF8)
{
  std::istringstream stream(
        "# -*- coding: utf-8 -*-\n"
        "blueberry = blåbær\n"
        );
  diutil::GetLineConverter convertline("#");
  std::string l, found;
  while (convertline(stream, l)) {
    if (l.empty() || l[0]=='#')
      continue;
    found = l;
  }
  EXPECT_EQ("blueberry = blåbær", found);
}

TEST(TestUtilities, GetLineConverterLatin1)
{
  std::istringstream stream(
        // no coding specified, expecting iso8859-1
        "blue = bl\xE5\n"
        );
  diutil::GetLineConverter convertline("#");
  std::string l, found;
  while (convertline(stream, l)) {
    if (l.empty() || l[0]=='#')
      continue;
    found = l;
  }
  EXPECT_EQ("blue = bl\xC3\xA5", found); // this is utf-8
}

TEST(TestUtilities, GreatCircleDistance)
{
  const float BLINDERN_LON = 10.72005f, BLINDERN_LAT = 59.9423f;
  const float FANNARAK_LON =  7.9058f,  FANNARAK_LAT = 61.5158f;

  EXPECT_NEAR(232400, diutil::GreatCircleDistance(BLINDERN_LAT, FANNARAK_LAT, BLINDERN_LON, FANNARAK_LON), 100);
  EXPECT_NEAR(222400, diutil::GreatCircleDistance(89, 89, 10, 190), 100);
}

TEST(TestUtilities, Execute)
{
  QByteArray output;
  EXPECT_NE(0, diutil::execute("thIs_c0mm&_Is_unlIkely_t0_be_f0und_anywheRe", QStringList(), &output));
  EXPECT_TRUE(output.isEmpty());

  EXPECT_EQ(0, diutil::execute("true", QStringList(), &output));
  EXPECT_TRUE(output.isEmpty());

  EXPECT_EQ(1, diutil::execute("false", QStringList(), &output));
  EXPECT_TRUE(output.isEmpty());
}

TEST(TestUtilities, LineMerger)
{
  diutil::LineMerger lm;
  EXPECT_TRUE(lm.push("hei"));

  EXPECT_FALSE(lm.push("hell\\"));
  EXPECT_FALSE(lm.push("o, w\\"));
  EXPECT_TRUE(lm.push("orld!"));
  EXPECT_EQ("hello, world!", lm.mergedline());

  EXPECT_FALSE(lm.push("banana\\"));
  EXPECT_TRUE(lm.push(""));
  EXPECT_EQ("banana", lm.mergedline());

  EXPECT_TRUE(lm.push(""));
  EXPECT_EQ("", lm.mergedline());

  EXPECT_TRUE(lm.push("\\ok"));
  EXPECT_EQ("\\ok", lm.mergedline());
}

TEST(TestUtilities, LineMerger2)
{
  std::vector<std::string> lines;
  std::vector<size_t> linenos;
  diutil::LineMerger lm;
  std::istringstream is("good \\\nstart\n\ngood\\\n end\n");
  std::string line;
  while (std::getline(is, line)) {
    if (lm.push(line)) {
      lines.push_back(lm.mergedline());
      linenos.push_back(lm.lineno());
    }
  }
  if (!lm.complete()) {
    lines.push_back(lm.mergedline());
    linenos.push_back(lm.lineno());
  }

  ASSERT_EQ(3, lines.size());
  EXPECT_EQ("good start", lines[0]);
  EXPECT_EQ(1, linenos[0]);
  EXPECT_EQ("good end", lines[2]);
  EXPECT_EQ(4, linenos[2]);
}

TEST(TestUtilities, LineMerger3)
{
  std::vector<std::string> lines;
  std::vector<size_t> linenos;
  diutil::LineMerger lm;
  std::istringstream is("good \\\nstart\nbad end\\");
  std::string line;
  while (std::getline(is, line)) {
    if (lm.push(line)) {
      lines.push_back(lm.mergedline());
      linenos.push_back(lm.lineno());
    }
  }
  if (!lm.complete()) {
    lines.push_back(lm.mergedline());
    linenos.push_back(lm.lineno());
  }

  ASSERT_EQ(2, lines.size());
  EXPECT_EQ("good start", lines[0]);
  EXPECT_EQ(1, linenos[0]);
  EXPECT_EQ("bad end", lines[1]);
  EXPECT_EQ(3, linenos[1]);
}

TEST(TestUtilities, RemoveCommentAndTrim)
{
  {
    std::string t1 = "hei";
    diutil::remove_comment_and_trim(t1);
    EXPECT_EQ("hei", t1);
  }
  {
    std::string t1 = "  yes # no ";
    diutil::remove_comment_and_trim(t1);
    EXPECT_EQ("yes", t1);
  }
  {
    std::string t1 = "  hello  world ";
    diutil::remove_comment_and_trim(t1);
    EXPECT_EQ("hello  world", t1);
  }
}

TEST(TestUtilities, RemoveQuote)
{
  {
    std::string t1 = "\"hei\"";
    diutil::remove_quote(t1);
    EXPECT_EQ("hei", t1);
  }
  {
    std::string t1 = "\"no\" ", ex = t1;
    diutil::remove_quote(t1);
    EXPECT_EQ(ex, t1); // unchanged, space at end
  }
  {
    std::string t1 = "\"hello world", ex = t1;
    diutil::remove_quote(t1);
    EXPECT_EQ(ex, t1); // unchanged, no quote at end
  }
  {
    std::string t1 = "\"", ex = t1;
    diutil::remove_quote(t1);
    EXPECT_EQ(ex, t1); // unchanged, too short
  }
}

TEST(TestUtilities, RemoveStartEnd)
{
  {
    std::string t1 = "[hei]";
    diutil::remove_start_end_mark(t1, '[', ']');
    EXPECT_EQ("hei", t1);
  }
  {
    std::string t1 = "[hei";
    diutil::remove_start_end_mark(t1, '[', ']');
    EXPECT_EQ("hei", t1);
  }
  {
    std::string t1 = "\"no\" ";
    diutil::remove_start_end_mark(t1);
    EXPECT_EQ("no\" ", t1); // weird, only start removed
  }
  {
    std::string t1 = "hello world\"", ex = t1;
    diutil::remove_start_end_mark(t1);
    EXPECT_EQ(ex, t1); // unchanged, no start mark
  }
}

TEST(TestUtilities, AppendText)
{
  {
    std::string t = "";
    diutil::appendText(t, "world");
    EXPECT_EQ("world", t);
  }
  {
    std::string t = "";
    diutil::appendText(t, "world", "-");
    EXPECT_EQ("world", t);
  }
  {
    std::string t = "hello";
    diutil::appendText(t, "world", "-");
    EXPECT_EQ("hello-world", t);
  }
}

TEST(TestUtilities, NearestElementSet)
{
  using diutil::nearest_element;

  const std::set<int> data = {2, 5, 7, 13};
  EXPECT_EQ(data.begin(), nearest_element(data, 0));
  EXPECT_EQ(data.begin(), nearest_element(data, 3));
  EXPECT_EQ(++data.begin(), nearest_element(data, 4));
  EXPECT_EQ(--data.end(), nearest_element(data, 14));
}

TEST(TestUtilities, NearestElementMap)
{
  using diutil::nearest_element;

  const std::map<int, int> data = {{2, 0}, {5, 0}, {7, 0}, {13, 0}};
  EXPECT_EQ(data.begin(), nearest_element(data, 0));
  EXPECT_EQ(data.begin(), nearest_element(data, 3));
  EXPECT_EQ(++data.begin(), nearest_element(data, 4));
  EXPECT_EQ(--data.end(), nearest_element(data, 14));
}

TEST(TestUtilities, NearestElementMiTime)
{
  using diutil::nearest_element;
  using namespace miutil;

  const miTime t0(2018, 4, 1, 0, 0, 0);
  std::map<miTime, int> data;
  data.insert(std::make_pair(addHour(t0, 2), 0));
  data.insert(std::make_pair(addHour(t0, 5), 0));
  data.insert(std::make_pair(addHour(t0, 7), 0));
  data.insert(std::make_pair(addHour(t0, 13), 0));

  EXPECT_EQ(data.begin(), nearest_element(data, addHour(t0, 0), miTime::minDiff));
  EXPECT_EQ(data.begin(), nearest_element(data, addHour(t0, 3), miTime::minDiff));
  EXPECT_EQ(++data.begin(), nearest_element(data, addHour(t0, 4), miTime::minDiff));
  EXPECT_EQ(--data.end(), nearest_element(data, addHour(t0, 14), miTime::minDiff));
}

static plottimes_t makeTimes()
{
  plottimes_t times;
  const miutil::miTime t0(2018, 9, 9, 0, 0), t1 = miutil::addHour(t0, 24);
  for (miutil::miTime t = t0; t <= t1; t.addHour(3))
    times.insert(t);
  return times;
}

TEST(TestUtilities, StepTimeN)
{
  const plottimes_t times = makeTimes();
  const miutil::miTime t0600(2018, 9, 9, 6, 0);

  using namespace miutil;
  plottimes_t::const_iterator it;

  EXPECT_EQ(9, times.size());

  it = step_time(times, t0600, 2);
  EXPECT_TRUE(it != times.end() && *it == miutil::miTime(2018, 9, 9, 12, 0));

  it = step_time(times, t0600, -2);
  EXPECT_TRUE(it != times.end() && *it == miutil::miTime(2018, 9, 9, 0, 0));

  EXPECT_EQ(times.begin(), step_time(times, t0600, -4));
  EXPECT_EQ(--times.end(), step_time(times, t0600, 12));
}

TEST(TestUtilities, StepTimeT)
{
  const plottimes_t times = makeTimes();
  const miutil::miTime t0600(2018, 9, 9, 6, 0);

  using namespace miutil;
  plottimes_t::const_iterator it;

  EXPECT_EQ(9, times.size());

  it = step_time(times, t0600, addHour(t0600, 2));
  EXPECT_TRUE(it != times.end() && *it == miutil::miTime(2018, 9, 9, 9, 0));

  it = step_time(times, t0600, addHour(t0600, -2));
  EXPECT_TRUE(it != times.end() && *it == miutil::miTime(2018, 9, 9, 3, 0));

  EXPECT_EQ(times.begin(), step_time(times, t0600, addHour(t0600, -12)));
  EXPECT_EQ(--times.end(), step_time(times, t0600, addHour(t0600, 24)));
}

TEST(TestUtilities, StringFromTime)
{
  using namespace miutil;

  const miTime t0(2018, 4, 1, 12, 34, 56);
  EXPECT_EQ("2018040112", stringFromTime(t0, false));
  EXPECT_EQ("201804011234", stringFromTime(t0, true));
}

TEST(TestUtilities, TimeFromString)
{
  using namespace miutil;
  EXPECT_TRUE(timeFromString("201804").undef());
  EXPECT_TRUE(timeFromString("201804-1").undef());
  EXPECT_TRUE(timeFromString("20180401").undef());
  EXPECT_EQ(miTime(2018, 4, 1, 12, 0, 0), timeFromString("2018040112"));
  EXPECT_EQ(miTime(2018, 4, 1, 12, 34, 0), timeFromString("201804011234"));
}

TEST(TestUtilities, TestCaseInsensitiveCompare)
{
  using diutil::icompare;
  EXPECT_EQ(icompare("FISH", "fish"), 0);
  EXPECT_GT(icompare("FISHERMAN", "fish"), 0);
  EXPECT_LT(icompare("fish", "FISHerman"), 0);
  EXPECT_LT(icompare("dog", "fish"), 0);
#if 0
  const char rock[5] = "r\0ck", roll[5] = "r\0ll";
  const std::string r0ck(rock, rock+4), r0ll(roll, roll + 4);
  EXPECT_LT(icompare(r0ck, r0ll), 0);
#endif

  using diutil::iequals;
  EXPECT_TRUE(iequals("fish", "fish"));
  EXPECT_TRUE(iequals("FISH", "FISH"));
  EXPECT_TRUE(iequals("FISH", "fish"));
  EXPECT_TRUE(iequals("FIsH", "fiSh"));

  using diutil::icontains;
  EXPECT_TRUE(icontains("FiShErMaN", "fish"));
  EXPECT_TRUE(icontains("FiShErMaN", "is"));
  EXPECT_FALSE(icontains("FiShErMaN", "not"));
  EXPECT_TRUE(icontains("FiShErMaN", "mAn"));

  using diutil::op_iless;
  typedef std::map<std::string, bool, op_iless> ci_map_t;
  ci_map_t animal_is_fish;
  animal_is_fish.insert(std::make_pair("herring", true));
  animal_is_fish.insert(std::make_pair("elEphanT", false));

  EXPECT_TRUE(animal_is_fish.find("herring") != animal_is_fish.end());
  EXPECT_TRUE(animal_is_fish.find("HERRING") != animal_is_fish.end());
  EXPECT_TRUE(animal_is_fish.find("ring") == animal_is_fish.end());
  EXPECT_TRUE(animal_is_fish.find("HORSE") == animal_is_fish.end());
}
