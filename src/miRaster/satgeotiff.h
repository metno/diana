/*
  libmiRaster - met.no tiff interface
  
  $Id: satimg.h 3 2007-09-13 08:15:31Z juergens $

  Copyright (C) 2006 met.no

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
 * �ystein God�y, met.no/FOU, 14/01/1999
 * Ariunaa Bertelsen, ariunaa.bertelsen@smhi.se, 10/09/2009
 */

#ifndef _AUSATGEOTIFF_H
#define _AUSATGEOTIFF_H


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "satimg.h"
#include "ImageCache.h"

#include <puTools/miStringFunctions.h>
#include <puTools/miTime.h>

using namespace std;
using namespace satimg;

namespace metno {
class GeoTiff
{
public:
  // Functions
  //  short selalg(const dto& d, const ucs& upos, const float& hmax, const float& hmin);
  static int JulianDay(satimg::usi yy, satimg::usi mm, satimg::usi dd);
  static int day_night(const std::string& infile);
  static int read_diana(const std::string& infile, unsigned char *image[], int nchan,
			int chan[], satimg::dihead& ginfo);
  static int head_diana(const std::string& infile, satimg::dihead &ginfo);
  static int fillhead_diana(const std::string& str, const std::string& tag, satimg::dihead &ginfo);
};
}

#endif /* _AUSATGEOTIFF_H */
