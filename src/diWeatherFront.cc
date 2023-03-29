/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2022 met.no

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

#include "diWeatherFront.h"

#include "diGLPainter.h"
#include "diGlUtilities.h"
#include "diStaticPlot.h"
#include "util/misc_util.h"

#include <mi_fieldcalc/math_util.h>

#include <puTools/miStringFunctions.h>

#include <sstream>

#define MILOGGER_CATEGORY "diana.WeatherFront"
#include <miLogger/miLogging.h>

using namespace::miutil;

namespace /* anonymous */ {
const float RAD_TO_DEG = 180 / M_PI;

void drawHalfCircle(DiGLPainter* gl, bool fill, const QPointF& pos, float angle_deg, float radius)
{
  const int nflag= 19;
  float xflag[nflag], yflag[nflag];
  float flagstep= M_PI/(nflag-1);

  for (int j=0; j<nflag; j++) {
    xflag[j]= radius * cos(j*flagstep);
    yflag[j]= radius * sin(j*flagstep);
  }

  diutil::GlMatrixPushPop pushpop(gl);
  gl->Translatef(pos.x(), pos.y(), 0.0);
  gl->Rotatef(angle_deg,0.0,0.0,1.0);
  gl->Begin(fill ? DiGLPainter::gl_POLYGON : DiGLPainter::gl_LINE_STRIP);
  for (int j=0; j<nflag; j++)
    gl->Vertex2f(xflag[j],yflag[j]);
  gl->End();
}
} // namespace anonymous

std::vector<editToolInfo> WeatherFront::allFronts; //info about fronts
std::map<std::string,int> WeatherFront::frontTypes;   //finds front type number from name
float WeatherFront::defaultLineWidth=8;

WeatherFront::WeatherFront()
  : ObjectPlot(wFront)
  , linewidth(defaultLineWidth)
{
  METLIBS_LOG_SCOPE(LOGVAL(linewidth));
  setType(0);
}

WeatherFront::WeatherFront(int ty)
  : ObjectPlot(wFront)
  , linewidth(defaultLineWidth)
{
  METLIBS_LOG_DEBUG(LOGVAL(linewidth));
  setType(ty);
}

WeatherFront::WeatherFront(const std::string& tystring)
  : ObjectPlot(wFront)
  , linewidth(defaultLineWidth)
{
  METLIBS_LOG_DEBUG(LOGVAL(linewidth));
  // set correct fronttype
  if (!setType(tystring))
    METLIBS_LOG_ERROR("type '" << tystring << "' not found!");
}

WeatherFront::WeatherFront(const WeatherFront& o)
    : ObjectPlot(o)
    , linewidth(o.linewidth)
    , scaledlinewidth(o.scaledlinewidth)
    , npoints(o.npoints)
    , xplot(diutil::copy_array(o.xplot.get(), npoints))
    , yplot(diutil::copy_array(o.yplot.get(), npoints))
{
  METLIBS_LOG_SCOPE();
}

WeatherFront::~WeatherFront()
{
}

WeatherFront& WeatherFront::operator=(WeatherFront rhs)
{
  using std::swap;
  swap(*this, rhs);
  return *this;
}

void WeatherFront::swap(WeatherFront& o)
{
  ObjectPlot::swap(o);

  using std::swap;
  swap(linewidth, o.linewidth);
  swap(scaledlinewidth, o.scaledlinewidth);
  swap(npoints, o.npoints);
  swap(xplot, o.xplot);
  swap(yplot, o.yplot);
  swap(first, o.first);
}

void WeatherFront::defineFronts(std::vector<editToolInfo> fronts)
{
  allFronts = fronts;
  for (unsigned int i = 0;i<fronts.size();i++)
    frontTypes[fronts[i].name] = i;
}



