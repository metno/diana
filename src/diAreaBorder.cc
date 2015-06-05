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

#include "diAreaBorder.h"
#include "diGLPainter.h"

#include <sstream>

#define MILOGGER_CATEGORY "diana.AreaBorder"
#include <miLogger/miLogging.h>

using namespace::miutil;

AreaBorder::AreaBorder()
{
  typeOfObject = Border;
  linewidth=2;    // default linewidth of border
  transitionwidth = 8;// default transitionwidth
  type=7;         // default fronttype
}

AreaBorder::~AreaBorder()
{
}

void AreaBorder::plot(DiGLPainter* gl, PlotOrder porder)
{
  if (isVisible){
    int end = nodePoints.size();
    if (0 < end) {
      setWindowInfo();

      delete[] x;
      delete[] y;
      delete[] x_s;
      delete[] y_s;

      gl->PushMatrix();

      // spline
      int div= 5;
      int length=nodePoints.size();
      x= new float[length];
      y= new float[length];
      length = 0;

      for (unsigned int i=0; i<nodePoints.size(); i++){
        //skip points that are in the same position
        if (i!=0 && nodePoints[i] == nodePoints[i-1])
          continue;
        x[length] = nodePoints[i].x();
        y[length] = nodePoints[i].y();
        length++;
      }

      x_s= new float[(length-1)*div+length]; // x_s array of SMOOTH x coordinates
      y_s= new float[(length-1)*div+length];

      s_length=smoothline(length, &x[0], &y[0], 0, length-1,div, &x_s[0], &y_s[0]);


      gl->Color3f(0.0,0.0,0.0);
      gl->LineWidth(linewidth);

      //draw border
      gl->Begin(DiGLPainter::gl_LINE_STRIP);        // Draws the smooth line
      for (int i=0; i<s_length; i++)
        gl->Vertex2f(x_s[i],y_s[i]);
      gl->End();

      drawThickLine(gl);

      gl->PopMatrix();
      drawNodePoints(gl);
    }
  }
}


/**
 * Algorithm for marking points on line
 * Idea is that if mousepointer is moved over a line
 * all nodepoints shines up / are marked
 * If on nodepoint only this point is marked
 * Checks this only if current status is passive.
 */
bool AreaBorder::showLine(float x, float y){
  if (boundBox.isinside(x,y))
    inBoundBox=true;
  else
    inBoundBox=false;
  if (isInside(x,y)){
    markPoint(x,y); // checks wheather on nodepoint and marks
    return true;
  }
  else if (onLine(x,y)){ // else checkes whether on line
    markAllPoints();
    return true;
  }
  else {
    return false;
  }
  return false;
}


void AreaBorder::increaseSize(float val)
{
  setTransitionWidth(transitionwidth+val/2);
}


std::string AreaBorder::writeTypeString()
{
  std::string ret ="Object=Border;\n";
  ret+="Type=AreaBorder";
  ret+=";\n";
  return ret;
}


void AreaBorder::drawThickLine(DiGLPainter* gl)
{
  int p_length= s_length - 1;
  int i;

  float *x1= new float[p_length];
  float *y1= new float[p_length];
  float *x2= new float[p_length];
  float *y2= new float[p_length];
  float *x3= new float[p_length];
  float *y3= new float[p_length];
  float *x4= new float[p_length];
  float *y4= new float[p_length];
  float dxp,dyp, dx,dy,ds;

  float w= transitionwidth * 0.5 * scaleToField;

  for (i=0; i<p_length; i++) {
    dx= x_s[i+1]-x_s[i];
    dy= y_s[i+1]-y_s[i];
    ds= sqrtf(dx*dx+dy*dy);
    dxp= -dy * w / ds;
    dyp=  dx * w / ds;
    x1[i]= x_s[i] + dxp;
    y1[i]= y_s[i] + dyp;
    x2[i]= x_s[i] - dxp;
    y2[i]= y_s[i] - dyp;
    x3[i]= x_s[i+1] + dxp;
    y3[i]= y_s[i+1] + dyp;
    x4[i]= x_s[i+1] - dxp;
    y4[i]= y_s[i+1] - dyp;
  }

  // a very quick solution, not correct!
  for (i=1; i<p_length; i++) {
    x3[i-1]= x1[i]= (x3[i-1]+x1[i])*0.5;
    y3[i-1]= y1[i]= (y3[i-1]+y1[i])*0.5;
    x4[i-1]= x2[i]= (x4[i-1]+x2[i])*0.5;
    y4[i-1]= y2[i]= (y4[i-1]+y2[i])*0.5;
  }

  const DiGLPainter::GLubyte borderpattern[] = {
      0x88, 0x88, 0x88, 0x88, 0x00, 0x00, 0x00, 0x00,
      0x22, 0x22, 0x22, 0x22, 0x00, 0x00, 0x00, 0x00,
      0x88, 0x88, 0x88, 0x88, 0x00, 0x00, 0x00, 0x00,
      0x22, 0x22, 0x22, 0x22, 0x00, 0x00, 0x00, 0x00,
      0x88, 0x88, 0x88, 0x88, 0x00, 0x00, 0x00, 0x00,
      0x22, 0x22, 0x22, 0x22, 0x00, 0x00, 0x00, 0x00,
      0x88, 0x88, 0x88, 0x88, 0x00, 0x00, 0x00, 0x00,
      0x22, 0x22, 0x22, 0x22, 0x00, 0x00, 0x00, 0x00,
      0x88, 0x88, 0x88, 0x88, 0x00, 0x00, 0x00, 0x00,
      0x22, 0x22, 0x22, 0x22, 0x00, 0x00, 0x00, 0x00,
      0x88, 0x88, 0x88, 0x88, 0x00, 0x00, 0x00, 0x00,
      0x22, 0x22, 0x22, 0x22, 0x00, 0x00, 0x00, 0x00,
      0x88, 0x88, 0x88, 0x88, 0x00, 0x00, 0x00, 0x00,
      0x22, 0x22, 0x22, 0x22, 0x00, 0x00, 0x00, 0x00,
      0x88, 0x88, 0x88, 0x88, 0x00, 0x00, 0x00, 0x00,
      0x22, 0x22, 0x22, 0x22, 0x00, 0x00, 0x00, 0x00};

  gl->ShadeModel(DiGLPainter::gl_FLAT);
  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK,DiGLPainter::gl_FILL);
  gl->Enable(DiGLPainter::gl_POLYGON_STIPPLE);
  gl->PolygonStipple (borderpattern);
  gl->Begin(DiGLPainter::gl_QUAD_STRIP);
  for (i=0; i<p_length; i++) {
    gl->Vertex2f(x1[i],y1[i]);
    gl->Vertex2f(x2[i],y2[i]);
  }
  i= p_length - 1;
  gl->Vertex2f(x3[i],y3[i]);
  gl->Vertex2f(x4[i],y4[i]);
  gl->End();
  gl->Disable(DiGLPainter::gl_POLYGON_STIPPLE);

  delete[] x1;
  delete[] y1;
  delete[] x2;
  delete[] y2;
  delete[] x3;
  delete[] y3;
  delete[] x4;
  delete[] y4;
}
