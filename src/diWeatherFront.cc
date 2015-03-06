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

#include <diWeatherFront.h>
#include <puTools/miStringFunctions.h>
#include <sstream>

#define MILOGGER_CATEGORY "diana.WeatherFront"
#include <miLogger/miLogging.h>

using namespace::miutil;
using namespace std;

vector<editToolInfo> WeatherFront::allFronts; //info about fronts
map<std::string,int> WeatherFront::frontTypes;   //finds front type number from name
float WeatherFront::defaultLineWidth=8;

// Default constructor

WeatherFront::WeatherFront() : ObjectPlot(wFront),linewidth(defaultLineWidth)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("WeatherFront default constructror: " << defaultLineWidth);
#endif
  setType(0);         // default fronttype
}

// Constructor taking front type as argument

WeatherFront::WeatherFront(int ty) : ObjectPlot(wFront),linewidth(defaultLineWidth)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("WeatherFront(int) constructor: " << defaultLineWidth);
#endif
  setType(ty);         // fronttype
}

WeatherFront::WeatherFront(std::string tystring) : ObjectPlot(wFront),linewidth(defaultLineWidth)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("WeatherFront(std::string) constructor: " << defaultLineWidth);
#endif
  // set correct fronttype
  if (!setType(tystring))
    METLIBS_LOG_ERROR("WeatherFront constructor error, type " << tystring << " not found !!!");

}



//copy constructor

WeatherFront::WeatherFront(const WeatherFront &rhs) : ObjectPlot(rhs){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("WeatherFront copy constructror");
#endif
  linewidth=rhs.linewidth;    // default linewidth of front
  rubber = false; //draw rubber ?
  s_length = rhs.s_length;
  x_s = new float[s_length];
  y_s = new float[s_length];
  for (int i = 0;i<s_length;i++){
    x_s[i] = rhs.x_s[i];
    y_s[i] = rhs.y_s[i];
  }
}

// Destructor
WeatherFront::~WeatherFront(){
}


void WeatherFront::defineFronts(vector<editToolInfo> fronts)
{
  allFronts = fronts;
  for (unsigned int i = 0;i<fronts.size();i++)
    frontTypes[fronts[i].name] = i;
}




void WeatherFront::recalculate(){
  //METLIBS_LOG_DEBUG("WeatherFront::recalculate");
  // Makes smooth lines
  int div=divSpline;  // div = subdivision points between to edge points

  if (x != NULL)  delete[] x;
  if (y != NULL)  delete[] y;
  if (x_s != NULL)  delete[] x_s;
  if (y_s != NULL)  delete[] y_s;
  x= y= x_s= y_s= 0;
  s_length= 0;

  int length=nodePoints.size();
  if (length < 1) return;
  x= new float[length]; // x array of x coordinates
  y= new float[length]; // y array of y coordinates
  length = 0;

  for (unsigned int i=0; i<nodePoints.size(); i++){
    //skip points that are in the same position
    if (i!=0 && nodePoints[i]==nodePoints[i-1]) continue;
    x[length]=nodePoints[i].x;
    y[length]=nodePoints[i].y;
    length++;
  }

  // smooth line made in x_s and y_s

  x_s=new float[(length-1)*div+length]; // x_s array of SMOOTH x coordinates
  y_s=new float[(length-1)*div+length];
  s_length=smoothline(length, &x[0], &y[0] , 0, length-1,div, &x_s[0], &y_s[0]);

  for (int i=0; i<s_length; i++){
    float x=x_s[i];
    float y=y_s[i];
    if (! boundBox.isinside(x,y)){
      if (x < boundBox.x1){ boundBox.x1=x; }
      if (x > boundBox.x2){ boundBox.x2=x; }
      if (y < boundBox.y1){ boundBox.y1=y; }
      if (y > boundBox.y2){ boundBox.y2=y; }
    }
  }


}


// -- virtual change of object-state
// if set to passive: smooth and triangulate area
void WeatherFront::setState(const state s)
{
  currentState= s;
}

