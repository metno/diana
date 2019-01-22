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

#include "diana_config.h"

#include "satimg.h"

#include <puTools/miStringFunctions.h>
#include <sstream>
#include <tiffio.h>

#define MILOGGER_CATEGORY "metno.MiTiff"
#include "miLogger/miLogging.h"

using namespace miutil;

namespace {
const int MAXCHANNELS = 64;

int fillhead_diana(const std::string& str, const std::string& tag, satimg::dihead& ginfo, float& gridRot, float& trueLat);

struct CloseTIFF
{
  void operator()(TIFF* tiff) { TIFFClose(tiff); }
};
} // namespace

int satimg::MITIFF_read_diana(const std::string& infile, unsigned char* image[], int nchan, int chan[], dihead& ginfo)
{
  ginfo.noofcl = 0;

  const int pal = MITIFF_head_diana(infile, ginfo);
  if (pal == -1)
    return -1;

  if (nchan == 0) {
    return 1;
  }

  std::unique_ptr<TIFF, CloseTIFF> in(TIFFOpen(infile.c_str(), "rc"));
  if (!in) {
    METLIBS_LOG_ERROR("TIFFOpen failed, probably not a TIFF file: '" << infile << "'");
    return -1;
  }

  // Read image data into matrix.
  TIFFGetField(in.get(), 256, &ginfo.xsize);
  TIFFGetField(in.get(), 257, &ginfo.ysize);
  const int size = ginfo.xsize * ginfo.ysize;

  /*
   * Memory allocated for image data in this function (*image) is freed
   * in function main process.
   */
  if (ginfo.zsize > MAXCHANNELS) {
    METLIBS_LOG_ERROR("NOT ENOUGH POINTERS AVAILABLE TO HOLD DATA!");
    return -1;
  }

  for (int i = 0; i < nchan; i++) {
    if(i!=0 || chan[0]!=0){ /*TIFFsetDirectory chrashes if chan[0]=0,why??*/
      if (!TIFFSetDirectory(in.get(), chan[i]))
        return -1;
    }
    image[i] = new unsigned char [size+1];
    if (!image[i])
      return -1;

    int compression = COMPRESSION_NONE;
    if (!TIFFGetField(in.get(), TIFFTAG_COMPRESSION, &compression))
      compression = COMPRESSION_NONE;

    int status;
    if (compression == COMPRESSION_NONE)
      status = TIFFReadRawStrip(in.get(), 0, image[i], size);
    else
      status = TIFFReadEncodedStrip(in.get(), 0, image[i], size);
    if (status == -1)
      return -1;
  }

  return pal;
}

