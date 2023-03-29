/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2018 met.no

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

#include <diQuickMenues.h>
#include <puTools/miStringFunctions.h>

#include <gtest/gtest.h>


TEST(TestQuickMenues, UpdateMapArea)
{
  {
    std::vector<std::string> lines;
    lines.push_back("WEBMAP hello");
    lines.push_back("MAP here=there area=Norge backcolour=white contour=on");
    lines.push_back("WEBMAP bye");

    std::vector<std::string> expect;
    expect.push_back("WEBMAP hello");
    expect.push_back("AREA name=Norge");
    expect.push_back("MAP here=there backcolour=white contour=on");
    expect.push_back("WEBMAP bye");

    EXPECT_TRUE(updateCommandSyntax(lines));
    EXPECT_EQ(expect, lines);
  }

  {
    std::vector<std::string> lines;
    lines.push_back("AREA name=Norge");
    lines.push_back("MAP here=there backcolour=white contour=on");
    std::vector<std::string> expect = lines;

    EXPECT_FALSE(updateCommandSyntax(lines));
    EXPECT_EQ(expect, lines);
  }
}

TEST(TestQuickMenues, UpdateField)
{
  {
    std::vector<std::string> lines;
    lines.push_back("FIELD @NORLAMEPS.@ECUTC Lameps_Probability_Wind>25m/s colour=green3");

    std::vector<std::string> expect;
    expect.push_back("FIELD model=@NORLAMEPS.@ECUTC plot=Lameps_Probability_Wind>25m/s colour=green3");

    EXPECT_TRUE(updateCommandSyntax(lines));
    EXPECT_EQ(expect, lines);
  }

  {
    std::vector<std::string> lines;
    lines.push_back("FIELD  model=AROME-MetCoOp.2.5KM refhour=6 plot=T.2M colour=red plottype=fill_cell ");
    std::vector<std::string> expect = lines;

    EXPECT_FALSE(updateCommandSyntax(lines));
    EXPECT_EQ(expect, lines);
  }

  {
    std::vector<std::string> lines;
    lines.push_back("FIELD ( AROME-MetCoOp.2.5KM T.2M - HIRLAM.8KM.00 T.2M ) level=500hPa colour=red");
    std::vector<std::string> expect;
    expect.push_back("FIELD ( model=AROME-MetCoOp.2.5KM plot=T.2M"
        " - model=HIRLAM.8KM.00 plot=T.2M ) vcoord=pressure vlevel=500hPa colour=red");

    EXPECT_TRUE(updateCommandSyntax(lines));
    EXPECT_EQ(expect, lines);
  }

  {
    std::vector<std::string> lines;
    lines.push_back("FIELD ( AROME T.2M level=500hPa - HIRLAM T.2M level=300hPa ) colour=red");
    std::vector<std::string> expect;
    expect.push_back("FIELD ( model=AROME plot=T.2M vcoord=pressure vlevel=500hPa"
        " - model=HIRLAM plot=T.2M vcoord=pressure vlevel=300hPa ) colour=red");

    EXPECT_TRUE(updateCommandSyntax(lines));
    EXPECT_EQ(expect, lines);
  }
}

TEST(TestQuickMenues, UpdateField2)
{
  { // challenge: two spaces at start
    std::vector<std::string> lines;
    lines.push_back("FIELD  @LOCALEC.GEO.0.25.@EC MSLP dim=1");
    lines.push_back("FIELD  @LOCALAROME-MetCoOp-PP.@Z nedb.1t.max-pp colour=blue");
    std::vector<std::string> expect;
    expect.push_back("FIELD model=@LOCALEC.GEO.0.25.@EC plot=MSLP dim=1");
    expect.push_back("FIELD model=@LOCALAROME-MetCoOp-PP.@Z plot=nedb.1t.max-pp colour=blue");

    EXPECT_TRUE(updateCommandSyntax(lines));
    EXPECT_EQ(expect, lines);
  }

  { // challenge: drop level=0m
    std::vector<std::string> lines;
    lines.push_back("FIELD WAM.10km.@Z Significant_Wave_Height level=0m colour=red");
    std::vector<std::string> expect;
    expect.push_back("FIELD model=WAM.10km.@Z plot=Significant_Wave_Height colour=red");

    EXPECT_TRUE(updateCommandSyntax(lines));
    EXPECT_EQ(expect, lines);
  }
}

namespace {

std::vector<quickMenu> makeQM()
{
  std::vector<quickMenu> qm;
  {
    quickMenu q0;
    q0.name = "haheho";
    {
      quickMenuOption o;
      o.key = "this";
      o.options.push_back("ha");
      o.options.push_back("he");
      o.options.push_back("ho");
      o.def = "he";
      q0.opt.push_back(o);
    }
    qm.push_back(q0);
  }

  {
    quickMenu q1;
    q1.name = "hihihi";
    {
      quickMenuOption o;
      o.key = "hello";
      o.options.push_back("hi");
      o.options.push_back("hihi");
      o.options.push_back("hihihi");
      o.def = "hi";
      q1.opt.push_back(o);
    }
    {
      quickMenuOption o;
      o.key = "bye";
      o.options.push_back("ha");
      o.options.push_back("hadet");
      o.options.push_back("hadetbra");
      o.def = "hadet";
      q1.opt.push_back(o);
    }
    {
      quickMenuOption o;
      o.key = "greeting";
      o.options.push_back("hello");
      o.options.push_back("hi");
      o.options.push_back("ciao");
      o.def = "hello";
      q1.opt.push_back(o);
    }
    {
      quickMenuOption o;
      o.key = "hellothere";
      o.options.push_back("anton");
      o.options.push_back("bob");
      o.options.push_back("claus");
      o.def = "claus";
      q1.opt.push_back(o);
    }
    qm.push_back(q1);
  }

  return qm;
}

const static std::string QM_LOG_SEP = "=======================";

} // namespace

