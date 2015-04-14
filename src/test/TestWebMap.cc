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
