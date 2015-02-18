/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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

#include "diLocationPlot.h"

#include <cmath>
#include <set>

#define MILOGGER_CATEGORY "diana.LocationPlot"
#include <miLogger/miLogging.h>

using namespace::miutil;

LocationPlot::LocationPlot()
{
  METLIBS_LOG_SCOPE();

  visible= false;
  locdata.locationType= location_unknown;
  numPos= 0;
  px= 0;
  py= 0;
}


LocationPlot::~LocationPlot()
{
  METLIBS_LOG_SCOPE();
  cleanup();
}


void LocationPlot::cleanup()
{
  METLIBS_LOG_SCOPE();
  delete[] px;
  delete[] py;
  px= py= 0;
  numPos= 0;
  locinfo.clear();
  visible= false;
  selectedName.clear();

  locdata.name.clear();
  locdata.locationType= location_unknown;
  locdata.elements.clear();
  locdata.annotation.clear();
}


bool LocationPlot::setData(const LocationData& locationdata)
{
  METLIBS_LOG_SCOPE();
  cleanup();

  // check if sensible input data
  //bool ok= true;

  int nelem = locationdata.elements.size();
  if (nelem == 0) {
    METLIBS_LOG_INFO("nelem==0!");
    return false;
  }

  std::set<std::string> nameset;

  for (int i = 0; i < nelem; i++) {
    if (locationdata.elements[i].name.empty()) {
      METLIBS_LOG_INFO("i=" << i << " locationdata.elements[i].name.empty()!");
      return false;
    } else if (nameset.find(locationdata.elements[i].name) == nameset.end())
      nameset.insert(locationdata.elements[i].name);
    else {
      METLIBS_LOG_INFO("duplicate name: " << i
          << " locationdata.elements[i].name!" << locationdata.elements[i].name);
      return false;
    }
    if (locationdata.elements[i].xpos.size()
        != locationdata.elements[i].ypos.size()) {
      METLIBS_LOG_INFO("LocationPlot::setData " << i
          << " locationdata.elements[i].xpos.size()!=locationdata.elements[i].ypos.size()!"
          << locationdata.elements[i].xpos.size() << ","
          << locationdata.elements[i].ypos.size());
      return false;
    }
  }

  locdata = locationdata;

  numPos = 0;
  locinfo.resize(nelem);
  for (int i = 0; i < nelem; i++) {
    locinfo[i].beginpos = numPos;
    numPos += locdata.elements[i].xpos.size();
    locinfo[i].endpos = numPos;
  }

  px = new float[numPos];
  py = new float[numPos];
  Projection p;
  Rectangle r;
  posArea = Area(p, r); // impossible area spec

  visible = true;

  // ADC - name appearing on StatusPlotButtons
  setPlotName(locdata.annotation);

  return true;
}


void LocationPlot::updateOptions(const LocationData& locationdata)
{
  METLIBS_LOG_SCOPE();

  // change colour etc. (not positions, name,...)
  locdata.colour=            locationdata.colour;
  locdata.linetype=          locationdata.linetype;
  locdata.linewidth=         locationdata.linewidth;
  locdata.colourSelected=    locationdata.colourSelected;
  locdata.linetypeSelected=  locationdata.linetypeSelected;
  locdata.linewidthSelected= locationdata.linewidthSelected;
}


bool LocationPlot::changeProjection()
{
  METLIBS_LOG_SCOPE();

  if (numPos<1 || posArea.P()==getStaticPlot()->getMapArea().P()) return false;

  int nlines= locdata.elements.size();
  int np1, np= 0;

  for (int l=0; l<nlines; l++) {
    np1= locdata.elements[l].xpos.size();
    for (int i=0; i<np1; i++) {
      px[np+i]= locdata.elements[l].xpos[i];
      py[np+i]= locdata.elements[l].ypos[i];
    }
    np+=np1;
  }

  if (!getStaticPlot()->geo2xy(numPos, px, py)) {
     METLIBS_LOG_INFO("getPoints error");
     return false;
  }

  posArea= getStaticPlot()->getMapArea();

  float xmin,xmax,ymin,ymax,dmax,dx,dy;
  int n1,n2, numLines= locinfo.size();

  for (int l=0; l<numLines; l++) {
    n1= locinfo[l].beginpos;
    n2= locinfo[l].endpos;
    xmin= xmax= px[n1];
    ymin= ymax= py[n1];
    dmax= 0.0f;
    for (int n=n1+1; n<n2; n++) {
      dx= px[n-1]-px[n];
      dy= py[n-1]-py[n];
      if (dmax<dx*dx+dy*dy) dmax=dx*dx+dy*dy;
      if      (xmin>px[n]) xmin= px[n];
      else if (xmax<px[n]) xmax= px[n];
      if      (ymin>py[n]) ymin= py[n];
      else if (ymax<py[n]) ymax= py[n];
    }
    locinfo[l].xmin= xmin;
    locinfo[l].xmax= xmax;
    locinfo[l].ymin= ymin;
    locinfo[l].ymax= ymax;
    locinfo[l].dmax= sqrtf(dmax);
  }

  return true;
}


