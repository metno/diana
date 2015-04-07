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

#include <diSpectrumPlot.h>
#include <diSpectrumOptions.h>
#include <diFontManager.h>
#include <diPlotOptions.h>
#include <diColour.h>
#include <diContouring.h>

#include <puTools/miStringFunctions.h>

#include <qglobal.h>

#include <iomanip>
#include <sstream>

#include <GL/gl.h>
#if !defined(USE_PAINTGL)
#include <glp/glpfile.h>
#endif

#define MILOGGER_CATEGORY "diana.SpectrumPlot"
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;

// static
FontManager*  SpectrumPlot::fp= 0; // fontpack

GLPfile* SpectrumPlot::psoutput= 0; // PostScript module
bool     SpectrumPlot::hardcopy= false; // producing postscript

int SpectrumPlot::plotw= 100;
int SpectrumPlot::ploth= 100;
int SpectrumPlot::numplot= 0;
int SpectrumPlot::nplot=   0;

bool SpectrumPlot::firstPlot= true;

float  SpectrumPlot::freqMax= 5.0f;
float  SpectrumPlot::specRadius= 0.0f;
float  SpectrumPlot::dytext1= 0.0f;
float  SpectrumPlot::dytext2= 0.0f;
float  SpectrumPlot::xplot1= 0.0f;
float  SpectrumPlot::xplot2= 0.0f;
float  SpectrumPlot::yplot1= 0.0f;
float  SpectrumPlot::yplot2= 0.0f;
float  SpectrumPlot::rwind= 0.0f;
float  SpectrumPlot::xwind= 0.0f;
float  SpectrumPlot::ywind= 0.0f;
float  SpectrumPlot::xefdiag= 0.0f;
float  SpectrumPlot::yefdiag= 0.0f;
float  SpectrumPlot::dxefdiag= 0.0f;
float  SpectrumPlot::dyefdiag= 0.0f;
float  SpectrumPlot::vxefdiag= 0.0f;
float  SpectrumPlot::vyefdiag= 0.0f;
int    SpectrumPlot::numyefdiag= 0;
int    SpectrumPlot::incyefdiag= 0;
float  SpectrumPlot::xhmo= 0.0f;
float  SpectrumPlot::yhmo= 0.0f;
float  SpectrumPlot::dxhmo= 0.0f;
float  SpectrumPlot::dyhmo= 0.0f;
float  SpectrumPlot::vyhmo= 0.0f;
float  SpectrumPlot::fontSize1= 0.0f;
float  SpectrumPlot::fontSize2= 0.0f;
float  SpectrumPlot::fontSizeLabel= 0.0f;
float* SpectrumPlot::xcircle= 0;
float* SpectrumPlot::ycircle= 0;

float  SpectrumPlot::eTotalMax= 0.0;


// Default constructor
SpectrumPlot::SpectrumPlot()
	: numDirec(0), numFreq(0), sdata(0), xdata(0), ydata(0)
{

  METLIBS_LOG_DEBUG("++ SpectrumPlot::Default Constructor");


  if (!fp) {
    fp= new FontManager();
    fp->parseSetup();
    fp->setFont("BITMAPFONT");
    fp->setFontFace(glText::F_NORMAL);
    fp->setScalingType(glText::S_FIXEDSIZE);
  }

}


// Destructor
SpectrumPlot::~SpectrumPlot() {

  METLIBS_LOG_DEBUG("++ SpectrumPlot::Destructor");


  delete[] sdata;
  delete[] xdata;
  delete[] ydata;
}


