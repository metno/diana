/*
 Diana - A Free Meteorological Visualisation Tool

 $Id$

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

#include <diWeatherArea.h>
#include <diTesselation.h>
#include <polyStipMasks.h>

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.WeatherArea"
#include <miLogger/miLogging.h>

using namespace ::miutil;
using namespace std;

vector<editToolInfo> WeatherArea::allAreas; //info about areas
map<std::string, int> WeatherArea::areaTypes; //finds area type number from name
float WeatherArea::defaultLineWidth = 4;

// Default constructor
WeatherArea::WeatherArea() :
  ObjectPlot(wArea), linewidth(defaultLineWidth), fillArea(false), itsFilltype(NULL)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("WeatherArea default constructor");
#endif

  setType(0);
}

WeatherArea::WeatherArea(int ty) :
  ObjectPlot(wArea), linewidth(defaultLineWidth), fillArea(false), itsFilltype(NULL)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("WeatherArea(int) constructor");
#endif

  setType(ty);
}

WeatherArea::WeatherArea(std::string tystring) :
  ObjectPlot(wArea), linewidth(defaultLineWidth), fillArea(false), itsFilltype(NULL)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("WeatherArea(std::string) constructor");
#endif
  // set correct areatype
  if (!setType(tystring))
    METLIBS_LOG_DEBUG("WeatherArea constructor error, type " << tystring
        << " not found !!!");

}

// Destructor
WeatherArea::~WeatherArea()
{
}

void WeatherArea::defineAreas(vector<editToolInfo> areas)
{
  WeatherArea::allAreas = areas;
  for (unsigned int i = 0; i < areas.size(); i++)
    areaTypes[areas[i].name] = i;
}

// calculates spline between nodepoints and triangulates area
void WeatherArea::recalculate()
{
  //METLIBS_LOG_ERROR("WeatherArea::recalculate()");
  int length = nodePoints.size();
  if (!spline) {
    if (length < 3)
      return;
  } else { // Makes smooth lines
    int div = divSpline; // div = subdivision points between to edge points

    if (x != 0)
      delete[] x;
    if (y != 0)
      delete[] y;

    if (x_s != 0)
      delete[] x_s;
    if (y_s != 0)
      delete[] y_s;
    x = y = x_s = y_s = 0;
    s_length = 0;

    if (length < 2)
      return;

    // length incl. points to smooth a closed line
    x = new float[length + 3]; // array of x coordinates
    y = new float[length + 3]; // array of y coordinates

    int n = length;
    length = 1;

    for (int i = 0; i < n; i++) {
      //skip points that are in the same position
      if (i != 0 && nodePoints[i] == nodePoints[i - 1])
        continue;
      x[length] = nodePoints[i].x;
      y[length] = nodePoints[i].y;
      length++;
    }

    if (x[length - 1] != x[1] || y[length - 1] != y[1]) {
      // make a loop
      x[length] = x[1];
      y[length] = y[1];
      length++;
    }

    x[0] = x[length - 2];
    y[0] = y[length - 2];
    x[length] = x[2];
    y[length] = y[2];
    length++;

    // smooth areaborder if number-of-points > 3 + 2 endpoints
    if (length > 5) {
      // smooth line put in x_s and y_s
      s_length = (length - 3) * div + length - 2;
      x_s = new float[s_length]; // arrays of SMOOTH coordinates
      y_s = new float[s_length];
      s_length = smoothline(length, &x[0], &y[0], 1, length - 2, div, &x_s[0],
          &y_s[0]);
    }

    if (s_length > 3) {
      float sx, sy;
      for (int i = 0; i < s_length - 1; i++) {
        sx = x_s[i];
        sy = y_s[i];
        // update boundbox
        if (sx < boundBox.x1) {
          boundBox.x1 = sx;
        }
        if (sx > boundBox.x2) {
          boundBox.x2 = sx;
        }
        if (sy < boundBox.y1) {
          boundBox.y1 = sy;
        }
        if (sy > boundBox.y2) {
          boundBox.y2 = sy;
        }
      }
    }
  } //end of spline
}

// -- virtual change of object-state
// if set to passive: smooth and triangulate area
// if set to active:  use crude plotting
void WeatherArea::setState(const state s)
{
  //METLIBS_LOG_DEBUG("setState ");
  currentState = s;
}

void WeatherArea::plot(PlotOrder zorder)
{
  if (!isEnabled())
    return;

  // if this object is visible
  if (isVisible) {
    setWindowInfo();

    //enable blending and set colour
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(objectBorderColour.R(),objectBorderColour.G(),objectBorderColour.B(),objectBorderColour.A());

    if (itsLinetype.stipple) {
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(itsLinetype.factor, itsLinetype.bmap);
    }

    float lwidth = 1.0;
    //change the linewidth of generricarea according to great circle distance
    if (drawIndex == Genericarea ) {
      float scalefactor = getStaticPlot()->getGcd() / 7000000;
      if (scalefactor <= 1)
        lwidth = linewidth;
      else if (scalefactor > 1.0 && scalefactor < 4)
        lwidth = linewidth / scalefactor;
      else if (scalefactor >= 4)
        lwidth = 1;
    }

    glLineWidth(lwidth);
    int end = 0;
    if (currentState == active || !spline || s_length == 0) { // crude plotting: no splines or areafill
      end = nodePoints.size();
      glBegin(GL_LINE_STRIP);
      for (int i = 0; i < end; i++)
        glVertex2f(nodePoints[i].x, nodePoints[i].y);
      if (end && !rubber)
        glVertex2f(nodePoints[0].x, nodePoints[0].y);
      glEnd();
      if (rubber)
        plotRubber();

    } else { // full plot: spline-border
      if (drawIndex == Sigweather && currentState == passive) {
        drawSigweather();
      } else {
        end = s_length;
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i < end; i++)
          glVertex2f(x_s[i], y_s[i]);
        glEnd();
      }
    }

    //draw fillArea
    if (fillArea && currentState == passive && end > 2) {
      int j = 0;
      int npos = end;
      GLdouble *gldata = new GLdouble[npos * 3];

      glShadeModel(GL_FLAT);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glColor4ub(objectColour.R(),objectColour.G(),objectColour.B(),objectColour.A());
      if ( itsFilltype != NULL ) {
        glEnable(GL_POLYGON_STIPPLE);
        glPolygonStipple(itsFilltype);
      }
      if (!spline) {
        // check for identical end-points
        if (nodePoints[0] == nodePoints[npos - 1])
          npos--;
        for (int i = 0; i < npos; i++) {
          gldata[j] = nodePoints[i].x;
          gldata[j + 1] = nodePoints[i].y;
          gldata[j + 2] = 0.0;
          j += 3;
        }
      } else {
        // check for identical end-points
        if (x_s[0] == x_s[npos - 1] && y_s[0] == y_s[npos - 1])
          npos--;
        for (int i = 0; i < npos; i++) {
          gldata[j] = x_s[i];
          gldata[j + 1] = y_s[i];
          gldata[j + 2] = 0.0;
          j += 3;
        }
      }
      beginTesselation();
      tesselation(gldata, 1, &npos);
      endTesselation();
      delete[] gldata;
    }

    // for PostScript generation
    getStaticPlot()->UpdateOutput();
    glDisable(GL_LINE_STIPPLE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_POLYGON_STIPPLE);
    glDisable(GL_BLEND);

    // draws the edge rectangles / points that defines marking and moving
    drawNodePoints();
    getStaticPlot()->UpdateOutput();
  }
}


/**
 * Algorithm for marking points on line
 * Idea is that if mousepointer is moved over a line
 * all nodepoints shine up / are marked
 * If on nodepoint only this point is marked
 * Checks this only if current status is passive.
 */
