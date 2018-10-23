/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018 met.no

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

#ifndef diVprofSimpleValues_h
#define diVprofSimpleValues_h

#include "diVprofValues.h"

struct VprofSimpleValues : public VprofValues
{
private:
  typedef std::vector<VprofGraphData_cp> series_v;

public:
  VprofSimpleValues();

  void add(VprofGraphData_cp s);
  void calculate();
  static void addCalculationInputVariables(std::set<std::string>& variables);

  difield::ValuesDefined isDefined() const override { return defined_; }
  VprofGraphData_cp series(const std::string& id) const override;

private:
  difield::ValuesDefined defined_;
  series_v series_;
};

typedef std::shared_ptr<VprofSimpleValues> VprofSimpleValues_p;
typedef std::shared_ptr<const VprofSimpleValues> VprofSimpleValues_cp;

#endif // diVprofSimpleValues_h
