/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018 met.no

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

#include "diLinetype.h"
#include "diPlotCommandFactory.h"
#include "diPlotOptions.h"
#include "diSatDialogData.h"
#include "qtSatDialog.h"
#include "util/misc_util.h"

#include <gtest/gtest.h>

#include <sstream>

#define MILOGGER_CATEGORY "diana.test.SatDialog"
#include "miLogger/miLogging.h"

namespace {
class TestSatDialogData : public SatDialogData
{
public:
  TestSatDialogData();

  const SatImage_v& initSatDialog() override { return sdi; }
  SatFile_v getSatFiles(const std::string& image_name, const std::string& subtype_name, bool update) override;
  std::vector<std::string> getSatChannels(const std::string& image_name, const std::string& subtype_name, int index) override;
  std::vector<Colour> getSatColours(const std::string& /*image_name*/, const std::string& /*subtype_name*/) override { return scolours; }

  SatImage_v sdi;
  std::vector<Colour> scolours;
};

TestSatDialogData::TestSatDialogData()
{
  SatImage noaa_image;
  noaa_image.image_name = "NOAA";
  noaa_image.subtype_names = std::vector<std::string>{"N-Europa"};
  sdi.push_back(noaa_image);

  scolours.push_back(Colour(0xFF, 0, 0));
  scolours.push_back(Colour(0, 0xFF, 0));
  scolours.push_back(Colour(0, 0, 0xFF));
}

std::vector<std::string> TestSatDialogData::getSatChannels(const std::string& image_name, const std::string& subtype_name, int index)
{
  METLIBS_LOG_SCOPE(LOGVAL(image_name) << LOGVAL(subtype_name) << LOGVAL(index));
  for (const auto& i : sdi) {
    if (i.image_name == image_name) {
      METLIBS_LOG_SCOPE(LOGVAL(i.image_name));
      for (const auto& f : i.subtype_names) {
        if (f == subtype_name) {
          return std::vector<std::string>{"day_night", "2+4",   "4+2", "1+2+4", "2+3+4", "3+4+5", "5+4+3", "2+6+4",
                                          "6+4+5",     "5+4+6", "1",   "2",     "3",     "4",     "5",     "6"};
        }
      }
    }
  }
  static const std::vector<std::string> EMPTY;
  return EMPTY;
}

SatFile_v TestSatDialogData::getSatFiles(const std::string& /*image_name*/, const std::string& /*subtype_name*/, bool /*update*/)
{
  SatFile_v sfi;

  SatFile fi;
  fi.name = "/no/such/file.tiff";
  fi.time = miutil::miTime(2018, 9, 9, 0, 0, 0);
  fi.palette = false;
  sfi.push_back(fi);

  return sfi;
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
  // FIXME without linetypes defined, SatDialog crashes
  Linetype::define(ltd.name, ltd.bmap, ltd.factor);
}
} // namespace

TEST(TestSatDialog, PutGetOKStringRaw)
{
  initLinesAndColours();
  TestSatDialogData* data = new TestSatDialogData;
  std::unique_ptr<SatDialog> dialog(new SatDialog(data));

  const std::string cmd = "SAT NOAA N-Europa day_night timediff=60 mosaic=false cut=0.02 alphacut=0 alpha=1 table=true enabled=false";
  const PlotCommand_cpv cmds_put = makeCommands({cmd});
  dialog->putOKString(cmds_put);

  const PlotCommand_cpv cmds_get = dialog->getOKString();
  ASSERT_EQ(cmds_get.size(), 1);

  EXPECT_EQ(cmd, cmds_get[0]->toString());
}

TEST(TestSatDialog, ReadGarbageSatOptionsLog)
{
  std::vector<std::string> vstr;
  vstr.push_back("SAT ARCHIVE radar PSC timediff=60 mosaic=0 alpha=1");
  vstr.push_back("UK Analysis FAX timediff=60 alpha=1 mosaic=1 cut=0.25");
  vstr.push_back("meteosat overview timediff=60");
  vstr.push_back("timediff=60 mosaic=0 cut=0.125 alphacut=0 alpha=1 font=BITMAPFONT face=normal");

  SatDialog::imageoptions_t satoptions;
  SatDialog::readSatOptionsLog(vstr, satoptions);
  EXPECT_EQ(2, satoptions.size());

  {
    SatDialog::imageoptions_t::const_iterator itn = satoptions.find("ARCHIVE");
    ASSERT_NE(satoptions.end(), itn);
    SatDialog::subtypeoptions_t::const_iterator ita = itn->second.find("radar");
    ASSERT_NE(itn->second.end(), ita);
    SatPlotCommand_cp cmd = ita->second;
    EXPECT_EQ("ARCHIVE", cmd->image_name);
    EXPECT_EQ("radar", cmd->subtype_name);
    EXPECT_EQ("PSC", cmd->plotChannels);
    EXPECT_EQ(60, cmd->timediff);
    EXPECT_EQ(1.0, cmd->alpha);
    EXPECT_EQ(false, cmd->mosaic);
  }
  {
    SatDialog::imageoptions_t::const_iterator itn = satoptions.find("UK");
    ASSERT_NE(satoptions.end(), itn);
    SatDialog::subtypeoptions_t::const_iterator ita = itn->second.find("Analysis");
    ASSERT_NE(itn->second.end(), ita);
    SatPlotCommand_cp cmd = ita->second;
    EXPECT_EQ("UK", cmd->image_name);
    EXPECT_EQ("Analysis", cmd->subtype_name);
    EXPECT_EQ("FAX", cmd->plotChannels);
    EXPECT_EQ(1, cmd->alpha);
    EXPECT_EQ(60, cmd->timediff);
    EXPECT_EQ(1.0, cmd->alpha);
    EXPECT_EQ(true, cmd->mosaic);
  }
}
