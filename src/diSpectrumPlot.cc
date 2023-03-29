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

#include "diana_config.h"

#include "diSpectrumPlot.h"

#include "diColour.h"
#include "diContouring.h"
#include "diGLPainter.h"
#include "diPlotOptions.h"
#include "diSpectrumOptions.h"
#include "diUtilities.h"
#include "diField/diField.h"

#include <mi_fieldcalc/MetConstants.h>
#include <mi_fieldcalc/math_util.h>

#include <puTools/miStringFunctions.h>

#include <QPolygonF>
#include <QString>

#include <iomanip>
#include <sstream>

#define MILOGGER_CATEGORY "diana.SpectrumPlot"
#include <miLogger/miLogging.h>

using namespace miutil;

static const float DEG_TO_RAD = M_PI / 180;

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

float  SpectrumPlot::eTotalMax= 0.0;


SpectrumPlot::SpectrumPlot()
  : numDirec(0), numFreq(0), sdata(0), xdata(0), ydata(0)
{
  METLIBS_LOG_SCOPE();
}


SpectrumPlot::~SpectrumPlot()
{
  METLIBS_LOG_SCOPE();

  delete[] sdata;
  delete[] xdata;
  delete[] ydata;
}


// static
void SpectrumPlot::startPlot(int numplots, int w, int h,
    SpectrumOptions *spopt, DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE("numplots=" << numplots);

  numplot= numplots;
  nplot= 0;
  plotw= w;
  ploth= h;

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

  float x1= xplot1 - 0.01*s;
  float x2= xplot2 + 0.01*s;
  float y1= yplot1 - 0.01*s;
  float y2= yplot2 + 0.01*s;

  float dx= x2 - x1;
  float dy= y2 - y1;
  const float paspr = plotw/float(ploth);
  if (paspr > dx/dy) {
    dx= (dy*paspr - dx) * 0.5;
    x1-=dx;
    x2+=dx;
  } else {
    dy= (dx/paspr - dy) * 0.5;
    y1-=dy;
    y2+=dy;
  }

  //----------------------

  gl->clear(Colour(spopt->backgroundColour));
  gl->LoadIdentity();
  gl->Ortho(x1,x2,y1,y2,-1.,1.);

  if (numplot>0 || !firstPlot) {
    gl->setVpGlSize(plotw, ploth, x2-x1, y2-y1);

    float chx,chy,fontsize=8.;
    gl->setFontSize(fontsize);
    gl->getCharSize('0',chx,chy);

    fontSize1= fontsize*(dytext1*0.5/chy);
    fontSize2= fontsize*(dytext2*0.5/chy);
    fontSizeLabel= fontSize1*0.5;
    if (fontSizeLabel>14.)
      fontSizeLabel= 14.;

    firstPlot= false;
  }
}


