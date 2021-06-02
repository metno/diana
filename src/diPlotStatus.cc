/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2021 met.no

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

#include "diPlotStatus.h"

#include <numeric>

PlotStatus::PlotStatus()
    : statuscounts_{} // init with zeroes
{
}

PlotStatus::PlotStatus(PlotStatusValue ps, int n)
    : PlotStatus()
{
  add(ps, n);
}

bool PlotStatus::operator==(const PlotStatus& other) const
{
  for (int i = 0; i <= P_STATUS_MAX; ++i)
    if (statuscounts_[i] != other.statuscounts_[i])
      return false;
  return true;
}

void PlotStatus::add(const PlotStatus& pcs)
{
  for (int i = 0; i <= P_STATUS_MAX; ++i)
    statuscounts_[i] += pcs.statuscounts_[i];
}

int PlotStatus::count() const
{
  return std::accumulate(statuscounts_, statuscounts_ + P_STATUS_MAX + 1, 0);
}