bool WeatherArea::showLine(float x, float y)
{
  markedChanged = false;

  if (boundBox.isinside(x, y)) {
    if (inBoundBox == false)
      markedChanged = true;
    inBoundBox = true;
  } else {
    if (inBoundBox == true)
      markedChanged = true;
    inBoundBox = false;
  }

  // for very small areas - select the whole thing..
  if (boundBox.isinside(x, y)) {
    if (boundBox.width() / getStaticPlot()->getPlotSize().width() < 0.07 || boundBox.height()
        / getStaticPlot()->getPlotSize().height() < 0.07) {
      markAllPoints();
      return true;
    }
  }
  // checks whether on nodepoint (and mark it)
  if (isInside(x, y)) {
    markPoint(x, y);
    return true;
  }
  // else checkes whether on line (and mark all nodepoints)
  else if (onLine(x, y)) {
    markAllPoints();
    return true;
  } else {
    unmarkAllPoints();
    return false;
  }
  return false;
}

void WeatherArea::setType(int ty)
{
  if (-1 < ty && ty < int(allAreas.size()))
    type = ty;
  else if (ty == int(allAreas.size()))
    type = 0;
  else if (ty == -1)
    type = allAreas.size() - 1;
  else
    return;
  setIndex(allAreas[type].index);
  setBasisColor(allAreas[type].colour);
  setObjectBorderColor(allAreas[type].borderColour);
  setSpline(allAreas[type].spline);
  setLineType(allAreas[type].linetype);
  setLineWidth(defaultLineWidth + allAreas[type].sizeIncrement);
  setFillArea(allAreas[type].filltype);

}