// draws the weather front
void WeatherFront::plot(PlotOrder zorder)
{
  if (!isEnabled())
    return;

  if (isVisible){
    int end = nodePoints.size();
    if (0<end){

      setWindowInfo();

      //recalculate();

      if (drawIndex == SigweatherFront)
        scaledlinewidth=siglinewidth*2;
      else {
        //change the linewidth according to great circle distance
        float scalefactor = getStaticPlot()->getGcd()/7000000;
        if (scalefactor <=1 )
          scaledlinewidth = linewidth;
        else if (scalefactor > 1.0 && scalefactor < 4)
          scaledlinewidth = linewidth/scalefactor;
        else if (scalefactor >=4)
          scaledlinewidth = 1;
      }

      //enable blending and set colour
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glColor4ub(objectColour.R(),objectColour.G(),objectColour.B(),objectColour.A());
      switch (drawIndex){
      case Cold:
        drawColds();
        break;
      case Warm:
        drawWarms();
        break;
      case Occluded:
        drawOccluded();
        break;
      case Stationary:
        drawStationary();
        break;
      case TroughLine:
        drawTroughLine(); // nothing to draw, only the line (below)
        break;
      case SquallLine:
        drawSquallLine();
        break;
      case SigweatherFront:
        if (currentState == passive) drawSigweather();
        break;
      case ArrowLine:
        drawArrowLine();
        break;
      }

      // for PostScript generation
      getStaticPlot()->UpdateOutput();

      //draw line
      if ( currentState == active || drawIndex != SigweatherFront ) {
        glPushMatrix();

        //draw the line, first set colour and linewidth
        glColor4ub(objectColour.R(),objectColour.G(),objectColour.B(),objectColour.A());
        glLineWidth(scaledlinewidth/2);
        if (itsLinetype.stipple) {
           glEnable(GL_LINE_STIPPLE);
           glLineStipple(itsLinetype.factor,itsLinetype.bmap);
        }
        glBegin(GL_LINE_STRIP);        // Draw smooth line
        if (spline){
          for (int i=0; i<s_length; i++)
            glVertex2f((x_s[i]),(y_s[i]));
        } else{                        //Draw unsmoothed line
          for (int i=0; i<end; i++)
            glVertex2f(nodePoints[i].x,nodePoints[i].y);
        }
        glEnd();
        glPopMatrix();
        if (rubber) plotRubber();
      }

      getStaticPlot()->UpdateOutput();
      if (itsLinetype.stipple) {
        glDisable(GL_LINE_STIPPLE);
      }

      glDisable(GL_BLEND);

      drawNodePoints();
      getStaticPlot()->UpdateOutput();
    }
  }
}


int WeatherFront::hitPoint(float x,float y){
  //find out between which points we should split in two
  if (onLine(x,y)){
    return insert;
  }
  else return 0;
}


bool WeatherFront::addFront(ObjectPlot * qfront){
  //qfront is added to front
  float x,y,xnew,ynew;

  if (addTop){
    x=nodePoints.front().x;
    y=nodePoints.front().y;
  } else {
    x=nodePoints.back().x;
    y=nodePoints.back().y;
  }
  int end = qfront->getXYZsize();
  vector<float> xq=qfront->getX();
  vector<float> yq=qfront->getY();
  currentState = active;
  if (qfront->isBeginPoint(x,y,xnew,ynew)){
    movePoint(x,y,xnew,ynew);
    for (int i = 1;i<end;i++){
      addPoint(xq[i],yq[i]);
    }
  } else if (qfront->isEndPoint(x,y,xnew,ynew)){
    movePoint(x,y,xnew,ynew);
    for (int i = end-2;i>-1;i--){
      addPoint(xq[i],yq[i]);
    }
  } else{
    return false;
  }
  currentState = passive;
  return true;
}


ObjectPlot* WeatherFront::splitFront(float x,float y){
  //splits front in two at x,y
  //pointer to new front is returned
  WeatherFront* newFront = new WeatherFront();
  newFront->setType(type);
  currentState = active;
  addTop=false;
  newFront->currentState=active;
  newFront->setWindowInfo();
  int ihit = hitPoint(x,y);
  if (ihit==0){
    delete newFront;
    return NULL;
  }
  int start = nodePoints.size()-1;
  for(int i=start;i>=ihit;i--){
    newFront->addPoint(nodePoints[i].x,nodePoints[i].y);
    nodePoints.erase(nodePoints.begin()+i);
  }
  //last add point x,y to the beginning of new front
  newFront->addPoint(x,y);
  addPoint(x,y);

  unmarkAllPoints();
  newFront->unmarkAllPoints();

  return newFront;
}



/**
 * Algorithm for marking points on line
 * Idea is that if mousepointer is moved over a line
 * all nodepoints shine up / are marked
 * If on nodepoint only this point is marked
 * Checks this only if current status is passive.
 */
bool WeatherFront::showLine(float x, float y){
  markedChanged=false;

  if (boundBox.isinside(x,y)) {
    if (inBoundBox==false) markedChanged=true;
    inBoundBox=true;
  }
  else{
    if (inBoundBox==true) markedChanged=true;
    inBoundBox=false;
  }

  if (isInside(x,y)){
    markPoint(x,y); // checks whether on nodepoint and marks
    return true;
  }
  else if (onLine(x,y)){ // else checkes whether on line
    markAllPoints();
    return true;
  }
  else {
    unmarkAllPoints();
    return false;
  }
  return false;
}


