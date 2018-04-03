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

#ifndef _diMapMode_h
#define _diMapMode_h

#include <diKeys.h>

/// The state of the applications main work area
enum mapMode{
  normal_mode,  // normal map-zoom, -pan etc.
  fedit_mode,   // edit fields
  draw_mode,    // drawing of symbols
  combine_mode  // combine products
};


enum field_modes{
  field_manip   // field manipulation tools
};

enum object_modes{
  front_drawing,  // draw fronts
  symbol_drawing, // draw symbols, free text etc
  area_drawing    // draw areas
};

enum combine_modes{
  set_region,     // select region
  set_borders     // edit border
};

#endif // _diMapMode_h
