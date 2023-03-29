/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018-2022 met.no

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

#include "diColour.h"
#include <diPlotOptions.h>
#include <diPolyContouring.h>

#include <mi_fieldcalc/FieldDefined.h>

#include <boost/range/end.hpp>
#include <vector>
#include <cmath>

#include <gtest/gtest.h>


TEST(TestDianaLevels, List10)
{
  const DianaLevelList10 ll({1.0, 3.0}, 10);

  EXPECT_EQ(0, ll.level_for_value(0.1));
  EXPECT_EQ(1, ll.level_for_value(2));
  EXPECT_EQ(10, ll.level_for_value(1e11));

  EXPECT_FLOAT_EQ(1, ll.value_for_level(0));
  EXPECT_FLOAT_EQ(3, ll.value_for_level(1));
  EXPECT_FLOAT_EQ(10, ll.value_for_level(2));
  EXPECT_FLOAT_EQ(3e4, ll.value_for_level(9));
}

TEST(TestDianaLevels, List)
{
  const DianaLevelList ll({8, 10.8, 13.9, 17.2, 20.8, 24.5, 28.5, 32.7});

  EXPECT_EQ(0, ll.level_for_value(5));
  EXPECT_EQ(1, ll.level_for_value(9));
  EXPECT_EQ(2, ll.level_for_value(11));

  EXPECT_FLOAT_EQ(8, ll.value_for_level(0));
  EXPECT_FLOAT_EQ(10.8, ll.value_for_level(1));
}

TEST(TestDianaLevels, StepInterval2Base0)
{
  DianaLevelStep step(2, 0);

  EXPECT_EQ(step.level_for_value(0), 1);
  EXPECT_EQ(step.level_for_value(-3), -1);

  EXPECT_EQ(step.level_for_value(-0.4), 0);
  EXPECT_EQ(step.level_for_value(8.4), 5);

  EXPECT_EQ(step.value_for_level(0), 0);
}

TEST(TestDianaLevels, StepInterval2Base2)
{
  DianaLevelStep step(2, 2);

  EXPECT_EQ(step.level_for_value(2), 1);
  EXPECT_EQ(step.level_for_value(0), 0);
  EXPECT_EQ(step.level_for_value(-3), -2);

  EXPECT_EQ(step.level_for_value(-0.4), -1);
  EXPECT_EQ(step.level_for_value(8.4), 4);

  EXPECT_EQ(step.value_for_level(0), 2);
  EXPECT_EQ(step.value_for_level(-1), 0);
  EXPECT_EQ(step.value_for_level(4), 10);
}

TEST(TestDianaLevels, StepInterval2Base3)
{
  DianaLevelStep step(2, 3);

  EXPECT_EQ(step.level_for_value(3), 1);
  EXPECT_EQ(step.level_for_value(-3), -2);
  EXPECT_EQ(step.level_for_value(0), -1);

  EXPECT_EQ(step.level_for_value(-0.4), -1);
  EXPECT_EQ(step.level_for_value(8.4), 3);

  EXPECT_EQ(step.value_for_level(0), 3);
  EXPECT_EQ(step.value_for_level(-2), -1); // minimum is not an integer multiple
}

TEST(TestDianaLevelSelect, StepInterval2)
{
  PlotOptions po;
  po.lineinterval = 2;
  po.minvalue = 0;
  po.maxvalue = 8;
  po.base = 0;
  DianaLevels_p levels = dianaLevelsForPlotOptions(po, fieldUndef);
  ASSERT_TRUE(levels);
  DianaLevelSelector dls(po, *levels, DianaLines::LINES_LABELS | DianaLines::FILL);

  EXPECT_TRUE(dls.line(levels->level_for_value(-2)));
  EXPECT_TRUE(dls.line(levels->level_for_value(0)));
  EXPECT_FALSE(dls.line(levels->level_for_value(8)));
  EXPECT_FALSE(dls.line(levels->level_for_value(9)));

  EXPECT_FALSE(dls.fill(levels->level_for_value(-2)));
  EXPECT_TRUE(dls.fill(levels->level_for_value(0))); // line no, fill yes
  EXPECT_FALSE(dls.fill(levels->level_for_value(8)));
  EXPECT_FALSE(dls.fill(levels->level_for_value(9)));
}

