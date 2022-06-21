/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013-2022 met.no

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

#include <diPlotOptions.h>

#include <mi_fieldcalc/FieldDefined.h>

#include <vector>
#include <cmath>

#include <gtest/gtest.h>

using namespace std;

static bool equal_vectors(const vector<float>& a, const vector<float>& b)
{
  if (a.size() != b.size())
    return false;
  for (size_t i=0; i<a.size(); ++i)
    if (std::abs(a[i] - b[i]) > 1e-6)
      return false;
  return true;
}

static bool test_autoExpandFloatVector(const string& text, const vector<float>& expected)
{
  const vector<float> actual = PlotOptions::autoExpandFloatVector(text);
  if (not equal_vectors(actual, expected)) {
    //cout << "error with '" << text << "', unexpected result ";
    //copy(actual.begin(), actual.end(), ostream_iterator<float>(cout, ","));
    //cout << " size=" << actual.size() << endl;
    return false;
  }
  return true;
}

TEST(TestPlotOptions, AutoExpand)
{
  const vector<float> expected1{0.1f, 0.2f, 0.5f, 1, 2, 4, 6, 10, 15, 20, 25, 30, 35, 40};
  ASSERT_TRUE(test_autoExpandFloatVector("0.1,0.2,0.5,1,2,4,6,10,15,...40", expected1));

  const vector<float> expected2{0.01f, 0.02f, 0.04f, 0.06f, 0.10f, 0.15f, 0.20f, 0.25f, 0.3f, 0.4f, 0.5f, 1, 2, 5};
  ASSERT_TRUE(test_autoExpandFloatVector("0.01,0.02,0.04,0.06,0.10,0.15,0.20,0.25,0.3,0.4,0.5,1.,2.,5.", expected2));

  const vector<float> expected3{0.01f, 0.03f};
  ASSERT_TRUE(test_autoExpandFloatVector("0.01,0.03", expected3));

  const vector<float> expected4{0.01f, 0.03f, 0.05f, 0.07f};
  ASSERT_TRUE(test_autoExpandFloatVector("0.01,0.03,...0.07", expected4));
}

TEST(TestPlotOptions, testParsePlotOptionColour)
{
  Colour::define("red",255,0,0,0);
  const miutil::KeyValue_v cols(1, miutil::KeyValue("colour", "red"));
  miutil::KeyValue_v unused;
  PlotOptions poptions;
  PlotOptions::parsePlotOption(cols,poptions,unused);
  EXPECT_EQ("red", poptions.linecolour.Name());
  EXPECT_TRUE(unused.empty());
}

TEST(TestPlotOptions, testParsePlotOptionUnused)
{
  miutil::KeyValue_v un;
  miutil::add(un, "nosuchoption", "true");
  un.push_back(miutil::KeyValue("thisoptiondoesnotexist"));

  const miutil::KeyValue_v lw(1, miutil::KeyValue("linewidth", "5"));

  miutil::KeyValue_v postr = un;
  postr.insert(postr.end(), lw.begin(), lw.end());

  miutil::KeyValue_v unused;
  PlotOptions poptions;
  PlotOptions::parsePlotOption(postr, poptions, unused);
  EXPECT_EQ(5, poptions.linewidth);
  EXPECT_EQ(un, unused);
}

#if 0
TEST(TestPlotOptions, testParsePlotOptionDuplicateUnused)
{
  const miutil::KeyValue_v un {
    miutil::kv("nosuchoption", "no"),
    miutil::kv("notanoptioneither", "17"),
    miutil::kv("nosuchoption", "yes")
  };

  miutil::KeyValue_v unused;
  PlotOptions poptions;
  PlotOptions::parsePlotOption(un, poptions, unused);
  ASSERT_EQ(2, unused.size());
  EXPECT_EQ("yes", unused[0].value());
  EXPECT_EQ("17", unused[1].value());
}
#endif

TEST(TestPlotOptions, SplitKeyValueKeepQuotes)
{
  // quote only at start => quotes not stripped
  const std::string vQS = "\"this text\",is=simple";
  // quotes at start and end => quotes stripped
  const std::string vQSE = "\"this text\",is,a=\"bit complicated\"";
  const std::string in = "LABEL text1=" + vQS + " text2=" + vQSE + " tcolour=blue";
  miutil::KeyValue_v annokv = miutil::splitKeyValue(in, true);
  ASSERT_EQ(4, annokv.size());
  EXPECT_EQ("LABEL", annokv[0].key());
  EXPECT_EQ(vQS, annokv[1].value());
  EXPECT_EQ(vQSE, annokv[2].value());
  EXPECT_EQ("blue", annokv[3].value());

  EXPECT_EQ(in, miutil::mergeKeyValue(annokv));
}

