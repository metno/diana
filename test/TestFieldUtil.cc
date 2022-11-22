/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2022 met.no

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

#include <diField/diField.h>
#include <diFieldUtil.h>

#include <gtest/gtest.h>

TEST(FieldUtil, ConvertUnitKC)
{
  Field_p input = std::make_shared<Field>();
  input->reserve(4, 4);
  input->fill(10);
  input->unit = "degC";
  ASSERT_EQ(miutil::ALL_DEFINED, input->defined());

  Field_p output = convertUnit(input, "K");
  ASSERT_TRUE(output != nullptr);
  EXPECT_EQ(miutil::ALL_DEFINED, output->defined());
  EXPECT_EQ("K", output->unit);
  EXPECT_NEAR(input->data[0] + 273.15, output->data[0], 0.125);
}

TEST(FieldUtil, ConvertUnitSpecial)
{
  EXPECT_FALSE(convertUnit(nullptr, "cm"));

  Field_p input = std::make_shared<Field>();
  input->reserve(4, 4);
  input->fill(10);
  input->unit = "";

  Field_p output = convertUnit(input, "K");
  ASSERT_TRUE(output != nullptr);
  EXPECT_EQ(input.get(), output.get());
}

TEST(FieldUtil, MergeSetupAndQuickMenuOptions)
{
  const miutil::KeyValue_v setup{{"line.interval", "5"}, {"line.width", "2"}};
  const miutil::KeyValue_v qmenu{{"line.values", "1,2,3,4,5"}, {"line.width", "3"}};

  const miutil::KeyValue_v expect{setup[1], qmenu[0], qmenu[1]};
  const auto actual = mergeSetupAndQuickMenuOptions(setup, qmenu);
  EXPECT_EQ(actual, expect);
}