void WeatherFront::recalculate()
{
  //METLIBS_LOG_DEBUG("WeatherFront::recalculate");
  // Makes smooth lines
  int div = DIV_SPLINE;  // div = subdivision points between to edge points

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
    x[length]=nodePoints[i].x();
    y[length]=nodePoints[i].y();
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
void WeatherFront::plot(DiGLPainter* gl, PlotOrder zorder)
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
      gl->Enable(DiGLPainter::gl_BLEND);
      gl->BlendFunc(DiGLPainter::gl_SRC_ALPHA, DiGLPainter::gl_ONE_MINUS_SRC_ALPHA);
      gl->setColour(objectColour);
      switch (drawIndex){
      case Cold:
        drawColds(gl);
        break;
      case Warm:
        drawWarms(gl);
        break;
      case Occluded:
        drawOccluded(gl);
        break;
      case Stationary:
        drawStationary(gl);
        break;
      case TroughLine:
        drawTroughLine(gl); // nothing to draw, only the line (below)
        break;
      case SquallLine:
        drawSquallLine(gl);
        break;
      case SigweatherFront:
        if (currentState == passive)
          drawSigweather(gl);
        break;
      case ArrowLine:
        drawArrowLine(gl);
        break;
      }

      //draw line
      if (currentState == active || drawIndex != SigweatherFront) {
        diutil::GlMatrixPushPop pushpop(gl);

        //draw the line, first set colour and linewidth
        gl->setLineStyle(objectColour, scaledlinewidth/2, itsLinetype);
        gl->Begin(DiGLPainter::gl_LINE_STRIP);
        if (spline) { // Draw smooth line
          for (int i=0; i<s_length; i++)
            gl->Vertex2f((x_s[i]),(y_s[i]));
        } else { // Draw unsmoothed line
          for (int i=0; i<end; i++)
            gl->Vertex2f(nodePoints[i].x(), nodePoints[i].y());
        }
        gl->End();
        pushpop.PopMatrix();
        if (rubber)
          plotRubber(gl);
      }

      gl->Disable(DiGLPainter::gl_LINE_STIPPLE);
      gl->Disable(DiGLPainter::gl_BLEND);

      drawNodePoints(gl);
    }
  }
}


int WeatherFront::hitPoint(float x,float y)
{
  //find out between which points we should split in two
  if (onLine(x,y)){
    return insert;
  }
  else return 0;
}