TEST(TestDianaLevelSelect, StepInterval2NoZero)
{
  PlotOptions po;
  po.lineinterval = 2;
  po.minvalue = -8;
  po.maxvalue = 8;
  po.base = 0;
  po.zeroLine = false;
  DianaLevels_p levels = dianaLevelsForPlotOptions(po, fieldUndef);
  ASSERT_TRUE(levels);
  DianaLevelSelector dls(po, *levels, DianaLines::LINES_LABELS | DianaLines::FILL);

  EXPECT_EQ(0, levels->level_for_value(-0.1));
  EXPECT_EQ(1, levels->level_for_value(0));
  EXPECT_EQ(1, levels->level_for_value(0.1));

  EXPECT_TRUE(dls.line(levels->level_for_value(-10)));
  EXPECT_FALSE(dls.line(levels->level_for_value(-2)));
  EXPECT_FALSE(dls.line(levels->level_for_value(-1)));
  EXPECT_TRUE(dls.line(levels->level_for_value(0)));
  EXPECT_TRUE(dls.line(levels->level_for_value(1)));
  EXPECT_TRUE(dls.line(levels->level_for_value(+2)));
  EXPECT_FALSE(dls.line(levels->level_for_value(+10)));

  EXPECT_FALSE(dls.fill(levels->level_for_value(-10)));
  EXPECT_TRUE(dls.fill(levels->level_for_value(-8)));
  EXPECT_FALSE(dls.fill(levels->level_for_value(-2)));
  EXPECT_FALSE(dls.fill(levels->level_for_value(0)));
  EXPECT_TRUE(dls.fill(levels->level_for_value(+2)));
  EXPECT_TRUE(dls.fill(levels->level_for_value(+4)));
  EXPECT_FALSE(dls.fill(levels->level_for_value(+10)));
}

namespace {
PlotOptions po_for_line_values()
{
  PlotOptions po;
  po.set_lineinterval(0);
  po.set_linevalues("0.2,0.5,1,2,4,6,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80");
  return po;
}
} // namespace

TEST(TestDianaLevelSelect, LineValues)
{
  auto po = po_for_line_values();
  DianaLevels_p levels = dianaLevelsForPlotOptions(po, fieldUndef);
  ASSERT_TRUE(levels);
  DianaLevelSelector dls(po, *levels, DianaLines::LINES_LABELS | DianaLines::FILL);

  EXPECT_EQ(0, levels->level_for_value(0));
  EXPECT_EQ(0, levels->level_for_value(0.2));
  EXPECT_EQ(1, levels->level_for_value(0.4));
  EXPECT_EQ(1, levels->level_for_value(0.5));

  EXPECT_TRUE(dls.line(levels->level_for_value(0)));
  EXPECT_TRUE(dls.line(levels->level_for_value(0.2)));
  EXPECT_TRUE(dls.line(levels->level_for_value(0.5)));

  EXPECT_FALSE(dls.fill(levels->level_for_value(0)));
  EXPECT_FALSE(dls.fill(levels->level_for_value(0.2)));
  EXPECT_TRUE(dls.fill(levels->level_for_value(0.5)));
}

TEST(TestDianaLevelSelect, LineValuesNoZero)
{
  auto po = po_for_line_values();
  po.zeroLine = false;
  DianaLevels_p levels = dianaLevelsForPlotOptions(po, fieldUndef);
  ASSERT_TRUE(levels);
  DianaLevelSelector dls(po, *levels, DianaLines::LINES_LABELS | DianaLines::FILL);

  EXPECT_TRUE(dls.line(levels->level_for_value(0)));
  EXPECT_TRUE(dls.line(levels->level_for_value(0.2)));
  EXPECT_TRUE(dls.line(levels->level_for_value(0.5)));

  EXPECT_FALSE(dls.fill(levels->level_for_value(0)));
  EXPECT_FALSE(dls.fill(levels->level_for_value(0.2)));
  EXPECT_TRUE(dls.fill(levels->level_for_value(0.5)));
}

TEST(TestDianaLevelSelect, Undef)
{
  PlotOptions po;
  po.set_lineinterval(0);
  po.set_linevalues("2,3,4");
  po.undefMasking = 1;
  po.undefColour = Colour::BLACK;
  DianaLevels_p levels = dianaLevelsForPlotOptions(po, fieldUndef);
  ASSERT_TRUE(levels);
  DianaLevelSelector dls(po, *levels, DianaLines::LINES_LABELS | DianaLines::FILL | DianaLines::UNDEFINED);

  EXPECT_EQ(DianaLevels::UNDEF_LEVEL, levels->level_for_value(fieldUndef));
  EXPECT_NE(DianaLevels::UNDEF_LEVEL, levels->level_for_value(3));

  EXPECT_TRUE(dls.fill(DianaLevels::UNDEF_LEVEL));
}
