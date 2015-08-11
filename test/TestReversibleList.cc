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

#include <reversible_list.hh>
#include <algorithm>
#include <gtest/gtest.h>

TEST(TestReversibleList, Basic)
{
  typedef reversible_list<int> rlist_int;

  rlist_int l1;
  l1.push_back(21);
  l1.push_back(11);
  l1.reverse();
  l1.push_back(31);
  
  const int expected1[] = { 11, 21, 31 };
  ASSERT_EQ(3, l1.size());
  EXPECT_TRUE(std::equal(l1.begin(), l1.end(), expected1));

  rlist_int l2;
  l2 = l1;
  l1.reverse();

  ASSERT_EQ(3, l1.size());
  EXPECT_TRUE(std::equal(l2.begin(), l2.end(), expected1));
  EXPECT_TRUE(std::equal(l1.rbegin(), l1.rend(), expected1));

  rlist_int l3;
  l3.push_front(12);
  l3.push_front(22);
  l3.push_front(32);
  l3.push_front(42);

  const int expected2[]  = { 42, 32, 22, 12 };
  const int expected2r[] = { 12, 22, 32, 42 };
  ASSERT_EQ(4, l3.size());
  EXPECT_TRUE(std::equal(l3.begin(),  l3.end(),  expected2));
  EXPECT_TRUE(std::equal(l3.rbegin(), l3.rend(), expected2r));
  
  l3.reverse();

  ASSERT_EQ(4, l3.size());
  EXPECT_TRUE(std::equal(l3.begin(),  l3.end(),  expected2r));
  EXPECT_TRUE(std::equal(l3.rbegin(), l3.rend(), expected2));

  l3.move_back(l1);

  const int expected3[] = { 12, 22, 32, 42, 31, 21, 11 };
  ASSERT_EQ(7, l3.size());
  EXPECT_TRUE(std::equal(l3.begin(), l3.end(), expected3));

  l3.reverse();
  l3.pop_front();
  l3.pop_back();

  const int expected4[] = { 21, 31, 42, 32, 22 };
  ASSERT_EQ(5, l3.size());
  EXPECT_TRUE(std::equal(l3.begin(), l3.end(), expected4));
}