bool WeatherArea::setType(std::string tystring)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("WeatherArea::setType(std::string)=" << tystring);
#endif
  if (areaTypes.find(tystring) != areaTypes.end()) {
    type = areaTypes[tystring];
    setType(type);
    return true;
  }
  return false;
}


bool WeatherArea::setSpline(bool s)
{
  //METLIBS_LOG_DEBUG("SetSpline " << s);
  if (spline != s) {
    spline = s;
    recalculate();
    return true;
  }
  return false;
}

void WeatherArea::setFillArea(const std::string& filltype)
{
  fillArea = false;
  if ( filltype == "diagleft" ) {
    itsFilltype = diagleft;
    fillArea = true;
  } else if( filltype == "zigzag" ) {
    itsFilltype = zigzag;
    fillArea = true;
  } else if( filltype == "paralyse" ) {
    itsFilltype = paralyse;
    fillArea = true;
  } else if ( filltype == "ldiagleft2" ) {
    itsFilltype = ldiagleft2;
    fillArea = true;
  } else if( filltype == "vdiagleft" ) {
    itsFilltype = vdiagleft;
    fillArea = true;
  } else if( filltype == "vldiagcross_little" ) {
    itsFilltype = vldiagcross_little;
    fillArea = true;
  }
}

void WeatherArea::setSelected(bool s)
{
  isSelected = s;
  fillArea = s;
  itsFilltype = NULL;

}


bool WeatherArea::isInsideArea(float x, float y)
{
  multiset<float> xset;
  int n;
  float px, py, xc;

  if (x_s != 0 && y_s != 0 && s_length > 4) {
    n = s_length - 2;

    px = x_s[n - 1];
    py = y_s[n - 1];
    for (int i = 0; i < n; i++) {
      if ((py < y && y_s[i] >= y) || (py >= y && y_s[i] < y)) {
        xc = px + (x_s[i] - px) * (y - py) / (y_s[i] - py);
        xset.insert(xc);
      }
      px = x_s[i];
      py = y_s[i];
    }
  } else if (nodePoints.size() > 2) {
    n = nodePoints.size() - 1;
    px = nodePoints[n - 1].x;
    py = nodePoints[n - 1].y;
    for (int i = 0; i < n; i++) {
      if ((py < y && nodePoints[i].y >= y) || (py >= y && nodePoints[i].y < y)) {
        xc = px + (nodePoints[i].x - px) * (y - py) / (nodePoints[i].y - py);
        xset.insert(xc);
      }
      px = nodePoints[i].x;
      py = nodePoints[i].y;
    }
  } else {
    return false;
  }

  multiset<float>::iterator p = xset.begin(), pend = xset.end();
  int c = 0;
  while (p != pend && *p <= x) {
    c++;
    p++;
  }
  if (c % 2 == 1)
    return true;

  return false;
}

string WeatherArea::writeTypeString()
{
  string ret = "Object=Area;\n";
  ret += "Type=";
  ret += allAreas[type].name;
  ret += ";\n";
  ret += "Linewidth=";
  ret += miutil::from_number(linewidth);
  ret += ";\n";
  return ret;
}

