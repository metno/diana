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

#ifndef diVprofValues_h
#define diVprofValues_h

#include "diVprofGraphData.h"
#include "diVprofText.h"

#include <memory>
#include <set>
#include <vector>

class VprofValues
{
public:
  VprofValues();
  virtual ~VprofValues();

  virtual miutil::ValuesDefined isDefined() const = 0;
  virtual VprofGraphData_cp series(const std::string& id) const = 0;

  VprofText text;
  bool prognostic;
};

typedef std::shared_ptr<VprofValues> VprofValues_p;
typedef std::shared_ptr<const VprofValues> VprofValues_cp;
typedef std::vector<VprofValues_cp> VprofValues_cpv;

#endif
