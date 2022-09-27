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

/* Changed by Lisbeth Bergholt 1999
 * RETURN VALUES:
 *-1 - Something is rotten
 * 0 - Normal and correct ending - no palette
 * 1 - Only header read
 * 2 - Normal and correct ending - palette-color image
 *
 * MITIFF_head_diana reads only image head
 */

/*
 * PURPOSE:
 * Read and decode TIFF image files with satellite data on the
 * multichannel format.
 *
 * RETURN VALUES:
 * 0 - Normal and correct ending - no palette
 * 2 - Normal and correct ending - palette-color image
 *
 * NOTES:
 * At present only single strip images are supported.
 *
 * REQUIRES:
 * Calls to other libraries:
 * The routine use the libtiff version 3.0 to read TIFF files.
 *
 * AUTHOR: Oystein Godoy, DNMI, 05/05/1995, changed by LB
 */

#include "satimg.h"

#include "diProjection.h"

#define MILOGGER_CATEGORY "metno.satimg"
#include "miLogger/miLogging.h"

using namespace miutil;

static const float DEG_TO_RAD = M_PI / 180;
static const float RAD_TO_DEG = 180 / M_PI;

bool satimg::proj4_value(std::string& proj4, const std::string& key, double& value, bool remove_key)
{
  const size_t pos = proj4.find(key);
  if (pos == std::string::npos)
    return false;

  value = atof(proj4.c_str() + pos + key.size());
  if (remove_key) {
    size_t pos_end = proj4.find(" ", pos);
    if (pos_end == std::string::npos)
      pos_end = proj4.size() - 1; // proj4 cannot be not empty
    proj4.erase(pos, pos_end - pos + 1);
  }

  return true;
}

namespace {
typedef unsigned short int usi;

// structs dto and ucs are used only by SatManager day_night, in a call to selalg
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
  float corner_x[4]; //!< x => lon_deg
  float corner_y[4]; //!< y => lat_deg
};

short selalg(const dto& d, const ucs& upos, const float& hmax, const float& hmin);
} // namespace


int satimg::day_night(const dihead& sinfo)
{
  ucs upos;
  unsigned int countx = 0, county = 0;
  for (int i = 0; i < 4; i++) {
    upos.corner_x[i] = sinfo.Bx + sinfo.Ax * countx;
    upos.corner_y[i] = sinfo.By + sinfo.Ay * county;

    countx += sinfo.xsize;
    if (countx > sinfo.xsize) {
      countx = 0;
      county += sinfo.ysize;
    }
  }
  sinfo.projection.convertToGeographic(4, upos.corner_x, upos.corner_y);

  struct dto d;
  d.ho = sinfo.time.hour();
  d.mi = sinfo.time.min();
  d.dd = sinfo.time.day();
  d.mm = sinfo.time.month();
  d.yy = sinfo.time.year();

  return selalg(d, upos, 5., -2.); // Why 5 and -2? From satsplit.c
}

/*
 * FUNCTION:
 * JulianDay
 *
 * PURPOSE:
 * Computes Julian day number (day of the year).
 *
 * RETURN VALUES:
 * Returns the Julian Day.
 */

static const unsigned short int days_after_month[12] = {
  31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};

int satimg::JulianDay(usi yy, usi mm, usi dd)
{
  int dn = dd;
  if (mm >= 2 && mm < 12) {
    const int index = mm - 1 /* previous month */ - 1 /* 0-based */;
    dn += days_after_month[index];

    if (mm >= 3) {
      const bool is_leap = ((yy%4 == 0 && yy%100 != 0) || yy%400 == 0);
      if (is_leap)
        dn += 1;
    }
  }

  return dn;
}

namespace {
/*
 * NAME:
 * selalg
 *
 * PURPOSE:
 * This file contains functions that are used for selecting the correct
 * algoritm according to the available daylight in a satellite scene.
 * The algoritm uses only corner values to chose from.
 *
 * AUTHOR:
 * Oystein Godoy, met.no/FOU, 23/07/1998
 * MODIFIED:
 * Oystein Godoy, met.no/FOU, 06/10/1998
 * Selection conditions changed. Error in nighttime test removed.
 */
short selalg(const dto& d, const ucs& upos, const float& hmax, const float& hmin)
{
  float max = 0, min = 0;

  // Decode day and time information for use in formulas.
  const float daynr = satimg::JulianDay(d.yy, d.mm, d.dd);
  const float gmttime = d.ho + d.mi / 60.0;

  const float theta0 = (2 * M_PI * daynr) / 365;
  const float inclination = 0.006918 - (0.399912 * cos(theta0)) + (0.070257 * sin(theta0)) - (0.006758 * cos(2 * theta0)) + (0.000907 * sin(2 * theta0)) -
                            (0.002697 * cos(3 * theta0)) + (0.001480 * sin(3 * theta0));

  for (int i = 0; i < 4; i++) {
    const float lon_deg = upos.corner_x[i];
    const float lat_deg = upos.corner_y[i], lat_rad = lat_deg * DEG_TO_RAD;

    // Estimates zenith angle in the pixel
    const float lat = gmttime + lon_deg * 0.0667;
    const float hourangle = std::abs(lat - 12.) * 0.2618;

    const float coszenith = (cos(lat_rad) * cos(hourangle) * cos(inclination)) + (sin(lat_rad) * sin(inclination));
    const float sunh = 90. - acosf(coszenith) * RAD_TO_DEG;

    if (sunh < min) {
      min = sunh;
    } else if ( sunh > max) {
      max = sunh;
    }
  }

  /*
   hmax and hmin are input variables to the function determining
   the maximum and minimum sunheights. A twilight scene is defined
   as being in between these limits. During daytime scenes all corners
   of the image have sunheights larger than hmax, during nighttime
   all corners have sunheights below hmin (usually negative values).

   Return values,
   0= no algorithm chosen (twilight)
   1= nighttime algorithm
   2= daytime algorithm.
   */
  if (max > hmax && fabs(max) > (fabs(min)+hmax)) {
    return 2;
  } else if (min < hmin && fabs(min) > (fabs(max)+hmin)) {
    return 1;
  } else {
    return 0;
  }
}
} // namespace