int satimg::MITIFF_head_diana(const std::string& infile, dihead& ginfo)
{
  METLIBS_LOG_SCOPE();
  std::unique_ptr<TIFF, CloseTIFF> in(TIFFOpen(infile.c_str(), "rc"));
  if (!in) {
    METLIBS_LOG_ERROR("TIFFOpen failed, probably not a TIFF file: '" << infile << "'");
    return -1;
  }

  const std::string fieldname[] = {"Satellite:",
                                   "Date and Time:",
                                   "SatDir:",
                                   "Channels:",
                                   "In this file:",
                                   "Xsize:",
                                   "Ysize:",
                                   "Map projection:",
                                   "Proj string:",
                                   "TrueLat:",
                                   "GridRot:",
                                   "Xunit:",
                                   "Yunit:",
                                   "NPX:",
                                   "NPY:",
                                   "Ax:",
                                   "Ay:",
                                   "Bx:",
                                   "By:",
                                   "Calibration VIS:",
                                   "Calibration IR:",
                                   "Table_calibration:",
                                   "COLOR INFO:",
                                   "NWP INFO:"};
  const size_t FIELDS = sizeof(fieldname) / sizeof(fieldname[0]);

  float gridRot = 60, trueLat = 0;
  ginfo.noofcl = 0;

  // Test whether this is a color palette image or not. pmi==3 => color palette
  short pmi;
  if (TIFFGetField(in.get(), 262, &pmi) && pmi == 3) {
    unsigned short int *red, *green, *blue;
    if (!TIFFGetField(in.get(), 320, &red, &green, &blue)) {
      return 2;
    }
    for (int i=0; i<256; i++) {
      ginfo.cmap[0][i] = red[i];
      ginfo.cmap[1][i] = green[i];
      ginfo.cmap[2][i] = blue[i];
    }
  }

  char* description = 0;
  if (!TIFFGetField(in.get(), 270, &description) || !description)
    return -1;
  std::string desc_str(description);

  // read all common fields
  while(true){
    //Find key word
    size_t i= 0;
    size_t nn = desc_str.npos;
    while (nn==desc_str.npos && i<FIELDS) {
      nn = desc_str.find(fieldname[i]);
      i++;
    }
    if (i == FIELDS)
      break; // no key word found
    i--;
    nn += fieldname[i].size();
    desc_str = desc_str.substr(nn,desc_str.size()-nn+1);
    //find next key word
    size_t j = 0;
    size_t mm = std::string::npos;
    while (mm == std::string::npos && j < FIELDS) {
      mm = desc_str.find(fieldname[j]);
      j++;
    }
    std::string value;
    if (j < FIELDS + 1)
      value = desc_str.substr(0,mm);
    else
      value = desc_str;

    miutil::trim(value);
    if (fillhead_diana(value, fieldname[i], ginfo, gridRot, trueLat) != 0)
      return -1;
  }

  // If the mitiff image contains no proj string, it is probably transformed to +R=6371000
  // and adjusted to fit nwp-data and maps.
  // These adjustments require no conversion between +R=6371000 and ellps=WGS84,
  // and therefore no +datum or +towgs84 are given.
  if (ginfo.proj_string.empty()) {
    std::ostringstream proj4;
    proj4 << "+proj=stere";
    proj4 << " +lon_0=" << gridRot;
    proj4 << " +lat_ts=" << trueLat;
    proj4 << " +lat_0=90";
    proj4 << " +R=6371000";
    proj4 << " +units=km";
    proj4 << " +x_0=" << (ginfo.Bx * -1000.);
    proj4 << " +y_0=" << (ginfo.By * -1000.) + (ginfo.Ay * ginfo.ysize * 1000.);
    ginfo.proj_string = proj4.str();
  }

  ginfo.Bx = ginfo.By = 0;

  return (pmi == 3) ? 2 : 0;
}

namespace {
/*
 * PURPOSE:
 * Fillhead extracts the standard information in the header.
 */
int fillhead_diana(const std::string& str, const std::string& tag, satimg::dihead& ginfo, float& gridRot, float& trueLat)
{
  if (tag == "Satellite:")
    ginfo.satellite = str;

  else if (tag == "Date and Time:") {
    //Format hour:min day/month-year
    int hour,minute,day,month,year;
    if (sscanf(str.c_str(), "%2d:%2d %2d/%2d-%4d", &hour, &minute, &day, &month, &year) != 5)
      return 1; // invalid time
    if (year == 0)
      year = 2000;
    if (hour == 24) {
      hour = 0;
      ginfo.time = miTime(year, month, day, hour, minute, 0);
      ginfo.time.addDay(1);
    } else {
      ginfo.time = miTime(year, month, day, hour, minute, 0);
    }
  }

  else if (tag =="Channels:")
    ginfo.zsize = (unsigned short int) atoi(str.c_str());

  else if (tag == "In this file:") ginfo.channel = str;

  else if (tag == "Xsize:") ginfo.xsize = atoi(str.c_str());
  else if (tag == "Ysize:") ginfo.ysize = atoi(str.c_str());
  else if (tag == "Proj string:") ginfo.proj_string = str;
  else if (tag == "TrueLat:") {
    trueLat = miutil::to_float(str);
    if (miutil::contains(str, "S"))
      trueLat *= -1.0;
  } else if (tag == "GridRot:")
    gridRot = miutil::to_float(str);
  else if (tag == "Bx:") ginfo.Bx = (float) atof(str.c_str());
  else if (tag == "By:") ginfo.By = (float) atof(str.c_str());
  else if (tag == "Ax:") ginfo.Ax = (float) atof(str.c_str());
  else if (tag == "Ay:") ginfo.Ay = (float) atof(str.c_str());
  else if (tag == "Calibration VIS:") ginfo.cal_vis = str;
  else if (tag == "Calibration IR:")  ginfo.cal_ir  = str;
  else if (tag == "Table_calibration:")  ginfo.cal_table.push_back(str);
  else if (tag == "COLOR INFO:"){
    const std::vector<std::string> token = miutil::split(str, "\n", false);
    int ntoken=token.size();
    if (ntoken < 3)
      return 1;
    ginfo.name = token[0];
    ginfo.noofcl = atoi(token[1].c_str());
    if (ntoken - 2 < ginfo.noofcl)
      ginfo.noofcl = ntoken - 2;
    if (ntoken - 2 > ginfo.noofcl)
      ntoken = ginfo.noofcl + 2;
    for (int i=2; i<ntoken; i++)
      ginfo.clname.push_back(token[i]);
  }

  return 0;
}

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
  float Bx;        /* UCS Bx */
  float By;        /* UCS By */
  float Ax;        /* UCS Ax */
  float Ay;        /* UCS Ay */
  unsigned int iw; /* image width (pixels) */
  unsigned int ih; /* image height (pixels) */
};