TEST(TestPlotOptions, DiffSimple)
{
  PlotOptions poA, poB;
  poB.linewidth *= 2;

  const auto diff = PlotOptions::diff(poA, poB);
  EXPECT_EQ(miutil::mergeKeyValue(diff), "linewidth=2");
}

TEST(TestPlotOptions, DiffPatterns)
{
  PlotOptions poA, poB;
  PlotOptions::parsePlotOption(miutil::kv(PlotOptions::key_patterns, "dashed"), poB);

  EXPECT_EQ(miutil::mergeKeyValue(PlotOptions::diff(poA, poB)), "patterns=dashed");
  EXPECT_EQ(miutil::mergeKeyValue(PlotOptions::diff(poB, poA)), "patterns=off");

  const PlotOptions po1 = PlotOptions(poA).apply(PlotOptions::diff(poA, poB));
  EXPECT_EQ(po1, poB);

  const PlotOptions po2 = PlotOptions(poB).apply(PlotOptions::diff(poB, poA));
  EXPECT_EQ(po2, poA);
}

TEST(TestPlotOptions, DiffPaletteColours)
{
  PlotOptions poA, poB;
  PlotOptions::parsePlotOption(miutil::kv(PlotOptions::key_palettecolours, "light_red"), poB);

  EXPECT_EQ(miutil::mergeKeyValue(PlotOptions::diff(poA, poB)), "palettecolours=light_red");
  EXPECT_EQ(miutil::mergeKeyValue(PlotOptions::diff(poB, poA)), "palettecolours=off");

  {
    const PlotOptions po = PlotOptions(poA).apply(poA.diffTo(poB));
    EXPECT_EQ(po.palettename, "light_red");
    EXPECT_TRUE(po.contourShading);
    EXPECT_EQ(po, poB);
  }
  {
    const PlotOptions po = PlotOptions(poB).apply(poA.diffFrom(poB));
    EXPECT_TRUE(po.palettename.empty());
    EXPECT_FALSE(po.contourShading);
    EXPECT_EQ(po, poA);
  }
}

TEST(TestPlotOptions, DiffColours1)
{
  const PlotOptions poA = PlotOptions().apply(miutil::kv(PlotOptions::key_colour, "255:0:0")).apply(miutil::kv(PlotOptions::key_colour_2, "0:255:0"));
  EXPECT_TRUE(poA.options_1);
  EXPECT_TRUE(poA.options_2);

  const PlotOptions poB = PlotOptions().apply(miutil::kv(PlotOptions::key_bcolour, "0:0:255"));
  EXPECT_TRUE(poB.options_1);
  EXPECT_FALSE(poB.options_2);

  {
    const PlotOptions po = PlotOptions(poA).apply(poA.diffTo(poB));
    EXPECT_EQ(po, poB);
  }
  {
    const PlotOptions po = PlotOptions(poB).apply(poA.diffFrom(poB));
    EXPECT_EQ(po, poA);
  }
}

TEST(TestPlotOptions, DiffColours2)
{
  const PlotOptions poA = PlotOptions().apply(miutil::kv(PlotOptions::key_lcolour, "255:0:0")).apply(miutil::kv(PlotOptions::key_colour_2, "0:255:0"));
  const PlotOptions poB = PlotOptions().apply(miutil::kv(PlotOptions::key_colour, "0:0:255"));

  {
    const PlotOptions po = PlotOptions(poA).apply(poA.diffTo(poB));
    EXPECT_EQ(po, poB);
  }
  {
    const PlotOptions po = PlotOptions(poB).apply(poA.diffFrom(poB));
    EXPECT_EQ(po, poA);
  }
}

TEST(TestPlotOptions, DiffMinMax)
{
  const PlotOptions poA = PlotOptions().apply(miutil::kv(PlotOptions::key_minvalue, "17"));
  const PlotOptions poB = PlotOptions().apply(miutil::kv(PlotOptions::key_maxvalue_2, "255"));

  {
    const PlotOptions po = PlotOptions(poA).apply(poA.diffTo(poB));
    ASSERT_EQ(po.minvalue, -fieldUndef);
    EXPECT_EQ(po, poB);
  }
  {
    const PlotOptions po = PlotOptions(poB).apply(poA.diffFrom(poB));
    ASSERT_EQ(po.maxvalue_2, fieldUndef);
    EXPECT_EQ(po, poA);
  }
}

