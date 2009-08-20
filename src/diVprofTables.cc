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
#include <diVprofTables.h>
#include <diSetupParser.h>
#include <glpfile.h>
#include <iostream>
#include <math.h>

// static members

FontManager* VprofTables::fp= 0; // fontpack

vector<float> VprofTables::charsizes;
vector<float> VprofTables::fontsizes;
miString VprofTables::defaultFont="BITMAPFONT";

GLPfile* VprofTables::psoutput=0; // PostScript module
bool VprofTables::hardcopy=false; // producing postscript

float VprofTables::chxbas= 0.;   // character size
float VprofTables::chybas= 0.;
float VprofTables::chxlab= 0.;   // labels etc. character size
float VprofTables::chylab= 0.;
float VprofTables::chxtxt= 0.;   // text (name etc.) character size
float VprofTables::chytxt= 0.;

float VprofTables::xysize[mxysize][4]= { 0. };

int   VprofTables::iptab[mptab]= { 0  }; // pressure in unit hPa
float VprofTables::pptab[mptab]= { 0. }; // pressure in unit hPa
float VprofTables::pitab[mptab]= { 0. }; // exner function (pi)
float VprofTables::pplog[mptab]= { 0. }; // ln(pressure)
float VprofTables::yptabd[mptab]= { 0. };// y
float VprofTables::xztabd[mptab]= { 0. };// x(t.celsius=0)
float VprofTables::yptab[mptab]= { 0. }; // y
float VprofTables::xztab[mptab]= { 0. }; // x(t.celsius=0)
float VprofTables::dx1degree= 0.;    // x distance for 1 degree celsius

vector<VprofText> VprofTables::vptext; // info for text plotting


// default constructor
VprofTables::VprofTables()
{
  if (!fp) {
    SetupParser sp;
    fp= new FontManager();
    fp->parseSetup(sp);
    fp->setFont(defaultFont);
    fp->setFontFace(glText::F_NORMAL);
    fp->setScalingType(glText::S_FIXEDSIZE);
  }
}


// destructor
VprofTables::~VprofTables()
{
}


void VprofTables::clearCharsizes()
{
  charsizes.clear();
  fontsizes.clear();
}


void VprofTables::addCharsize(float chy)
{
  charsizes.push_back(chy);
}


void VprofTables::setFontsize(float chy)
{
  chy*=0.8;
  float d, dnear= 1.e+35;
  int n= charsizes.size();
  int near= -1;
  for (int i=0; i<n; i++) {
    d= fabsf(charsizes[i] - chy);
    if (d<dnear) {
      dnear= d;
      near= i;
    }
  }
  if (near>=0) fp->setFontSize(fontsizes[near]);
//###########################################################################
//if (near>=0) cerr<<"setfont "<<near<<" "<<chy<<" "<<fontsizes[near]<<endl;
//###########################################################################//###########################################################################
}


void VprofTables::makeFontsizes(float wx, float wy, int vx, int vy)
{
  fp->setVpSize(float(vx),float(vy));
  fp->setGlSize(wx,wy);

  float size, scale= float(vy)/wy;

  // scale to get the wanted height of characters
  scale*=1.333333;

  fontsizes.clear();

  int n= charsizes.size();

  for (int i=0; i<n; i++) {
    size= charsizes[i]*scale;
    //fp->setFontSize(size);
    fontsizes.push_back(size);
  }
}


