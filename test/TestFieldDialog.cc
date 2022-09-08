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

#include "diFieldDialogData.h"
#include "diFieldUtil.h"
#include "diLinetype.h"
#include "diPlotCommandFactory.h"
#include "diPlotOptions.h"
#include "qtFieldDialog.h"
#include "util/misc_util.h"

#include <gtest/gtest.h>

#include <sstream>

#define MILOGGER_CATEGORY "diana.test.FieldDialog"
#include "miLogger/miLogging.h"

namespace {
const std::string MODEL1 = "supermodel";
const std::string PARAM1 = "param_a";

class TestFieldDialogData : public FieldDialogData
{
public:
  TestFieldDialogData();

  FieldModelGroupInfo_v getFieldModelGroups() override { return fieldModelGroups; }
  attributes_t getFieldGlobalAttributes(const std::string& /*model*/, const std::string& /*refTime*/) override { return attributes_t(); }
  int getFieldPlotDimension(const std::vector<std::string>& plotOrParamNames, bool predefinedPlot) override { return 1; }
  void updateFieldReferenceTimes(const std::string& model) override { if (model == MODEL1) fieldReferenceTimeUpdates += 1; }
  std::set<std::string> getFieldReferenceTimes(const std::string& m) override { return fieldReferenceTimes[m]; }
  std::string getBestFieldReferenceTime(const std::string& m, int ro, int rh) override { return ::getBestReferenceTime(getFieldReferenceTimes(m), ro, rh); }
  void getSetupFieldOptions(std::map<std::string, miutil::KeyValue_v>& fieldoptions) override { fieldoptions = setupFieldOptions; }

  void getFieldPlotGroups(const std::string& m, const std::string& rt, bool predefinedPlots, FieldPlotGroupInfo_v& vfgi) override;
  plottimes_t getFieldTime(std::vector<FieldRequest>& requests) override;

  FieldModelGroupInfo_v fieldModelGroups;

  //! key: model name, value: reference times
  std::map<std::string, std::set<std::string>> fieldReferenceTimes;

  //! key: plot name, value: options
  std::map<std::string, miutil::KeyValue_v> setupFieldOptions;

  int fieldReferenceTimeUpdates;
};

TestFieldDialogData::TestFieldDialogData()
    : fieldReferenceTimeUpdates(0)
{
  FieldModelGroupInfo fmg;
  fmg.groupName = "My Favourite Test Group";
  fmg.groupType = FieldModelGroupInfo::STANDARD_GROUP;
  fmg.models.push_back(FieldModelInfo(MODEL1, ""));
  fieldModelGroups.push_back(fmg);
  fieldReferenceTimes[MODEL1] = {"2018-09-10T00:00:00", "2018-09-10T06:00:00"};
}

void TestFieldDialogData::getFieldPlotGroups(const std::string& m, const std::string& rt, bool predefinedPlots, FieldPlotGroupInfo_v& vfgi)
{
  METLIBS_LOG_SCOPE(LOGVAL(m) << LOGVAL(rt) << LOGVAL(predefinedPlots));
  vfgi.clear();
  if (predefinedPlots) {
    METLIBS_LOG_DEBUG("only predefinedPlots==false for now");
    return;
  }
  const std::map<std::string, std::set<std::string>>::const_iterator itM = fieldReferenceTimes.find(m);
  if (itM == fieldReferenceTimes.end()) {
    METLIBS_LOG_DEBUG("model '" << m << "' not found");
    return;
  }
  const std::set<std::string>::const_iterator itRT = itM->second.find(rt);
  if (itRT == itM->second.end()) {
    METLIBS_LOG_DEBUG("reftime '" << rt << "' not found");
    return;
  }

  FieldPlotGroupInfo fpgi;
  FieldPlotInfo fpi;
  fpi.fieldName = PARAM1;
  fpi.groupName = "field_group_1";
  fpgi.plots.push_back(fpi);
  vfgi.push_back(fpgi);

  METLIBS_LOG_DEBUG(LOGVAL(vfgi.size()));
}

plottimes_t TestFieldDialogData::getFieldTime(std::vector<FieldRequest>& /*requests*/)
{
  return plottimes_t();
}

void initLinesAndColours()
{
  Colour black;
  Colour::define(black.Name(), black.R(), black.G(), black.B(), black.A());
  Colour::define("red", 0xFF, 0, 0, black.A());
  Colour::define("green", 0, 0xFF, 0, black.A());
  Colour::define("blue", 0, 0, 0xFF, black.A());

  Linetype::init();
  const Linetype& ltd = Linetype::getDefaultLinetype();
  // FIXME without linetypes defined, FieldDialog crashes
  Linetype::define(ltd.name, ltd.bmap, ltd.factor);
}
} // namespace

