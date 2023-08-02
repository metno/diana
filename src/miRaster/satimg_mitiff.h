/*
  libmiRaster - met.no tiff interface

  Copyright (C) 2006-2022 met.no

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
 * satellite geometry.
 *
 * AUTHOR:
 * Øystein Godøy, met.no/FOU, 14/01/1999
 */

#ifndef DIANA_SATIMG_MITIFF_H
#define DIANA_SATIMG_MITIFF_H

#include "satimg.h"

namespace satimg {

int MITIFF_read_diana(const std::string& infile, unsigned char* image[], int nchan, const int chan[], dihead& ginfo);
int MITIFF_head_diana(const std::string& infile, dihead& ginfo);

int MITIFF_day_night(const std::string& infile);

} // namespace satimg

#endif /* DIANA_SATIMG_MITIFF_H */
