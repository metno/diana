/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#include "diana_config.h"

#include "diVprofReader.h"

#include <puTools/miStringFunctions.h>

// static
VprofSelectedModel VprofSelectedModel::fromSpaceText(const std::string& text)
{
  VprofSelectedModel sm;
  const std::vector<std::string> vstr = miutil::split(text, " ");
  if (vstr.size() > 0) {
    sm.model = vstr[0];
  }
  if (vstr.size() > 1) {
    sm.reftime = vstr[1];
  }
  return sm;
}

VprofReader::~VprofReader()
{
}

std::vector<std::string> VprofReader::getReferencetimes(const std::string&)
{
  return std::vector<std::string>();
}