// static
bool SpectrumPlot::startPSoutput(const printOptions& po){
  if (hardcopy) return false;
  printOptions pro= po;
  printerManager printman;

  int feedsize= 10000000;
  int print_options= 0;

  // Fit output to page
  if (pro.fittopage)
    print_options= print_options | GLP_FIT_TO_PAGE;

  // set colour mode
  if (pro.colop==d_print::greyscale){
    print_options= print_options | GLP_GREYSCALE;
    // calculate background colour intensity
//     float bci=
//       backgroundColour.fB()*0.0820 +
//       backgroundColour.fG()*0.6094 +
//       backgroundColour.fR()*0.3086;
//     if (bci < 0.2)
//       print_options= print_options | GLP_REVERSE;

    if (pro.drawbackground)
      print_options= print_options | GLP_DRAW_BACKGROUND;

  } else if (pro.colop==d_print::blackwhite) {
    print_options= print_options | GLP_BLACKWHITE;

  } else {
    if (pro.drawbackground)
      print_options= print_options | GLP_DRAW_BACKGROUND;
  }

  // set orientation
  if (pro.orientation==d_print::ori_landscape)
    print_options= print_options | GLP_LANDSCAPE;
  else if (pro.orientation==d_print::ori_portrait)
    print_options= print_options | GLP_PORTRAIT;
  else
    print_options= print_options | GLP_AUTO_ORIENT;

  // calculate line, point (and font?) scale
  if (!pro.usecustomsize)
    pro.papersize= printman.getSize(pro.pagesize);
  d_print::PaperSize a4size;
  float scale= 1.0;
  if (abs(pro.papersize.vsize)>0)
    scale= a4size.vsize/pro.papersize.vsize;

  // check if extra output-commands
  map<string,string> extra;
  printman.checkSpecial(pro,extra);

  // make GLPfile object
  psoutput = new GLPfile(const_cast<char*>(pro.fname.c_str()),
			 print_options, feedsize, &extra,
			 pro.doEPS);

  // set line and point scale
  psoutput->setScales(0.5*scale, 0.5*scale);

  psoutput->StartPage();
  // set viewport
  psoutput->setViewport(pro.viewport_x0, pro.viewport_y0,
			pro.viewport_width, pro.viewport_height);

  hardcopy= true;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  if (!fp) {
    fp= new FontManager();
    fp->parseSetup();
    fp->setFont("BITMAPFONT");
    fp->setFontFace(glText::F_NORMAL);
    fp->setScalingType(glText::S_FIXEDSIZE);
  }
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  // inform fontpack
  if (fp){
    fp->startHardcopy(psoutput);
  }

  return true;
}


// static
void SpectrumPlot::addHCStencil(const int& size,
			        const float* x, const float* y)
{
  if (!psoutput)
    return;
  psoutput->addStencil(size,x,y);
}


// static
void SpectrumPlot::addHCScissor(const int x0, const int y0,
			        const int  w, const int  h)
{
  if (!psoutput)
    return;
  psoutput->addScissor(x0,y0,w,h);
}


// static
void SpectrumPlot::removeHCClipping()
{
  if (!psoutput)
    return;
  psoutput->removeClipping();
}


// static
void SpectrumPlot::UpdateOutput(){
  // for postscript output
  if (psoutput) psoutput->UpdatePage(true);
}

// static
void SpectrumPlot::resetPage(){
  // for postscript output
  if (psoutput) psoutput->addReset();
}


// static
bool SpectrumPlot::startPSnewpage()
{
  if (!hardcopy || !psoutput) return false;
  glFlush();
  if (psoutput->EndPage() != 0) {
    METLIBS_LOG_ERROR("startPSnewpage: EndPage BAD!!!");
  }
  psoutput->StartPage();
  return true;
}


// static
bool SpectrumPlot::endPSoutput(){
  if (!hardcopy || !psoutput) return false;
  glFlush();
  if (psoutput->EndPage() == 0) {
    delete psoutput;
    psoutput = 0;
  }
  hardcopy= false;

  if (fp) fp->endHardcopy();

  return true;
}