/* Draw triangles on smooth line */
void WeatherFront::drawColds(){
  // METLIBS_LOG_DEBUG("WeatherFront::drawColds");
  // colds are blue triangles on the front

  float r= scaledlinewidth*2*getDwidth();
  int end= s_length;
  int ncount=0;

  float xstart,ystart,xend,yend,xtop,ytop,dxs = 0.0,dys = 0.0,fraction;
  float s,slim,sprev = 0.0, x1,y1,s1,x2,y2,xm,ym,sm;
  int i,istart,j;

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glLineWidth(1);

  slim= r*0.75;
  s= 0.;
  i=1;

  while (i<end) {

    while (s<slim && i<end) {
      sprev= s;
      dxs= x_s[i] - x_s[i-1];
      dys= y_s[i] - y_s[i-1];
      s+= sqrtf(dxs*dxs+dys*dys);
      i++;
    }
    if (s<slim) break;
    i--;
    istart= i-1;
    fraction= (slim-sprev)/(s-sprev);
    xstart= x_s[i-1] + dxs * fraction;
    ystart= y_s[i-1] + dys * fraction;
    s= 0.;
    slim= r*2.;

    while (s<slim && i<end) {
      dxs= x_s[i] - xstart;
      dys= y_s[i] - ystart;
      sprev= s;
      s= sqrtf(dxs*dxs+dys*dys);
      i++;
    }
    if (s<slim) break;

    i--;
    if (istart==i-1) {
      fraction= slim/s;
      xend= xstart + dxs * fraction;
      yend= ystart + dys * fraction;
    } else {
      x1= x_s[i-1];
      y1= y_s[i-1];
      s1= sqrtf((x1-xstart)*(x1-xstart)+(y1-ystart)*(y1-ystart));
      x2= x_s[i];
      y2= y_s[i];
      for (j=0; j<10; j++) {
        xm= (x1+x2)*0.5;
        ym= (y1+y2)*0.5;
        sm= sqrtf((xm-xstart)*(xm-xstart)+(ym-ystart)*(ym-ystart));
        if ((s1-slim)*(sm-slim)<=0.) {
          x2= xm;
          y2= ym;
        } else {
          x1= xm;
          y1= ym;
          s1= sm;
        }
      }
      xend= (x1+x2)*0.5;
      yend= (y1+y2)*0.5;
    }

    dxs= xend - xstart;
    dys= yend - ystart;

    xtop= (xstart+xend)*0.5 - dys*0.6;
    ytop= (ystart+yend)*0.5 + dxs*0.6;

    if (ncount%2==0){
      glBegin(GL_POLYGON);
      glVertex2f(xstart,ystart);
      glVertex2f(xend,yend);
      glVertex2f(xtop,ytop);
      glEnd();
    }
    ncount++;

    dxs= xend - x_s[i-1];
    dys= yend - y_s[i-1];
    slim= sqrtf(dxs*dxs+dys*dys) + r*1.5;
    s= 0.;

  }

  getStaticPlot()->UpdateOutput();

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

/*
   Draw arch on smooth line
 */
void WeatherFront::drawWarms(){
  //  METLIBS_LOG_DEBUG("WeatherFront::drawWarms");
  // warms are red arches on the front

  int ncount=0;

  float r= scaledlinewidth*2*getDwidth();
  int end= s_length;

  float xstart,ystart,xend,yend,dxs = 0.0,dys = 0.0,fraction;
  float s,slim,sprev =0.0, x1,y1,s1,x2,y2,xm,ym,sm;
  int i,istart,j;

  const int nwarmflag= 19;
  float xwarmflag[nwarmflag], ywarmflag[nwarmflag];
  float flagstep= M_PI/(nwarmflag-1);

  for (j=0; j<nwarmflag; j++) {
    xwarmflag[j]= r*cos(j*flagstep);
    ywarmflag[j]= r*sin(j*flagstep);
  }

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glLineWidth(1);

  slim= r*0.75;
  s= 0.;
  i=1;

  while (i<end) {

    while (s<slim && i<end) {
      sprev= s;
      dxs= x_s[i] - x_s[i-1];
      dys= y_s[i] - y_s[i-1];
      s+= sqrtf(dxs*dxs+dys*dys);
      i++;
    }
    if (s<slim) break;

    i--;
    istart= i-1;
    fraction= (slim-sprev)/(s-sprev);
    xstart= x_s[i-1] + dxs * fraction;
    ystart= y_s[i-1] + dys * fraction;
    s= 0.;
    slim= r*2.;

    while (s<slim && i<end) {
      dxs= x_s[i] - xstart;
      dys= y_s[i] - ystart;
      sprev= s;
      s= sqrtf(dxs*dxs+dys*dys);
      i++;
    }
    if (s<slim) break;

    i--;
    if (istart==i-1) {
      fraction= slim/s;
      xend= xstart + dxs * fraction;
      yend= ystart + dys * fraction;
    } else {
      x1= x_s[i-1];
      y1= y_s[i-1];
      s1= sqrtf((x1-xstart)*(x1-xstart)+(y1-ystart)*(y1-ystart));
      x2= x_s[i];
      y2= y_s[i];
      for (j=0; j<10; j++) {
        xm= (x1+x2)*0.5;
        ym= (y1+y2)*0.5;
        sm= sqrtf((xm-xstart)*(xm-xstart)+(ym-ystart)*(ym-ystart));
        if ((s1-slim)*(sm-slim)<=0.) {
          x2= xm;
          y2= ym;
        } else {
          x1= xm;
          y1= ym;
          s1= sm;
        }
      }
      xend= (x1+x2)*0.5;
      yend= (y1+y2)*0.5;
    }

    dxs= xend - xstart;
    dys= yend - ystart;
    xm= (xstart+xend)*0.5;
    ym= (ystart+yend)*0.5;

    if (ncount%2==0){
      glPushMatrix();
      glTranslatef(xm, ym, 0.0);
      glRotatef(atan2(dys,dxs)*RAD_TO_DEG,0.0,0.0,1.0);
      glBegin(GL_POLYGON);
      for (j=0; j<nwarmflag; j++)
        glVertex2f(xwarmflag[j],ywarmflag[j]);
      glEnd();
      glPopMatrix();
    }
    ncount++;


    dxs= xend - x_s[i-1];
    dys= yend - y_s[i-1];
    slim= sqrtf(dxs*dxs+dys*dys) + r*1.5;
    s= 0.;

  }

  getStaticPlot()->UpdateOutput();

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}


/*
  Draws triangles and arch on smooth line
 */
void WeatherFront::drawOccluded(){
  //  METLIBS_LOG_DEBUG("WeatherFront::drawOccluded");

  float r= scaledlinewidth*2*getDwidth();
  int end= s_length;

  float xstart,ystart,xend,yend,xtop,ytop,dxs = 0.0,dys = 0.0,fraction;
  float xstart1 = 0.0,ystart1 = 0.0,xend1 = 0.0,yend1 = 0.0;
  float s,slim,sprev = 0.0, x1,y1,s1,x2,y2,xm,ym,sm;
  int i,istart,j;
  int ndrawflag= 0;
  int ncount=0;

  const int nwarmflag= 19;
  float xwarmflag[nwarmflag], ywarmflag[nwarmflag];
  float flagstep= M_PI/(nwarmflag-1);

  for (j=0; j<nwarmflag; j++) {
    xwarmflag[j]= r*cos(j*flagstep);
    ywarmflag[j]= r*sin(j*flagstep);
  }

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glLineWidth(1);

  slim= r*0.75;
  s= 0.;
  i=1;

  while (i<end) {

    while (s<slim && i<end) {
      sprev= s;
      dxs= x_s[i] - x_s[i-1];
      dys= y_s[i] - y_s[i-1];
      s+= sqrtf(dxs*dxs+dys*dys);
      i++;
    }
    if (s<slim) break;

    i--;
    istart= i-1;
    fraction= (slim-sprev)/(s-sprev);
    xstart= x_s[i-1] + dxs * fraction;
    ystart= y_s[i-1] + dys * fraction;

    istart= i-1;
    s= 0.;
    slim= r*2.;

    while (s<slim && i<end) {
      dxs= x_s[i] - xstart;
      dys= y_s[i] - ystart;
      sprev= s;
      s= sqrtf(dxs*dxs+dys*dys);
      i++;
    }
    if (s<slim) break;

    i--;
    if (istart==i-1) {
      fraction= slim/s;
      xend= xstart + dxs * fraction;
      yend= ystart + dys * fraction;
    } else {
      x1= x_s[i-1];
      y1= y_s[i-1];
      s1= sqrtf((x1-xstart)*(x1-xstart)+(y1-ystart)*(y1-ystart));
      x2= x_s[i];
      y2= y_s[i];
      for (j=0; j<10; j++) {
        xm= (x1+x2)*0.5;
        ym= (y1+y2)*0.5;
        sm= sqrtf((xm-xstart)*(xm-xstart)+(ym-ystart)*(ym-ystart));
        if ((s1-slim)*(sm-slim)<=0.) {
          x2= xm;
          y2= ym;
        } else {
          x1= xm;
          y1= ym;
          s1= sm;
        }
      }
      xend= (x1+x2)*0.5;
      yend= (y1+y2)*0.5;
    }




    ndrawflag++;

    if (ndrawflag==1) {

      xstart1= xstart;
      ystart1= ystart;
      xend1=   xend;
      yend1=   yend;

    } else {

      dxs= xend1 - xstart1;
      dys= yend1 - ystart1;
      xm= (xstart1+xend1)*0.5;
      ym= (ystart1+yend1)*0.5;

      if (ncount%2==0){
        glPushMatrix();
        glTranslatef(xm, ym, 0.0);
        glRotatef(atan2(dys,dxs)*RAD_TO_DEG,0.0,0.0,1.0);
        glBegin(GL_POLYGON);
        for (j=0; j<nwarmflag; j++)
          glVertex2f(xwarmflag[j],ywarmflag[j]);
        glEnd();
        glPopMatrix();

        dxs= xend - xstart;
        dys= yend - ystart;
        xtop= (xstart+xend)*0.5 - dys*0.6;
        ytop= (ystart+yend)*0.5 + dxs*0.6;

        glBegin(GL_POLYGON);
        glVertex2f(xstart,ystart);
        glVertex2f(xend,yend);
        glVertex2f(xtop,ytop);
        glEnd();
      }
      ncount++;

      ndrawflag=0;
    }

    dxs= xend - x_s[i-1];
    dys= yend - y_s[i-1];
    slim= sqrtf(dxs*dxs+dys*dys);
    if (ndrawflag==0) slim+= r*1.5;
    s= 0.;

  }

  getStaticPlot()->UpdateOutput();

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

/*
  Draws stationary front on smooth line
 */
void WeatherFront::drawStationary(){
  //  METLIBS_LOG_DEBUG("WeatherFront::drawStationary");
  // colds are blue triangles on the front

  float r= scaledlinewidth*2*getDwidth();
  int end= s_length;

  float xstart,ystart,xend,yend,xtop,ytop,dxs = 0.0,dys = 0.0,fraction;
  float s,slim,sprev = 0.0, x1,y1,s1,x2,y2,xm,ym,sm;
  int i,istart,j;
  int ndrawflag= 0;
  int ncount=0;

  const int nwarmflag= 19;
  float xwarmflag[nwarmflag], ywarmflag[nwarmflag];
  float flagstep= M_PI/(nwarmflag-1);

  for (j=0; j<nwarmflag; j++) {
    xwarmflag[j]= r*cos(j*flagstep);
    ywarmflag[j]= r*sin(j*flagstep);
  }

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glLineWidth(1);

  slim= r*0.5;
  s= 0.;
  i=1;

  while (i<end) {

    while (s<slim && i<end) {
      sprev= s;
      dxs= x_s[i] - x_s[i-1];
      dys= y_s[i] - y_s[i-1];
      s+= sqrtf(dxs*dxs+dys*dys);
      i++;
    }
    if (s<slim) break;

    i--;
    istart= i-1;
    fraction= (slim-sprev)/(s-sprev);
    xstart= x_s[i-1] + dxs * fraction;
    ystart= y_s[i-1] + dys * fraction;

    istart= i-1;
    s= 0.;
    slim= r*2.;

    while (s<slim && i<end) {
      dxs= x_s[i] - xstart;
      dys= y_s[i] - ystart;
      sprev= s;
      s= sqrtf(dxs*dxs+dys*dys);
      i++;
    }
    if (s<slim) break;

    i--;
    if (istart==i-1) {
      fraction= slim/s;
      xend= xstart + dxs * fraction;
      yend= ystart + dys * fraction;
    } else {
      x1= x_s[i-1];
      y1= y_s[i-1];
      s1= sqrtf((x1-xstart)*(x1-xstart)+(y1-ystart)*(y1-ystart));
      x2= x_s[i];
      y2= y_s[i];
      for (j=0; j<10; j++) {
        xm= (x1+x2)*0.5;
        ym= (y1+y2)*0.5;
        sm= sqrtf((xm-xstart)*(xm-xstart)+(ym-ystart)*(ym-ystart));
        if ((s1-slim)*(sm-slim)<=0.) {
          x2= xm;
          y2= ym;
        } else {
          x1= xm;
          y1= ym;
          s1= sm;
        }
      }
      xend= (x1+x2)*0.5;
      yend= (y1+y2)*0.5;
    }


    if (ncount%2==0){
      ndrawflag++;

      if (ndrawflag==1) {

        dxs= xend - xstart;
        dys= yend - ystart;
        xm= (xstart+xend)*0.5;
        ym= (ystart+yend)*0.5;

        glColor3f(1.,0.,0.);
        glPushMatrix();
        glTranslatef(xm, ym, 0.0);
        glRotatef(atan2(dys,dxs)*RAD_TO_DEG,0.0,0.0,1.0);
        glBegin(GL_POLYGON);
        for (j=0; j<nwarmflag; j++)
          glVertex2f(xwarmflag[j],ywarmflag[j]);
        glEnd();
        glPopMatrix();

      } else {

        dxs= xend - xstart;
        dys= yend - ystart;
        xtop= (xstart+xend)*0.5 + dys*0.6;
        ytop= (ystart+yend)*0.5 - dxs*0.6;

        glColor3f(0.,0.,1.);
        glBegin(GL_POLYGON);
        glVertex2f(xstart,ystart);
        glVertex2f(xend,yend);
        glVertex2f(xtop,ytop);
        glEnd();

        ndrawflag=0;
      }
    }
    ncount++;

    dxs= xend - x_s[i-1];
    dys= yend - y_s[i-1];
    slim= sqrtf(dxs*dxs+dys*dys) + r;
    s= 0.;

  }

  getStaticPlot()->UpdateOutput();

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}


/*
  Draws Squall line crosses on smooth line
 */
void WeatherFront::drawSquallLine(){
  //  METLIBS_LOG_DEBUG("WeatherFront::drawSquallLine");
  // colds are blue triangles on the front

  //float r= scaledlinewidth*2*getDwidth();
  float r= scaledlinewidth*getDwidth();
  int end= s_length;
  int ncount=0;

  float dxs = 0.0,dys = 0.0,fraction;
  float s,slim,sprev = 0.0, xm,ym;
  int i;

  slim= r*2.;
  s= 0.;
  r*=1.5;
  i=1;

  glLineWidth(scaledlinewidth/2);

  while (i<end) {

    while (s<slim && i<end) {
      sprev= s;
      dxs= x_s[i] - x_s[i-1];
      dys= y_s[i] - y_s[i-1];
      s+= sqrtf(dxs*dxs+dys*dys);
      i++;
    }
    if (s<slim) break;

    i--;
    fraction= (slim-sprev)/(s-sprev);
    xm= x_s[i-1] + dxs * fraction;
    ym= y_s[i-1] + dys * fraction;

    if (ncount%2==0){

      glPushMatrix();
      glTranslatef(xm, ym, 0.0);
      glRotatef(atan2(dys,dxs)*RAD_TO_DEG,0.0,0.0,1.0);

      glBegin(GL_LINES);
      glVertex2f(-r,-r);
      glVertex2f( r, r);
      glVertex2f( r,-r);
      glVertex2f(-r, r);
      glEnd();

      glPopMatrix();
    }
    ncount++;

    dxs= xm - x_s[i-1];
    dys= ym - y_s[i-1];
    slim= sqrtf(dxs*dxs+dys*dys) + r*4.;
    s= 0.;

  }
  getStaticPlot()->UpdateOutput();
}


/*
  Draws arrowline
 */
void WeatherFront::drawArrowLine(){
  float r= scaledlinewidth*2*getDwidth();
  int end= s_length;
  int ncount=0;

  float xstart,ystart,xend,yend,xtop1,ytop1,xtop2,ytop2,dxs,dys,fraction;
  float s,slim,sprev;
  int i,istart;

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  //slim= r*4;
  slim= r*2.75;
  s= dxs= dys= sprev= 0.;
  i=1;

    i=end-1;
    while (s<slim && i>0) {
      sprev= s;
      dxs= x_s[i] - x_s[i-1];
      dys= y_s[i] - y_s[i-1];
      s+= sqrtf(dxs*dxs+dys*dys);
    //METLIBS_LOG_DEBUG("********** s1 =   " << s);
      i--;
      ncount++;
    }
    i++;
    istart= i-1;
    fraction= (slim-sprev)/(s-sprev);
    xstart= x_s[i] - dxs * fraction;
    ystart= y_s[i] - dys * fraction;
    s= 0.;
    slim= r*1.2;
    //slim= r*2.;

    while (s<slim && i>0) {
      dxs= x_s[i] - x_s[i-1];
      dys= y_s[i] - y_s[i-1];
      sprev= s;
      s= sqrtf(dxs*dxs+dys*dys);
      i--;
    }
    //METLIBS_LOG_DEBUG("********** i2 =   " << i);

      fraction= slim/s;
      xend= xstart - dxs * fraction;
      yend= ystart - dys * fraction;

    dxs= xend - xstart;
    dys= yend - ystart;

    xtop1= (xstart+xend)*0.5 - dys*0.6;
    ytop1= (ystart+yend)*0.5 + dxs*0.6;

    xtop2= (xstart+xend)*0.5 + dys*0.6;
    ytop2= (ystart+yend)*0.5 - dxs*0.6;

  //METLIBS_LOG_DEBUG("********** ncount =   " << ncount);
    /*   glPointSize(4.0);
  	//glLoadIdentity();
	glColor3f(1.0, 0.0, 0.0); //red 
	// Draw filtered points
	glEnable(GL_POINT_SMOOTH);
	glBegin(GL_POINTS);
		glVertex2f(xstart,ystart);
	glEnd();
	glColor3f(0.0, 0.0, 1.0);     //blue
	// Draw filtered points
	glEnable(GL_POINT_SMOOTH);
	glBegin(GL_POINTS);
		glVertex2f(xtop1,ytop1);
		glVertex2f(xtop2,ytop2);
	glEnd();


  	glColor3f(1.0, 0.75, 0.0);   //orange
	// Draw filtered points
	glEnable(GL_POINT_SMOOTH);
	glBegin(GL_POINTS);
		//glVertex2f(x_s[s_length-1], y_s[s_length-1]);
		glVertex2f(xend,yend);
	glEnd();*/
	//glLoadIdentity();
    if (ncount > 0) {
      glBegin(GL_POLYGON);
      glVertex2f(x_s[s_length-1], y_s[s_length-1]);
      glVertex2f(xtop1,ytop1);
      glVertex2f(xtop2,ytop2);
      glEnd();
    }
  getStaticPlot()->UpdateOutput();

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

}





/*
  Draws TroughLine
 */
void WeatherFront::drawTroughLine(){
  //  METLIBS_LOG_DEBUG("WeatherFront::drawTroughLine");
  float r= scaledlinewidth*2*getDwidth();
  int end= s_length;
  int ncount=0;

  float xstart,ystart,xend,yend,xtop1,ytop1,xtop2,ytop2,dxs,dys,fraction;
  float s,slim,sprev, x1,y1,s1,x2,y2,xm,ym,sm;
  int i,istart,j;

 // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glLineWidth(scaledlinewidth);

  slim= r*0.75;
  s= sprev= dys= dxs= 0.;
  i=1;

  while (i<end) {

    while (s<slim && i<end) {
      sprev= s;
      dxs= x_s[i] - x_s[i-1];
      dys= y_s[i] - y_s[i-1];
      s+= sqrtf(dxs*dxs+dys*dys);
    //METLIBS_LOG_DEBUG("********** s1 =   " << s);
      i++;
    }
    if (s<slim) break;
    METLIBS_LOG_DEBUG("********** s1 =   " << s);
    METLIBS_LOG_DEBUG("********** i1 =   " << i);
    METLIBS_LOG_DEBUG("********** dxs1 =   " << dxs);
    METLIBS_LOG_DEBUG("********** dys1 =   " << dys);
    METLIBS_LOG_DEBUG("********** slim =   " << slim);
    METLIBS_LOG_DEBUG("********** sprev =   " << sprev);
    i--;
    istart= i-1;
    fraction= (slim-sprev)/(s-sprev);
    xstart= x_s[i-1] + dxs * fraction;
    ystart= y_s[i-1] + dys * fraction;
    s= 0.;
    slim= r*2.;

    while (s<slim && i<end) {
      dxs= x_s[i] - xstart;
      dys= y_s[i] - ystart;
      sprev= s;
      s= sqrtf(dxs*dxs+dys*dys);
      i++;
    }
    if (s<slim) break;
    //METLIBS_LOG_DEBUG("********** i2 =   " << i);

    i--;
    if (istart==i-1) {
      fraction= slim/s;
      xend= xstart + dxs * fraction;
      yend= ystart + dys * fraction;
    } else {
      x1= x_s[i-1];
      y1= y_s[i-1];
      s1= sqrtf((x1-xstart)*(x1-xstart)+(y1-ystart)*(y1-ystart));
      x2= x_s[i];
      y2= y_s[i];
      for (j=0; j<10; j++) {
        xm= (x1+x2)*0.5;
        ym= (y1+y2)*0.5;
        sm= sqrtf((xm-xstart)*(xm-xstart)+(ym-ystart)*(ym-ystart));
        if ((s1-slim)*(sm-slim)<=0.) {
          x2= xm;
          y2= ym;
        } else {
          x1= xm;
          y1= ym;
          s1= sm;
        }
      }
      xend= (x1+x2)*0.5;
      yend= (y1+y2)*0.5;
    }

    dxs= xend - xstart;
    dys= yend - ystart;

    xtop1= (xstart+xend)*0.5 - dys*0.6;
    ytop1= (ystart+yend)*0.5 + dxs*0.6;

    xtop2= (xstart+xend)*0.5 + dys*0.6;
    ytop2= (ystart+yend)*0.5 - dxs*0.6;

    if (ncount%2==0){
    //METLIBS_LOG_DEBUG("********** ncount =   " << ncount);
      /*glBegin(GL_POLYGON);
      glVertex2f(xstart,ystart);
      glVertex2f(xend,yend);
      glVertex2f(xtop,ytop);
      glEnd();*/
//	glColor3f(1.0, 0.75, 0.0);   //orange
//
      if (ncount%4==0) {
         glBegin(GL_LINES);
         //glVertex2f(xstart,ystart);
         glVertex2f(xend,yend);
         glVertex2f(xtop1,ytop1);
         glEnd();
      } else { 
         glBegin(GL_LINES);
         //glVertex2f(xstart,ystart);
         glVertex2f(xend,yend);
         glVertex2f(xtop2,ytop2);
         glEnd();
      }

    }
    ncount++;

    dxs= xend - x_s[i-1];
    dys= yend - y_s[i-1];
    slim= sqrtf(dxs*dxs+dys*dys) + r*1.5;
    s= 0.;

  }

  getStaticPlot()->UpdateOutput();

  //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

}   //end of drawing TroughLine



void WeatherFront::flip(){
  int end = nodePoints.size();
  for (int j=0; j < end/2; j++){
    float x=nodePoints[j].x;
    float y=nodePoints[j].y;
    bool joined = nodePoints[j].joined;
    nodePoints[j].x=nodePoints[end-1-j].x;
    nodePoints[j].y=nodePoints[end-1-j].y;
    nodePoints[j].joined=nodePoints[end-1-j].joined;
    nodePoints[end-1-j].x=x;
    nodePoints[end-1-j].y=y;
    nodePoints[end-1-j].joined=joined;
  }
  recalculate();
}


void WeatherFront::setType(int ty){
  if (-1<ty && ty<int(allFronts.size()))
    type=ty;
  else if (ty==int(allFronts.size()))
    type=0;
  else if (ty==-1)
    type =allFronts.size()-1;
  else
    return;

  setIndex(allFronts[type].index);
  setBasisColor(allFronts[type].colour);
  setSpline(allFronts[type].spline);
  setLineType(allFronts[type].linetype);
  setLineWidth(defaultLineWidth + allFronts[type].sizeIncrement);

}

bool WeatherFront::setType(std::string tystring){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("WeatherFront::setType(std::string)=" << tystring <<  endl);
#endif
  if (frontTypes.find(tystring)!=frontTypes.end()){
    type = frontTypes[tystring];
    setType(type);
    return true;
  }
  return false;
}


bool WeatherFront::setSpline(bool s){
  if (spline!=s){
    spline=s;
    recalculate();
    return true;
  }
  return false;
}

string WeatherFront::writeTypeString()
{
  string ret ="Object=Front;\n";
  ret+="Type=";
  ret+=allFronts[type].name;
  ret+=";\n";
  ret+="Linewidth=";
  ret+=miutil::from_number(linewidth);
  ret+=";\n";
  return ret;
}



void WeatherFront::drawSigweather(){
  //METLIBS_LOG_DEBUG("WeatherFront::drawSigweather");
  recalculate();
  first=true;
  npoints=0;
  if (!smooth()) return;
  if (x_s != 0)  delete[] x_s;
  if (y_s != 0)  delete[] y_s;
  x_s= y_s= 0;
  s_length= npoints;
  x_s=new float[s_length];
  y_s=new float[s_length];
  for (int i = 0; i < npoints; i++){
    x_s[i]=xplot[i];
    y_s[i]=yplot[i];
  }
  if (xplot!=0) delete[] xplot;
  if (yplot!=0) delete[] yplot;
  xplot=yplot=0;
  //smooth once more for better fit...
  first=false;
  if (!smooth()) return;
  if (x_s != 0)  delete[] x_s;
  if (y_s != 0)  delete[] y_s;
  x_s= y_s= 0;
  glLineWidth(siglinewidth);
  for (int i = 0; i < npoints-1; i++){
    float deltay,deltax;
    deltay = yplot[i+1]-yplot[i];
    deltax = xplot[i+1]-xplot[i];
    float hyp = sqrtf(deltay*deltay+deltax*deltax);
    const int nflag= 19;
    float xflag[nflag], yflag[nflag];
    float flagstep= M_PI/(nflag-1);
    for (int j=0; j<nflag; j++) {
      xflag[j]= hyp/2*cos(j*flagstep);
      yflag[j]= hyp/2*sin(j*flagstep);
    }
    float xx1=xplot[i]+deltax/2;
    float yy1=yplot[i]+deltay/2;
    glPushMatrix();
    glTranslatef(xx1, yy1, 0.0);
    glRotatef(atan2(deltay,deltax)*RAD_TO_DEG,0.0,0.0,1.0);
    glBegin(GL_LINE_STRIP);
    for (int j=0; j<nflag; j++)
      glVertex2f(xflag[j],yflag[j]);
    glEnd();
    glPopMatrix();
  }
  if (xplot!=0) delete[] xplot;
  if (yplot!=0) delete[] yplot;
  xplot=yplot=0;
  recalculate();
}


bool WeatherFront::smooth(){
  //METLIBS_LOG_DEBUG("WeatherFront::smoth");
  //produces a curve with evenly spaced points
  float totalLength=0;
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
  xplot[npoints - 1] = x_s[s_length - 1];
  yplot[npoints - 1] = y_s[s_length - 1];
  return true;
}
