/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#include "diLocationPlot.h"

#include "diGLPainter.h"
#include "diLinetype.h"
#include "diStaticPlot.h"

#include <mi_fieldcalc/math_util.h>

#include <QPolygonF>

#include <algorithm>
#include <cmath>
#include <set>

#define MILOGGER_CATEGORY "diana.LocationPlot"
#include <miLogger/miLogging.h>

using namespace::miutil;

LocationPlot::LocationPlot()
    : visible(false)
    , numPos(0)
    , px(nullptr)
    , py(nullptr)
{
  METLIBS_LOG_SCOPE();
  locdata.locationType = location_unknown;
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
  px = py = nullptr;
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

  if (locationdata.elements.empty()) {
    METLIBS_LOG_INFO("nelem==0!");
    return false;
  }

  std::set<std::string> nameset;

  for (const auto& le : locationdata.elements) {
    if (le.name.empty()) {
      METLIBS_LOG_WARN("skip LocationPlot with empty name");
      return false;
    }
    if (nameset.find(le.name) != nameset.end()) {
      METLIBS_LOG_INFO("duplicate name: '" << le.name << "'");
      return false;
    }
    if (le.xpos.size() != le.ypos.size()) {
      METLIBS_LOG_WARN("skip LocationPlot with xpos.size() != ypos.size()");
      return false;
    }
    nameset.insert(le.name);
  }

  locdata = locationdata;

  numPos = 0;
  const int nelem = locationdata.elements.size();
  locinfo.resize(nelem);
  for (int i = 0; i < nelem; i++) {
    locinfo[i].beginpos = numPos;
    numPos += locdata.elements[i].xpos.size();
    locinfo[i].endpos = numPos;
  }

  px = new float[numPos];
  py = new float[numPos];
  switchProjection(posArea);

  visible = true;

  // ADC - name appearing on StatusPlotButtons
  setPlotName(locdata.annotation);

  return true;
}

void LocationPlot::changeProjection(const Area& mapArea, const Rectangle& plotSize)
{
  METLIBS_LOG_SCOPE();

  sizeOfCross_ = plotSize.width() * 0.004;
  if (posArea.P() != mapArea.P())
    switchProjection(mapArea);
}

void LocationPlot::switchProjection(const Area& mapArea)
{
  posArea = mapArea;

  if (numPos < 1)
    return;

  size_t np = 0;
  for (const auto& le : locdata.elements) {
    std::copy(le.xpos.begin(), le.xpos.end(), &px[np]);
    std::copy(le.ypos.begin(), le.ypos.end(), &py[np]);
    np += le.xpos.size();
  }

  if (!getStaticPlot()->GeoToMap(numPos, px, py)) {
     METLIBS_LOG_INFO("getPoints error");
     return;
  }

  for (auto& li : locinfo) {
    li.xmin = li.xmax = px[li.beginpos];
    li.ymin = li.ymax = py[li.beginpos];
    li.dmax = 0.0f;
    for (int n = li.beginpos + 1; n < li.endpos; n++) {
      miutil::maximize(li.dmax, miutil::absval2(px[n - 1] - px[n], py[n - 1] - py[n]));
      miutil::minimaximize(li.xmin, li.xmax, px[n]);
      miutil::minimaximize(li.ymin, li.ymax, py[n]);
    }
    li.dmax = std::sqrt(li.dmax);
  }
}

std::string LocationPlot::getEnabledStateKey() const
{
  return "locationplot-" + getName();
}

std::string LocationPlot::find(int x, int y)
{
  METLIBS_LOG_SCOPE();
  const float maxdist= 10.0f;

  std::string name;
  const XY pos = getStaticPlot()->PhysToMap(XY(x, y));

  float dmax= maxdist*getStaticPlot()->getPhysToMapScaleX();
  float dmin2= dmax*dmax;
  int   lmin= -1;
  float dx,dy,sx,sy,sdiv;
  int n1,n2,ndiv, numLines= locinfo.size();

  for (int l=0; l<numLines; l++) {
    if (pos.x()>locinfo[l].xmin-dmax && pos.x()<locinfo[l].xmax+dmax &&
        pos.y()>locinfo[l].ymin-dmax && pos.y()<locinfo[l].ymax+dmax) {
      n1= locinfo[l].beginpos;
      n2= locinfo[l].endpos;
      ndiv= int(locinfo[l].dmax/dmax) + 1;
      if (ndiv<2) {
	for (int n=n1; n<n2; n++) {
	  dx= px[n]-pos.x();
	  dy= py[n]-pos.y();
          float d2 = miutil::absval2(dx, dy);
          if (dmin2 > d2) {
            dmin2 = d2;
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
	    dx= px[n-1]+sx*float(j)-pos.x();
	    dy= py[n-1]+sy*float(j)-pos.y();
            float d2 = miutil::absval2(dx, dy);
            if (dmin2 > d2) {
              dmin2 = d2;
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


void LocationPlot::plot(DiGLPainter* gl, PlotOrder zorder)
{
  METLIBS_LOG_SCOPE();
  if (zorder != PO_LINES || !isEnabled())
    return;

  if (numPos<1)
    return;

  const Colour lc(locdata.colour);
  const Colour& c1 = getStaticPlot()->notBackgroundColour(lc);
  float    w1= locdata.linewidth;
  Linetype l1= Linetype(locdata.linetype);
  gl->setLineStyle(c1, w1, l1);

  int lselected = -1;

  const int numLines= locinfo.size();
  for (int l=0; l<numLines; l++) {
    if (locdata.elements[l].name!=selectedName) {
      drawLineOrPoint(gl, l);
    } else {
      lselected = l;
    }
  }

  if (lselected>=0) {
    const Colour lc(locdata.colourSelected);
    const Colour& c2= getStaticPlot()->notBackgroundColour(lc);
    float    w2= locdata.linewidthSelected;
    Linetype l2= Linetype(locdata.linetypeSelected);
    gl->setLineStyle(c2, w2, l2);

    drawLineOrPoint(gl, lselected);
  }

  gl->Disable(DiGLPainter::gl_LINE_STIPPLE);
}

void LocationPlot::drawLineOrPoint(DiGLPainter* gl, int l)
{
  const int n1 = locinfo[l].beginpos, n2= locinfo[l].endpos;
  if ((n2 - n1) > 1) {
    gl->Begin(DiGLPainter::gl_LINE_STRIP);
    for (int n=n1; n<n2; n++)
      gl->Vertex2f(px[n],py[n]);
    gl->End();
  } else {
    gl->drawCross(px[n1], py[n1], sizeOfCross_);
  }
}

void LocationPlot::getAnnotation(std::string &str, Colour &col) const
{
  if (visible) {
    str = locdata.annotation;
    col = Colour(locdata.colour);
  } else {
    str.clear();
  }
}
