/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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
#ifndef diVcrossField_h
#define diVcrossField_h

#include "diVcrossSource.h"

#include <map>

class VcrossPlot;
class FieldManager;

/**
  \brief Vertical Crossection prognostic data from a field source
*/
class VcrossField : public VcrossSource
{
public:
  VcrossField(const std::string& modelname, FieldManager* fieldm);
  virtual ~VcrossField();
  virtual void cleanup();
  virtual bool update();

  virtual std::vector<std::string> getParameterIds() const
    { return params; }

  virtual const crossections_t& getCrossections() const
    { return mCrossections; }

  virtual const std::vector<miutil::miTime>& getTimes() const
    { return validTime; }

  virtual VcrossData* getCrossData(const std::string& csname, const std::set<std::string>& parameters, const miutil::miTime& time);
  virtual VcrossData* getTimeData(const std::string& csname, const std::set<std::string>& parameters, int csPositionIndex);

  std::string setLatLon(float lat, float lon);

private:
  int findCrossection(const std::string& csname) const;

private:
  FieldManager* fieldManager;

  crossections_t mCrossections;

  std::vector<miutil::miTime> validTime;
  std::vector<int> forecastHour;
  std::vector<std::string> params;

  // Holds positions clicked on the map
  float startLatitude;
  float startLongitude;
  float stopLatitude;
  float stopLongitude;
};

#endif // diVcrossField_h
