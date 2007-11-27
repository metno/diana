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

#include <diPlot.h>
#include <glpfile.h>
#include <diFontManager.h>

// static class members
Area Plot::area;           // chosen projec./area in gridcoordinates
Area Plot::requestedarea;  // requested projec./area in gridcoordinates
Rectangle Plot::maprect;   // legal plotarea for maps in gc
Rectangle Plot::fullrect;  // full plotarea in gc
GridConverter Plot::gc;    // Projection-converter
miTime Plot::ctime;        // current time
float Plot::pwidth=0;      // physical plotwidth
float Plot::pheight=0;     // physical plotheight
float Plot::gcd=0;         // great circle distance (corner to corner) 
FontManager* Plot::fp=0;   // master fontpack
bool Plot::dirty=true;     // plotsize has changed
SetupParser Plot::setup;   // setup-info and parser class
GLPfile* Plot::psoutput=0; // PostScript module
bool Plot::hardcopy=false; // producing postscript
int Plot::pressureLevel=-1;// current pressure level
int Plot::oceandepth=-1;   // current ocean depth
miString Plot::bgcolour="";// name of background colour
Colour Plot::backgroundColour;
Colour Plot::backContrastColour;
bool Plot::panning=false;  // panning in progress
vector<float> Plot::xyLimit; // MAP ... xyLimit=x1,x2,y1,y2
vector<float> Plot::xyPart;  // MAP ... xyPart=x1%,x2%,y1%,y2%

// Default constructor
Plot::Plot()
  : enabled(true),datachanged(true),rgbmode(true){
  
  if (!fp) fp= new FontManager();
}

// Equality operator
bool Plot::operator==(const Plot &rhs) const{
  return false;
}

// some init after setup is read
void Plot::afterSetup(){
  if (fp) fp->parseSetup( setup );
}

void Plot::enable(const bool f){
  enabled= f;
}

bool Plot::setMapArea(const Area& a, bool keepcurrentarea){
  if (a.P().Gridtype()!=Projection::undefined_projection){
    // change plot-Area
    area= a;
    if (!keepcurrentarea) {
      if (xyLimit.size()==4) {
	Rectangle rect(xyLimit[0],xyLimit[2],xyLimit[1],xyLimit[3]);
	area.setR(rect);
      } else if (xyPart.size()==4) {
	Rectangle rect1= area.R();
	Rectangle rect;
	rect.x1= rect1.x1 + rect1.width() *xyPart[0];
	rect.x2= rect1.x1 + rect1.width() *xyPart[1];
	rect.y1= rect1.y1 + rect1.height()*xyPart[2];
	rect.y2= rect1.y1 + rect1.height()*xyPart[3];
	area.setR(rect);
      }
    }
    setDirty(true);
    return true;
  } else {
    // undefined projection
    // if grid[0]=0, Area should not be set
    float gridspec[Projection::speclen];
    a.P().Gridspec(gridspec);
    if (gridspec[0]==0){
      return false;
    }
    // use previous defined Area
    // add support for pure rotation later
    return true;
  }
}


void Plot::setPlotSize(const Rectangle& r){
  if (fullrect==r) return;
  fullrect= r;
  setDirty(true);

  if (fp) fp->setGlSize(r.width(), r.height());
}


void Plot::setMapSize(const Rectangle& r){
  if (maprect==r) return;
  maprect= r;
  setDirty(true);
}

void Plot::setPhysSize(const float w, const float h){
  pwidth= w;
  pheight= h;
  if (fp) fp->setVpSize(w, h);
  setDirty(true);
}

void Plot::getPhysSize(float& w, float& h){
  w= pwidth;
  h= pheight;
}

Area Plot::findBestMatch(const Area& newa){
  Area a= newa;
  // NEW - ADC 9.12.2002
  if (area.P() == Projection::undefined_projection)
    return a;
  //-----

  int npos= 4;
  float *xpos = new float[npos];
  float *ypos = new float[npos];
  
  xpos[0]= area.R().x1;
  ypos[0]= area.R().y1;
  xpos[1]= area.R().x1;
  ypos[1]= area.R().y2;
  xpos[2]= area.R().x2;
  ypos[2]= area.R().y2;
  xpos[3]= area.R().x2;
  ypos[3]= area.R().y1;

  if (!gc.getPoints(area,newa,npos,xpos,ypos)) {
    cerr << "findBestMatch: getPoints error" << endl;
    delete[] xpos;
    delete[] ypos;
    return a;
  }

  const float MAX=100000000;
  float maxx= -10000, maxy= -10000,
    minx= 10000, miny= 10000;
  for (int i=0; i<npos; i++){
    // check for impossible numbers
    if (xpos[i] < -MAX || xpos[i] > MAX){
      delete[] xpos;
      delete[] ypos;
      return a;
    }
    if (xpos[i] < minx) minx= xpos[i];
    if (ypos[i] < miny) miny= ypos[i];
    if (xpos[i] > maxx) maxx= xpos[i];
    if (ypos[i] > maxy) maxy= ypos[i];
  }

  a.setR(Rectangle(minx,miny,maxx,maxy));

  delete[] xpos;
  delete[] ypos;
  return a;
}


