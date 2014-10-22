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

#include "diPlot.h"

#include "diFontManager.h"
#include "diPlotModule.h"

#include <puTools/miStringFunctions.h>

#include <qglobal.h>
#if !defined(USE_PAINTGL)
#include <glp/glpfile.h>
#endif

#define MILOGGER_CATEGORY "diana.Plot"
#include <miLogger/miLogging.h>

using namespace ::miutil;
using namespace ::std;
using namespace d_print;

GridConverter StaticPlot::gc; // Projection-converter

StaticPlot::StaticPlot()
  : pwidth(0)         // physical plotwidth
  , pheight(0)        // physical plotheight
  , dirty(true)       // plotsize has changed
  , pressureLevel(-1) // current pressure level
  , oceandepth(-1)    // current ocean depth
  , gcd(0)            // great circle distance (corner to corner)
  , panning(false)    // panning in progress
  , fp(0)             // master fontpack
  , psoutput(0)       // PostScript module
  , hardcopy(false)   // producing postscript
{
}

StaticPlot::~StaticPlot()
{
}

Plot::Plot()
  : enabled(true)
  , rgbmode(true)
{
  METLIBS_LOG_SCOPE();
  if (not getStaticPlot()->getFontPack())
    getStaticPlot()->restartFontManager();
}

bool Plot::operator==(const Plot &rhs) const{
  return false;
}

StaticPlot* Plot::getStaticPlot() const
{
  return PlotModule::instance()->getStaticPlot();
}

void StaticPlot::initFontManager()
{
  METLIBS_LOG_SCOPE();
  if (fp)
    fp->parseSetup();
}

void StaticPlot::restartFontManager()
{
  METLIBS_LOG_SCOPE();
  delete fp;
  fp = new FontManager();
  initFontManager();
}

void Plot::setEnabled(bool e)
{
  enabled = e;
}

bool StaticPlot::setMapArea(const Area& a, bool keepcurrentarea){

  if (a.P().isDefined()){
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
//    float gridspec[Projection::speclen];
//    a.P().Gridspec(gridspec);
//    if (gridspec[0]==0){
//      return false;
//    }
    // use previous defined Area
    // add support for pure rotation later
    return true;
  }
}


void StaticPlot::setPlotSize(const Rectangle& r){
  if (fullrect==r) return;
  fullrect= r;
  setDirty(true);

  //if (fp) fp->setGlSize(r.width(), r.height());
  if (fp) fp->setGlSize(r.x1, r.x2, r.y1, r.y2);
}


void StaticPlot::setMapSize(const Rectangle& r){
  if (maprect==r) return;
  maprect= r;
  setDirty(true);
}

void StaticPlot::setPhysSize(float w, float h){
  pwidth= w;
  pheight= h;
  if (fp) fp->setVpSize(w, h);
  setDirty(true);
}

void StaticPlot::getPhysSize(float& w, float& h){
  w= pwidth;
  h= pheight;
}

Area StaticPlot::findBestMatch(const Area& newa){

  if (!area.P().isDefined())
    return newa;

  const int npos= 4;
  float xpos[npos], ypos[npos];
  xpos[0]= area.R().x1;
  ypos[0]= area.R().y1;
  xpos[1]= area.R().x1;
  ypos[1]= area.R().y2;
  xpos[2]= area.R().x2;
  ypos[2]= area.R().y2;
  xpos[3]= area.R().x2;
  ypos[3]= area.R().y1;

  if (!gc.getPoints(area.P(),newa.P(),npos,xpos,ypos)) {
    METLIBS_LOG_ERROR("findBestMatch: getPoints error");
    return newa;
  }

  const float MAX=100000000;
  float maxx= -100000000, maxy= -1000000000,
    minx= 100000000, miny= 1000000000;
  for (int i=0; i<npos; i++){
    // check for impossible numbers
    if (xpos[i] < -MAX || xpos[i] > MAX){
      return newa;
    }
    if (xpos[i] < minx) minx= xpos[i];
    if (ypos[i] < miny) miny= ypos[i];
    if (xpos[i] > maxx) maxx= xpos[i];
    if (ypos[i] > maxy) maxy= ypos[i];
  }

  Area a = newa;
  a.setR(Rectangle(minx,miny,maxx,maxy));
  return a;
}


