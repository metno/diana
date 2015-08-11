/*
  libmiRaster - met.no tiff interface

  $Id$

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "satimg.h"
#include <tiffio.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <puTools/miStringFunctions.h>

using namespace std;
using namespace miutil;

int satimg::MITIFF_read_diana(const std::string& infile, unsigned char *image[],
    int nchan, int chan[], dihead &ginfo)
{


  int i,status,size;
  int pal;
  int compression = COMPRESSION_NONE;

  TIFF *in;

  ginfo.noofcl = 0;

  // read header
  pal = MITIFF_head_diana(infile,ginfo);
  if(pal == -1)
    return(-1);

  if( nchan == 0 ){
    return(1);
  }

  // Open TIFF files

  in=TIFFOpen(infile.c_str(), "rc");
  if (!in) {
    printf(" This is no TIFF file! (2)\n");
    return(-1);
  }


  // Read image data into matrix.
  TIFFGetField(in, 256, &ginfo.xsize);
  TIFFGetField(in, 257, &ginfo.ysize);
  size = ginfo.xsize*ginfo.ysize;

  /*
   * Memory allocated for image data in this function (*image) is freed
   * in function main process.
   */
  if (ginfo.zsize > MAXCHANNELS) {
    printf("\n\tNOT ENOUGH POINTERS AVAILABLE TO HOLD DATA!\n");
    TIFFClose(in);
    return(-1);
  }

  for (i=0; i<nchan; i++) {
    if(i!=0 || chan[0]!=0){ /*TIFFsetDirectory chrashes if chan[0]=0,why??*/
      if (TIFFSetDirectory(in, chan[i]) == 0) {
        TIFFClose(in);
        return(-1);
      }
    }
    image[i] = new unsigned char [size+1];
    if (!image[i]) {
      TIFFClose(in);
      return(-1);
    }

    status = TIFFGetField(in, TIFFTAG_COMPRESSION, &compression);
    if ( !status ) {
      compression = COMPRESSION_NONE;
    }

    if ( compression == COMPRESSION_NONE ) {
      status = TIFFReadRawStrip(in, 0, image[i], size);
    } else {
      status = TIFFReadEncodedStrip(in, 0, image[i], size);
    }
    if (status == -1) {
      TIFFClose(in);
      return(-1);
    }
  }

  TIFFClose(in);

  return(pal);

}


int satimg::MITIFF_head_diana(const std::string& infile, dihead &ginfo) {

  int j,status;
  TIFF *in;
  short pmi;
  unsigned short int *red, *green, *blue;
  char *description;
  char *description_o;
  const std::string fieldname[FIELDS]={
      "Satellite:",
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
      "NWP INFO:"
  };

  ginfo.trueLat= 60.0;
  ginfo.gridRot=  0.0;
  ginfo.noofcl = 0;


  // Open TIFF files and initialize IFD


  in=TIFFOpen(infile.c_str(), "rc");
  if (!in) {
    printf(" This is no TIFF file! (2)\n");
    return(-1);
  }

  //    Test whether this is a color palette image or not.
  //    pmi= 3 : color palette

  status = TIFFGetField(in, 262, &pmi);
  if(pmi==3){

    status = TIFFGetField(in, 320, &red, &green, &blue);
    if (status != 1) {
      TIFFClose(in);
      return(2);
    }
    for (int i=0; i<256; i++) {
      ginfo.cmap[0][i] = red[i];
      ginfo.cmap[1][i] = green[i];
      ginfo.cmap[2][i] = blue[i];
    }
  }

  description_o = (char *) malloc(TIFFHEAD*sizeof(char));
  if (!description_o) {
    TIFFClose(in);
    return(-1);
  }
  description = description_o;
  TIFFGetField(in, 270, &description);
  std::string desc_str(description);



  // read all common fields

  while(true){
    //Find key word
    int i= 0;
    size_t nn = desc_str.npos;
    while (nn==desc_str.npos && i<FIELDS) {
      nn = desc_str.find(fieldname[i]);
      i++;
    }
    if(i==FIELDS) break; //no key word found
    i--;
    nn += fieldname[i].size();
    desc_str = desc_str.substr(nn,desc_str.size()-nn+1);
    //find next key word
    j= 0;
    size_t mm = desc_str.npos;
    while (mm==desc_str.npos && j<FIELDS) {
      mm = desc_str.find(fieldname[j]);
      j++;
    }
    std::string value;
    if(j<FIELDS+1)
      value = desc_str.substr(0,mm);
    else
      value = desc_str;

    miutil::trim(value);
    fillhead_diana(value, fieldname[i], ginfo);
  }


  free(description_o);
  TIFFClose(in);

  if(pmi==3)
    return(2);
  else
    return(0);
}

/*
 * PURPOSE:
 * Fillhead extracts the standard information in the header.
 */
