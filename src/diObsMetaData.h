/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diObsData.h 2025 2010-05-18 13:20:28Z lisbethb $

  Copyright (C) 2006 met.no

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
#ifndef diObsMetaData_h
#define diObsMetaData_h

#include <diObsData.h>

#include <map>
#include <string>

/**
  \brief Observation metadata
*/
class ObsMetaData
{
public:
  typedef std::map<std::string, ObsData > string_ObsData_m;
  
  void setMetaData(const string_ObsData_m& obs)
    { metaData = obs; }
  const string_ObsData_m& getMetaData() const
    { return metaData; }

  void addStationsToUrl(std::string& url) const;

private:
  string_ObsData_m metaData;
};

#endif