TEST(TestQuickMenues, WriteQuickMenuLog)
{
  const std::vector<quickMenu> qm = makeQM();
  const std::vector<std::string> actual = writeQuickMenuLog(qm);
  const std::vector<std::string> expected = {">name=haheho", "%this=he",        QM_LOG_SEP,          ">name=hihihi", "%hello=hi",
                                             "%bye=hadet",   "%greeting=hello", "%hellothere=claus", QM_LOG_SEP};
  EXPECT_EQ(expected, actual);
}

TEST(TestQuickMenues, ReadQuickMenuLogSimple)
{
  std::vector<quickMenu> qm = makeQM();
  const std::vector<std::string> loglines = {">name=hihihi", "%nosuchoptions=ignore", "%hello=hihi", QM_LOG_SEP, ">update=haheho", "%this=ha", QM_LOG_SEP};
  readQuickMenuLog(qm, loglines);
  EXPECT_EQ("ha", qm[0].opt[0].def);
  EXPECT_EQ("hihi", qm[1].opt[0].def);
  EXPECT_EQ("hadet", qm[1].opt[1].def);
}

TEST(TestQuickMenues, ReadQuickMenuLogIgnorePlotCommands)
{
  std::vector<quickMenu> qm = makeQM();
  const std::vector<std::string> loglines = {">name=hihihi", "%hello=hihihi", "PLOTCOMMAND not known", QM_LOG_SEP};
  readQuickMenuLog(qm, loglines);
  EXPECT_EQ("hihihi", qm[1].opt[0].def);
}

TEST(TestQuickMenues, UsedOptions)
{
  std::vector<quickMenu> qm = makeQM();
  const std::set<int> ac = qm[1].used_options("PLOT @hellothere\nMAP area=@bye\n");
  const std::set<int> ex = {1 /*bye*/, 3 /*hellothere*/};
  EXPECT_EQ(ex, ac);
}

TEST(TestQuickMenues, ExpandOptions)
{
  const std::vector<quickMenu> qm = makeQM();
  std::vector<std::string> cmd = {"PLOT @hellothere", "MAP area=@greeting"};
  qm[1].expand_options(cmd);

  const std::vector<std::string> ex = {"PLOT claus", "MAP area=hello"};
  EXPECT_EQ(ex, cmd);
}

TEST(TestQuickMenues, ReadQuickMenu1)
{
  std::istringstream in("\"hei\n"
                        "[OPT1=black,red,,blue\n" // empty value before end
                        "[OPT12=on,off\n"
                        ">quick 1\n"
                        "AREA name=model/sat-area\n"
                        ">quick 2\n"
                        "AREA name=model/sat-area\n"
                        "MAP backcolour=@OPT1 lon=off lat=off frame=@OPT12\n"
                        "LABEL data font=BITMAPFONT fontsize=8\n");
  quickMenu qm;
  ASSERT_TRUE(readQuickMenu(qm, in));
  EXPECT_EQ("hei", qm.name);

  {
    ASSERT_EQ(2, qm.opt.size());
    const quickMenuOption& qmo0 = qm.opt[0];
    EXPECT_EQ("OPT1", qmo0.key);
    EXPECT_EQ(qmo0.def, qmo0.options.front());
    ASSERT_EQ(4, qmo0.options.size());
    EXPECT_EQ("red", qmo0.options[1]);
    EXPECT_TRUE(qmo0.options[2].empty());

    const quickMenuOption& qmo1 = qm.opt[1];
    EXPECT_EQ("OPT12", qmo1.key);
    EXPECT_EQ(qmo1.def, qmo1.options.front());
    ASSERT_EQ(2, qmo1.options.size());
  }
  {
    ASSERT_EQ(2, qm.menuitems.size());

    const quickMenuItem& qmi0 = qm.menuitems[0];
    EXPECT_EQ("quick 1", qmi0.name);
    ASSERT_EQ(1, qmi0.command.size());

    const quickMenuItem& qmi1 = qm.menuitems[1];
    EXPECT_EQ("quick 2", qmi1.name);
    ASSERT_EQ(3, qmi1.command.size());
  }
}

TEST(TestQuickMenues, MakeQuickMenuName)
{
  EXPECT_EQ("moose", makeQuickMenuName("/forest/with/moose.quick"));
  EXPECT_EQ("goose", makeQuickMenuName("goose.quick"));
  EXPECT_EQ("mouse", makeQuickMenuName("/elephant/or/mouse"));
}
