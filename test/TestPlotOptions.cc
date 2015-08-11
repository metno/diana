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
    if (fabs(a[i] - b[i]) > 1e-6)
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
  const float expected1_f[] = { 0.1, 0.2, 0.5, 1, 2, 4, 6, 10, 15, 20, 25, 30, 35, 40 };
  ASSERT_TRUE(test_autoExpandFloatVector("0.1,0.2,0.5,1,2,4,6,10,15,...40",
          vector<float>(expected1_f, boost::end(expected1_f))));

  const float expected2_f[] = { 0.01, 0.02, 0.04, 0.06, 0.10, 0.15, 0.20, 0.25, 0.3, 0.4, 0.5, 1, 2, 5 };
  ASSERT_TRUE(test_autoExpandFloatVector("0.01,0.02,0.04,0.06,0.10,0.15,0.20,0.25,0.3,0.4,0.5,1.,2.,5.",
          vector<float>(expected2_f, boost::end(expected2_f))));

  const float expected3_f[] = { 0.01, 0.03 };
  ASSERT_TRUE(test_autoExpandFloatVector("0.01,0.03",
          vector<float>(expected3_f, boost::end(expected3_f))));

  const float expected4_f[] = { 0.01, 0.03, 0.05, 0.07 };
  ASSERT_TRUE(test_autoExpandFloatVector("0.01,0.03,...0.07",
          vector<float>(expected4_f, boost::end(expected4_f))));
}

TEST(TestPlotOptions, testParsePlotOptionColour)
{
  Colour::define("red",255,0,0,0);
  std::string colstr="colour=red";
  PlotOptions poptions;
  PlotOptions::parsePlotOption(colstr,poptions,true);
  EXPECT_EQ("red",poptions.textcolour.Name());
  EXPECT_EQ(colstr,poptions.toString());
}
