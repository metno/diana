/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2022 met.no

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

#include "diObsData.h"

ObsDataBasic::ObsDataBasic()
{
  clear();
}

void ObsDataBasic::clear()
{
  dataType.clear();
  id.clear();
  xpos = ypos = -1000;
  obsTime = miutil::miTime();
}

ObsDataMetar::ObsDataMetar()
{
  clear();
}

void ObsDataMetar::clear()
{
  ship_buoy = false;
  metarId.clear();
  CAVOK = false;
  REww.clear();
  ww.clear();
  cloud.clear();
}

void ObsData::clear_data()
{
  basic_.clear();
  metar_.clear();
  fdata.clear();
  stringdata.clear();
}

const std::string* ObsData::get_string(const std::string& key) const
{
  stringdata_t::const_iterator it = stringdata.find(key);
  if (it != stringdata.end())
    return &it->second;
  else
    return nullptr;
}

const float* ObsData::get_float(const std::string& key) const
{
  fdata_t::const_iterator it = fdata.find(key);
  if (it != fdata.end())
    return &it->second;

  return nullptr;
}