void VprofTables::xyclip(int npos, float *x, float *y, float xylim[4])
{
  //  plotter del(er) av sammenhengende linje som er innenfor gitt
  //  omraade, ogsaa linjestykker mellom 'nabopunkt' som begge er
  //  utenfor omraadet.
  //  (farge, linje-type og -tykkelse maa vaere satt paa forhaand)
  //
  //  grafikk: OpenGL
  //
  //  input:
  //  ------
  //  x(npos),y(npos): linje med 'npos' punkt (npos>1)
  //  xylim(1-4):      x1,x2,y1,y2 for aktuelt omraade

  int   nint, nc, n, i, k1, k2;
  float xa, xb, ya, yb, xint, yint, x1, x2, y1, y2;
  float xc[4], yc[4];

  if (npos<2) return;

  xa= xylim[0];
  xb= xylim[1];
  ya= xylim[2];
  yb= xylim[3];

  if (x[0]<xa || x[0]>xb || y[0]<ya || y[0]>yb) {
    k2= 0;
  } else {
    k2= 1;
    nint= 0;
    xint= x[0];
    yint= y[0];
  }

  for (n=1; n<npos; ++n) {
    k1= k2;
    k2= 1;

    if (x[n]<xa || x[n]>xb || y[n]<ya || y[n]>yb) k2=0;

    // sjekk om 'n' og 'n-1' er innenfor
    if (k1+k2==2) continue;

    // k1+k2=1: punkt 'n' eller 'n-1' er utenfor
    // k1+k2=0: sjekker om 2 nabopunkt som begge er utenfor omraadet
    //          likevel har en del av linja innenfor.

    x1= x[n-1];
    y1= y[n-1];
    x2= x[n];
    y2= y[n];

    // sjekker om 'n-1' og 'n' er utenfor paa samme side
    if (k1+k2==0  &&
	((x1<xa && x2<xa) || (x1>xb && x2>xb) ||
	 (y1<ya && y2<ya) || (y1>yb && y2>yb))) continue;

    // sjekker alle skjaerings-muligheter
    nc = -1;
    if (x1!=x2) {
      nc++;
      xc[nc]= xa;
      yc[nc]= y1+(y2-y1)*(xa-x1)/(x2-x1);
      if (yc[nc]<ya || yc[nc]>yb || (xa-x1)*(xa-x2)>0.) nc--;
      nc++;
      xc[nc]= xb;
      yc[nc]= y1+(y2-y1)*(xb-x1)/(x2-x1);
      if (yc[nc]<ya || yc[nc]>yb || (xb-x1)*(xb-x2)>0.) nc--;
    }
    if (y1!=y2) {
      nc++;
      yc[nc]= ya;
      xc[nc]= x1+(x2-x1)*(ya-y1)/(y2-y1);
      if (xc[nc]<xa || xc[nc]>xb || (ya-y1)*(ya-y2)>0.) nc--;
      nc++;
      yc[nc]= yb;
      xc[nc]= x1+(x2-x1)*(yb-y1)/(y2-y1);
      if (xc[nc]<xa || xc[nc]>xb || (yb-y1)*(yb-y2)>0.) nc--;
    }

    if (k2==1) {
      // foerste punkt paa linjestykke innenfor
      nint= n-1;
      xint= xc[0];
      yint= yc[0];
    } else if (k1==1) {
      // siste punkt paa linjestykke innenfor
      glBegin(GL_LINE_STRIP);
      glVertex2f(xint, yint);
      for (i=nint+1; i<n; i++)
	glVertex2f(x[i], y[i]);
      glVertex2f(xc[0], yc[0]);
      glEnd();
    } else if (nc>0) {
      // to 'nabopunkt' utenfor, men del av linja innenfor
      glBegin(GL_LINE_STRIP);
      glVertex2f(xc[0], yc[0]);
      glVertex2f(xc[1], yc[1]);
      glEnd();
    }
  }

  if (k2==1) {
    // siste punkt er innenfor omraadet
    glBegin(GL_LINE_STRIP);
    glVertex2f(xint, yint);
    for (i=nint+1; i<npos; i++)
      glVertex2f(x[i], y[i]);
    glEnd();
  }
}


bool VprofTables::startPSoutput(const printOptions& po){
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
  if (abs(pro.papersize.vsize>0))
    scale= a4size.vsize/pro.papersize.vsize;

  // check if extra output-commands
  map<string,string> extra;
  printman.checkSpecial(pro,extra);

  // make GLPfile object
  psoutput = new GLPfile(const_cast<char*>(pro.fname.cStr()),
			 print_options, feedsize, &extra,
			 pro.doEPS);

  // set line and point scale
  psoutput->setScales(0.5*scale, 0.5*scale);

  psoutput->StartPage();
  // set viewport
  psoutput->setViewport(pro.viewport_x0, pro.viewport_y0,
			pro.viewport_width, pro.viewport_height);

  hardcopy= true;

  // inform fontpack
  if (fp){
    fp->startHardcopy(psoutput);
  }

  return true;
}


void VprofTables::addHCStencil(const int& size,
			       const float* x, const float* y)
{
  if (!psoutput)
    return;
  psoutput->addStencil(size,x,y);
}

void VprofTables::addHCScissor(const int x0, const int y0,
			       const int  w, const int  h)
{
  if (!psoutput)
    return;
  psoutput->addScissor(x0,y0,w,h);
}

void VprofTables::removeHCClipping()
{
  if (!psoutput)
    return;
  psoutput->removeClipping();
}

// for postscript output
void VprofTables::UpdateOutput(){
  if (psoutput) psoutput->UpdatePage(true);
}


bool VprofTables::startPSnewpage()
{
  if (!hardcopy || !psoutput) return false;
  glFlush();
  if (psoutput->EndPage() != 0) {
    cerr << "startPSnewpage: EndPage BAD!!!" << endl;
  }
  psoutput->StartPage();
  return true;
}

void VprofTables::resetPage()
{
  if (!hardcopy || !psoutput) return;

  psoutput->addReset();
}


bool VprofTables::endPSoutput(){
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