bool WeatherFront::addFront(ObjectPlot * qfront)
{
  //qfront is added to front
  float x,y,xnew,ynew;

  if (addTop){
    x=nodePoints.front().x();
    y=nodePoints.front().y();
  } else {
    x=nodePoints.back().x();
    y=nodePoints.back().y();
  }
  const int end = qfront->getXYZsize();
  const std::vector<XY> xyq = qfront->getXY();
  currentState = active;
  if (qfront->isBeginPoint(x,y,xnew,ynew)){
    movePoint(x,y,xnew,ynew);
    for (int i = 1;i<end;i++){
      addPoint(xyq[i].x(), xyq[i].y());
    }
  } else if (qfront->isEndPoint(x,y,xnew,ynew)){
    movePoint(x,y,xnew,ynew);
    for (int i = end-2;i>-1;i--){
      addPoint(xyq[i].x(), xyq[i].y());
    }
  } else {
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
    newFront->addPoint(nodePoints[i].x(), nodePoints[i].y());
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
void WeatherFront::drawColds(DiGLPainter* gl)
{
  // METLIBS_LOG_DEBUG("WeatherFront::drawColds");
  // colds are blue triangles on the front

  float r= scaledlinewidth*2*getDwidth();
  int end= s_length;
  int ncount=0;

  float xstart,ystart,xend,yend,xtop,ytop,dxs = 0.0,dys = 0.0,fraction;
  float s,slim,sprev = 0.0, x1,y1,s1,x2,y2,xm,ym,sm;
  int i,istart,j;

  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
  gl->LineWidth(1);

  slim= r*0.75;
  s= 0.;
  i=1;

  while (i<end) {

    while (s<slim && i<end) {
      sprev= s;
      dxs= x_s[i] - x_s[i-1];
      dys= y_s[i] - y_s[i-1];
      s += miutil::absval(dxs, dys);
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
      s = miutil::absval(dxs, dys);
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
      s1 = miutil::absval((x1 - xstart), (y1 - ystart));
      x2= x_s[i];
      y2= y_s[i];
      for (j=0; j<10; j++) {
        xm= (x1+x2)*0.5;
        ym= (y1+y2)*0.5;
        sm = miutil::absval((xm - xstart), (ym - ystart));
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
      gl->drawTriangle(true, QPointF(xstart,ystart), QPointF(xend,yend), QPointF(xtop,ytop));
    }
    ncount++;

    dxs= xend - x_s[i-1];
    dys= yend - y_s[i-1];
    slim = miutil::absval(dxs, dys) + r * 1.5;
    s= 0.;

  }

  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_LINE);
}

/*
   Draw arch on smooth line
 */
void WeatherFront::drawWarms(DiGLPainter* gl)
{
  //  METLIBS_LOG_DEBUG("WeatherFront::drawWarms");
  // warms are red arches on the front

  int ncount=0;

  const float r= scaledlinewidth*2*getDwidth();
  int end= s_length;

  float xstart,ystart,xend,yend,dxs = 0.0,dys = 0.0,fraction;
  float s,slim,sprev =0.0, x1,y1,s1,x2,y2,xm,ym,sm;
  int i,istart,j;

  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
  gl->LineWidth(1);

  slim= r*0.75;
  s= 0.;
  i=1;

  while (i<end) {

    while (s<slim && i<end) {
      sprev= s;
      dxs= x_s[i] - x_s[i-1];
      dys= y_s[i] - y_s[i-1];
      s += miutil::absval(dxs, dys);
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
      s = miutil::absval(dxs, dys);
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
      s1 = miutil::absval((x1 - xstart), (y1 - ystart));
      x2= x_s[i];
      y2= y_s[i];
      for (j=0; j<10; j++) {
        xm= (x1+x2)*0.5;
        ym= (y1+y2)*0.5;
        sm = miutil::absval((xm - xstart), (ym - ystart));
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

    if ((ncount%2) == 0)
      drawHalfCircle(gl, true, QPointF(xm, ym), atan2(dys,dxs)*RAD_TO_DEG, r);
    ncount++;

    dxs= xend - x_s[i-1];
    dys= yend - y_s[i-1];
    slim = miutil::absval(dxs, dys) + r * 1.5;
    s= 0.;
  }

  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_LINE);
}


/*
  Draws triangles and arch on smooth line
 */
void WeatherFront::drawOccluded(DiGLPainter* gl)
{
  const float r= scaledlinewidth*2*getDwidth();
  int end= s_length;

  float xstart,ystart,xend,yend,xtop,ytop,dxs = 0.0,dys = 0.0,fraction;
  float xstart1 = 0.0,ystart1 = 0.0,xend1 = 0.0,yend1 = 0.0;
  float s,slim,sprev = 0.0, x1,y1,s1,x2,y2,xm,ym,sm;
  int i,istart,j;
  int ndrawflag= 0;
  int ncount=0;

  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
  gl->LineWidth(1);

  slim= r*0.75;
  s= 0.;
  i=1;

  while (i<end) {

    while (s<slim && i<end) {
      sprev= s;
      dxs= x_s[i] - x_s[i-1];
      dys= y_s[i] - y_s[i-1];
      s += miutil::absval(dxs, dys);
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
      s = miutil::absval(dxs, dys);
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
      s1 = miutil::absval((x1 - xstart), (y1 - ystart));
      x2= x_s[i];
      y2= y_s[i];
      for (j=0; j<10; j++) {
        xm= (x1+x2)*0.5;
        ym= (y1+y2)*0.5;
        sm = miutil::absval((xm - xstart), (ym - ystart));
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
        drawHalfCircle(gl, true, QPointF(xm, ym), atan2(dys,dxs)*RAD_TO_DEG, r);

        dxs= xend - xstart;
        dys= yend - ystart;
        xtop= (xstart+xend)*0.5 - dys*0.6;
        ytop= (ystart+yend)*0.5 + dxs*0.6;

        gl->drawTriangle(true, QPointF(xstart,ystart), QPointF(xend,yend), QPointF(xtop,ytop));
      }
      ncount++;

      ndrawflag=0;
    }

    dxs= xend - x_s[i-1];
    dys= yend - y_s[i-1];
    slim = miutil::absval(dxs, dys);
    if (ndrawflag==0)
      slim+= r*1.5;
    s= 0.;

  }

  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_LINE);
}

/*
  Draws stationary front on smooth line
 */
void WeatherFront::drawStationary(DiGLPainter* gl)
{
  //  METLIBS_LOG_DEBUG("WeatherFront::drawStationary");
  // colds are blue triangles on the front

  const float r= scaledlinewidth*2*getDwidth();
  int end= s_length;

  float xstart,ystart,xend,yend,xtop,ytop,dxs = 0.0,dys = 0.0,fraction;
  float s,slim,sprev = 0.0, x1,y1,s1,x2,y2,xm,ym,sm;
  int i,istart,j;
  int ndrawflag= 0;
  int ncount=0;

  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
  gl->LineWidth(1);

  slim= r*0.5;
  s= 0.;
  i=1;

  while (i<end) {

    while (s<slim && i<end) {
      sprev= s;
      dxs= x_s[i] - x_s[i-1];
      dys= y_s[i] - y_s[i-1];
      s += miutil::absval(dxs, dys);
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
      s = miutil::absval(dxs, dys);
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
      s1 = miutil::absval((x1 - xstart), (y1 - ystart));
      x2= x_s[i];
      y2= y_s[i];
      for (j=0; j<10; j++) {
        xm= (x1+x2)*0.5;
        ym= (y1+y2)*0.5;
        sm = miutil::absval((xm - xstart), (ym - ystart));
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

        gl->setColour(Colour::RED);
        drawHalfCircle(gl, true, QPointF(xm, ym), atan2(dys,dxs)*RAD_TO_DEG, r);

      } else {

        dxs= xend - xstart;
        dys= yend - ystart;
        xtop= (xstart+xend)*0.5 + dys*0.6;
        ytop= (ystart+yend)*0.5 - dxs*0.6;

        gl->setColour(Colour::BLUE);
        gl->drawTriangle(true, QPointF(xstart, ystart), QPointF(xend,yend), QPointF(xtop,ytop));

        ndrawflag=0;
      }
    }
    ncount++;

    dxs= xend - x_s[i-1];
    dys= yend - y_s[i-1];
    slim = miutil::absval(dxs, dys) + r;
    s= 0.;
  }

  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_LINE);
}


/*
  Draws Squall line crosses on smooth line
 */
void WeatherFront::drawSquallLine(DiGLPainter* gl)
{
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

  gl->LineWidth(scaledlinewidth/2);

  while (i<end) {

    while (s<slim && i<end) {
      sprev= s;
      dxs= x_s[i] - x_s[i-1];
      dys= y_s[i] - y_s[i-1];
      s += miutil::absval(dxs, dys);
      i++;
    }
    if (s<slim) break;

    i--;
    fraction= (slim-sprev)/(s-sprev);
    xm= x_s[i-1] + dxs * fraction;
    ym= y_s[i-1] + dys * fraction;

    if (ncount%2==0){

      diutil::GlMatrixPushPop pushpop(gl);
      gl->Translatef(xm, ym, 0.0);
      gl->Rotatef(atan2(dys,dxs)*RAD_TO_DEG,0.0,0.0,1.0);
      gl->drawCross(0, 0, r, true);
    }
    ncount++;

    dxs= xm - x_s[i-1];
    dys= ym - y_s[i-1];
    slim = miutil::absval(dxs, dys) + r * 4.;
    s= 0.;
  }
}

/*
  Draws arrowline
 */
void WeatherFront::drawArrowLine(DiGLPainter* gl)
{
  float r= scaledlinewidth*2*getDwidth();
  int end= s_length;
  int ncount=0;

  float xstart,ystart,xend,yend,xtop1,ytop1,xtop2,ytop2,dxs,dys,fraction;
  float s,slim,sprev;
  int i;

  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);

  //slim= r*4;
  slim= r*2.75;
  s= dxs= dys= sprev= 0.;
  i=1;

    i=end-1;
    while (s<slim && i>0) {
      sprev= s;
      dxs= x_s[i] - x_s[i-1];
      dys= y_s[i] - y_s[i-1];
      s += miutil::absval(dxs, dys);
      // METLIBS_LOG_DEBUG("********** s1 =   " << s);
      i--;
      ncount++;
    }
    i++;
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
      s = miutil::absval(dxs, dys);
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

    if (ncount > 0) {
      gl->drawTriangle(true, QPointF(x_s[s_length-1], y_s[s_length-1]),
          QPointF(xtop1, ytop1), QPointF(xtop2,ytop2));
    }

  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_LINE);
}

