/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015 met.no

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

#include <wmsclient/WebMapUtilities.h>
#include <QStringList>
#include <gtest/gtest.h>

TEST(WebMapUtilities, ParseDecimals)
{
  using diutil::detail::parseDecimals;

  int value = 0;
  size_t idx = 0;
  EXPECT_TRUE(parseDecimals(value, "1234", idx, 4));
  EXPECT_EQ(1234, value);
  EXPECT_EQ(4, idx);

  value = 0;
  idx = 0;
  EXPECT_TRUE(parseDecimals(value, "123456+78", idx, 4));
  EXPECT_EQ(1234, value);
  EXPECT_EQ(4, idx);

  value = 0;
  idx = 0;
  EXPECT_TRUE(parseDecimals(value, "123456+78", idx, 4, true));
  EXPECT_EQ(123456, value);
  EXPECT_EQ(6, idx);

  value = 0;
  idx = 0;
  EXPECT_TRUE(parseDecimals(value, "1234+5678", idx, 4));
  EXPECT_EQ(1234, value);
  EXPECT_EQ(4, idx);

  value = 0;
  idx = 0;
  EXPECT_TRUE(parseDecimals(value, "123456", idx, 4, true));
  EXPECT_EQ(123456, value);
  EXPECT_EQ(6, idx);

  value = 0;
  idx = 0;
  EXPECT_TRUE(parseDecimals(value, "1234-56-34", idx, 4, true));
  EXPECT_EQ(1234, value);
  EXPECT_EQ(4, idx);

  value = 0;
  idx = 5;
  EXPECT_TRUE(parseDecimals(value, "1234-56-34", idx, 2));
  EXPECT_EQ(56, value);
  EXPECT_EQ(7, idx);

  value = 0;
  idx = 8;
  EXPECT_TRUE(parseDecimals(value, "1234-56-78", idx, 2));
  EXPECT_EQ(78, value);
  EXPECT_EQ(10, idx);

  value = 0;
  idx = 0;
  EXPECT_FALSE(parseDecimals(value, "123", idx, 4, true));
  idx = 3;
  EXPECT_FALSE(parseDecimals(value, "12:3", idx, 2, true));
}