short selalg(const dto& d, const ucs& upos, const float& hmax, const float& hmin);
} // namespace

int satimg::day_night(const std::string& infile)
{
  dihead sinfo;
  if (MITIFF_head_diana(infile, sinfo) != 0)
    return -1;
  return day_night(sinfo);
}

int satimg::day_night(const dihead& sinfo)
{
  struct ucs upos;
  struct dto d;
  upos.Ax = sinfo.Ax;
  upos.Ay = sinfo.Ay;
  upos.Bx = sinfo.Bx;
  upos.By = sinfo.By;
  upos.iw = sinfo.xsize;
  upos.ih = sinfo.ysize;
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
  float max = 0., min = 0.;

  const float Pi = 3.141592654;
  const double radian = Pi / 180.;

  const float CentralMer = 0.;
  const float TrueScaleLat = 60.;
  const double TrueLatRad = TrueScaleLat * radian;
  const float DistPolEkv = 6378. * (1. + sin(TrueLatRad));

  // Decode day and time information for use in formulas.
  const float daynr = satimg::JulianDay(d.yy, d.mm, d.dd);
  const float gmttime = d.ho + d.mi / 60.0;

  const float theta0 = (2 * Pi * daynr) / 365;
  const float inclination = 0.006918 - (0.399912 * cos(theta0)) + (0.070257 * sin(theta0)) - (0.006758 * cos(2 * theta0)) + (0.000907 * sin(2 * theta0)) -
                            (0.002697 * cos(3 * theta0)) + (0.001480 * sin(3 * theta0));

  //  Estimates latitude and longitude for the corner pixels.
  unsigned int countx = 0, county = 0;
  for (int i = 0; i < 4; i++) {
    const float xval = upos.Bx + upos.Ax * ((float)countx + 0.5);
    const float yval = upos.By - fabsf(upos.Ay) * ((float)county + 0.5);

    countx += upos.iw;
    if (countx > upos.iw) {
      countx = 0;
      county += upos.ih;
    }
    const float northings = yval;
    const float eastings = xval;

    const double Rp = pow(double(eastings * eastings + northings * northings), 0.5);

    const float latitude = (90. - (1. / radian) * atan(Rp / DistPolEkv) * 2) * radian;
    const float longitude = (CentralMer + (1. / radian) * atan2(eastings, -northings)) * radian;

    /*
     * Estimates zenith angle in the pixel.
     */
    const float lat = gmttime + ((longitude / radian) * 0.0667);
    const float hourangle = std::abs(lat - 12.) * 0.2618;

    const float coszenith = (cos(latitude) * cos(hourangle) * cos(inclination)) + (sin(latitude) * sin(inclination));
    const float sunh = 90. - (acosf(coszenith) / radian);

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
