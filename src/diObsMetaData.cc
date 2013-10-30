/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diObsAscii.cc 2603 2011-08-15 08:58:42Z lisbethb $

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diObsMetaData.h>
#include <diObsPlot.h>
#include <vector>
#include <curl/curl.h>


void ObsMetaData::addStationsToUrl(std::string& filename)
{

  std::string string;
  std::map<std::string, ObsData>::iterator p = metaData.begin();
  for ( ; p != metaData.end(); ++p ) {
    string +=("&s=" + p->second.id);
  }

  miutil::replace(filename, "STATIONS",string);
}

