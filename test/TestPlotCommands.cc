/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2018 met.no

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

#include <diFieldPlotCommand.h>
#include <diKVListPlotCommand.h>
#include <diPlotCommandFactory.h>
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

TEST(TestPlotCommands, FieldFromFactory)
{
  const std::string model = "hei", plot = "ho";
  miutil::KeyValue_v options;
  options << miutil::KeyValue("colour", "blue") << miutil::KeyValue("plottype", "contour") << miutil::KeyValue("alltimesteps", "1");
  const std::string text = "FIELD model=" + model + " plot=" + plot + " " + miutil::mergeKeyValue(options);

  PlotCommand_cp pc = makeCommand(text);
  FieldPlotCommand_cp cmd = std::dynamic_pointer_cast<const FieldPlotCommand>(pc);
  ASSERT_TRUE(!!cmd);
  EXPECT_FALSE(cmd->isEdit);
  EXPECT_FALSE(cmd->hasMinusField());
  EXPECT_EQ(model, cmd->field.model);
  EXPECT_EQ(options, cmd->options());
  EXPECT_EQ(text, cmd->toString());
}

TEST(TestPlotCommands, FieldDifferenceFromFactory)
{
  const std::string model1 = "no", plot1 = "hei", model2 = "de", plot2 = "hallo";
  miutil::KeyValue_v options;
  options << miutil::KeyValue("colour", "blue") << miutil::KeyValue("plottype", "contour");
  const std::string text = "FIELD ( model=" + model1 + " plot=" + plot1 + " - model=" + model2 + " plot=" + plot2 + " ) " + miutil::mergeKeyValue(options);

  PlotCommand_cp pc = makeCommand(text);
  FieldPlotCommand_cp cmd = std::dynamic_pointer_cast<const FieldPlotCommand>(pc);
  ASSERT_TRUE(!!cmd);
  EXPECT_FALSE(cmd->isEdit);
  EXPECT_TRUE(cmd->hasMinusField());
  EXPECT_EQ(model1, cmd->field.model);
  EXPECT_EQ(plot1, cmd->field.plot);
  EXPECT_EQ(model2, cmd->minus.model);
  EXPECT_EQ(plot2, cmd->minus.plot);
  EXPECT_EQ(options, cmd->options());
  EXPECT_EQ(text, cmd->toString());
}