void Plot::setDirty(const bool f){
  //cerr << "SetDirty " << (f ? "true" : "false") << endl;
  dirty= f;

}


void Plot::setGcd(const float dist){
  gcd = dist;

}

void Plot::setColourMode(const bool isrgb){
  rgbmode= isrgb;
//   if (rgbmode) fp= std_fp;
//   else fp= ovr_fp;
}

void Plot::xyClear(){
  xyLimit.clear();
  xyPart.clear();
}

void Plot::setPlotInfo(const miString& pin)
{
  pinfo= pin;
  // fill poptions with values from pinfo
  poptions.parsePlotOption(pinfo,poptions);

  enabled= poptions.enabled;
}

miString Plot::getPlotInfo(int n)
{
  //return current plot info string
  if(n==0) return pinfo;
  //return n elements of current plot info string
  vector<miString> token = pinfo.split(n," ",true);
  token.pop_back(); //remove last part
  miString str;
  str.join(token," ");
  return str;
}

bool Plot::startPSoutput(const printOptions& po){
  if (hardcopy) return false;
  
  printOptions pro= po;
  int feedsize= 30000000;
  int print_options= 0;

  // Fit output to page
  if (pro.fittopage)
    print_options= print_options | GLP_FIT_TO_PAGE;

  // set colour mode
  if (pro.colop==greyscale){
    print_options= print_options | GLP_GREYSCALE;

    /* Reversing colours should be an option from the GUI?
    // calculate background colour intensity
    float bci=
    backgroundColour.fB()*0.0820 +
    backgroundColour.fG()*0.6094 +
    backgroundColour.fR()*0.3086;
    
    if (bci < 0.2)
    print_options= print_options | GLP_REVERSE;
    */
    
    if (pro.drawbackground)
      print_options= print_options | GLP_DRAW_BACKGROUND;
    
  } else if (pro.colop==blackwhite) {
    print_options= print_options | GLP_BLACKWHITE;
    
  } else {
    if (pro.drawbackground)
      print_options= print_options | GLP_DRAW_BACKGROUND;
  }

  // set orientation
  if (pro.orientation==ori_landscape)
    print_options= print_options | GLP_LANDSCAPE;
  else if (pro.orientation==ori_portrait)
    print_options= print_options | GLP_PORTRAIT;
  else
    print_options= print_options | GLP_AUTO_ORIENT;

  // calculate line, point (and font?) scale
  if (!pro.usecustomsize)
    pro.papersize= printman.getSize(pro.pagesize);
  PaperSize a4size;
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
  hardcopy= true;

  // inform fontpack
  if (fp){
    fp->startHardcopy(psoutput);
  }

  return true;
}

// for postscript output
void Plot::UpdateOutput(){
  if (psoutput) psoutput->UpdatePage(true);
}

bool Plot::startPSnewpage()
{
  if (!hardcopy || !psoutput) return false;
  glFlush();
  if (psoutput->EndPage() != 0) {
    cerr << "startPSnewpage: EndPage BAD!!!" << endl;
  }
  psoutput->StartPage();
  return true;
}

bool Plot::endPSoutput(){
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


void Plot::psAddImage(const GLvoid* data,GLint size,GLint nx,GLint ny,
		      GLfloat x,GLfloat y,GLfloat sx,GLfloat sy,
		      GLint x1,GLint y1,GLint x2,GLint y2,
		      GLenum format,GLenum type){
  if (!psoutput) return;
  psoutput->AddImage(data,size,nx,ny,x,y,sx,sy,x1,y1,x2,y2,format,type);
}


void Plot::addHCStencil(const int& size, const float* x, const float* y)
{
  if (!psoutput)
    return;
  psoutput->addStencil(size,x,y);
}

// Scissoring in GL coordinates
void Plot::addHCScissor(const double x0, const double y0,
			const double  w, const double  h)
{
  if (!psoutput)
    return;
  psoutput->addScissor(x0,y0,w,h);
}

// Scissoring in pixel coordinates
void Plot::addHCScissor(const int x0, const int y0,
			const int  w, const int  h)
{
  if (!psoutput)
    return;
  psoutput->addScissor(x0,y0,w,h);
}

void Plot::removeHCClipping()
{
  if (!psoutput)
    return;
  psoutput->removeClipping();
}

void Plot::resetPage()
{
  if (!psoutput)
    return;
  psoutput->addReset();
}

// panning in progress
void Plot::panPlot(const bool b)
{
  panning= b;
}