void StaticPlot::setDirty(bool f)
{
  //METLIBS_LOG_SCOPE(LOGVAL(f));
  dirty= f;
}


void StaticPlot::setGcd(float dist){
  gcd = dist;

}

void Plot::setColourMode(bool isrgb){
  rgbmode= isrgb;
//   if (rgbmode) fp= std_fp;
//   else fp= ovr_fp;
}

// static method
void StaticPlot::xyClear(){
  xyLimit.clear();
  xyPart.clear();
}

void Plot::setPlotInfo(const std::string& pin)
{
  pinfo= pin;
  // fill poptions with values from pinfo
  poptions.parsePlotOption(pinfo,poptions);

  enabled= poptions.enabled;
}

std::string Plot::getPlotInfo(int n) const
{
  //return current plot info string
  if(n==0) return pinfo;
  //return n elements of current plot info string
  vector<std::string> token = miutil::split(pinfo, n, " ", true);
  token.pop_back(); //remove last part
  std::string str;
  //  str.join(token," ");
  for(unsigned int i=0;i<token.size();i++){
    str+=token[i];
    if(i<token.size()-1) str+=" ";
  }
  return str;
}
std::string Plot::getPlotInfo(const std::string& return_tokens) const
{
  //return n elements of current plot info string
  vector<std::string> return_token = miutil::split(return_tokens, 0, ",");
  vector<std::string> token = miutil::split(pinfo, 0, " ");
  std::string str;

  for(unsigned int i=0;i<token.size();i++){
    vector<std::string> stoken = miutil::split(token[i], 0, "=");
    if( stoken.size() == 2 ) {
      size_t j=0;
      while ( j<return_token.size() && return_token[j] != stoken[0] )
        ++j;
      if ( j < return_token.size() ) {
        str += token[i] + " ";
      }
    }
  }

  //probably old FIELD string syntax
  if ( str.empty() ) {
    return getPlotInfo(3);
  }

  return str;
}

bool StaticPlot::startPSoutput(const printOptions& po){
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
  psoutput = new GLPfile(const_cast<char*>(pro.fname.c_str()),
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
void StaticPlot::UpdateOutput(){
  if (psoutput) psoutput->UpdatePage(true);
}

bool StaticPlot::startPSnewpage()
{
  if (!hardcopy || !psoutput) return false;
  glFlush();
  if (psoutput->EndPage() != 0) {
    METLIBS_LOG_WARN("startPSnewpage: EndPage BAD!!!");
  }
  psoutput->StartPage();
  return true;
}

bool StaticPlot::endPSoutput(){
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


void StaticPlot::psAddImage(const GLvoid* data,GLint size,GLint nx,GLint ny,
		      GLfloat x,GLfloat y,GLfloat sx,GLfloat sy,
		      GLint x1,GLint y1,GLint x2,GLint y2,
		      GLenum format,GLenum type){
  if (!psoutput) return;
  bool blend_image_to_background = true;
  psoutput->AddImage(data, size, nx, ny, x, y, sx, sy, x1, y1, x2, y2, format,
      type, blend_image_to_background);
}


void StaticPlot::addHCStencil(int size, const float* x, const float* y)
{
  if (!psoutput)
    return;
  psoutput->addStencil(size,x,y);
}

// Scissoring in GL coordinates
void StaticPlot::addHCScissor(double x0, double y0,
			      double  w, double  h)
{
  if (!psoutput)
    return;
  psoutput->addScissor(x0,y0,w,h);
}

// Scissoring in pixel coordinates
void StaticPlot::addHCScissor(int x0, int y0,
			      int  w, int  h)
{
  if (!psoutput)
    return;
  psoutput->addScissor(x0,y0,w,h);
}

void StaticPlot::removeHCClipping()
{
  if (!psoutput)
    return;
  psoutput->removeClipping();
}

void StaticPlot::resetPage()
{
  if (!psoutput)
    return;
  psoutput->addReset();
}

// panning in progress
void StaticPlot::panPlot(bool b)
{
  panning= b;
}

bool StaticPlot::geo2xy(int n, float* x, float* y)
{
  return gc.geo2xy(getMapArea(), n, x, y);
}
