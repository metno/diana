/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013 met.no

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

#ifndef diVcrossSource_h
#define diVcrossSource_h

#include <diField/diVcrossData.h>

#include <puTools/miTime.h>

#include <set>
#include <string>
#include <vector>

class LocationElement;
class VcrossData;

class VcrossSource
{
public:
  typedef std::vector<VcrossData::Cut> crossections_t;
  typedef std::vector<miutil::miTime> times_t;

public:
  VcrossSource(const std::string& modelName)
    : mModelName(modelName) { }

  virtual ~VcrossSource();

  virtual void cleanup() { };
  virtual bool update() = 0;
  virtual std::vector<std::string> getParameterIds() const = 0;

  /** @return a list of all crossections from this source */
  virtual const crossections_t& getCrossections() const = 0;

  /** @return a list of all times for which the crossections from this source have data */
  virtual const times_t& getTimes() const = 0;

  /** Get data for a crossection.
   */
  virtual VcrossData* getCrossData(const std::string& cs, const std::set<std::string>& parameters, const miutil::miTime& time) = 0;
  virtual VcrossData* getTimeData(const std::string& cs, const std::set<std::string>& parameters, int csIndex) = 0;

  const std::string& modelName() const
    { return mModelName; }

protected:
  std::string mModelName;
};

#endif // diVcrossSource_h