//static
void SpectrumPlot::plotDiagram(SpectrumOptions *spopt, DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE();

  gl->setLineStyle(Colour(spopt->frameColour), spopt->frameLinewidth);

  // frame
  if (spopt->pFrame)
    gl->drawRect(false, xplot1,yplot1,xplot2,yplot2);

  // mark wind position/directions
  if (spopt->pWind || spopt->pPeakDirection)
    gl->drawCross(xwind, ywind, rwind*0.25);

  // the total energy graph, axes with arrows
  const float dw = dytext1*1.5;
  gl->drawArrow(xefdiag, yefdiag, xefdiag+dxefdiag, yefdiag, dw);
  gl->drawArrow(xefdiag, yefdiag, xefdiag, yefdiag+dyefdiag, dw);

  // tics on the axes (labels later)
  for (int i=0; i<numyefdiag; i++) {
    const float v = float(i*incyefdiag);
    const float y = yefdiag+v*dyefdiag/vyefdiag;
    gl->drawLine(xefdiag-dytext1*0.3, y, xefdiag, y);
  }
  { const int n = int(vxefdiag*10.-0.5);
    for (int i=0; i<=n; i++) {
      const float v = float(i)*0.1;
      const float x = xefdiag+v*dxefdiag/vxefdiag;
      gl->drawLine(x, yefdiag, x, yefdiag-dytext1*0.3);
    }
  }

  // mark text positions
  if (spopt->pFrame) {
    gl->drawLine(xplot1, yplot2-dytext2, xplot2, yplot2-dytext2);
    gl->drawLine(xplot1, yplot1+dytext1, xplot2, yplot1+dytext1);
    gl->drawLine(xplot1, yplot1+dytext1*2.0f, xplot2, yplot1+dytext1*2.0f);
  }

  // hmo ticks (labels later)
  { const int n= int(vyhmo+0.1);
    for (int i=5; i<=n; i+=5) {
      const float v = float(i);
      const float y = yhmo+v*dyhmo/vyhmo;
      gl->drawLine(xhmo-dytext1*0.1, y, xhmo+dxhmo, y);
    }
  }

  // hmo
  gl->drawRect(false, xhmo, yhmo, xhmo+dxhmo, yhmo+dyhmo);

  const float freqStep= 0.05;
  for (float freq= freqStep; freq<freqMax+0.1*freqStep; freq+=freqStep)
    gl->drawCircle(false, 0, 0, freq);

  { const int NPOINTS = 120, ISTEP = 10;
    const float STEP = 2 * M_PI / NPOINTS;
    for (int i=0; i<NPOINTS; i+=ISTEP) {
      const float angle = i*STEP;
      gl->drawLine(0, 0, specRadius*std::cos(angle), specRadius*std::sin(angle));
    }
  }

  if (fontSize1>0.0 && spopt->pFixedText) {

    gl->setLineStyle(Colour(spopt->fixedTextColour));

    float chx, chy, chx2, chy2, x, y, dx, dy;

    gl->setFontSize(fontSize1);
    gl->getCharSize('0',chx,chy);
    gl->getTextSize("0.0", chx2, chy2);

    // the real size (x ok)
    chy*=0.75;

    QString str = "F(f,dir) [m2/Hz/rad]";
    gl->getTextSize(str, dx, dy);
    x=(xplot1+xplot2-dx)*0.5;
    y= yplot2-dytext2-dytext1+chy*0.25;
    gl->drawText(str, x, y, 0.0);

    str= "0.05 Hz/circle";
    x= specRadius*1.05f*cos(60.0f*DEG_TO_RAD);
    y= specRadius*1.05f*sin(60.0f*DEG_TO_RAD);
    gl->drawText(str, x, y, 0.0);

    str= "F(f) [m2/Hz]";
    x= xplot1 + dytext1*0.25;
    y= yefdiag + dyefdiag + chy*0.35;
    gl->drawText(str, x, y, 0.0);
    // tick labels
    for (int i=0; i<numyefdiag; i++) {
      int ivalue= i*incyefdiag;
      const float v = float(ivalue);
      const float y = yefdiag+v*dyefdiag/vyefdiag;
      const float s= (ivalue<10) ? 1.2 : 2.2;
      gl->drawText(QString::number(ivalue),
          xefdiag-dytext1*0.3-chx*s, y-chy*0.5, 0.0);
    }

    str= "freq.[Hz]";
    x= xefdiag + dxefdiag + chy*0.5;
    y= yefdiag - chy*0.5;
    gl->drawText(str, x, y, 0.0);
    // tick labels
    { const int n= int(vxefdiag*10.-0.5);
      for (int i=0; i<=n; i++) {
        const float v = float(i)*0.1;
        x=xefdiag+v*dxefdiag/vxefdiag;
        if (i==0) str="0.0";
        else      str=QString::number(v);
        gl->drawText(str, x-chx2*0.5, yefdiag-dytext1*0.3-chy*1.1, 0.0);
      }
    }

    str= "HMO[m]";
    gl->getTextSize(str, dx, dy);
    x= xplot2 - dx - dytext1*0.25;
    y= yhmo - dy;
    gl->drawText(str, x, y, 0.0);
    // hmo tick labels
    { const int n= int(vyhmo+0.1);
      for (int i=5; i<=n; i+=5) {
        const float v = float(i);
        y=yhmo+v*dyhmo/vyhmo;
        const float s= (i<10) ? 1.0f : 2.0f;
        gl->drawText(QString::number(i),
            xhmo-dytext1*0.1-chx*s, y-chy*0.5, 0.0);
      }
    }
  }
}