/*
  Draws TroughLine
 */
void WeatherFront::drawTroughLine(DiGLPainter* gl)
{
  //  METLIBS_LOG_DEBUG("WeatherFront::drawTroughLine");
  float r= scaledlinewidth*2*getDwidth();
  int end= s_length;
  int ncount=0;

  float xstart,ystart,xend,yend,xtop1,ytop1,xtop2,ytop2,dxs,dys,fraction;
  float s,slim,sprev, x1,y1,s1,x2,y2,xm,ym,sm;
  int i,istart,j;

 // gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
  gl->LineWidth(scaledlinewidth);

  slim= r*0.75;
  s= sprev= dys= dxs= 0.;
  i=1;

  while (i<end) {

    while (s<slim && i<end) {
      sprev= s;
      dxs= x_s[i] - x_s[i-1];
      dys= y_s[i] - y_s[i-1];
      s += miutil::absval(dxs, dys);
      // METLIBS_LOG_DEBUG("********** s1 =   " << s);
      i++;
    }
    if (s<slim)
      break;
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
      s = miutil::absval(dxs, dys);
      i++;
    }
    if (s<slim)
      break;

    i--;
    if (istart==i-1) {
      fraction= slim/s;
      xend= xstart + dxs * fraction;
      yend= ystart + dys * fraction;
    } else {
      x1= x_s[i-1];
      y1= y_s[i-1];
      s1 = miutil::absval((x1 - xstart), (y1 - ystart));
      x2= x_s[i];
      y2= y_s[i];
      for (j=0; j<10; j++) {
        xm= (x1+x2)*0.5;
        ym= (y1+y2)*0.5;
        sm = miutil::absval((xm - xstart), (ym - ystart));
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
      if (ncount%4==0) {
        gl->drawLine(xend, yend, xtop1, ytop1);
      } else {
        gl->drawLine(xend, yend, xtop2, ytop2);
      }
    }
    ncount++;

    dxs= xend - x_s[i-1];
    dys= yend - y_s[i-1];
    slim = miutil::absval(dxs, dys) + r * 1.5;
    s= 0.;
  }
}

