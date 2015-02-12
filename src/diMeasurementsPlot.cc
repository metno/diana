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

#include <diMeasurementsPlot.h>
#include <sstream>
#include <diField/diField.h>
#include <GL/gl.h>

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.MeasurementsPlot"
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;

MeasurementsPlot::MeasurementsPlot()
 :Plot()
{
  oldArea=getStaticPlot()->getMapArea();
  lineWidth=1;
}


MeasurementsPlot::~MeasurementsPlot()
{
}


bool MeasurementsPlot::prepare(void)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif

  //Change projection

  if (oldArea.P() == getStaticPlot()->getMapArea().P()) //nothing to do
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
  getStaticPlot()->gc.getPoints(oldArea.P(),getStaticPlot()->getMapArea().P(),npos,xpos,ypos);//) {

  for (int i=0; i<npos; i++){
    x[i] = xpos[i];
    y[i] = ypos[i];
  }

  delete[] xpos;
  delete[] ypos;

  oldArea = getStaticPlot()->getMapArea();
  return true;
}

void MeasurementsPlot::measurementsPos(const vector<string>& vstr)
{
#ifdef DEBUGPRINT
  for(size_t  i=0;i<vstr.size();i++)
    METLIBS_LOG_DEBUG("++ MeasurementsPlot::measurementsPos() " << vstr[i]);
#endif

  int nvstr = vstr.size();
  for(int k=0; k<nvstr; k++){

    vector<float> longitude;
    vector<float> latitude;

    std::string value,orig_value,key;
    std::string pin = vstr[k];
    vector<std::string> tokens = miutil::split_protected(pin, '"','"');
    int n = tokens.size();

    for( int i=0; i<n; i++){
      vector<std::string> stokens = miutil::split(tokens[i], 0, "=");
#ifdef DEBUGPRINT
      METLIBS_LOG_DEBUG("stokens:");
      for (int j=0; j<stokens.size(); j++)
        METLIBS_LOG_DEBUG("  " << stokens[j]);
#endif
      if( stokens.size() == 1) {
        key= miutil::to_lower(stokens[0]);
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
        key        = miutil::to_lower(stokens[0]);
        orig_value = stokens[1];
        value      = miutil::to_lower(stokens[1]);
        if (key == "longitudelatitude" ) {
          vector<std::string> lonlat = miutil::split(value, 0, ",");
          int npos=lonlat.size()/2;
          for( int i=0; i<npos; i++){
            longitude.push_back(atof(lonlat[2*i].c_str()));
            latitude.push_back(atof(lonlat[2*i+1].c_str()));
          }
        } else if (key == "latitudelongitude" ) {
          vector<std::string> latlon = miutil::split(value, 0, ",");
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
    getStaticPlot()->geo2xy(nlon, xpos, ypos);

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
  METLIBS_LOG_DEBUG("++ MeasurementsPlot::plot() ++");
#endif

  if (!isEnabled())
    return false;

  if (colour==getStaticPlot()->getBackgroundColour())
    colour= getStaticPlot()->getBackContrastColour();
  glColor4ubv(colour.RGBA());
  glLineWidth(float(lineWidth)+0.1f);

  float d= 5*getStaticPlot()->getPlotSize().width()/getStaticPlot()->getPhysWidth();

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




