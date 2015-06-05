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

/*
  DESCRIPTION:    Plots classification table for classified data
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diLegendPlot.h"
#include "diImageGallery.h"
#include "diGLPainter.h"

#include <puTools/miStringFunctions.h>

#include <cmath>
#include "polyStipMasks.h"

#define MILOGGER_CATEGORY "diana.LegendPlot"
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;

LegendPlot::LegendPlot()
  : staticPlot_(0)
{
  METLIBS_LOG_SCOPE();
  showplot = true;
  x1title = 0;
  x2title = 0;
  y1title= 0;
  y2title= 0;
  xRatio = 0.01;
  yRatio = 0.01;
}


LegendPlot::LegendPlot(const std::string& str)
  : staticPlot_(0)
{
  METLIBS_LOG_SCOPE();

  showplot = true;
  x1title = 0;
  x2title = 0;
  y1title= 0;
  y2title= 0;
  xRatio = 0.01;
  yRatio = 0.01;

  std::string sstr(str);
  miutil::replace(sstr, '"',' ');
  miutil::trim(sstr);
  vector<std::string> vstr = miutil::split(sstr, "=");
  if(vstr.size()==2){
    vector<std::string> tokens = miutil::split(vstr[1], ";",false);
    int n=tokens.size();
    if(n>0){
      if (poptions.tableHeader)
        titlestring = tokens[0];
      for(int i=1;i<n;i+=3){
        if(i+3>n) break;
        ColourCode cc;
        cc.colour = Colour(tokens[i]);
        cc.pattern = tokens[i+1];
        cc.colourstr = tokens[i+2];
        //if string start with '|', do not plot colour/pattern box
        if(cc.colourstr.find('|')==1){
          cc.plotBox = false;
          miutil::remove(cc.colourstr, '|');
        } else {
          cc.plotBox = true;
        }
        colourcodes.push_back(cc);
      }
    }
  }
}


void LegendPlot::setData(const std::string& title,
    const vector<ColourCode>& colourcode)
{
  METLIBS_LOG_SCOPE();

  // fill the table with colours and textstrings from palette information
  titlestring = title;
  colourcodes = colourcode;
}

// Copy constructor
LegendPlot::LegendPlot(const LegendPlot &rhs)
{
  METLIBS_LOG_SCOPE();
  // elementwise copy
  memberCopy(rhs);
}

LegendPlot::~LegendPlot()
{
  METLIBS_LOG_SCOPE();
}

LegendPlot& LegendPlot::operator=(const LegendPlot &rhs)
{
  METLIBS_LOG_SCOPE();
  if (this == &rhs) return *this;

  // elementwise copy
  memberCopy(rhs);

  return *this;
}

bool LegendPlot::operator==(const LegendPlot &rhs) const
{
  METLIBS_LOG_SCOPE();
  return false;
}

void LegendPlot::memberCopy(const LegendPlot& rhs)
{
  METLIBS_LOG_SCOPE();
  // copy members
  titlestring= rhs.titlestring;
  colourcodes= rhs.colourcodes;
  x1title = rhs.x1title;
  x2title = rhs.x2title;
  y1title = rhs.y1title;
  y2title = rhs.y2title;
  xRatio  = rhs.xRatio;
  yRatio  = rhs.yRatio;
  suffix  = rhs.suffix;
  showplot = rhs.showplot;
}

void LegendPlot::getStringSize(DiGLPainter* gl, const std::string& str,
    float& width, float& height)
{
  gl->getTextSize(str, width, height);
  height *= 1.2; // FIXME
}

bool LegendPlot::plotLegend(DiGLPainter* gl, float x, float y)
{
  METLIBS_LOG_SCOPE();
  // fill the table with colours and textstrings from palette information

  int ncolours = colourcodes.size();
  if(!ncolours) return false;

  float width,height,maxwidth=0,maxheight=0,titlewidth=0;

  //colour code strings
  for (int i=0; i<ncolours; i++){
    colourcodes[i].colourstr += suffix;
    getStringSize(gl, colourcodes[i].colourstr, width, height);
    if (width>maxwidth) maxwidth= width;
    if (height>maxheight) maxheight= height;
  }

  //title
  int ntitle = 0;
  vector<std::string> vtitlestring;
  if((not titlestring.empty())){
    getStringSize(gl, titlestring, titlewidth, height);
    if(titlewidth>maxwidth){
      vector<std::string> vs = miutil::split(titlestring, " ");
      if (vs.size()>=5) {
        // handle field difference...
        std::string smove;
        int l, n= vs.size();
        for (int i=0; i<n; i++) {
          l= vs[i].length();
          if (l==1 && vs[i]=="(" && i<n-1) {
            smove="( ";
          } else if (l==1 && vs[i]=="-" && i<n-1) {
            smove="- ";
          } else if (l==1 && vs[i]==")" && vtitlestring.size()>0)  {
            int j= vtitlestring.size();
            vtitlestring[j-1] += " )";
          } else if (!smove.empty()) {
            vtitlestring.push_back(smove + vs[i]);
            smove.clear();
          } else {
            vtitlestring.push_back(vs[i]);
          }
        }
      } else {
        vtitlestring= vs;
      }
    } else {
      vtitlestring.push_back(titlestring);
    }
    ntitle = vtitlestring.size();
    titlewidth = 0;
    for (int i=0; i<ntitle; i++){
      getStringSize(gl, vtitlestring[i], width, height);
      if (width>titlewidth) titlewidth = width;
      if (height>maxheight)   maxheight= height;
    }
  }

  // position table

  float xborder;
  float yborder;
  getStringSize(gl, "c",xborder,yborder);
  xborder /=2;
  yborder /=2;
  titlewidth  = titlewidth + 2*xborder;
  float titleheight = maxheight*ntitle;
  float tablewidth  = maxwidth + 7*xborder;
  float tableheight = maxheight*ncolours + 2*yborder;
  if(titlewidth < tablewidth ) titlewidth = tablewidth;

  x1title = x;
  x2title = x + titlewidth;
  y2title = y;
  y1title = y - titleheight;

  float x1table = 0.0, x2table = 0.0;
  if(poptions.h_align==align_right){
    x1table = x2title-tablewidth;
    x2table = x2title;
  } else if(poptions.h_align==align_left){
    x1table = x1title;
    x2table = x1title+tablewidth;
  } else if(poptions.h_align==align_center){
    x1table = (x2title+x1title)/2.- tablewidth/2.;
    x2table = (x2title+x1title)/2.+ tablewidth/2.;
  }
  float y1table = y1title-tableheight;

  //draw title background
  if(ntitle>0){
    gl->setColour(poptions.fillcolour, false);
    gl->fillRect(x1title, y2title, x2title, y1title);

    //draw title
    gl->setColour(poptions.textcolour);
    float titley1 = y2title-yborder-maxheight/2;
    for (int i=0;i<ntitle;i++) {
      gl->drawText(vtitlestring[i],(x1title+xborder),titley1);
      titley1 -= maxheight;
    }
  }

  // draw table
  if (showplot){

    //draw table background
    gl->Enable(DiGLPainter::gl_BLEND);
    gl->BlendFunc(DiGLPainter::gl_SRC_ALPHA, DiGLPainter::gl_ONE_MINUS_SRC_ALPHA);
#if 0
    gl->Color4ubv(poptions.fillcolour.RGBA());
#else
    gl->setColour(poptions.fillcolour);
#endif
#if 0
    gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
    gl->Begin(DiGLPainter::gl_POLYGON);
    gl->Vertex2f(x1table,y1table);
    gl->Vertex2f(x1table,y1title);
    gl->Vertex2f(x2table,y1title);
    gl->Vertex2f(x2table,y1table);
    gl->End();
#else
    gl->fillRect(x1table, y1table, x2table, y1title);
#endif

    // draw table
    float x1box = x1table + xborder;
    float x2box = x1box   + 4*xborder;
    float y2box = y1title - yborder;
    float y1box = y2box   - maxheight;
    ImageGallery ig;
    gl->Enable(DiGLPainter::gl_POLYGON_STIPPLE);
    for (int i=0;i<ncolours;i++){
      if(colourcodes[i].plotBox){
        //draw colour/pattern box
        // draw background of colour/pattern boxes
        gl->setColour(poptions.fillcolour, false);
#if 1 // FIXME this is a strange painting call
        gl->Begin(DiGLPainter::gl_POLYGON);
        gl->Vertex2f(x1box,y1box);
        gl->Vertex2f(x1box,y2box);
        gl->Vertex2f(x2box,y2box);
        gl->Vertex2f(x2box,y1box);
        gl->End();
#else
        gl->fillRect(x1box, y1box, x2box, y2box);
#endif
        if((not colourcodes[i].pattern.empty())){
          DiGLPainter::GLubyte* p=ig.getPattern(colourcodes[i].pattern);
          if(p==0)
            gl->PolygonStipple(solid);
          else
            gl->PolygonStipple(p);
        }else{
          gl->PolygonStipple(solid);
        }
        gl->setColour(colourcodes[i].colour);
        gl->fillRect(x1box, y1box, x2box, y2box);

        // draw border of colour/pattern box
        gl->drawRect(x1box, y1box, x2box, y2box);
      }
      //draw textstring
      gl->setColour(poptions.textcolour);
      gl->drawText(colourcodes[i].colourstr, (x2box+xborder),(y1box+0.8*yborder));
      y2box -= maxheight;
      y1box -= maxheight;
    }
    gl->Disable(DiGLPainter::gl_POLYGON_STIPPLE);
  }
  gl->Disable(DiGLPainter::gl_BLEND);

  return true;
}

float LegendPlot::height(DiGLPainter* gl)
{
  int ncolours = colourcodes.size();
  if(!ncolours) return 0.0;

  gl->setFont(poptions.fontname,poptions.fontface,poptions.fontsize);
  float width,height,maxwidth=0,maxheight=0,titlewidth=0;

  //colour code strings
  for (int i=0; i<ncolours; i++){
    std::string cstring;
    cstring = colourcodes[i].colourstr;
    getStringSize(gl, cstring, width, height);
    if (height>maxheight) maxheight= height;
    if (width>maxwidth) maxwidth= width;
  }

  //title
  int ntitle=0;
  if((not titlestring.empty())){
    getStringSize(gl, titlestring, titlewidth, height);
    vector<std::string> vtitlestring;
    if(titlewidth>maxwidth){
      vtitlestring = miutil::split(titlestring, " ");
    } else {
      vtitlestring.push_back(titlestring);
    }
    ntitle = vtitlestring.size();
    titlewidth = 0;
    for (int i=0; i<ntitle; i++){
      getStringSize(gl, vtitlestring[i], width, height);
      if (height>maxheight)   maxheight= height;
    }
  }

  float xborder;
  float yborder;
  getStringSize(gl, "c",xborder,yborder);
  yborder /=4;
  float titleheight = maxheight*ntitle;
  float tableheight = maxheight*ncolours + 2*yborder;

  return (tableheight + titleheight);
}

float LegendPlot::width(DiGLPainter* gl)
{
  int ncolours = colourcodes.size();
  if(!ncolours) return 0.0;

  gl->setFont(poptions.fontname,poptions.fontface,poptions.fontsize);
  float width,height,maxwidth=0,titlewidth=0;

  //colour code strings
  for (int i=0; i<ncolours; i++){
    std::string cstring;
    cstring = colourcodes[i].colourstr;
    getStringSize(gl, cstring, width, height);
    if (width>maxwidth) maxwidth= width;
  }

  //title
  getStringSize(gl, titlestring, titlewidth, height);
  vector<std::string> vtitlestring;
  if(titlewidth>maxwidth){
    vtitlestring = miutil::split(titlestring, " ");
  } else {
    vtitlestring.push_back(titlestring);
  }
  int ntitle = vtitlestring.size();
  titlewidth = 0;
  for (int i=0; i<ntitle; i++){
    getStringSize(gl, vtitlestring[i], width, height);
    if (width>titlewidth) titlewidth = width;
  }

  float xborder;
  float yborder;
  getStringSize(gl, "c",xborder,yborder);
  xborder /=2;
  titlewidth  = titlewidth + 2*xborder;
  float tablewidth  = maxwidth + 6*xborder;
  if(titlewidth < tablewidth ) titlewidth = tablewidth;
  return titlewidth;
}