void WeatherFront::flip()
{
  int end = nodePoints.size();
  for (int j=0; j < end/2; j++){
    float x=nodePoints[j].x();
    float y=nodePoints[j].y();
    bool joined = nodePoints[j].joined();
    nodePoints[j].rx()=nodePoints[end-1-j].x();
    nodePoints[j].ry()=nodePoints[end-1-j].y();
    nodePoints[j].setJoined(nodePoints[end-1-j].joined());
    nodePoints[end-1-j].rx()=x;
    nodePoints[end-1-j].ry()=y;
    nodePoints[end-1-j].setJoined(joined);
  }
  recalculate();
}

void WeatherFront::setType(int ty)
{
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

bool WeatherFront::setType(const std::string& tystring)
{
  METLIBS_LOG_SCOPE(LOGVAL(tystring));
  if (frontTypes.find(tystring)!=frontTypes.end()) {
    type = frontTypes[tystring];
    setType(type);
    return true;
  }
  return false;
}


bool WeatherFront::setSpline(bool s)
{
  if (spline!=s) {
    spline=s;
    recalculate();
    return true;
  }
  return false;
}

std::string WeatherFront::writeTypeString()
{
  std::string ret ="Object=Front;\n";
  ret+="Type=";
  ret+=allFronts[type].name;
  ret+=";\n";
  ret+="Linewidth=";
  ret+=miutil::from_number(linewidth);
  ret+=";\n";
  return ret;
}



void WeatherFront::drawSigweather(DiGLPainter* gl)
{
  //METLIBS_LOG_DEBUG("WeatherFront::drawSigweather");
  recalculate();
  first=true;
  npoints=0;
  if (!smooth())
    return;
  delete[] x_s;
  delete[] y_s;
  x_s = y_s = 0;
  s_length= npoints;
  x_s=new float[s_length];
  y_s=new float[s_length];
  for (int i = 0; i < npoints; i++){
    x_s[i]=xplot[i];
    y_s[i]=yplot[i];
  }
  xplot.reset(nullptr);
  yplot.reset(nullptr);
  //smooth once more for better fit...
  first=false;
  if (!smooth())
    return;
  delete[] x_s;
  delete[] y_s;
  x_s = y_s = 0;
  gl->LineWidth(siglinewidth);
  for (int i = 0; i < npoints-1; i++){
    const float deltay = yplot[i+1]-yplot[i];
    const float deltax = xplot[i+1]-xplot[i];
    const float hyp = miutil::absval(deltay, deltax);
    const QPointF xxyy(xplot[i]+deltax/2, yplot[i]+deltay/2);
    drawHalfCircle(gl, false, xxyy, atan2(deltay,deltax)*RAD_TO_DEG, hyp / 2);
  }
  xplot.reset(nullptr);
  yplot.reset(nullptr);
  recalculate();
}


bool WeatherFront::smooth()
{
  //METLIBS_LOG_DEBUG("WeatherFront::smoth");
  //produces a curve with evenly spaced points
  float totalLength=0;
  for (int i = 0; i < s_length - 1; i++) {
    float deltay = y_s[i + 1] - y_s[i];
    float deltax = x_s[i + 1] - x_s[i];
    float hyp = miutil::absval(deltay, deltax);
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
  xplot.reset(new float[nplot]);
  yplot.reset(new float[nplot]);
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
      float hyp = miutil::absval(deltay, deltax);
      dist += hyp;
      //next step
      xstart = x_s[i];
      ystart = y_s[i];
    }
    float deltax, deltay, hyp;
    deltay = ystart - yplot[j - 1];
    deltax = xstart - xplot[j - 1];
    hyp = miutil::absval(deltay, deltax);
    float cosalfa = deltax / hyp;
    float sinalfa = deltay / hyp;
    xplot[j] = xplot[j - 1] + radius * cosalfa;
    yplot[j] = yplot[j - 1] + radius * sinalfa;
    deltay = yplot[j] - yplot[j - 1];
    deltax = xplot[j] - xplot[j - 1];
    hyp = miutil::absval(deltay, deltax);
    totalDist += hyp;
    i_s = i - 1;
  }
  npoints = j;
  xplot[npoints - 1] = x_s[s_length - 1];
  yplot[npoints - 1] = y_s[s_length - 1];
  return true;
}