bool SpectrumPlot::plot(SpectrumOptions *spopt, DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE(posName << " " << validTime);

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
      miutil::KeyValue_v optstr;
      optstr.push_back(miutil::KeyValue("palettecolours", "standard"));
      optstr.push_back(miutil::KeyValue("repeat", "1"));
      PlotOptions::parsePlotOption(optstr,poptions);
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      gl->setLineStyle(Colour("black"), 1);

      bool res= contour(numDirec+1,numFreq,sdata,xdata,ydata,
          part,iconv,conv,xylim,
          idraw,zrange,zstep,zoff,nlines,rlines,
          ncol,icol,ntyp,ityp,nwid,iwid,nlim,rlim,
          idraw2,zrange2,zstep2,zoff2,nlines2,rlines2,
          ncol2,icol2,ntyp2,ityp2,nwid2,iwid2,nlim2,rlim2,
          ismooth,labfmt,chxlab,chylab,ibcol,
          ibmap,lbmap,bmap,nxbmap,nybmap,rbmap,
          gl, poptions, dummyArea, fieldUndef);
      if (!res)
        METLIBS_LOG_ERROR("SpectrumPlot::plot  Contour error");
    }

    if (spopt->pSpectrumLines) {

      poptions.contourShading = false;

      gl->setFont(diutil::SCALEFONT);
      gl->setFontSize(fontSizeLabel);
      gl->getCharSize('0',chxlab,chylab);
      // the real height (x ok)
      chylab*=0.75;
      labfmt[0]= -1;

      gl->setLineStyle(Colour(spopt->spectrumLineColour), spopt->spectrumLinewidth);
      bool res = contour(numDirec+1,numFreq,sdata,xdata,ydata,
          part,iconv,conv,xylim,
          idraw,zrange,zstep,zoff,nlines,rlines,
          ncol,icol,ntyp,ityp,nwid,iwid,nlim,rlim,
          idraw2,zrange2,zstep2,zoff2,nlines2,rlines2,
          ncol2,icol2,ntyp2,ityp2,nwid2,iwid2,nlim2,rlim2,
          ismooth,labfmt,chxlab,chylab,ibcol,
          ibmap,lbmap,bmap,nxbmap,nybmap,rbmap,
          gl, poptions, dummyArea, fieldUndef);
      if (!res)
        METLIBS_LOG_ERROR("SpectrumPlot::plot  Contour error");
    }
  }

  if (spopt->pPeakDirection && ddPeak > -1) {
    float dx= rwind * cos((90.-ddPeak)*DEG_TO_RAD);
    float dy= rwind * sin((90.-ddPeak)*DEG_TO_RAD);
    float xend= xwind + dx;
    float yend= ywind + dy;

    gl->setLineStyle(Colour(spopt->peakDirectionColour), spopt->peakDirectionLinewidth);
    gl->drawArrow(xwind, ywind, xend, yend);
  }

  // wind .....................................................
  if (spopt->pWind && wspeed>0.001){
    const float dd_rad = wdir*DEG_TO_RAD;
    const float ff_knots = miutil::ms2knots(wspeed);
    gl->setLineStyle(Colour(spopt->windColour), spopt->windLinewidth);
    gl->drawWindArrow(ff_knots * sin(dd_rad), ff_knots * cos(dd_rad), xwind, ywind, rwind * 0.85, false);
  }

  //...........................................................

  // total energy per frequency graph
  if (spopt->pEnergyColoured || spopt->pEnergyLine) {
    QPolygonF line, quads;
    for (int i=0; i<numFreq; i++) {
      if (frequences[i] > 0.5)
         break;
      const QPointF top(xefdiag + dxefdiag*frequences[i]/vxefdiag,
          yefdiag + dyefdiag*eTotal[i]/vyefdiag);
      line << top;
      quads << top << QPointF(top.x(), yefdiag);
    }
    if (spopt->pEnergyColoured) {
      gl->setLineStyle(Colour(spopt->energyFillColour), 1);
      gl->fillQuadStrip(quads);
    }
    if (spopt->pEnergyLine) {
      gl->setLineStyle(Colour(spopt->energyLineColour), spopt->energyLinewidth);
      gl->drawPolyline(line);
    }
  }

  // wave height (HMO)
  if (hmo > -1) {
    gl->setLineStyle(Colour(spopt->textColour), 1);
    gl->drawRect(true, xhmo, yhmo, xhmo+dxhmo, yhmo + dyhmo*hmo/vyhmo);
  }

  //---------------------

  gl->setFont(diutil::BITMAPFONT);
  if (spopt->pText) {

    float cx,cy;

    gl->setLineStyle(Colour(spopt->textColour), 1);

    gl->setFontSize(fontSize2);
    QString str = QString::fromStdString(modelName) + " SPECTRUM";
    float dx, dy;
    gl->getTextSize(str, dx, dy);
    if (dx>(xplot2-xplot1-dytext2*0.5)) {
      float fontsize= fontSize1*(xplot2-xplot1-dytext2*0.5)/dx;
      gl->setFontSize(fontsize);
      gl->getTextSize(str, dx, dy);
    }
    float x = (xplot1+xplot2-dx)*0.5;
    float y = yplot2-dy;
    gl->drawText(str, x, y, 0.0);

    gl->setFontSize(fontSize1);

    gl->getCharSize('0',cx,cy);
    // the real height (x ok)

    const QString str1 = QString("Position: %1   Time: %2 UTC (%3%4)")
        .arg(QString::fromStdString(posName))
        .arg(QString::fromStdString(validTime.isoTime(true,false)))
        .arg(QChar(forecastHour >= 0 ? '+' : '-'))
        .arg(forecastHour);
    gl->getTextSize(str1, dx, dy);

    if (dx>xplot2-xplot1-dytext1*0.5) {
      float fontsize= int(fontSize1*(xplot2-xplot1-dytext1*0.5)/dx);
      gl->setFontSize(fontsize);
    }
    x=xplot1+dytext1*0.25;
    y=yplot1+dytext1+dy*0.30;
    gl->drawText(str1, x, y, 0.0);

    if ( hmo>-1 && tPeak>-1 && ddPeak>-1 ) {
      // turn wave direction, use the meteorological direction
      float ddpeak = ddPeak > 180 ? ddPeak - 180 : ddPeak + 180;
      const QString str2 = QString("HMO= %1m   Tp= %2s   DDp= %3 deg")
          .arg(hmo, 4, 'f', 1)
          .arg(tPeak, 5, 'f', 1)
          .arg(ddpeak, 5, 'f', 1);
      x=xplot1+dytext1*0.25;
      y=yplot1+dy*0.30;
      gl->drawText(str2, x, y, 0.0);
    }
  }

  return true;
}
