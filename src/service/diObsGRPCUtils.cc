/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2022 met.no

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

#include "diObsGRPCUtils.h"

namespace diutil {
namespace grpc {
namespace obs {

void copyFromMiTime(diana_obs_v0::Time& ot, const miutil::miTime& mt)
{
  ot.set_year(mt.year());
  ot.set_month(mt.month());
  ot.set_day(mt.day());
  ot.set_hour(mt.hour());
  ot.set_minute(mt.min());
  ot.set_second(mt.sec());
  ot.set_sub_second(0);
}

miutil::miTime toMiTime(const diana_obs_v0::Time& ot)
{
  return miutil::miTime(ot.year(), ot.month(), ot.day(), ot.hour(), ot.minute(), ot.second());
}

std::set<miutil::miTime> timesFromTimeSpans(const diana_obs_v0::TimesResult& res)
{
  std::set<miutil::miTime> times_;
  for (const auto& rt : res.timespans()) {
    const miutil::miTime tb = toMiTime(rt.begin());
    times_.insert(tb);
    if (rt.has_end() && rt.step() > 0) {
      const float step = std::max(60.0f, rt.step()); // refuse steps < 60 seconds
      const miutil::miTime te = toMiTime(rt.end());
      miutil::miTime t = tb;
      while (true) {
        t.addSec(step);
        if (t >= te)
          break;
        times_.insert(t);
      }
      times_.insert(te);
    }
  }
  return times_;
}

} // namespace obs
} // namespace grpc
} // namespace diutil
