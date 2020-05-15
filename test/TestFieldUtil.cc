/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2020 met.no

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

#include <diFieldUtil.h>
#include <gtest/gtest.h>

using miutil::kv;

TEST(FieldUtil, mergeFieldOptions1)
{
  miutil::KeyValue_v cmdopts, setupopts;
  cmdopts << kv("colour", "red") << kv("plottype", "fill_cell") << kv("linewidth", 2) << kv("line.values", "1,2,4,8");
  setupopts << kv("colour", "blue") << kv("plottype", "contour") << kv("linetype", "solid") << kv("linewidth", 1) << kv("line.interval", 2);

  miutil::KeyValue_v expectedopts = cmdopts;
  expectedopts.insert(expectedopts.begin() + 2, setupopts[2]); // linetype.solid is the only one that is merged

  mergeFieldOptions(cmdopts, setupopts);

  ASSERT_EQ(expectedopts.size(), cmdopts.size()) << "expected: " << expectedopts << " actual:" << cmdopts;
  for (size_t i = 0; i < expectedopts.size(); ++i) {
    EXPECT_EQ(expectedopts[i], cmdopts[i]) << "i=" << i;
  }
}

TEST(FieldUtil, mergeFieldOptions2)
{
  miutil::KeyValue_v cmdopts, setupopts;
  cmdopts << kv("colour", "red") << kv("plottype", "fill_cell") << kv("linewidth", 2) << kv("log.line.values", "0.1,0.2,0.5");
  setupopts << kv("colour", "blue") << kv("plottype", "contour") << kv("linetype", "solid") << kv("linewidth", 1) << kv("line.interval", 2);

  miutil::KeyValue_v expectedopts = cmdopts;
  expectedopts.insert(expectedopts.begin() + 2, setupopts[2]); // linetype.solid is the only one that is merged

  mergeFieldOptions(cmdopts, setupopts);

  ASSERT_EQ(expectedopts.size(), cmdopts.size()) << "expected: " << expectedopts << " actual:" << cmdopts;
  for (size_t i = 0; i < expectedopts.size(); ++i) {
    EXPECT_EQ(expectedopts[i], cmdopts[i]) << "i=" << i;
  }
}

TEST(FieldUtil, mergeFieldOptions3)
{
  miutil::KeyValue_v cmdopts, setupopts;
  cmdopts << kv("colour", "red") << kv("plottype", "fill_cell") << kv("line.interval", 1);
  setupopts << kv("colour", "blue") << kv("plottype", "contour") << kv("linetype", "solid") << kv("linewidth", 1) << kv("line.interval", 2);

  miutil::KeyValue_v expectedopts = cmdopts;
  expectedopts.insert(expectedopts.begin() + 2, setupopts[2]); // linetype is merged
  expectedopts.insert(expectedopts.begin() + 3, setupopts[3]); // linewidth is merged

  mergeFieldOptions(cmdopts, setupopts);

  ASSERT_EQ(expectedopts.size(), cmdopts.size()) << "expected: " << expectedopts << " actual:" << cmdopts;
  for (size_t i = 0; i < expectedopts.size(); ++i) {
    EXPECT_EQ(expectedopts[i], cmdopts[i]) << "i=" << i;
  }
}

TEST(FieldUtil, mergeFieldOptions4)
{
  miutil::KeyValue_v cmdopts, setupopts;
  cmdopts << kv("colour", "red") << kv("line.values", "1,2,3") << kv("log.line.values", "1,10,100");
  setupopts << kv("colour", "blue") << kv("line.values", "10,11,12") << kv("log.line.values", "2,4,8");

  const miutil::KeyValue_v expectedopts = cmdopts;

  mergeFieldOptions(cmdopts, setupopts);

  ASSERT_EQ(expectedopts.size(), cmdopts.size()) << "expected: " << expectedopts << " actual:" << cmdopts;
  for (size_t i = 0; i < expectedopts.size(); ++i) {
    EXPECT_EQ(expectedopts[i], cmdopts[i]) << "i=" << i;
  }
}
