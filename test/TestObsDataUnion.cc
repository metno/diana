/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2022 met.no

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

#include <diObsDataUnion.h>
#include <diObsDataVector.h>

#include <algorithm>

#include <gtest/gtest.h>

TEST(TestObsDataUnion, Basic)
{
  auto odu = std::make_shared<ObsDataUnion>();
  {
    auto od = std::make_shared<ObsDataVector>();
    od->basic(0).id = "one-0";
    od->basic(1).id = "one-1";
    odu->add(od);
  }

  {
    auto od = std::make_shared<ObsDataVector>();
    od->basic(0).id = "two-0";
    odu->add(od);
  }

  {
    auto od = std::make_shared<ObsDataVector>();
    od->basic(0).id = "three-0";
    od->basic(1).id = "three-1";
    od->basic(2).id = "three-2";
    od->basic(3).id = "three-3";
    odu->add(od);
  }

  EXPECT_FALSE(odu->empty());
  ASSERT_EQ(7, odu->size());
  ASSERT_EQ("one-0", odu->basic(0).id);
  ASSERT_EQ("two-0", odu->basic(2).id);
  ASSERT_EQ("three-0", odu->basic(3).id);
  ASSERT_EQ("three-3", odu->basic(6).id);
  EXPECT_THROW(odu->basic(17), std::runtime_error);
}

TEST(TestObsDataUnion, Keys)
{
  auto odu = std::make_shared<ObsDataUnion>();
  {
    auto od = std::make_shared<ObsDataVector>();
    od->put_float(0, od->add_key("k-0"), 100);
    od->put_float(0, od->add_key("k-1"), 101);
    od->put_float(1, od->add_key("k-0"), 110);
    od->put_float(2, od->add_key("k-1"), 121);
    odu->add(od);
  }

  {
    auto od = std::make_shared<ObsDataVector>();
    od->put_float(0, od->add_key("k-2"), 202);
    odu->add(od);
  }

  {
    auto od = std::make_shared<ObsDataVector>();
    od->put_float(0, od->add_key("k-0"), 300);
    od->put_float(1, od->add_key("k-3"), 313);
    odu->add(od);
  }

  const auto key_names = odu->get_keys();
  EXPECT_EQ(4, key_names.size());
  EXPECT_TRUE(std::find(key_names.begin(), key_names.end(), "k-0") != key_names.end());
  EXPECT_TRUE(std::find(key_names.begin(), key_names.end(), "k-4") == key_names.end());

  const auto* f313 = odu->get_float(5, "k-3");
  ASSERT_TRUE(f313);
  EXPECT_EQ(313, *f313);
}