std::string LocationPlot::find(int x, int y)
{
  METLIBS_LOG_SCOPE();
  const float maxdist= 10.0f;

  std::string name;
  float xpos= x*getStaticPlot()->getPlotSize().width() /getStaticPlot()->getPhysWidth()  + getStaticPlot()->getPlotSize().x1;
  float ypos= y*getStaticPlot()->getPlotSize().height()/getStaticPlot()->getPhysHeight() + getStaticPlot()->getPlotSize().y1;

  float dmax= maxdist*getStaticPlot()->getPlotSize().width()/getStaticPlot()->getPhysWidth();
  float dmin2= dmax*dmax;
  int   lmin= -1;
  float dx,dy,sx,sy,sdiv;
  int n1,n2,ndiv, numLines= locinfo.size();

  for (int l=0; l<numLines; l++) {
    if (xpos>locinfo[l].xmin-dmax && xpos<locinfo[l].xmax+dmax &&
        ypos>locinfo[l].ymin-dmax && ypos<locinfo[l].ymax+dmax) {
      n1= locinfo[l].beginpos;
      n2= locinfo[l].endpos;
      ndiv= int(locinfo[l].dmax/dmax) + 1;
      if (ndiv<2) {
	for (int n=n1; n<n2; n++) {
	  dx= px[n]-xpos;
	  dy= py[n]-ypos;
	  if (dmin2>dx*dx+dy*dy) {
	    dmin2= dx*dx+dy*dy;
	    lmin= l;
	  }
	}
      } else {
	sdiv= 1.0f/float(ndiv);
	ndiv++;
	for (int n=n1+1; n<n2; n++) {
          sx= (px[n]-px[n-1])*sdiv;
          sy= (py[n]-py[n-1])*sdiv;
	  for (int j=0; j<ndiv; j++) {
	    dx= px[n-1]+sx*float(j)-xpos;
	    dy= py[n-1]+sy*float(j)-ypos;
	    if (dmin2>dx*dx+dy*dy) {
	      dmin2= dx*dx+dy*dy;
	      lmin= l;
	    }
	  }
	}
      }
    }
  }

  if (lmin>=0 && selectedName!=locdata.elements[lmin].name)
    name= selectedName= locdata.elements[lmin].name;

  return name;
}


bool LocationPlot::plot()
{
  METLIBS_LOG_SCOPE();
  if (!isEnabled()) return false;

  if (numPos<1) return false;

  if (posArea.P()!=getStaticPlot()->getMapArea().P()) {
    if (!changeProjection()) return false;
  }

  Colour   c1= Colour(locdata.colour);
  Colour   c2= Colour(locdata.colourSelected);
  float    w1= locdata.linewidth;
  float    w2= locdata.linewidthSelected;
  Linetype l1= Linetype(locdata.linetype);
  Linetype l2= Linetype(locdata.linetypeSelected);

  if (c1==getStaticPlot()->getBackgroundColour()) c1= getStaticPlot()->getBackContrastColour();
  if (c2==getStaticPlot()->getBackgroundColour()) c2= getStaticPlot()->getBackContrastColour();

  glColor3ubv(c1.RGB());
  glLineWidth(w1);
  if (l1.stipple) {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(l1.factor,l1.bmap);
  }

  int lselected= -1;

  const int numLines= locinfo.size();
  for (int l=0; l<numLines; l++) {
    if (locdata.elements[l].name!=selectedName) {
      drawLineOrPoint(l);
    } else {
      lselected= l;
    }
  }

  getStaticPlot()->UpdateOutput();
  glDisable(GL_LINE_STIPPLE);

  if (lselected>=0) {
    glColor3ubv(c2.RGB());
    glLineWidth(w2);
    if (l2.stipple) {
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(l2.factor,l2.bmap);
    }
    drawLineOrPoint(lselected);
    getStaticPlot()->UpdateOutput();
    glDisable(GL_LINE_STIPPLE);
  }

  return true;
}


void LocationPlot::drawLineOrPoint(int l)
{
  const int n1 = locinfo[l].beginpos, n2= locinfo[l].endpos;
  if ((n2 - n1) > 1) {
    glBegin(GL_LINE_STRIP);
    for (int n=n1; n<n2; n++)
      glVertex2f(px[n],py[n]);
    glEnd();
  } else {
    const float size = getStaticPlot()->getPlotSize().width() * 0.004;
    glBegin(GL_LINES);
    glVertex2f(px[n1]-size,py[n1]);
    glVertex2f(px[n1]+size,py[n1]);
    glVertex2f(px[n1],py[n1]-size);
    glVertex2f(px[n1],py[n1]+size);
    glEnd();
  }
}


void LocationPlot::getAnnotation(std::string &str, Colour &col)
{
  if (visible) {
    str = locdata.annotation;
    col = Colour(locdata.colour);
  } else {
    str.clear();
  }
}
