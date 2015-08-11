/*
 libdiHDF5 - SMHI HDF5 interface

 $Id$

 Copyright (C) 2006 met.no

 Contact information:
 Norwegian Meteorological Institute
 Box 43 Blindern
 0313 OSLO
 NORWAY
 email: diana-smhi@met.no

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
 * Header file for module reading from HDF5 file data and computing
 * satellite geometry. 
 * 
 * AUTHOR:
 * �ystein God�y, met.no/FOU, 14/01/1999
 */

#ifndef _AUSATH5_H
#define _AUSATH5_H

//#ifndef DEBUGPRINT
//#define DEBUGPRINT
//#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <hdf5.h>
#include <projects.h>

#include <puTools/miTime.h>

//#define MEMERROR -999
//#define MAXCHANNELS 64
//#define HDF5HEAD 4096
//#define FIELDS 23
//#define DIMY	1021
//#define DIMX    848

namespace satimgh5 {

typedef unsigned short int usi;
typedef unsigned char uca;

typedef struct myItems {
  std::string name;
  std::string value;
} myItems;

typedef struct oneFloat {
  float f;
} oneFloat;

typedef struct arrFloat {
  //float f[1024];
  double f[1024];
} arrFloat;

/*
 * Old structure to hold information for diana, this one was originally
 * created for use with met.no/TIFF files.
 */
struct dihead {
  std::string satellite;
  miutil::miTime time;
  unsigned short int satdir;
  unsigned short int ch[8];
  std::string channel;
  unsigned int zsize;
  unsigned int xsize;
  unsigned int ysize;
  float trueLat;
  float gridRot;
  float Ax;
  float Ay;
  float Bx;
  float By;
  //calibration for visual and infrared channels
  std::string cal_vis;
  std::string cal_ir;
  // table calibration
  std::vector<std::string> cal_table;
  float AVis;
  float BVis;
  float AIr;
  float BIr;
  // information for color palette imagery.
  std::string name;
  usi noofcl;
  std::vector<std::string> clname;
  unsigned short int cmap[3][256];

  // Info from setup file.
  std::string metainfo;
  std::string channelinfo;
  std::string datasetinfo;
};

struct diheadHDF5 {
  std::string area;
  std::string camethod;
  std::vector<std::string> nodes;
  std::string place;
  std::string software;
  std::string system;
  float rga_A;
  float rga_B;
  float rga_C;
  miutil::miTime endDateTime;
  std::string endDate;
  std::string endTime;
  float gain;
  float nodata;
  float offset;
  float prodpar;
  std::string product;
  std::string quantity;
  miutil::miTime startDateTime;
  std::string startDate;
  std::string startTime;
  float undetect;
  miutil::miTime dateTime;
  std::string date;
  std::string time;
  std::string object;
  int sets;
  std::string version;
  float LL_lat;
  float LL_lon;
  float UR_lat;
  float UR_lon;
  std::string projdef;
  float xscale;
  float yscale;
  int xsize;
  int ysize;
  float trueLat;
  float gridRot;
  std::vector<std::string> area_extent;
};

//structs dto and ucs are used only by SatManager day_night, in a
//call to selalg

struct dto {
  usi ho; /* satellite hour */
  usi mi; /* satellite minute */
  usi dd; /* satellite day */
  usi mm; /* satellite month */
  usi yy; /* satellite year */
};

struct ucs {
  float Bx; /* UCS Bx */
  float By; /* UCS By */
  float Ax; /* UCS Ax */
  float Ay; /* UCS Ay */
  unsigned int iw; /* image width (pixels) */
  unsigned int ih; /* image height (pixels) */
};

// Functions
short selalg(const dto& d, const ucs& upos, const float& hmax,
    const float& hmin);
int JulianDay(usi yy, usi mm, usi dd);
int day_night(const std::string& infile);
int HDF5_read_diana(const std::string& infile, unsigned char *image[],
    int nchan, int chan[], dihead& ginfo);
int HDF5_head_diana(const std::string& infile, dihead &ginfo);
int fillhead_diana(const std::string& str, const std::string& tag,
    dihead &ginfo);
herr_t getValue(hid_t file, std::string theGroups, const char* theAttr,
    std::string &resString);
herr_t fill_head_diana(hid_t file, std::string inputStr,
    diheadHDF5 &HDF5info, int chan);
int extractDataProjDef(diheadHDF5 &HDF5info);
int getValueFromCompundDataset(hid_t file, std::string dataset,
    std::vector<myItems> input, diheadHDF5 &HDF5info);
}
;

#endif /* _AUSATH5_H */

