/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2020-2021 met.no

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

#include <diColourShading.h>
#include <diField/diField.h>
#include <diFieldPlot.h>
#include <diFieldPlotManager.h>
#include <diPlotModule.h>

#include <util/diKeyValue.h>

#include <util/string_util.h>

#include <gtest/gtest.h>

using miutil::kv;

namespace {
const std::string palettename = "testpalette";
void init()
{
  static bool initialized = false;
  if (initialized)
    return;
  initialized = true;
  ColourShading::define(palettename, {Colour(128,0,0), Colour(0,128,0), Colour(0,0,128)});
}
} // namespace

TEST(FieldPlot, GetAnnotations)
{
  init();
  std::unique_ptr<PlotModule> plotmodule(new PlotModule); // required in FieldPlot ctor

  std::unique_ptr<FieldPlot> fp(new FieldPlot(nullptr));
  fp->prepare({}, {}, FieldPlotCommand::fromKV({{PlotOptions::key_plottype, fpt_fill_cell},{PlotOptions::key_table, "1"},{PlotOptions::key_palettecolours, palettename}}, false));

  Field_p field = std::make_shared<Field>();
  field->reserve(2, 2);
  field->fieldText = "myfieldname";
  fp->setData({ field }, miutil::miTime("2020-05-05 00:00:00"));
  EXPECT_EQ(P_OK_DATA, fp->getStatus());

  const std::string anno0end = ",colour=blue", anno0 = "table"+anno0end, anno1 = "arrow";
  std::vector<std::string> annotations { anno0, anno1 };
  fp->getDataAnnotations(annotations);
  ASSERT_EQ(3, annotations.size());
  EXPECT_EQ(anno0, annotations[0]);
  EXPECT_EQ(anno1, annotations[1]);

  EXPECT_EQ("table=\"" + field->fieldText + ";0:0:128:255;;> 20;0:128:0:255;;10 - 20;128:0:0:255;;< 10\",colour=blue", annotations[2]);
}