TEST(TestFieldDialog, PutGetOKStringRaw)
{
  initLinesAndColours();
  TestFieldDialogData* data = new TestFieldDialogData;
  std::unique_ptr<FieldDialog> dialog(new FieldDialog(0, data));

  const PlotCommand_cpv cmds_put = makeCommands({"FIELD model=" + MODEL1 + " refhour=0 parameter=" + PARAM1});
  dialog->putOKString(cmds_put);
  EXPECT_EQ(data->fieldReferenceTimeUpdates, 1);

  const PlotCommand_cpv cmds_get = dialog->getOKString();
  ASSERT_EQ(cmds_get.size(), 1);

  std::ostringstream expect;
  expect << "FIELD model=" << MODEL1 << " reftime=" << *(data->getFieldReferenceTimes(MODEL1).begin()) << " parameter=" << PARAM1 << " line.interval=10";
  EXPECT_EQ(expect.str(), cmds_get[0]->toString());
}

TEST(TestFieldDialog, PutGetOKStringMinus)
{
  initLinesAndColours();
  TestFieldDialogData* data = new TestFieldDialogData;
  std::unique_ptr<FieldDialog> dialog(new FieldDialog(0, data));

  // clang-format off
  const PlotCommand_cpv cmds_put = makeCommands({"FIELD ( model=" + MODEL1 + " parameter=" + PARAM1
                                                    + " - model=" + MODEL1 + " parameter=" + PARAM1
                                                    + " ) refhour=0"});
  // clang-format on
  dialog->putOKString(cmds_put);
  EXPECT_EQ(data->fieldReferenceTimeUpdates, 1); // same model, should update only once

  const PlotCommand_cpv cmds_get = dialog->getOKString();
  ASSERT_EQ(cmds_get.size(), 1);

  std::ostringstream expect;
  // clang-format off
  expect << "FIELD ( "
         << "model=" << MODEL1 << " reftime=" << *(data->getFieldReferenceTimes(MODEL1).begin()) << " parameter=" << PARAM1
         << " - "
         << "model=" << MODEL1 << " reftime=" << *(data->getFieldReferenceTimes(MODEL1).begin()) << " parameter=" << PARAM1
         << " )"
         << " line.interval=10";
  // clang-format on
  EXPECT_EQ(expect.str(), cmds_get[0]->toString());
}

TEST(TestFieldDialog, PutGetOKStringLog)
{
  initLinesAndColours();
  TestFieldDialogData* data = new TestFieldDialogData;
  std::unique_ptr<FieldDialog> dialog(new FieldDialog(0, data));

  const miutil::KeyValue_v log {
    miutil::kv(PlotOptions::key_colour, "red"),
    miutil::kv(PlotOptions::key_plottype, fpt_fill_cell),
    miutil::kv(PlotOptions::key_basevalue, 2),
    miutil::kv(PlotOptions::key_frame, 0)
  };
  dialog->readLog({PARAM1 + " " + miutil::mergeKeyValue(log)}, "none", "none");

  const PlotCommand_cpv cmds_put = makeCommands({"FIELD model=" + MODEL1 + " refhour=0 parameter=" + PARAM1});
  dialog->putOKString(cmds_put);

  const PlotCommand_cpv cmds_get = dialog->getOKString();
  ASSERT_EQ(cmds_get.size(), 1);

  // log options should be ignored by putOKString
  std::ostringstream expect;
  expect << "FIELD model=" << MODEL1 << " reftime=" << *(data->getFieldReferenceTimes(MODEL1).begin()) << " parameter=" << PARAM1 << " line.interval=10";
  EXPECT_EQ(expect.str(), cmds_get[0]->toString());
}

