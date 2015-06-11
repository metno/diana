/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diMItiff.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.MItiff"
#include <miLogger/miLogging.h>

MItiff::MItiff()
{
}

bool MItiff::readMItiffPalette(const std::string& filename, std::vector<Colour>& col)
{
  satimg::dihead ginfo;

  // if not colour palette image
  if(satimg::MITIFF_head_diana(filename, ginfo)!= 2)
    return false;

  // index -> RGB
  const int colmapsize=256;
  unsigned char colmap[3][colmapsize];
  //convert colormap
  for( int k=0; k<3; k++)
    for( int j=0; j<colmapsize; j++)
      colmap[k][j]= int (ginfo.cmap[k][j]/65535.0*255.0);

  //clean up
  col.clear();

  int ncolours = ginfo.noofcl;
  for( int j=0; j<=ncolours; j++)
    col.push_back(Colour(colmap[0][j],colmap[1][j],colmap[2][j]));

  return true;
}

bool MItiff::readMItiffHeader(SatFileInfo& file)
{
  satimg::dihead ginfo;

  int rres =satimg::MITIFF_head_diana(file.name, ginfo);

  if (rres ==2)
    file.palette=true;
  else if (rres==0)
    file.palette=false;
  else{
    METLIBS_LOG_ERROR("MITIFF_head_diana returned false:"<<file.name);
    return false;
  }

  if ( !ginfo.time.undef()) {
    file.time = ginfo.time;
  }
  file.opened = true;

  std::string ch=ginfo.channel;
  file.channel=miutil::split(ch, " ");

  return true;
}

bool MItiff::readMItiff(const std::string& filename, Sat& sd, int index)
{
  //Read TIFF-file using libsatimg, MITIFF_read_diana returns the images
  //for each channel (index[i]) in rawimage[i], and  information about the
  // satellite pictures in the structure ginfo

  satimg::dihead    ginfo;

  int rres= satimg::MITIFF_read_diana(filename,&sd.rawimage[index], sd.no,sd.index, ginfo);
  if (rres == -1) {
    METLIBS_LOG_ERROR("MITIFF_read_diana returned false:" << filename);
    return false;
  }

  if (rres == 2) {
    // read palette files (colour index)
    sd.palette=true;

    sd.paletteInfo.name = ginfo.name;
    sd.paletteInfo.noofcl = ginfo.noofcl;
    sd.paletteInfo.clname = ginfo.clname;
    for(int j=0; j<3; j++)
      for(int i=0;i<256;i++)
        sd.paletteInfo.cmap[j][i] = int(ginfo.cmap[j][i]/65535.0*255.0);
  }

  //name from file
  sd.satellite_name = ginfo.satellite;

  //time
  sd.time = ginfo.time;

  //dimension
  sd.area.nx=ginfo.xsize;
  sd.area.ny=ginfo.ysize;

  //grid
  sd.TrueLat= ginfo.trueLat;
  sd.GridRot= ginfo.gridRot;
  sd.Ax = ginfo.Ax;
  sd.Ay = ginfo.Ay;
  sd.Bx = ginfo.Bx;
  sd.By = ginfo.By;

  //Projection
  sd.projection = ginfo.projection;
  // Use proj4string from setupfile if present
  if ( sd.proj_string.empty() ) {
    sd.proj_string = ginfo.proj_string;
  }

  // Calibration
  sd.cal_vis = ginfo.cal_vis;
  sd.cal_ir = ginfo.cal_ir;
  sd.cal_table = ginfo.cal_table;

  return true;
}

bool  MItiff::day_night(SatFileInfo &fInfo, std::string& channels)
{
  int aa = satimg::day_night(fInfo.name);

  if(aa<0)
    return false;

  //ex: channelinfo=twilight:4;day:1+2+4;night:3+4+5
  std::vector<std::string> vchannels = miutil::split(fInfo.channelinfo,";");

  if (vchannels.size() == 3) {

    std::map< std::string, std::string > mchannels;
    for (int i=0; i<3 ; ++i) {
      std::vector<std::string> token = miutil::split(vchannels[i],":");
      if (token.size() == 2) {
        mchannels[token[0]] = token[1];
      }
    }
    if (aa == 0) {       //twilight
      channels = mchannels["twilight"];
    } else if (aa == 2) { //day
      channels = mchannels["day"];
    } else if (aa == 1) { //night
      channels = mchannels["night"];
    }
  } else {
    if (aa == 0){       //twilight
      channels = fInfo.channel.at(4-1); //"4"
    } else if (aa == 2) { //day
      channels = fInfo.channel.at(1-1) + "+" + fInfo.channel.at(2-1) + "+" + fInfo.channel.at(4-1); //"1+2+4";
    } else if (aa == 1) { //night
      channels = fInfo.channel.at(3-1) + "+" + fInfo.channel.at(4-1) + "+" + fInfo.channel.at(5-1); //"3+4+5";
    }
  }

  return true;
}
