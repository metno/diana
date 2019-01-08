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
 * satellite geometry.
 *
 * AUTHOR:
 * Øystein Godøy, met.no/FOU, 14/01/1999
 */

#ifndef _AUSAT_H
#define _AUSAT_H

#include <puTools/miTime.h>

#include <vector>

namespace satimg {

typedef unsigned short int usi;

/*
   * Old structure to hold information for diana, this one was originally
   * created for use with met.no/TIFF files.
   */
struct dihead
{
  std::string satellite;
  miutil::miTime time;
  std::string channel;
  unsigned int zsize;
  unsigned int xsize;
  unsigned int ysize;
  std::string proj_string;
  float Ax; ///< === Sat::area.resolutionX
  float Ay; ///< === Sat::area.resolutionY
  float Bx; ///< === Sat::area.rect.x1
  float By; ///< === Sat::area.rect.y1
  // calibration for visual and infrared channels
  std::string cal_vis;
  std::string cal_ir;
  // table calibration
  std::vector<std::string> cal_table;
  float AIr;
  float BIr;
  // information for color palette imagery.
  std::string name;
  usi noofcl;
  std::vector<std::string> clname;
  unsigned short int cmap[3][256];
  // Info from setup file.
  std::string metadata;
  std::string channelinfo;
  std::string paletteinfo;
  int hdf5type;
  float nodata;
};

// structs dto and ucs are used only by SatManager day_night, in a
// call to selalg

struct dto
{
  usi ho; /* satellite hour */
  usi mi; /* satellite minute */
  usi dd; /* satellite day */
  usi mm; /* satellite month */
  usi yy; /* satellite year */
};

struct ucs
{
  float Bx;        /* UCS Bx */
  float By;        /* UCS By */
  float Ax;        /* UCS Ax */
  float Ay;        /* UCS Ay */
  unsigned int iw; /* image width (pixels) */
  unsigned int ih; /* image height (pixels) */
};

// Functions
short selalg(const dto& d, const ucs& upos, const float& hmax, const float& hmin);
int JulianDay(usi yy, usi mm, usi dd);
int day_night(const std::string& infile);
int MITIFF_read_diana(const std::string& infile, unsigned char* image[], int nchan, int chan[], dihead& ginfo);
int MITIFF_head_diana(const std::string& infile, dihead& ginfo);

} // namespace satimg

#endif /* _AUSAT_H */
