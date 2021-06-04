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

#ifndef PLOTSTATUS_H
#define PLOTSTATUS_H

enum PlotStatusValue {
  P_UNKNOWN,  //! unknown or irrelevant status
  P_WAITING,  //!< waiting for data
  P_OK_EMPTY, //!< plot is good but has no data
  P_OK_DATA,  //!< plot is good and has data
  P_ERROR,    //!< error in plot
  P_STATUS_MAX = P_ERROR
};

class PlotStatus
{
public:
  PlotStatus();
  explicit PlotStatus(PlotStatusValue ps, int n = 1);

  bool operator==(const PlotStatus& other) const;
  bool operator!=(const PlotStatus& other) const { return !(*this == other); }

  void add(PlotStatusValue ps, int n = 1) { statuscounts_[ps] += n; }
  void add(const PlotStatus& pcs);
  int get(PlotStatusValue ps) const { return statuscounts_[ps]; }
  int count() const;

private:
  int statuscounts_[P_STATUS_MAX + 1];
};

#endif // PLOTSTATUS_H