// static
void SpectrumPlot::startPlot(int numplots, int w, int h,
			     SpectrumOptions *spopt)
{

  METLIBS_LOG_DEBUG("SpectrumPlot::startPlot numplots=" << numplots);


  numplot= numplots;
  nplot= 0;
  plotw= w;
  ploth= h;

  Colour cback(spopt->backgroundColour);

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  glClearColor(cback.fR(),cback.fG(),cback.fB(),1.0);
  glClear( GL_COLOR_BUFFER_BIT );

  //----------------------

//freqMax= 0.50;
  freqMax= spopt->freqMax;

  // the spectrum origin is at x=y=0

  // numbers suitable for freqMax=0.50, scale sizes
  float s= freqMax/0.50;

  specRadius= freqMax;
  dytext1= 0.08*s;
  dytext2= 0.12*s;
  xplot1= -specRadius-0.40*s;
  xplot2=  specRadius+0.25*s;
  yplot1= -specRadius-dytext1*5.0;
  yplot2=  specRadius+dytext1*1.5+dytext2;
  rwind= 0.18*s;
  xwind= xplot1+0.25*s;
  ywind= 0.35*s;
  xefdiag= xplot1+0.15*s;
  yefdiag= yplot1+dytext1*3.25;
  dxefdiag= 0.10*s - xefdiag;
  dyefdiag= specRadius+0.20*s;
//vxefdiag= freqMax;         // max frequency on the graph
  vxefdiag= 0.5;             // max frequency on the graph
  vyefdiag= 7.0;             // default max energy on the graph
  numyefdiag= 7;
  incyefdiag= 1;
  xhmo= xplot2-0.12*s;
  yhmo= yplot1+dytext1*3.0;
  dxhmo= 0.08*s;
  dyhmo= specRadius*2.0;
  vyhmo= 15.0;               // max waveheight

  if(eTotalMax>vyefdiag) {
    while (eTotalMax>float(numyefdiag*incyefdiag)) incyefdiag++;
    vyefdiag=float(numyefdiag*incyefdiag);
  }

  if (!xcircle || !ycircle) {
    xcircle= new float[120];
    ycircle= new float[120];
    for (int i=0; i<120; i++) {
      xcircle[i]= cos(float(i*3)*DEG_TO_RAD);
      ycircle[i]= sin(float(i*3)*DEG_TO_RAD);
    }
  }

  float x1= xplot1 - 0.01*s;
  float x2= xplot2 + 0.01*s;
  float y1= yplot1 - 0.01*s;
  float y2= yplot2 + 0.01*s;

  float dx= x2 - x1;
  float dy= y2 - y1;
  float pw= plotw;
  float ph= ploth;
  if (pw/ph>dx/dy) {
    dx= (dy*pw/ph - dx) * 0.5;
    x1-=dx;
    x2+=dx;
  } else {
    dy= (dx*ph/pw - dy) * 0.5;
    y1-=dy;
    y2+=dy;
  }

  //----------------------

  glLoadIdentity();
  glOrtho(x1,x2,y1,y2,-1.,1.);


  if (!fp) {
    fp= new FontManager();
    fp->parseSetup();
    fp->setFont("BITMAPFONT");
    fp->setFontFace(glText::F_NORMAL);
    fp->setScalingType(glText::S_FIXEDSIZE);
  }

  if (numplot>0 || !firstPlot) {
    fp->setVpSize(pw,ph);
    fp->setGlSize(x2-x1,y2-y1);

    float chx,chy,fontsize=8.;
    fp->setFontSize(fontsize);
    fp->getCharSize('0',chx,chy);

    fontSize1= fontsize*(dytext1*0.5/chy);
    fontSize2= fontsize*(dytext2*0.5/chy);
    fontSizeLabel= fontSize1*0.5;
    if (fontSizeLabel>14.) fontSizeLabel= 14.;

    firstPlot= false;
  }

}