TEST(WebMapUtilities, ParseWmsIso8601)
{
  using diutil::WmsTime;
  using diutil::parseWmsIso8601;

  { const WmsTime wt = parseWmsIso8601("-123456");
    EXPECT_EQ(WmsTime::YEAR, wt.resolution);
    EXPECT_EQ(-123456, wt.year);
  }
  { const WmsTime wt = parseWmsIso8601("124");
    EXPECT_EQ(WmsTime::INVALID, wt.resolution);
  }
  { const WmsTime wt = parseWmsIso8601("2015-04");
    EXPECT_EQ(WmsTime::MONTH, wt.resolution);
    EXPECT_EQ(2015, wt.year);
    EXPECT_EQ(4,    wt.month);
  }
  { const WmsTime wt = parseWmsIso8601("2015-04-07");
    EXPECT_EQ(WmsTime::DAY, wt.resolution);
    EXPECT_EQ(2015, wt.year);
    EXPECT_EQ(4,    wt.month);
    EXPECT_EQ(7,    wt.day);
  }
  { const WmsTime wt = parseWmsIso8601("2015-04-07T09");
    EXPECT_EQ(WmsTime::INVALID, wt.resolution); // no timezone
  }
  { const WmsTime wt = parseWmsIso8601("2015-04-07T09Z");
    EXPECT_EQ(WmsTime::HOUR, wt.resolution);
    EXPECT_EQ(2015, wt.year);
    EXPECT_EQ(4,    wt.month);
    EXPECT_EQ(7,    wt.day);
    EXPECT_EQ(9,    wt.hour);
    EXPECT_EQ(0,    wt.timezone_hours);
    EXPECT_EQ(0,    wt.timezone_minutes);
  }
  { const WmsTime wt = parseWmsIso8601("2015-04-07T09+01");
    EXPECT_EQ(WmsTime::HOUR, wt.resolution);
    EXPECT_EQ(2015, wt.year);
    EXPECT_EQ(4,    wt.month);
    EXPECT_EQ(7,    wt.day);
    EXPECT_EQ(9,    wt.hour);
    EXPECT_EQ(1,    wt.timezone_hours);
    EXPECT_EQ(0,    wt.timezone_minutes);
  }
  { const WmsTime wt = parseWmsIso8601("2015-04-07T09-02:15");
    EXPECT_EQ(WmsTime::HOUR, wt.resolution);
    EXPECT_EQ(2015, wt.year);
    EXPECT_EQ(4,    wt.month);
    EXPECT_EQ(7,    wt.day);
    EXPECT_EQ(9,    wt.hour);
    EXPECT_EQ(-2,   wt.timezone_hours);
    EXPECT_EQ(-15,  wt.timezone_minutes);
  }
  { const WmsTime wt = parseWmsIso8601("2015-04-07T09:10-01");
    EXPECT_EQ(WmsTime::MINUTE, wt.resolution);
    EXPECT_EQ(2015, wt.year);
    EXPECT_EQ(4,    wt.month);
    EXPECT_EQ(7,    wt.day);
    EXPECT_EQ(9,    wt.hour);
    EXPECT_EQ(10,   wt.minute);
    EXPECT_EQ(-1,   wt.timezone_hours);
    EXPECT_EQ(0,    wt.timezone_minutes);
  }
  { const WmsTime wt = parseWmsIso8601("2015-04-07T09:10:11Z");
    EXPECT_EQ(WmsTime::SECOND, wt.resolution);
    EXPECT_EQ(2015, wt.year);
    EXPECT_EQ(4,    wt.month);
    EXPECT_EQ(7,    wt.day);
    EXPECT_EQ(9,    wt.hour);
    EXPECT_EQ(10,   wt.minute);
    EXPECT_EQ(11,   wt.second);
    EXPECT_EQ(0,    wt.timezone_hours);
    EXPECT_EQ(0,    wt.timezone_minutes);
  }
  { const WmsTime wt = parseWmsIso8601("2015-04-07T09:10:11.22Z");
    EXPECT_EQ(WmsTime::SUBSECOND, wt.resolution);
    EXPECT_EQ(2015,  wt.year);
    EXPECT_EQ(4,     wt.month);
    EXPECT_EQ(7,     wt.day);
    EXPECT_EQ(9,     wt.hour);
    EXPECT_EQ(10,    wt.minute);
    EXPECT_FLOAT_EQ(11.22, wt.second);
    EXPECT_EQ(0,    wt.timezone_hours);
    EXPECT_EQ(0,    wt.timezone_minutes);
  }
}

TEST(WebMapUtilities, WmsTimeToMiTime)
{
  using diutil::WmsTime;
  using diutil::parseWmsIso8601;
  using diutil::to_miTime;

  { const WmsTime wt = parseWmsIso8601("2015-04-07T09:10Z");
    EXPECT_NE(WmsTime::INVALID, wt.resolution);
    const miutil::miTime mt = to_miTime(wt);
    EXPECT_FALSE(mt.undef());
    EXPECT_EQ(miutil::miTime(2015, 4, 7, 9, 10, 0), mt);
  }

  { const WmsTime wt = parseWmsIso8601("2015-04-07");
    EXPECT_NE(WmsTime::INVALID, wt.resolution);
    const miutil::miTime mt = to_miTime(wt);
    EXPECT_FALSE(mt.undef());
    EXPECT_EQ(miutil::miTime(2015, 4, 7, 0, 0, 0), mt);
  }
}

TEST(WebMapUtilities, ParseDouble)
{
  using diutil::detail::parseDouble;

  double value = 0;
  size_t idx = 0;
  EXPECT_TRUE(parseDouble(value, "1234", idx));
  EXPECT_FLOAT_EQ(1234.0, value);
  EXPECT_EQ(4, idx);

  value = 0;
  idx = 0;
  EXPECT_TRUE(parseDouble(value, "1234.0", idx));
  EXPECT_FLOAT_EQ(1234.0, value);
  EXPECT_EQ(6, idx);

  value = 0;
  idx = 0;
  EXPECT_TRUE(parseDouble(value, "1234X", idx));
  EXPECT_FLOAT_EQ(1234.0, value);
  EXPECT_EQ(4, idx);

  value = 0;
  idx = 0;
  EXPECT_TRUE(parseDouble(value, "12.34X", idx));
  EXPECT_FLOAT_EQ(12.34, value);
  EXPECT_EQ(5, idx);

  value = 0;
  idx = 0;
  EXPECT_FALSE(parseDouble(value, "T1234.0", idx));
}

