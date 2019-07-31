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

#include <diObsReaderTimeInterval.h>
#include <util/time_util.h>

#include <puTools/miStringFunctions.h>

#include <gtest/gtest.h>

namespace {
class ObsReaderDummy : public ObsReaderTimeInterval
{
public:
  void getData(ObsDataRequest_cp /*request*/, ObsDataResult_p /*result*/) {}
};

typedef std::shared_ptr<ObsReaderTimeInterval> ObsReaderTimeInterval_p;
} // namespace

TEST(TestObsReaderTimeInterval, Times)
{
  const miutil::miTime t_from("2019-06-01 00:00:00");
  const miutil::miTime t_to("2019-06-02 00:00:00");
  const int interval = 3600;

  ObsReaderTimeInterval_p mu = std::make_shared<ObsReaderDummy>();
  EXPECT_TRUE(mu->configure("time_from", t_from.isoTime()));
  EXPECT_TRUE(mu->configure("time_to", t_to.isoTime()));
  EXPECT_TRUE(mu->configure("time_interval", miutil::from_number(interval)));

  const std::set<miutil::miTime> times = mu->getTimes(false, true);
  EXPECT_EQ(26, times.size());
  EXPECT_TRUE(times.count(miutil::addSec(t_from, -interval / 2)));
  EXPECT_TRUE(times.count(miutil::addSec(t_from, +interval / 2)));
  EXPECT_TRUE(times.count(miutil::addSec(t_to, +interval / 2)));
}

TEST(TestObsReaderTimeInterval, CheckTimes)
{
  ObsReaderTimeInterval_p mu = std::make_shared<ObsReaderDummy>();
  EXPECT_TRUE(mu->configure("time_from_offset", "-3600"));
  EXPECT_TRUE(mu->configure("time_to_offset", "7200"));
  EXPECT_TRUE(mu->configure("time_interval", "3600"));

  const miutil::miTime now("2019-06-01 00:00:00");
  const miutil::miTime later = miutil::addHour(now, 1);

  {
    const std::set<miutil::miTime>& times = mu->calculateTimes(now);
    const std::set<miutil::miTime> ex_times = {miutil::miTime("2019-05-31 22:30:00"), miutil::miTime("2019-05-31 23:30:00"),
                                               miutil::miTime("2019-06-01 00:30:00"), miutil::miTime("2019-06-01 01:30:00"),
                                               miutil::miTime("2019-06-01 02:30:00")};
    EXPECT_EQ(ex_times, times);
  }
  {
    EXPECT_TRUE(mu->checkTimes(later));
    const std::set<miutil::miTime>& times = mu->calculateTimes(later);
    const std::set<miutil::miTime> ex_times = {miutil::miTime("2019-05-31 23:30:00"), miutil::miTime("2019-06-01 00:30:00"),
                                               miutil::miTime("2019-06-01 01:30:00"), miutil::miTime("2019-06-01 02:30:00"),
                                               miutil::miTime("2019-06-01 03:30:00")};
    EXPECT_EQ(ex_times, times);
  }
}
