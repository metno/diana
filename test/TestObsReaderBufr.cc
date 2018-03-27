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

#include <diObsReaderBufr.h>
#include <util/time_util.h>

#include <gtest/gtest.h>

namespace {
const static std::string BUFR_SYNO_DIR(TEST_SRCDIR "/testdata/syno/");
const static std::string BUFR_SYNO = BUFR_SYNO_DIR + "mini_syno_[yyyymmddHH].bufr";
const static std::string BUFR_SYNO_ARCHIVE = BUFR_SYNO_DIR + "mini_archive_syno_[yyyymmddHH].bufr";

const miutil::miTime t_01_00("2017-12-14 01:00");
const miutil::miTime t_06_00("2017-12-14 06:00");
const miutil::miTime t_11_00("2017-12-14 11:00");

typedef std::shared_ptr<ObsReaderBufr> ObsReaderBufr_p;
}

TEST(TestObsReaderBufr, Configure)
{
  ObsReader_p bufr = std::make_shared<ObsReaderBufr>();
  EXPECT_TRUE(bufr->configure("bufr", BUFR_SYNO));
  EXPECT_TRUE(bufr->configure("archive_bufr", BUFR_SYNO_ARCHIVE));

  EXPECT_EQ(13, bufr->getTimes(false, true).size());

  EXPECT_TRUE(bufr->checkForUpdates(true)); // change from no-archive to archive => changed file list
  EXPECT_EQ(13 + 6, bufr->getTimes(true, true).size());
}

TEST(TestObsReaderBufr, GetData)
{
  ObsReaderBufr_p bufr = std::make_shared<ObsReaderBufr>();
  EXPECT_TRUE(bufr->configure("bufr", BUFR_SYNO));
  bufr->setTimeRange(-120, 120);

  ObsDataRequest_p req = std::make_shared<ObsDataRequest>();
  req->obstime = t_11_00;
  req->timeDiff = 10;
  req->level = -2;
  ObsDataResult_p res = std::make_shared<ObsDataResult>();
  bufr->getData(req, res);

  EXPECT_EQ(15, res->data().size());
  for (const ObsData& obs : res->data())
    EXPECT_EQ(t_11_00, obs.obsTime);
}

TEST(TestObsReaderBufr, GetDataNotSynoptic)
{
  ObsReaderBufr_p bufr = std::make_shared<ObsReaderBufr>();
  EXPECT_TRUE(bufr->configure("bufr", BUFR_SYNO));
  bufr->setTimeRange(-120, 120);
  bufr->setSynoptic(false);

  ObsDataRequest_p req = std::make_shared<ObsDataRequest>();
  req->obstime = t_11_00;
  req->timeDiff = 60;
  req->level = -2;
  ObsDataResult_p res = std::make_shared<ObsDataResult>();
  bufr->getData(req, res);

  EXPECT_EQ(47, res->data().size());

  const miutil::miTime t0 = miutil::addMin(t_11_00, -req->timeDiff);
  const miutil::miTime t1 = miutil::addMin(t_11_00, +req->timeDiff);
  for (const ObsData& obs : res->data()) {
    EXPECT_LE(t0, obs.obsTime);
    EXPECT_GE(t1, obs.obsTime);
  }
}
