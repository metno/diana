/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017 met.no

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

#include <diKVListPlotCommand.h>
#include <diStringPlotCommand.h>

#include <gtest/gtest.h>

TEST(TestPlotCommands, StringPlotCommand)
{
  { const StringPlotCommand s("GO", "GO forward");
    EXPECT_EQ("GO", s.commandKey());
    EXPECT_EQ("GO forward", s.command());
    EXPECT_EQ("GO forward", s.toString());
  }
  { const StringPlotCommand s("GO forward");
    EXPECT_EQ("GO", s.commandKey());
    EXPECT_EQ("GO forward", s.command());
  }
  { const StringPlotCommand s("   GO  forward  ");
    EXPECT_EQ("GO", s.commandKey());
  }
}

TEST(TestPlotCommands, KVListPlotCommand)
{
  { const KVListPlotCommand c("GO", "straight right=bad   left=\"also bad\"  ");
    EXPECT_EQ("GO", c.commandKey());
    EXPECT_EQ(3, c.size());

    const size_t i_straight = c.find("straight");
    EXPECT_EQ(0, i_straight);
    EXPECT_EQ("", c.get(i_straight).value());

    const size_t i_right = c.rfind("right");
    EXPECT_EQ(1, i_right);
    EXPECT_EQ("bad", c.get(i_right).value());

    const size_t i_left = c.rfind("left", 2);
    EXPECT_EQ(2, i_left);
    EXPECT_EQ("left", c.get(i_left).key());
    EXPECT_EQ("also bad", c.get(i_left).value());

    EXPECT_EQ("GO straight right=bad left=\"also bad\"", c.toString());
  }
  { KVListPlotCommand c("GO");
    c.add(miutil::KeyValue("straight"));
    c.add("right", "bad");
    c.add("left", "also bad");

    EXPECT_EQ("GO", c.commandKey());
    EXPECT_EQ(3, c.size());
    EXPECT_EQ(0, c.rfind("straight", 1));
    EXPECT_EQ(size_t(-1), c.find("back"));

    EXPECT_EQ("GO straight right=bad left=\"also bad\"", c.toString());
  }
}