//static
void SpectrumPlot::plotDiagram(SpectrumOptions *spopt)
{

  METLIBS_LOG_DEBUG("++ SpectrumPlot::plotDiagram");

  int n;
  float v,x,y,dx,dy;

  glDisable(GL_LINE_STIPPLE);

  Colour c= Colour(spopt->frameColour);
  glColor3ubv(c.RGB());
  glLineWidth(spopt->frameLinewidth);

  // frame
  if (spopt->pFrame) {
    glBegin(GL_LINE_LOOP);
    glVertex2f(xplot1,yplot1);
    glVertex2f(xplot2,yplot1);
    glVertex2f(xplot2,yplot2);
    glVertex2f(xplot1,yplot2);
    glEnd();
  }

  glBegin(GL_LINES);

  // mark wind position/directions
  if (spopt->pWind || spopt->pPeakDirection) {
    float dw= rwind*0.25;
    glVertex2f(xwind-dw,ywind);
    glVertex2f(xwind+dw,ywind);
    glVertex2f(xwind,ywind-dw);
    glVertex2f(xwind,ywind+dw);
  }

  // the total energy graph, axes
  glVertex2f(xefdiag,yefdiag);
  glVertex2f(xefdiag+dxefdiag,yefdiag);
  glVertex2f(xefdiag,yefdiag);
  glVertex2f(xefdiag,yefdiag+dyefdiag);
  // arrows on the axes
  float dw= dytext1*0.5;
  glVertex2f(xefdiag+dxefdiag,yefdiag);
  glVertex2f(xefdiag+dxefdiag-dw,yefdiag+dw*0.667);
  glVertex2f(xefdiag+dxefdiag,yefdiag);
  glVertex2f(xefdiag+dxefdiag-dw,yefdiag-dw*0.667);
  glVertex2f(xefdiag,yefdiag+dyefdiag);
  glVertex2f(xefdiag-dw*0.667,yefdiag+dyefdiag-dw);
  glVertex2f(xefdiag,yefdiag+dyefdiag);
  glVertex2f(xefdiag+dw*0.667,yefdiag+dyefdiag-dw);
  // tics on the axes (labels later)
  for (int i=0; i<numyefdiag; i++) {
    v=float(i*incyefdiag);
    y=yefdiag+v*dyefdiag/vyefdiag;
    glVertex2f(xefdiag-dytext1*0.3,y);
    glVertex2f(xefdiag,y);
  }
  n= int(vxefdiag*10.-0.5);
  for (int i=0; i<=n; i++) {
    v=float(i)*0.1;
    x=xefdiag+v*dxefdiag/vxefdiag;
    glVertex2f(x,yefdiag);
    glVertex2f(x,yefdiag-dytext1*0.3);
  }

  // mark text positions
  if (spopt->pFrame) {
    glVertex2f(xplot1,yplot2-dytext2);
    glVertex2f(xplot2,yplot2-dytext2);
    glVertex2f(xplot1,yplot1+dytext1);
    glVertex2f(xplot2,yplot1+dytext1);
    glVertex2f(xplot1,yplot1+dytext1*2.0f);
    glVertex2f(xplot2,yplot1+dytext1*2.0f);
  }

  // hmo ticks (labels later)
  n= int(vyhmo+0.1);
  for (int i=5; i<=n; i+=5) {
    v=float(i);
    y=yhmo+v*dyhmo/vyhmo;
    glVertex2f(xhmo-dytext1*0.1,y);
    glVertex2f(xhmo+dxhmo,y);
  }

  glEnd();

  // hmo
  glBegin(GL_LINE_LOOP);
  glVertex2f(xhmo,yhmo);
  glVertex2f(xhmo+dxhmo,yhmo);
  glVertex2f(xhmo+dxhmo,yhmo+dyhmo);
  glVertex2f(xhmo,yhmo+dyhmo);
  glEnd();

  float freqStep= 0.05;
  float freq= freqStep;
  while (freq<freqMax+0.1*freqStep) {
    glBegin(GL_LINE_LOOP);
    for (int i=0; i<120; i++)
      glVertex2f(freq*xcircle[i],freq*ycircle[i]);
    glEnd();
    freq+=freqStep;
  }

  glBegin(GL_LINES);
  for (int i=0; i<120; i+=10) {
    glVertex2f(0.0f,0.0f);
    glVertex2f(specRadius*xcircle[i],specRadius*ycircle[i]);
  }
  glEnd();

  if (fontSize1>0.0 && spopt->pFixedText) {

    Colour tc= Colour(spopt->fixedTextColour);
    glColor3ubv(tc.RGB());

    float chx,chy,chx2,chy2;

    fp->setFontSize(fontSize1);
    fp->getCharSize('0',chx,chy);
    fp->getStringSize("0.0",chx2,chy2);

    // the real size (x ok)
    chy*=0.75;
    chy2*=0.75;

    std::string str;

    str="F(f,dir) [m2/Hz/rad]";
    fp->getStringSize(str.c_str(),dx,dy);
    x=(xplot1+xplot2-dx)*0.5;
    y= yplot2-dytext2-dytext1+chy*0.25;
    fp->drawStr(str.c_str(),x,y,0.0);

    str= "0.05 Hz/circle";
    x= specRadius*1.05f*cos(60.0f*DEG_TO_RAD);
    y= specRadius*1.05f*sin(60.0f*DEG_TO_RAD);
    fp->drawStr(str.c_str(),x,y,0.0);

    str= "F(f) [m2/Hz]";
    x= xplot1 + dytext1*0.25;
    y= yefdiag + dyefdiag + chy*0.35;
    fp->drawStr(str.c_str(),x,y,0.0);
    // tick labels
    for (int i=0; i<numyefdiag; i++) {
      int ivalue= i*incyefdiag;
      v=float(ivalue);
      y=yefdiag+v*dyefdiag/vyefdiag;
      str=miutil::from_number(ivalue);
      float s= (ivalue<10) ? 1.2 : 2.2;
      fp->drawStr(str.c_str(),xefdiag-dytext1*0.3-chx*s,y-chy*0.5,0.0);
    }

    str= "freq.[Hz]";
    x= xefdiag + dxefdiag + chy*0.5;
    y= yefdiag - chy*0.5;
    fp->drawStr(str.c_str(),x,y,0.0);
    // tick labels
    n= int(vxefdiag*10.-0.5);
    for (int i=0; i<=n; i++) {
      v=float(i)*0.1;
      x=xefdiag+v*dxefdiag/vxefdiag;
      if (i==0) str="0.0";
      else      str=miutil::from_number(v);
      fp->drawStr(str.c_str(),x-chx2*0.5,yefdiag-dytext1*0.3-chy*1.1,0.0);
    }

    str= "HMO[m]";
    fp->getStringSize(str.c_str(),dx,dy);
    x= xplot2 - dx - dytext1*0.25;
    y= yhmo - dy;
    fp->drawStr(str.c_str(),x,y,0.0);
    // hmo tick labels
    n= int(vyhmo+0.1);
    for (int i=5; i<=n; i+=5) {
      v=float(i);
      y=yhmo+v*dyhmo/vyhmo;
      str=miutil::from_number(i);
      float s= (i<10) ? 1.0f : 2.0f;
      fp->drawStr(str.c_str(),xhmo-dytext1*0.1-chx*s,y-chy*0.5,0.0);
    }

  }

  UpdateOutput();
}