TEST(WebMapUtilities, ParseWmsIso8601Interval)
{
  using diutil::WmsTime;
  using diutil::WmsInterval;
  using diutil::parseWmsIso8601Interval;

  { const WmsInterval wi = parseWmsIso8601Interval("P1Y");
    EXPECT_EQ(WmsTime::YEAR, wi.resolution);
    EXPECT_EQ(1, wi.year);
  }
  { const WmsInterval wi = parseWmsIso8601Interval("P12M");
    EXPECT_EQ(WmsTime::MONTH, wi.resolution);
    EXPECT_EQ(12, wi.month);
  }
  { const WmsInterval wi = parseWmsIso8601Interval("P2Y3M");
    EXPECT_EQ(WmsTime::MONTH, wi.resolution);
    EXPECT_EQ(2, wi.year);
    EXPECT_EQ(3, wi.month);
  }
  { const WmsInterval wi = parseWmsIso8601Interval("PT3M");
    EXPECT_EQ(WmsTime::MINUTE, wi.resolution);
    EXPECT_EQ(3, wi.minute);
  }
  { const WmsInterval wi = parseWmsIso8601Interval("PT4H0M");
    EXPECT_EQ(WmsTime::MINUTE, wi.resolution);
    EXPECT_EQ(4, wi.hour);
  }
  { const WmsInterval wi = parseWmsIso8601Interval("P1YT0S");
    EXPECT_EQ(WmsTime::SECOND, wi.resolution);
    EXPECT_EQ(1, wi.year);
  }
}

TEST(WebMapUtilities, ExpandWmsTimes)
{
  using diutil::expandWmsTimes;

  { const QStringList actual = expandWmsTimes("2015-04-10T12:00:00Z/2015-04-10T21:00:00Z/PT3H");
    const char* expected[] = {
      "2015-04-10T12:00:00Z", "2015-04-10T15:00:00Z",
      "2015-04-10T18:00:00Z", "2015-04-10T21:00:00Z"
    };
    const int N = sizeof(expected)/sizeof(expected[0]);
    ASSERT_LE(N, actual.size());
    EXPECT_EQ(N, actual.size());
    for (int i=0; i<N; ++i)
      EXPECT_EQ(expected[i], actual[i].toStdString()) << "i=" << i;
  }

  { const QStringList actual = expandWmsTimes("2014-12-31T00:00:00Z/2015-01-02T00:00:00Z/PT12H");
    const char* expected[] = {
      "2014-12-31T00:00:00Z", "2014-12-31T12:00:00Z",
      "2015-01-01T00:00:00Z", "2015-01-01T12:00:00Z",
      "2015-01-02T00:00:00Z"
    };
    const int N = sizeof(expected)/sizeof(expected[0]);
    ASSERT_LE(N, actual.size());
    EXPECT_EQ(N, actual.size());
    for (int i=0; i<N; ++i)
      EXPECT_EQ(expected[i], actual[i].toStdString()) << "i=" << i;
  }

  { const QStringList actual = expandWmsTimes("2001-01-01/2001-01-05"); // hack for NVE
    const char* expected[] = { "2001-01-01", "2001-01-02", "2001-01-03", "2001-01-04", "2001-01-05" };
    const int N = sizeof(expected)/sizeof(expected[0]);
    ASSERT_LE(N, actual.size());
    EXPECT_EQ(N, actual.size());
    for (int i=0; i<N; ++i)
      EXPECT_EQ(expected[i], actual[i].toStdString()) << "i=" << i;
  }
}

TEST(WebMapUtilities, ExpandWmsValues)
{
  using diutil::expandWmsValues;

  { const QStringList actual = expandWmsValues("3/12/3");
    const char* expected[] = { "3", "6", "9", "12" };
    const int N = sizeof(expected)/sizeof(expected[0]);
    ASSERT_LE(N, actual.size());
    EXPECT_EQ(N, actual.size());
    for (int i=0; i<N; ++i)
      EXPECT_EQ(expected[i], actual[i].toStdString()) << "i=" << i;
  }
}
