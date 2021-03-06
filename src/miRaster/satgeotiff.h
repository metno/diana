/*
  libmiRaster - met.no tiff interface

  Copyright (C) 2006-2019 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
 * PURPOSE:
 * Header file for module reading AVHRR 10 bits data and computing
 * satellite geometry. Changed for reading geotiff satellite files. 
 * 
 * AUTHOR:
 * Øystein Godøy, met.no/FOU, 14/01/1999
 * Ariunaa Bertelsen, ariunaa.bertelsen@smhi.se, 10/09/2009
 */

#ifndef AUSATGEOTIFF_H
#define AUSATGEOTIFF_H

#include "satimg.h"

namespace metno {
class GeoTiff
{
public:
  static int read_diana(const std::string& infile, unsigned char* image[], int nchan, int chan[], satimg::dihead& ginfo);
  static int head_diana(const std::string& infile, satimg::dihead &ginfo);
};

} // namespace metno

#endif /* AUSATGEOTIFF_H */