int satimg::fillhead_diana(const std::string& str, const std::string& tag, dihead &ginfo) {

  if (tag == "Satellite:")  ginfo.satellite = str;

  else if (tag == "Date and Time:") {
    //Format hour:min day/month-year
    int hour,minute,day,month,year;
    if(sscanf(str.c_str(), "%2d:%2d %2d/%2d-%4d", &hour,&minute,&day, &month,&year)!=5)
      return 0;//invalid time
    if( hour == 24 ){
      hour = 0;
      ginfo.time = miTime(year,month,day,hour,minute,0);
      ginfo.time.addDay(1);
    }
    if( year == 0 ) year = 2000;
    ginfo.time = miTime(year,month,day,hour,minute,0);
  }

  else if (tag =="Channels:")
    ginfo.zsize = (unsigned short int) atoi(str.c_str());

  else if (tag == "In this file:") ginfo.channel = str;

  else if (tag == "Xsize:") ginfo.xsize = atoi(str.c_str());
  else if (tag == "Ysize:") ginfo.ysize = atoi(str.c_str());
  else if (tag == "Map projection:") ginfo.projection = str;
  else if (tag == "Proj string:") ginfo.proj_string = str;
  else if (tag == "TrueLat:") {
    ginfo.trueLat= (float) atof(str.c_str());
    if (miutil::contains(str, "S")) ginfo.trueLat *= -1.0;
  }
  else if (tag == "GridRot:") ginfo.gridRot= (float) atof(str.c_str());
  else if (tag == "Bx:") ginfo.Bx = (float) atof(str.c_str());
  else if (tag == "By:") ginfo.By = (float) atof(str.c_str());
  else if (tag == "Ax:") ginfo.Ax = (float) atof(str.c_str());
  else if (tag == "Ay:") ginfo.Ay = (float) atof(str.c_str());
  else if (tag == "Calibration VIS:") ginfo.cal_vis = str;
  else if (tag == "Calibration IR:")  ginfo.cal_ir  = str;
  else if (tag == "Table_calibration:")  ginfo.cal_table.push_back(str);
  else if (tag == "COLOR INFO:"){
    vector<std::string> token = miutil::split(str, "\n",false);
    int ntoken=token.size();
    if(ntoken<3) return 1;
    ginfo.name = token[0];
    ginfo.noofcl = atoi(token[1].c_str());
    if(ntoken-2<ginfo.noofcl) ginfo.noofcl = ntoken-2;
    if(ntoken-2>ginfo.noofcl) ntoken = ginfo.noofcl+2;
    for (int i=2; i<ntoken; i++)
      ginfo.clname.push_back(token[i]);
  }

  return(0);
}


int satimg::day_night(const std::string& infile)
{

  dihead sinfo;

  if( MITIFF_head_diana(infile, sinfo)!= 0){
    cerr <<" satimg: Error reading met.no/TIFF file:" << infile << endl;
    return -1;
  }
  ;


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

  int aa = selalg(d, upos, 5., -2.); //Why 5 and -2? From satsplit.c

  return aa;
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


int satimg::JulianDay(usi yy, usi mm, usi dd) {
  static unsigned short int daytab[2][13]=
  {{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
      {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};
  unsigned short int i, leap;
  int dn;

  leap = 0;
  if (yy%4 == 0 && yy%100 != 0 || yy%400 == 0) leap=1;

  dn = (int) dd;
  for (i=1; i<mm; i++)
  {dn += (int) daytab[leap][i];}

  return(dn);
}



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


short satimg::selalg(const dto& d, const ucs& upos, const float& hmax, const float& hmin) {

  int i, countx, county, overcompensated[2];
  int size;
  float inclination, hourangle, coszenith, sunh, xval, yval;
  float max = 0., min = 0.;
  float northings,  eastings,  latitude,  longitude;
  float DistPolEkv, daynr, gmttime;
  float Pi = 3.141592654;
  float TrueScaleLat = 60.;
  float CentralMer = 0.;
  float theta0, lat;
  double radian, Rp, TrueLatRad;

  radian = Pi/180.;
  TrueLatRad = TrueScaleLat*radian;
  DistPolEkv = 6378.*(1.+sin(TrueLatRad));
  size = upos.iw*upos.ih;

  /*
   * Decode day and time information for use in formulas.
   */
  daynr = (float) JulianDay((int) d.yy, (int) d.mm, (int) d.dd);
  gmttime = (float) d.ho+((float) d.mi/60.);

  theta0 = (2*Pi*daynr)/365;
  inclination = 0.006918-(0.399912*cos(theta0))+(0.070257*sin(theta0))
  -(0.006758*cos(2*theta0))+(0.000907*sin(2*theta0))
  -(0.002697*cos(3*theta0))+(0.001480*sin(3*theta0));

  for (i = 0; i < 2; i++) {
    overcompensated[i] = 0;
  }

  /*
   Estimates latitude and longitude for the corner pixels.
   */
  countx = 0;
  county = 0;
  for (i = 0; i < 4; i++) {
    xval = upos.Bx + upos.Ax*((float) countx + 0.5);
    yval = upos.By - fabsf(upos.Ay)*((float) county + 0.5);

    countx += upos.iw;
    if (countx > upos.iw) {
      countx = 0;
      county += upos.ih;
    }
    northings = yval;
    eastings  = xval;

    Rp = pow(double(eastings*eastings + northings*northings),0.5);

    latitude = 90.-(1./radian)*atan(Rp/DistPolEkv)*2.;
    longitude = CentralMer+(1./radian)*atan2(eastings,-northings);

    latitude=latitude*radian;
    longitude=longitude*radian;

    /*
     * Estimates zenith angle in the pixel.
     */
    lat = gmttime+((longitude/radian)*0.0667);
    hourangle = fabsf(lat-12.)*0.2618;

    coszenith = (cos(latitude)*cos(hourangle)*cos(inclination)) +
    (sin(latitude)*sin(inclination));
    sunh = 90.-(acosf(coszenith)/radian);

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
    return(2);
  } else if (min < hmin && fabs(min) > (fabs(max)+hmin)) {
    return(1);
  } else {
    return(0);
  }
  return(0);
}



