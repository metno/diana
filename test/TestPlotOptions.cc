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

#include <diPlotOptions.h>

#include <boost/range/end.hpp>
//#include <algorithm>
//#include <iterator>
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
  const float expected1_f[] = { 0.1f, 0.2f, 0.5f, 1, 2, 4, 6, 10, 15, 20, 25, 30, 35, 40 };
  ASSERT_TRUE(test_autoExpandFloatVector("0.1,0.2,0.5,1,2,4,6,10,15,...40",
          vector<float>(expected1_f, boost::end(expected1_f))));

  const float expected2_f[] = { 0.01f, 0.02f, 0.04f, 0.06f, 0.10f, 0.15f, 0.20f, 0.25f, 0.3f, 0.4f, 0.5f, 1, 2, 5 };
  ASSERT_TRUE(test_autoExpandFloatVector("0.01,0.02,0.04,0.06,0.10,0.15,0.20,0.25,0.3,0.4,0.5,1.,2.,5.",
          vector<float>(expected2_f, boost::end(expected2_f))));

  const float expected3_f[] = { 0.01f, 0.03f };
  ASSERT_TRUE(test_autoExpandFloatVector("0.01,0.03",
          vector<float>(expected3_f, boost::end(expected3_f))));

  const float expected4_f[] = { 0.01f, 0.03f, 0.05f, 0.07f };
  ASSERT_TRUE(test_autoExpandFloatVector("0.01,0.03,...0.07",
          vector<float>(expected4_f, boost::end(expected4_f))));
}

TEST(TestPlotOptions, testParsePlotOptionColour)
{
  Colour::define("red",255,0,0,0);
  const miutil::KeyValue_v cols(1, miutil::KeyValue("colour", "red"));
  miutil::KeyValue_v unused;
  PlotOptions poptions;
  PlotOptions::parsePlotOption(cols,poptions,unused);
  EXPECT_EQ("red",poptions.textcolour.Name());
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

// example FIELD command:
// FIELD (  model=AROME-MetCoOp refhour=6 plot=T.2M vcoord=height vlevel=2m -  model=AROME-MetCoOp refhour=0 plot=T.2M vcoord=height vlevel=2m ) colour=red plottype=fill_cell linetype=solid linewidth=1 base=0 frame=0 line.interval=0.1 extreme.type=None extreme.size=1 extreme.radius=1 palettecolours=lightred,lightblue patterns=off table=1 repeat=0 value.label=1 line.smooth=0 field.smooth=0 label.size=1 grid.lines=0 grid.lines.max=0 undef.masking=1 undef.colour=255:255:255:255 undef.linewidth=1 undef.linetype=solid grid.value=0 colour_2=off dim=1 unit=celsius

TEST(TestPlotOptions, testParseFieldPlotOption)
{
  const miutil::KeyValue_v in_field = miutil::splitKeyValue("( model=AROME-MetCoOp refhour=6 plot=T.2M vlevel=2m - model=AROME-MetCoOp refhour=0 plot=T.2M vlevel=2m )");
  const miutil::KeyValue_v in_options = miutil::splitKeyValue("linetype=solid linewidth=5 plottype=fill_cell");
  miutil::KeyValue_v out_field;
  miutil::KeyValue_v input = in_field;
  input.insert(input.end(), in_options.begin(), in_options.end());
  PlotOptions poptions;
  PlotOptions::parsePlotOption(input, poptions, out_field);
  EXPECT_EQ(5, poptions.linewidth);
  EXPECT_EQ("solid", poptions.linetype.name);
  EXPECT_EQ(in_field, out_field);
}

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
