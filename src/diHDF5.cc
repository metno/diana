/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2008 met.no

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

//#define DEBUGPRINT

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diHDF5.h>

using namespace::miutil;

HDF5::HDF5()
{

}


bool HDF5::readHDF5Palette(SatFileInfo& file, vector<Colour>& col)
{

  satimgh5::dihead ginfo;

  ginfo.metadata = file.metadata;
  ginfo.paletteinfo = file.paletteinfo;
  ginfo.channelinfo = file.channelinfo;
  ginfo.hdf5type = file.hdf5type;

#ifdef DEBUGPRINT
  cerr << "HDF5::readHDF5Palette ginfo.metadata " << ginfo.metadata << endl;
  cerr << "HDF5::readHDF5Palette filename " << file.name << endl;
#endif

  // if not colour palette image
  //if (hdf5api.HDF5_head_diana(file.name, ginfo)!= 2)
  if(metno::satimgh5::HDF5_head_diana(file.name, ginfo)!= 2)
    return false;

  // index -> RGB
  const int colmapsize=256;
  unsigned char colmap[3][colmapsize];
  //convert colormap
  for (int k=0; k<3; k++)
    for (int j=0; j<colmapsize; j++)
      colmap[k][j]= int (ginfo.cmap[k][j]);

  //clean up
  col.clear();

  int ncolours = ginfo.noofcl;
  for (int j=0; j<=ncolours; j++)
    col.push_back(Colour(colmap[0][j], colmap[1][j], colmap[2][j]));

  return true;

}

bool HDF5::readHDF5Header(SatFileInfo& file)
{
#ifdef DEBUGPRINT
  cerr << "HDF5::readHDF5Header" << endl;
  cerr << "HDF5::readHDF5Header file.name: " << file.name << endl;
  cerr << "HDF5::readHDF5Header hdf5type: " << file.hdf5type << endl;

#endif

  metno::satimgh5::dihead ginfo;

  ginfo.metadata = file.metadata;
  ginfo.paletteinfo = file.paletteinfo;
  ginfo.channelinfo = file.channelinfo;
  ginfo.hdf5type = file.hdf5type;

  int rres =metno::satimgh5::HDF5_head_diana(file.name, ginfo);

  if (rres ==2)
    file.palette=true;
  else if (rres==0)
    file.palette=false;
  else {
    cerr <<"HDF5_head_diana returned false - rres: " << rres << " file: " << file.name << endl;
    return false;
  }

  file.time = ginfo.time;
  file.opened = true;

  miString ch=ginfo.channel;
  file.channel=ch.split(" ");
  return true;
  return true;

}

bool HDF5::readHDF5(const miString& filename, Sat& sd, int index)
{

  //Read HDF5-file using libsatimgh5, HDF5_read_diana returns the images
  //for each channel (index[i]) in rawimage[i], and  information about the
  // satellite pictures in the structure ginfo

#ifdef DEBUGPRINT
  cerr << "HDF5::readHDF5 (index:" << index << endl;
#endif

  metno::satimgh5::dihead ginfo;

  ginfo.metadata = sd.metadata;
  ginfo.paletteinfo = sd.paletteinfo;
  ginfo.channelinfo = sd.channelInfo;
  ginfo.hdf5type = sd.hdf5type;

  int rres= metno::satimgh5::HDF5_read_diana(filename, &sd.rawimage[index], &sd.origimage[index],
      sd.no,sd.index, ginfo);

  if (rres == -1) {
    cerr << "HDF5_read_diana returned false:" << filename << endl;
    return false;
  }

#ifdef DEBUGPRINT
  cerr << "HDF5::readHDF5 -> ginfo.name: " << ginfo.name << endl;
  cerr << "HDF5::readHDF5 -> ginfo.noofcl: " << ginfo.noofcl << endl;
  cerr << "HDF5::readHDF5 -> ginfo.satellite: " << ginfo.satellite << endl;
  cerr << "HDF5::readHDF5 -> ginfo.time: " << ginfo.time << endl;
  cerr << "HDF5::readHDF5 -> ginfo.xsize: " << ginfo.xsize << endl;
  cerr << "HDF5::readHDF5 -> ginfo.ysize: " << ginfo.ysize << endl;
  cerr << "HDF5::readHDF5 -> ginfo.trueLat: " << ginfo.trueLat << endl;
  cerr << "HDF5::readHDF5 -> ginfo.gridRot: " << ginfo.gridRot << endl;
  cerr << "HDF5::readHDF5 -> ginfo.Ax: " << ginfo.Ax << endl;
  cerr << "HDF5::readHDF5 -> ginfo.Ay: " << ginfo.Ay << endl;
  cerr << "HDF5::readHDF5 -> ginfo.Bx: " << ginfo.Bx << endl;
  cerr << "HDF5::readHDF5 -> ginfo.By: " << ginfo.By << endl;
  cerr << "HDF5::readHDF5 -> ginfo.cal_vis: " << ginfo.cal_vis << endl;
  cerr << "HDF5::readHDF5 -> ginfo.cal_ir: " << ginfo.cal_ir << endl;
  cerr << "HDF5::readHDF5 -> rres: " << rres << endl;
#endif

  if (rres == 2) {
    // read palette files (colour index)
    sd.palette=true;
    sd.paletteInfo.name = ginfo.name;
    sd.paletteInfo.noofcl = ginfo.noofcl;
    sd.paletteInfo.clname = ginfo.clname;

#ifdef DEBUGPRINT
    cerr << "HDF5::readHDF5 sd.paletteInfo.name: " << sd.paletteInfo.name << endl;
    for (int d = 0; d < sd.paletteInfo.clname.size(); d++)
      cerr << "HDF5::readHDF5 sd.paletteInfo.clname: " << sd.paletteInfo.clname[d] << endl;
#endif

    for (int j=0; j<3; j++)
      for (int i=0; i<256; i++)
        sd.paletteInfo.cmap[j][i] = int(ginfo.cmap[j][i]);
  }

  //name from file
  sd.satellite_name = ginfo.satellite;

  //time
  sd.time = ginfo.time;

  //dimension
  sd.nx=ginfo.xsize;
  sd.ny=ginfo.ysize;

  //grid
  sd.TrueLat= ginfo.trueLat;
  sd.GridRot= ginfo.gridRot;
  sd.Ax = ginfo.Ax;
  sd.Ay = ginfo.Ay;
  sd.Bx = ginfo.Bx;
  sd.By = ginfo.By;

  // Calibration
  sd.cal_vis = ginfo.cal_vis;
  sd.cal_ir = ginfo.cal_ir;
  sd.cal_table = ginfo.cal_table;

  return true;

}

/*
 bool HDF5::day_night(const miString& filename, miString& channels)
 {


 int aa = metno::satimgh5::day_night(filename);

 if(aa<0) return false;

 if(aa==0){       //twilight
 channels = "4";
 } else if(aa==2){ //day
 channels = "1+2+4";
 } else if(aa==1){ //night
 channels = "3+4+5";
 }

 return true;
 }
 */