TEST(TestPlotOptions, DiffLineIntervalValues)
{
  PlotOptions po;
  EXPECT_EQ(0, po.lineinterval);
  // default must yield empty string
  EXPECT_EQ(miutil::mergeKeyValue(po.toKeyValueList()), "");
  EXPECT_FALSE(po.use_lineinterval());
  EXPECT_FALSE(po.use_linevalues());
  EXPECT_FALSE(po.use_loglinevalues());

  po.set_linevalues("1,2,3");
  // must omit line.interval=0 as this is a side-effect of line.values=...
  EXPECT_EQ(miutil::mergeKeyValue(po.toKeyValueList()), "line.values=1,2,3");
  EXPECT_FALSE(po.use_lineinterval());
  EXPECT_TRUE(po.use_linevalues());
  EXPECT_FALSE(po.use_loglinevalues());

  po.set_lineinterval(10);
  // must emit line.values first, then line.interval=10 even though this is the default
  EXPECT_EQ(miutil::mergeKeyValue(po.toKeyValueList()), "line.interval=10 line.values=1,2,3");
  EXPECT_TRUE(po.use_lineinterval());
  EXPECT_FALSE(po.use_linevalues());
  EXPECT_FALSE(po.use_loglinevalues());

  po.set_loglinevalues("1,2,4");
  // must emit line.values and line.logvalues, and line.interval=10
  EXPECT_EQ(miutil::mergeKeyValue(po.toKeyValueList()), "line.interval=10 line.values=1,2,3 log.line.values=1,2,4");
  EXPECT_TRUE(po.use_lineinterval());
  EXPECT_FALSE(po.use_linevalues());
  EXPECT_FALSE(po.use_loglinevalues());

  po.set_lineinterval(0);
  // must emit line.values first, but omit line.interval=0 as this is the default
  EXPECT_EQ(miutil::mergeKeyValue(po.toKeyValueList()), "line.values=1,2,3 log.line.values=1,2,4");
  EXPECT_FALSE(po.use_lineinterval());
  EXPECT_FALSE(po.use_linevalues());
  EXPECT_TRUE(po.use_loglinevalues());
}

TEST(TestPlotOptions, LineInterval2)
{
  PlotOptions po;
  po.set_lineinterval_2("1");
  EXPECT_EQ(1, po.lineinterval_2);

  po.set_lineinterval_2("Off");
  EXPECT_EQ(0, po.lineinterval_2);
  po.set_lineinterval_2("Av");
  EXPECT_EQ(0, po.lineinterval_2);
}

TEST(TestPlotOptions, ColourForLcolour)
{
  const std::string RED = "255:0:0:255";
  const std::string RGB = RED + ",0:255:0:255,0:0:255:255";
  const std::string OP1_OFF = "options.1=false";

  {
    PlotOptions po;
    po.linecolour = Colour::RED;
    EXPECT_EQ("colour=" + RED, miutil::mergeKeyValue(po.toKeyValueList()));

    po.options_1 = false;
    EXPECT_EQ(OP1_OFF + " lcolour=" + RED, miutil::mergeKeyValue(po.toKeyValueList()));

    po.linecolour = Colour::BLACK;
    EXPECT_EQ(OP1_OFF, miutil::mergeKeyValue(po.toKeyValueList()));
  }
  {
    PlotOptions po;
    po.set_colour("off");
    EXPECT_EQ(OP1_OFF, miutil::mergeKeyValue(po.toKeyValueList()));

    po.linecolour = Colour::RED;
    EXPECT_EQ(OP1_OFF + " lcolour=" + RED, miutil::mergeKeyValue(po.toKeyValueList()));
  }
  {
    PlotOptions po;
    po.linecolour = Colour::RED;
    po.set_colour("off");
    EXPECT_EQ(OP1_OFF + " lcolour=" + RED, miutil::mergeKeyValue(po.toKeyValueList()));
  }
  {
    PlotOptions po;
    po.set_colours(RGB);
    po.linecolour = Colour::RED;
    EXPECT_EQ("colours=" + RGB + " colour=" + RED, miutil::mergeKeyValue(po.toKeyValueList()));

    po.options_1 = false;
    EXPECT_EQ("colours=" + RGB + " " + OP1_OFF + " lcolour=" + RED, miutil::mergeKeyValue(po.toKeyValueList()));

    po.linecolour = Colour::BLACK;
    EXPECT_EQ("colours=" + RGB + " " + OP1_OFF, miutil::mergeKeyValue(po.toKeyValueList()));
  }
  {
    PlotOptions po;
    po.set_colours(RGB);
    po.options_1 = false;
    EXPECT_EQ("colours=" + RGB + " " + OP1_OFF, miutil::mergeKeyValue(po.toKeyValueList()));
  }
  {
    PlotOptions po;
    po.options_1 = false;
    po.set_colours(RGB); // sets options_1 to true
    EXPECT_EQ("colours=" + RGB, miutil::mergeKeyValue(po.toKeyValueList()));
  }
}