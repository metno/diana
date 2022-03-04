/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2022 met.no

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

#include <diObsReaderFile.h>

#include <gtest/gtest.h>

namespace {
const static std::string BUFR_SYNO_DIR(TEST_SRCDIR "/testdata/syno/");
const static std::string BUFR_SYNO = BUFR_SYNO_DIR + "mini_syno_[yyyymmddHH].bufr";
const static std::string BUFR_SYNO_ARCHIVE = BUFR_SYNO_DIR + "mini_archive_syno_[yyyymmddHH].bufr";

const miutil::miTime t_01_00("2017-12-14 01:00");
const miutil::miTime t_06_00("2017-12-14 06:00");
const miutil::miTime t_11_00("2017-12-14 11:00");

class FakeReaderFile : public ObsReaderFile
{
public:
  using ObsReaderFile::addPattern;

  std::set<miutil::miTime> filetimes;
  std::set<std::string> filenames;

  using ObsReaderFile::setSynoptic;

  void clear()
  {
    filetimes.clear();
    filenames.clear();
  }

protected:
  bool getDataFromFile(const FileInfo& fi, ObsDataRequest_cp, ObsDataResult_p) override
  {
    filetimes.insert(fi.time);
    filenames.insert(fi.filename);
    return true;
  }
};

typedef std::shared_ptr<FakeReaderFile> FakeReaderFile_p;

} // namespace

TEST(TestObsReaderFile, Configure)
{
  FakeReaderFile_p fake = std::make_shared<FakeReaderFile>();
  fake->addPattern(BUFR_SYNO, false);
  fake->addPattern(BUFR_SYNO_ARCHIVE, true);

  const std::set<miutil::miTime> times = fake->getTimes(false, true);
  EXPECT_EQ(13, times.size());
  EXPECT_TRUE(times.count(t_06_00));
  EXPECT_TRUE(times.count(t_11_00));

  EXPECT_TRUE(fake->checkForUpdates(true)); // change from no-archive to archive => changed file list
  EXPECT_EQ(13 + 6, fake->getTimes(true, true).size());
}

TEST(TestObsReaderFile, GetData)
{
  FakeReaderFile_p fake = std::make_shared<FakeReaderFile>();
  fake->addPattern(BUFR_SYNO, false);
  fake->setTimeRange(-120, 120);

  ObsDataRequest_p req = std::make_shared<ObsDataRequest>();
  req->obstime = t_11_00;
  req->timeDiff = 90;
  req->level = -2;

  fake->getData(req, std::make_shared<ObsDataResult>());
  EXPECT_EQ(1, fake->filetimes.size());

  // test again with synoptic=false

  fake->clear();
  fake->setSynoptic(false);
  fake->getData(req, std::make_shared<ObsDataResult>());
  EXPECT_EQ(5, fake->filetimes.size());
}

TEST(TestObsReaderFile, GetDataTimeDiffBelow0)
{
  FakeReaderFile_p fake = std::make_shared<FakeReaderFile>();
  fake->addPattern(BUFR_SYNO, false);
  fake->setTimeRange(-120, 120);

  ObsDataRequest_p req = std::make_shared<ObsDataRequest>();
  req->obstime = t_11_00;
  req->timeDiff = -1;
  req->level = -2;

  fake->getData(req, std::make_shared<ObsDataResult>());
  EXPECT_EQ(1, fake->filetimes.size());

  // test again with synoptic=false

  fake->clear();
  fake->setSynoptic(false);
  fake->getData(req, std::make_shared<ObsDataResult>());
  EXPECT_EQ(13, fake->filetimes.size());
}

TEST(TestObsReaderFile, GetDataNoTimeInFilename)
{
  FakeReaderFile_p fake = std::make_shared<FakeReaderFile>();
  // add as filenames, ie. no pattern
  fake->addPattern(BUFR_SYNO_DIR + "mini_syno_2017121400.bufr", false);
  fake->addPattern(BUFR_SYNO_DIR + "mini_syno_2017121401.bufr", false);
  fake->setTimeRange(-30, 30);

  ObsDataRequest_p req = std::make_shared<ObsDataRequest>();
  req->obstime = t_11_00;
  req->timeDiff = 30;
  req->level = -2;

  fake->getData(req, std::make_shared<ObsDataResult>());
  EXPECT_EQ(2, fake->filenames.size());

  // test again with synoptic=false => no change

  fake->clear();
  fake->setSynoptic(false);
  fake->getData(req, std::make_shared<ObsDataResult>());
  EXPECT_EQ(2, fake->filenames.size());
}
