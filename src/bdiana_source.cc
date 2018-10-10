/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017 met.no

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

#include "bdiana_source.h"

BdianaSource::BdianaSource()
    : use_time_(USE_LASTTIME)
{
}

BdianaSource::~BdianaSource()
{
}

bool BdianaSource::hasCutout()
{
  return false;
}

QRectF BdianaSource::cutout()
{
  return QRectF();
}

miutil::miTime BdianaSource::getTime()
{
  if (getTimeChoice() == USE_REFERENCETIME)
    return getReferenceTime();

  const plottimes_t times = getTimes();
  if (times.empty())
    return miutil::miTime::nowTime();

  if (getTimeChoice() == USE_NOWTIME) {
    const miutil::miTime now = miutil::miTime::nowTime();
    // select closest to now without overstepping
    for (plottimes_t::const_iterator it = times.begin(); it != times.end(); ++it) {
      if (*it >= now)
        return *((it != times.begin()) ? (--it) : it);
    }
  }
  if (getTimeChoice() == USE_FIRSTTIME)
    return *times.begin();

  // both USE_LASTTIME and not-found for USE_NOWTIME
  return *times.rbegin();
}

void BdianaSource::setTimeChoice(TimeChoice tc)
{
  use_time_ = tc;
}
