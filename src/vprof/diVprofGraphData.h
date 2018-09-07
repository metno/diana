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

#ifndef diVprofGraphData_h
#define diVprofGraphData_h

#include "diField/diFieldDefined.h"

#include <memory>
#include <string>

class VprofGraphData
{
public:
  VprofGraphData(const std::string& id);
  virtual ~VprofGraphData();

  const std::string id() const { return id_; }

  virtual float z(size_t index) const = 0;
  virtual float x(size_t index) const = 0;

  virtual const std::string& zUnit() const = 0;
  virtual const std::string& xUnit() const = 0;

  virtual bool empty() const { return length() == 0; }
  virtual size_t length() const = 0;
  virtual difield::ValuesDefined defined() const = 0;

private:
  std::string id_;
};

typedef std::shared_ptr<const VprofGraphData> VprofGraphData_cp;

#endif // diVprofGraphData_h