TEST(TestFieldDialog, PutGetOKStringSetup)
{
  initLinesAndColours();
  const miutil::KeyValue_v setup = {
    miutil::kv("plottype", fpt_fill_cell),
    miutil::kv("base", 7),
    miutil::kv("frame", 2),
    miutil::kv("colour", "red"),
    miutil::kv(PlotOptions::key_limits, "1,2,3"),
    miutil::kv("apples", "pears")
  };

  const miutil::KeyValue_v log = {
    miutil::kv("plottype", fpt_alpha_shade),
    miutil::kv("base", 5),
    miutil::kv("frame", 1),
    miutil::kv("colour", "blue")
  };

  TestFieldDialogData* data = new TestFieldDialogData;
  data->setupFieldOptions[PARAM1] = setup;
  std::unique_ptr<FieldDialog> dialog(new FieldDialog(nullptr, data));
  dialog->readLog({PARAM1 + " " + miutil::mergeKeyValue(log)}, "none", "none");

  const miutil::KeyValue_v cmd = {
    miutil::kv("colour", "green")
  };

  const PlotCommand_cpv cmds_put = makeCommands({"FIELD model=" + MODEL1 + " refhour=0 parameter=" + PARAM1 + " " + miutil::mergeKeyValue(cmd)});
  dialog->putOKString(cmds_put);

  const PlotCommand_cpv cmds_get = dialog->getOKString();
  ASSERT_EQ(cmds_get.size(), 1);

  std::ostringstream expect;
  expect << "FIELD model=" << MODEL1 << " reftime=" << *(data->getFieldReferenceTimes(MODEL1).begin()) << " parameter=" << PARAM1
         << " colour=green line.interval=10 limits=1,2,3 frame=2 plottype=fill_cell base=7 apples=pears";
  EXPECT_EQ(expect.str(), cmds_get[0]->toString());
}

TEST(TestFieldDialog, PutGetOKStringHourOffset)
{
  initLinesAndColours();
  TestFieldDialogData* data = new TestFieldDialogData;
  std::unique_ptr<FieldDialog> dialog(new FieldDialog(0, data));

  const std::string colours[3] = {"blue", "red", "green"};

  const PlotCommand_cpv cmds_put = makeCommands({
    "FIELD model=" + MODEL1 + " refhour=0 parameter=" + PARAM1 + " hour.offset=0  colour=" + colours[0],
    "FIELD model=" + MODEL1 + " refhour=0 parameter=" + PARAM1 + " hour.offset=6  colour=" + colours[1],
    "FIELD model=" + MODEL1 + " refhour=0 parameter=" + PARAM1 + " hour.offset=12 colour=" + colours[2],
  });
  dialog->putOKString(cmds_put);
  EXPECT_EQ(data->fieldReferenceTimeUpdates, 1);

  // must select fields to trigger the problem
  dialog->simulateSelectField(1);
  dialog->simulateSelectField(2);

  const PlotCommand_cpv cmds_get = dialog->getOKString();
  ASSERT_EQ(cmds_get.size(), 3);

  for (int i=0; i<3; ++i) {
    std::ostringstream expect;
    expect << "FIELD model=" << MODEL1 << " reftime=" << *(data->getFieldReferenceTimes(MODEL1).begin()) << " parameter=" << PARAM1;
    if (i > 0)
      expect << " hour.offset=" << (6*i);
    expect << " colour=" << colours[i] << " line.interval=10";
    EXPECT_EQ(expect.str(), cmds_get[i]->toString());
  }
}

TEST(TestFieldDialog, GetShortNameEmpty)
{
  initLinesAndColours();
  TestFieldDialogData* data = new TestFieldDialogData;
  std::unique_ptr<FieldDialog> dialog(new FieldDialog(0, data));

  dialog->putOKString(PlotCommand_cpv());
  EXPECT_EQ(dialog->getShortname(), "");
}

TEST(TestFieldDialog, GetShortName)
{
  initLinesAndColours();
  TestFieldDialogData* data = new TestFieldDialogData;
  std::unique_ptr<FieldDialog> dialog(new FieldDialog(0, data));

  const PlotCommand_cpv cmds_put = makeCommands({"FIELD model=" + MODEL1 + " refhour=0 parameter=" + PARAM1 + " colour=green"});
  dialog->putOKString(cmds_put);
  EXPECT_EQ(dialog->getShortname(), "<font color=\"#000099\">" + MODEL1 + " " + PARAM1 + "</font>");
}