void WeatherArea::drawSigweather()
{
  //METLIBS_LOG_DEBUG("WeatherArea::drawSigweather");
  //calculate total length
  //   if (!orientationClockwise())
  //     flip();
  recalculate();
  first = true;
  npoints = 0;
  if (!smooth())
    return;
  if (x_s != 0)
    delete[] x_s;
  if (y_s != 0)
    delete[] y_s;
  x_s = y_s = 0;
  s_length = npoints;
  x_s = new float[s_length];
  y_s = new float[s_length];
  for (int i = 0; i < npoints; i++) {
    x_s[i] = xplot[i];
    y_s[i] = yplot[i];
  }
  if (xplot != 0)
    delete[] xplot;
  if (yplot != 0)
    delete[] yplot;
  xplot = yplot = 0;
  //smooth once more for better fit...
  first = false;
  if (!smooth())
    return;
  if (x_s != 0)
    delete[] x_s;
  if (y_s != 0)
    delete[] y_s;
  x_s = y_s = 0;
  glLineWidth(siglinewidth);
  for (int i = 0; i < npoints - 1; i++) {
    float deltay, deltax;
    deltay = yplot[i + 1] - yplot[i];
    deltax = xplot[i + 1] - xplot[i];
    float hyp = sqrtf(deltay * deltay + deltax * deltax);
    const int nflag = 19;
    float xflag[nflag], yflag[nflag];
    float flagstep = M_PI / (nflag - 1);
    for (int j = 0; j < nflag; j++) {
      xflag[j] = hyp / 2 * cos(j * flagstep);
      yflag[j] = hyp / 2 * sin(j * flagstep);
    }
    float xx1 = xplot[i] + deltax / 2;
    float yy1 = yplot[i] + deltay / 2;
    glPushMatrix();
    glTranslatef(xx1, yy1, 0.0);
    glRotatef(atan2(deltay, deltax) * RAD_TO_DEG, 0.0, 0.0, 1.0);
    glBegin(GL_LINE_STRIP);
    for (int j = 0; j < nflag; j++)
      glVertex2f(xflag[j], yflag[j]);
    glEnd();
    glPopMatrix();
  }
  if (xplot != 0)
    delete[] xplot;
  if (yplot != 0)
    delete[] yplot;
  xplot = yplot = 0;
  recalculate();
}

bool WeatherArea::smooth()
{
  //produces a curve with evenly spaced points
  float totalLength = 0;
  for (int i = 0; i < s_length - 1; i++) {
    float deltay = y_s[i + 1] - y_s[i];
    float deltax = x_s[i + 1] - x_s[i];
    float hyp = sqrtf(deltay * deltay + deltax * deltax);
    totalLength += hyp;
  }
  int nplot;
  float radius;
  if (first) {
    radius = siglinewidth * 8 * getDwidth();
    nplot = (int) (totalLength / radius) + 1;
    if (nplot < 2)
      return false;
  } else
    nplot = npoints;
  radius = totalLength / (nplot - 1);
  xplot = new float[nplot];
  yplot = new float[nplot];
  xplot[0] = x_s[0];
  yplot[0] = y_s[0];
  int j, i_s = 0;//index of x_s,y_s
  float totalDist = 0; // distance
  bool end = false;
  for (j = 1; j < nplot && totalDist < totalLength && !end; j++) {
    //starting point for this step
    float xstart = xplot[j - 1];
    float ystart = yplot[j - 1];
    //how many steps ???
    float dist = 0; // distance
    int i = i_s;
    while (dist < radius) {
      i++;
      if (i == s_length) {
        i = 0;
        end = true;
      }
      float deltay = y_s[i] - ystart;
      float deltax = x_s[i] - xstart;
      float hyp = sqrtf(deltay * deltay + deltax * deltax);
      dist += hyp;
      //next step
      xstart = x_s[i];
      ystart = y_s[i];
    }
    float deltax, deltay, hyp;
    deltay = ystart - yplot[j - 1];
    deltax = xstart - xplot[j - 1];
    hyp = sqrtf(deltay * deltay + deltax * deltax);
    float cosalfa = deltax / hyp;
    float sinalfa = deltay / hyp;
    xplot[j] = xplot[j - 1] + radius * cosalfa;
    yplot[j] = yplot[j - 1] + radius * sinalfa;
    deltay = yplot[j] - yplot[j - 1];
    deltax = xplot[j] - xplot[j - 1];
    hyp = sqrtf(deltay * deltay + deltax * deltax);
    totalDist += hyp;
    i_s = i - 1;
  }
  npoints = j;
  xplot[npoints - 1] = xplot[0];
  yplot[npoints - 1] = yplot[0];
  return true;
}

void WeatherArea::flip()
{
  int end = nodePoints.size();
  for (int j = 0; j < end / 2; j++) {
    float x = nodePoints[j].x;
    float y = nodePoints[j].y;
    nodePoints[j].x = nodePoints[end - 1 - j].x;
    nodePoints[j].y = nodePoints[end - 1 - j].y;
    nodePoints[end - 1 - j].x = x;
    nodePoints[end - 1 - j].y = y;
  }
}

