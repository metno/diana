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

#include "time_util.h"
#include "nearest_element.h"

#include <puTools/miStringFunctions.h>

#include <iomanip>
#include <sstream>

namespace miutil {

const miutil::miTime unix_t0(1970, 1, 1, 0, 0, 0);

miTime addSec(const miTime& t, int seconds)
{
  miTime tt(t);
  tt.addSec(seconds);
  return tt;
}

miTime addMin(const miTime& t, int minutes)
{
  miTime tt(t);
  tt.addMin(minutes);
  return tt;
}

miTime addHour(const miTime& t, int hours)
{
  miTime tt(t);
  tt.addHour(hours);
  return tt;
}

plottimes_t::const_iterator nearest(const plottimes_t& times, const miTime& time)
{
  return diutil::nearest_element(times, time, miTime::secDiff);
}

plottimes_t::const_iterator step_time(const plottimes_t& times, const miTime& time, int steps)
{
  plottimes_t::const_iterator it = times.find(time);
  if (it != times.end()) {
    if (steps > 0) {
      while (it != times.end() && (--steps >= 0))
        ++it;
    } else if (steps < 0) {
      while (it != times.begin() && (++steps <= 0))
        --it;
    }
  }
  if (it == times.end() && !times.empty())
    it = --times.end();
  return it;
}

plottimes_t::const_iterator step_time(const plottimes_t& times, const miTime& time, const miTime& newTime)
{
  plottimes_t::const_iterator it = times.find(time);
  if (it != times.end()) {
    if (newTime > time) {
      while (it != times.end() && *it < newTime)
        ++it;
    } else {
      while (it != times.begin() && *it > newTime)
        --it;
    }
  }
  if (it == times.end() && !times.empty())
    it = --times.end();
  return it;
}

std::string stringFromTime(const miutil::miTime& t, bool addMinutes)
{
  static const std::string fmt = "%Y%m%d%H", fmt_min = fmt + "%M";
  return t.format(addMinutes ? fmt_min : fmt);
}

miutil::miTime timeFromString(const std::string& timeString)
{
  if (timeString.length() == 10 || timeString.length() == 12)
    return miutil::miTime(timeString);
  else
    return miutil::miTime();
}

} // namespace miutil
