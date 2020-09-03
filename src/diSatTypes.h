/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2020 met.no

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

#ifndef DIANA_DISATTYPES_H
#define DIANA_DISATTYPES_H

#include "diField/diProjection.h"

#include <puTools/miTime.h>

#include <vector>

//--------------------------------------------------
// image
//--------------------------------------------------

/**
   \brief GUI data for one geo image source
*/
struct SatFileInfo
{
  std::string name;
  std::string formattype; // mitiff or hdf5
  std::string metadata;
  Projection projection;
  std::string channelinfo;
  std::string paletteinfo;
  int hdf5type;
  miutil::miTime time;
  bool opened;
  std::vector<std::string> channel;
  bool palette;            // palette or rgb file
  SatFileInfo()
      : opened(false)
      , palette(false)
  {
  }
};

/**
   \brief geo image data value in one position
*/
struct SatValues
{
  std::string channel;
  float value;      ///<rgb
  std::string text; ///<palette
};

#endif // DIANA_DISATTYPES_H