bool SpectrumPlot::plot(SpectrumOptions *spopt)
{
  METLIBS_LOG_SCOPE(posName << " " << validTime);

  Colour c;

  if (posName.empty() || !sdata) return false;

  int nfreq=1;
  while (nfreq<numFreq-1 && frequences[nfreq]<freqMax) nfreq++;

  if (spopt->pSpectrumLines || spopt->pSpectrumColoured) {

    int ibmap;
    int nxbmap,nybmap,lbmap;
    float rbmap[4];
    int *bmap= 0;
    ibmap=0;
    lbmap=0;
    nxbmap=0;
    nybmap=0;
    rbmap[0]=0.;
    rbmap[1]=0.;
    rbmap[2]=0.;
    rbmap[3]=0.;

    int labfmt[3]= { 0, 0, 0 };

    int iconv= 2;
    float conv[6];
    int idraw;
    float zrange[2];
    //int nlines= 0;
    //float rlines[100];
    int ncol= 1;
    int icol[100];
    icol[0]= -1;
    int ntyp= 1;
    int ityp[100];
    ityp[0]= -1;
    int nwid= 1;
    int iwid[100];
    iwid[0]= -1;
    int nlim= 0;
    float rlim[2];
    int idraw2= 0;
    float zrange2[2]= { +1., -1. };
    float zstep2= 0.;
    float zoff2= 0.;
    int nlines2= 0;
    float rlines2[2];
    int ncol2= 1;
    int icol2[2]= { -1, -1 };
    int ntyp2= 1;
    int ityp2[2]= { -1, -1 };
    int nwid2= 1;
    int iwid2[2]= { -1, -1 };
    int nlim2= 0;
    float rlim2[2];
    int ismooth= 0;
    int ibcol= -1;

    float zstep= 0.3;
    float zoff=  0.0f;

    const int nlines=21;
    float rlines[nlines]= {0.1,0.2,0.4,0.6,0.8,1.,2.,4.,6.,8.,
    			   10.,20.,40.,60.,80.,100.,200.,400.,600.,800.,1000.};
    idraw = 3;

    zrange[0]= +1.;
    zrange[1]= -1.;

    ismooth = 0;
    if (ismooth<0) ismooth=0;

    PlotOptions poptions;
    float chxlab=10.;
    float chylab=14.;

    int   part[4]= { 0, numDirec, 0, nfreq };
    float xylim[4];
    xylim[0]= xplot1;
    xylim[1]= xplot2;
    xylim[2]= yplot1;
    xylim[3]= yplot2;

    Area dummyArea;

    if (spopt->pSpectrumColoured) {

//+++ poptions.contourShading= 1;
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      std::string optstr ="palettecolours=standard repeat=1";
      PlotOptions::parsePlotOption(optstr,poptions);
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      c= Colour("black");
      glColor3ubv(c.RGB());
      glLineWidth(1.0f);

      bool res=
           contour(numDirec+1,numFreq,sdata,xdata,ydata,
                   part,iconv,conv,xylim,
                   idraw,zrange,zstep,zoff,nlines,rlines,
     		   ncol,icol,ntyp,ityp,nwid,iwid,nlim,rlim,
                   idraw2,zrange2,zstep2,zoff2,nlines2,rlines2,
     		   ncol2,icol2,ntyp2,ityp2,nwid2,iwid2,nlim2,rlim2,
                   ismooth,labfmt,chxlab,chylab,ibcol,
                   ibmap,lbmap,bmap,nxbmap,nybmap,rbmap,
                   fp, poptions, psoutput, dummyArea, fieldUndef);

      if (!res) METLIBS_LOG_ERROR("SpectrumPlot::plot  Contour error");

      UpdateOutput();
    }

    if (spopt->pSpectrumLines) {

      poptions.contourShading= 0;

      fp->setFont("SCALEFONT");
      fp->setFontSize(fontSizeLabel);
      fp->getCharSize('0',chxlab,chylab);
      // the real height (x ok)
      chylab*=0.75;
      labfmt[0]= -1;

      Colour c= Colour(spopt->spectrumLineColour);
      glColor3ubv(c.RGB());
      glLineWidth(spopt->spectrumLinewidth);
      bool res=
           contour(numDirec+1,numFreq,sdata,xdata,ydata,
                   part,iconv,conv,xylim,
                   idraw,zrange,zstep,zoff,nlines,rlines,
     		   ncol,icol,ntyp,ityp,nwid,iwid,nlim,rlim,
                   idraw2,zrange2,zstep2,zoff2,nlines2,rlines2,
     		   ncol2,icol2,ntyp2,ityp2,nwid2,iwid2,nlim2,rlim2,
                   ismooth,labfmt,chxlab,chylab,ibcol,
                   ibmap,lbmap,bmap,nxbmap,nybmap,rbmap,
                   fp, poptions, psoutput, dummyArea, fieldUndef);

      if (!res) METLIBS_LOG_ERROR("SpectrumPlot::plot  Contour error");

      UpdateOutput();
    }
  }

  glDisable(GL_LINE_STIPPLE);

//###########################################################################
  bool showGrid= false;

  if (showGrid) {
    c= Colour("red");
    glColor3ubv(c.RGB());
    glLineWidth(1.0f);
    for (int j=0; j<numFreq; j++) {
      glBegin(GL_LINE_STRIP);
      for (int i=0; i<=numDirec; i++)
        glVertex2f(xdata[j*numFreq+i],ydata[j*numFreq+i]);
      glEnd();
    }
    for (int i=0; i<numDirec; i++) {
      if (i==0) glLineWidth(3.0f);
      glBegin(GL_LINE_STRIP);
      for (int j=0; j<numFreq; j++)
        glVertex2f(xdata[j*numFreq+i],ydata[j*numFreq+i]);
      glEnd();
      if (i==0) glLineWidth(1.0f);
    }
  }
//###########################################################################

  if (spopt->pPeakDirection && ddPeak > -1) {
    c= Colour(spopt->peakDirectionColour);
    glColor3ubv(c.RGB());
    glLineWidth(spopt->peakDirectionLinewidth);
    float dx= rwind * cos((270.-ddPeak)*DEG_TO_RAD);
    float dy= rwind * sin((270.-ddPeak)*DEG_TO_RAD);
    float xend= xwind + dx;
    float yend= ywind + dy;
    dx*=0.15;
    dy*=0.15;
    glBegin(GL_LINES);
    glVertex2f(xwind,ywind);
    glVertex2f(xend,yend);
    glVertex2f(xend,yend);
    glVertex2f(xend-dx-dy*0.667,yend-dy+dx*0.667);
    glVertex2f(xend,yend);
    glVertex2f(xend-dx+dy*0.667,yend-dy-dx*0.667);
    glEnd();
  }

  // wind .....................................................
  if (spopt->pWind && wspeed>0.001){

    float dd= wdir;
    float ff= wspeed * 3600.0/1852.0;

    c= Colour(spopt->windColour);
    glColor3ubv(c.RGB());
    glLineWidth(spopt->windLinewidth);

    glBegin(GL_LINES);

    int   n,n50,n10,n05;
    float gu,gv,gx,gy,dx,dy,dxf,dyf,gx50=0,gy50=0;
    float flagl = rwind*0.85;
    float flagstep = flagl/10.;
    float flagw = flagl * 0.35;
    float hflagw = 0.6;

    // find no. of 50,10 and 5 knot flags
    if (ff<182.49) {
      n05  = int(ff*0.2 + 0.5);
      n50  = n05/10;
      n05 -= n50*10;
      n10  = n05/2;
      n05 -= n10*2;
    } else if (ff<190.) {
      n50 = 3;  n10 = 3;  n05 = 0;
    } else if(ff<205.) {
      n50 = 4;  n10 = 0;  n05 = 0;
    } else if (ff<225.) {
      n50 = 4;  n10 = 1;  n05 = 0;
    } else {
      n50 = 5;  n10 = 0;  n05 = 0;
    }

    gx= xwind;
    gy= ywind;
    gu= sin(dd*DEG_TO_RAD);
    gv= cos(dd*DEG_TO_RAD);

    dx = flagstep*gu;
    dy = flagstep*gv;
    dxf = -flagw*gv - dx;
    dyf =  flagw*gu - dy;

    // direction
    glVertex2f(gx,gy);
    gx = gx - flagl*gu;
    gy = gy - flagl*gv;
    glVertex2f(gx,gy);

    // 50-knot flags, store for plot below
    if (n50>0) {
      gx50=gx;
      gy50=gy;
      gx+=dx*2.*float(n50);
      gy+=dy*2.*float(n50);
      gx+=dx; gy+=dy;
    }
    // 10-knot flags
    for (n=0; n<n10; n++) {
      glVertex2f(gx,gy);
      glVertex2f(gx+dxf,gy+dyf);
      gx+=dx; gy+=dy;
    }
    // 5-knot flag
    if (n05>0) {
      if (n50+n10==0) { gx+=dx; gy+=dy; }
      glVertex2f(gx,gy);
      glVertex2f(gx+hflagw*dxf,gy+hflagw*dyf);
    }
    glEnd();

    UpdateOutput();

    // draw 50-knot flags
    if (n50>0) {
      glShadeModel(GL_FLAT);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glBegin(GL_TRIANGLES);
      gx= gx50;
      gy= gy50;
      for (n=0; n<n50; n++) {
        glVertex2f(gx,gy);
	gx+=dx*2.;  gy+=dy*2.;
        glVertex2f(gx+dxf,gy+dyf);
        glVertex2f(gx,gy);
      }
      glEnd();
      UpdateOutput();
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
  }
  //...........................................................

  fp->setFont("BITMAPFONT");

  float x,y,dx,dy;

  // total energy per frequency graph
  if (spopt->pEnergyColoured) {
    c= Colour(spopt->energyFillColour);
    glColor3ubv(c.RGB());
    glLineWidth(1.0f);
    glShadeModel(GL_FLAT);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glBegin(GL_QUAD_STRIP);
    for (int i=0; i<numFreq; i++) {
      if ( frequences[i] > 0.5 )
         break;
      x= xefdiag + dxefdiag*frequences[i]/vxefdiag;
      y= yefdiag + dyefdiag*eTotal[i]/vyefdiag;
      glVertex2f(x,y);
      glVertex2f(x,yefdiag);
    }
    glEnd();
  }

  if (spopt->pEnergyLine) {
    c= Colour(spopt->energyLineColour);
    glColor3ubv(c.RGB());
    glLineWidth(spopt->energyLinewidth);
    glBegin(GL_LINE_STRIP);
    for (int i=0; i<numFreq; i++) {
      if ( frequences[i] > 0.5 )
         break;
      x= xefdiag + dxefdiag*frequences[i]/vxefdiag;
      y= yefdiag + dyefdiag*eTotal[i]/vyefdiag;
      glVertex2f(x,y);
    }
    glEnd();
  }

  // wave height (HMO)
  if ( hmo > -1 ) {
    c= Colour(spopt->textColour);
    glColor3ubv(c.RGB());
    glShadeModel(GL_FLAT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBegin(GL_POLYGON);
    y= yhmo + dyhmo*hmo/vyhmo;
    glVertex2f(xhmo,yhmo);
    glVertex2f(xhmo+dxhmo,yhmo);
    glVertex2f(xhmo+dxhmo,y);
    glVertex2f(xhmo,y);
    glEnd();
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  }

  //---------------------

  if (spopt->pText) {

    std::string str;
    float cx,cy;

    c= Colour(spopt->textColour);
    glColor3ubv(c.RGB());

    fp->setFontSize(fontSize2);
    str=modelName + " SPECTRUM";
    fp->getStringSize(str.c_str(),dx,dy);
    if (dx>(xplot2-xplot1-dytext2*0.5)) {
      float fontsize= fontSize1*(xplot2-xplot1-dytext2*0.5)/dx;
      fp->setFontSize(fontsize);
      fp->getStringSize(str.c_str(),dx,dy);
    }
    x=(xplot1+xplot2-dx)*0.5;
    y= yplot2-dy;
    fp->drawStr(str.c_str(),x,y,0.0);

    fp->setFontSize(fontSize1);

    fp->getCharSize('0',cx,cy);
    // the real height (x ok)
    cy*=0.75;

    std::string validtime= validTime.isoTime(true,false);

    ostringstream ostr1,ostr2;
    ostr1<<"Position: "<<posName<<"   Time: "<<validtime
         <<" UTC ("<<setiosflags(ios::showpos)<<forecastHour<<")";
    std::string str1= ostr1.str();
    fp->getStringSize(str1.c_str(),dx,dy);

    if (dx>xplot2-xplot1-dytext1*0.5) {
      float fontsize= int(fontSize1*(xplot2-xplot1-dytext1*0.5)/dx);
      fp->setFontSize(fontsize);
    }
    x=xplot1+dytext1*0.25;
    y=yplot1+dytext1+dy*0.30;
    fp->drawStr(str1.c_str(),x,y,0.0);

    if ( hmo>-1 && tPeak>-1 && ddPeak>-1 ) {
      ostr2<<"HMO= "<<setw(4)<<setprecision(1)<<setiosflags(ios::fixed)<<hmo
          <<"m   Tp= "<<setw(5)<<setprecision(1)<<setiosflags(ios::fixed)<<tPeak
          <<"s   DDp= "<<setw(5)<<setprecision(1)<<setiosflags(ios::fixed)<<ddPeak
          <<" deg";
      std::string str2= ostr2.str();
      x=xplot1+dytext1*0.25;
      y=yplot1+dy*0.30;
      fp->drawStr(str2.c_str(),x,y,0.0);
    }
  }

  UpdateOutput();

  return true;
}
