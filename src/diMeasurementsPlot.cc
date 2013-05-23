/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diMeasurementsPlot.cc 1 2007-09-12 08:06:42Z lisbethb $

  Copyright (C) 2006 met.no

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

#include <fstream>
#include <diCommonTypes.h>
#include <diMeasurementsPlot.h>
#include <sstream>
#include <math.h>
#include <stdio.h>
#include <diField/diField.h>
#include <GL/gl.h>

using namespace std; using namespace miutil;


MeasurementsPlot::MeasurementsPlot()
:Plot(){
  oldArea=area;
  lineWidth=1;
}


MeasurementsPlot::~MeasurementsPlot(){

}


bool MeasurementsPlot::prepare(void){
#ifdef DEBUGPRINT
  DEBUG_ << "++ MeasurementsPlot::prepare() ++";
#endif

  //Change projection

  if (oldArea.P() == area.P()) //nothing to do
    return true;

  int npos = x.size();
  if (npos==0)
    return true;

  float *xpos= new float[npos];
  float *ypos= new float[npos];

  for (int i=0; i<npos; i++){
    xpos[i] = x[i];
    ypos[i] = y[i];
  }


  // convert points to correct projection
  gc.getPoints(oldArea.P(),area.P(),npos,xpos,ypos);//) {

  for (int i=0; i<npos; i++){
    x[i] = xpos[i];
    y[i] = ypos[i];
  }

  delete[] xpos;
  delete[] ypos;

  oldArea = area;
  return true;
}

void MeasurementsPlot::measurementsPos(vector<miString>& vstr)
{
#ifdef DEBUGPRINT
  for(size_t  i=0;i<vstr.size();i++)
    DEBUG_ << "++ MeasurementsPlot::measurementsPos() " << vstr[i];
#endif

  int nvstr = vstr.size();
  for(int k=0; k<nvstr; k++){

    vector<float> longitude;
    vector<float> latitude;

    miString value,orig_value,key;
    miString pin = vstr[k];
    vector<miString> tokens = pin.split('"','"');
    int n = tokens.size();

    for( int i=0; i<n; i++){
      vector<miString> stokens = tokens[i].split('=');
#ifdef DEBUGPRINT
      DEBUG_ << "stokens:";
      for (int j=0; j<stokens.size(); j++)
        DEBUG_ << "  " << stokens[j];
#endif
      if( stokens.size() == 1) {
        key= stokens[0].downcase();
        if (key == "clear") {
          //          clearData();
        }
        else if (key == "delete"){
          lat.clear();
          lon.clear();
          x.clear();
          y.clear();
        }
      } else if( stokens.size() == 2) {
        key        = stokens[0].downcase();
        orig_value = stokens[1];
        value      = stokens[1].downcase();
        if (key == "longitudelatitude" ) {
          vector<miString> lonlat = value.split(',');
          int npos=lonlat.size()/2;
          for( int i=0; i<npos; i++){
            longitude.push_back(atof(lonlat[2*i].c_str()));
            latitude.push_back(atof(lonlat[2*i+1].c_str()));
          }
        } else if (key == "latitudelongitude" ) {
          vector<miString> latlon = value.split(',');
          int npos=latlon.size()/2;
          for( int i=0; i<npos; i++){
            latitude.push_back(atof(latlon[2*i].c_str()));
            longitude.push_back(atof(latlon[2*i+1].c_str()));
          }
        } else if (key == "colour" ) {
          colour = value;
        }
        else if (key == "linewidth" )
          lineWidth = atoi(value.c_str());
        else if (key == "linetype" )
          lineType = Linetype(value);
      }
    }

    //if no positions are given, return
    int nlon=longitude.size();
    if (nlon==0)
      continue;


    float *xpos= new float[nlon];
    float *ypos= new float[nlon];
    for (int i=0; i<nlon; i++){
      xpos[i] = longitude[i];
      ypos[i] = latitude[i];
    }
    gc.geo2xy(area,nlon,xpos,ypos);

    for (int i=0; i<nlon; i++) {
      lat.push_back(latitude[i]);
      lon.push_back(longitude[i]);
      x.push_back(xpos[i]);
      y.push_back(ypos[i]);
    }

    delete[] xpos;
    delete[] ypos;

  }

}

bool MeasurementsPlot::plot(){
#ifdef DEBUGPRINT
  DEBUG_ << "++ MeasurementsPlot::plot() ++";
#endif

  if ( !enabled )
    return false;

  if (colour==backgroundColour)
    colour= backContrastColour;
  glColor4ubv(colour.RGBA());
  glLineWidth(float(lineWidth)+0.1f);

  float d= 5*fullrect.width()/pwidth;

  // plot  posistions
  int m = x.size();
  glBegin(GL_LINES);
  for (int i=0; i<m; i++) {
    glVertex2f(x[i]-d,y[i]-d);
    glVertex2f(x[i]+d,y[i]+d);
    glVertex2f(x[i]-d,y[i]+d);
    glVertex2f(x[i]+d,y[i]-d);
  }
  glEnd();


  return true;
}




