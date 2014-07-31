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

#include "diObsMetaData.h"

#include <puTools/miStringFunctions.h>

void ObsMetaData::addStationsToUrl(std::string& filename) const
{
  std::string txt;
  for (string_ObsData_m::const_iterator p = metaData.begin(); p != metaData.end(); ++p)
    txt += "&s=" + p->second.id;

  miutil::replace(filename, "STATIONS", txt);
}
