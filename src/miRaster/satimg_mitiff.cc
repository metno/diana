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

#include "satimg_mitiff.h"

#include <puTools/miStringFunctions.h>

#include <tiffio.h>

#include <sstream>

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

int satimg::MITIFF_read_diana(const std::string& infile, unsigned char* image[], int nchan, const int chan[], dihead& ginfo)
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
  TIFFGetField(in.get(), TIFFTAG_IMAGEWIDTH, &ginfo.xsize);
  TIFFGetField(in.get(), TIFFTAG_IMAGELENGTH, &ginfo.ysize);
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
    if (i != 0 || chan[0] != 0) { /*TIFFsetDirectory chrashes if chan[0]=0,why??*/
      if (!TIFFSetDirectory(in.get(), chan[i]))
        return -1;
    }
    image[i] = new unsigned char[size + 1];
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
  if (TIFFGetField(in.get(), TIFFTAG_PHOTOMETRIC, &pmi) && pmi == PHOTOMETRIC_PALETTE) {
    unsigned short int *red, *green, *blue;
    if (!TIFFGetField(in.get(), TIFFTAG_COLORMAP, &red, &green, &blue)) {
      return 2;
    }
    for (int i = 0; i < 256; i++) {
      ginfo.cmap[0][i] = red[i];
      ginfo.cmap[1][i] = green[i];
      ginfo.cmap[2][i] = blue[i];
    }
  }

  char* description = 0;
  if (!TIFFGetField(in.get(), TIFFTAG_IMAGEDESCRIPTION, &description) || !description)
    return -1;
  std::string desc_str(description);

  // read all common fields
  while (true) {
    // Find key word
    size_t i = 0;
    size_t nn = desc_str.npos;
    while (nn == desc_str.npos && i < FIELDS) {
      nn = desc_str.find(fieldname[i]);
      i++;
    }
    if (i == FIELDS)
      break; // no key word found
    i--;
    nn += fieldname[i].size();
    desc_str = desc_str.substr(nn, desc_str.size() - nn + 1);
    // find next key word
    size_t j = 0;
    size_t mm = std::string::npos;
    while (mm == std::string::npos && j < FIELDS) {
      mm = desc_str.find(fieldname[j]);
      j++;
    }
    std::string value;
    if (j < FIELDS + 1)
      value = desc_str.substr(0, mm);
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
  if (!ginfo.projection.isDefined()) {
    std::ostringstream proj4;
    proj4 << "+proj=stere";
    proj4 << " +lon_0=" << gridRot;
    proj4 << " +lat_ts=" << trueLat;
    proj4 << " +lat_0=90";
    proj4 << " +R=6371000";
    proj4 << " +units=km";
    ginfo.projection.setProj4Definition(proj4.str());
    ginfo.By -= ginfo.Ay * ginfo.ysize;
  } else {
    double x_0, y_0;
    std::string proj4 = ginfo.projection.getProj4Definition();
    if (proj4_value(proj4, "+x_0=", x_0, true))
      ginfo.Bx = x_0 / -1000;
    if (proj4_value(proj4, "+y_0=", y_0, true))
      ginfo.By = y_0 / -1000;
    ginfo.projection.setProj4Definition(proj4);
  }

  return (pmi == PHOTOMETRIC_PALETTE) ? 2 : 0;
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
    // Format hour:min day/month-year
    int hour, minute, day, month, year;
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

  else if (tag == "Channels:")
    ginfo.zsize = (unsigned short int)atoi(str.c_str());

  else if (tag == "In this file:")
    ginfo.channel = str;

  else if (tag == "Xsize:")
    ginfo.xsize = atoi(str.c_str());
  else if (tag == "Ysize:")
    ginfo.ysize = atoi(str.c_str());
  else if (tag == "Proj string:")
    ginfo.projection.setProj4Definition(str);
  else if (tag == "TrueLat:") {
    trueLat = (float)atof(str.c_str()); // no miutil::to_float as the text is like "60 N"
    if (miutil::contains(str, "S"))
      trueLat *= -1.0;
  } else if (tag == "GridRot:")
    gridRot = miutil::to_float(str);
  else if (tag == "Bx:")
    ginfo.Bx = (float)atof(str.c_str());
  else if (tag == "By:")
    ginfo.By = (float)atof(str.c_str());
  else if (tag == "Ax:")
    ginfo.Ax = (float)atof(str.c_str());
  else if (tag == "Ay:")
    ginfo.Ay = (float)atof(str.c_str());
  else if (tag == "Calibration VIS:")
    ginfo.cal_vis = str;
  else if (tag == "Calibration IR:")
    ginfo.cal_ir = str;
  else if (tag == "Table_calibration:")
    ginfo.cal_table.push_back(str);
  else if (tag == "COLOR INFO:") {
    const std::vector<std::string> token = miutil::split(str, "\n", false);
    int ntoken = token.size();
    if (ntoken < 3)
      return 1;
    ginfo.name = token[0];
    ginfo.noofcl = atoi(token[1].c_str());
    if (ntoken - 2 < ginfo.noofcl)
      ginfo.noofcl = ntoken - 2;
    if (ntoken - 2 > ginfo.noofcl)
      ntoken = ginfo.noofcl + 2;
    for (int i = 2; i < ntoken; i++)
      ginfo.clname.push_back(token[i]);
  }

  return 0;
}
} // namespace

int satimg::MITIFF_day_night(const std::string& infile)
{
  dihead sinfo;
  if (MITIFF_head_diana(infile, sinfo) != 0)
    return -1;
  return day_night(sinfo);
}
