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

/*
  DESCRIPTION:    Plots classification table for classified data
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diLegendPlot.h>
#include <diFontManager.h>
#include <diImageGallery.h>
#include <iostream>
#include <math.h>
#include <polyStipMasks.h>

using namespace std; using namespace miutil;

// Default constructor
LegendPlot::LegendPlot()
  : Plot()
{
#ifdef DEBUGPRINT
  cerr << "++ LegendPlot::Default Constructor" << endl;
#endif
  showplot = true;
  x1title = 0;
  x2title = 0;
  y1title= 0;
  y2title= 0;
  xRatio = 0.01;
  yRatio = 0.01;

}


LegendPlot::LegendPlot(miString& str)
  : Plot()
{
#ifdef DEBUGPRINT
  cerr << "++ LegendPlot::Default Constructor" << endl;
#endif

  showplot = true;
  x1title = 0;
  x2title = 0;
  y1title= 0;
  y2title= 0;
  xRatio = 0.01;
  yRatio = 0.01;

  miString sstr = str.replace('"',' ');
  sstr.trim();
  vector<miString> vstr = sstr.split("=");
  if(vstr.size()==2){
    vector<miString> tokens = vstr[1].split(";",false);
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
          cc.colourstr.remove('|');
        } else {
          cc.plotBox = true;
        }
        colourcodes.push_back(cc);
      }
    }
  }
}


void LegendPlot::setData(const miString& title,
			     const vector<ColourCode>& colourcode)
{
#ifdef DEBUGPRINT
  cerr << "++ LegendPlot::setdata" << endl;
#endif

  // fill the table with colours and textstrings from palette information
  titlestring = title;
  colourcodes = colourcode;

}

// Copy constructor
LegendPlot::LegendPlot(const LegendPlot &rhs){
#ifdef DEBUGPRINT
  cerr << "++ LegendPlot::Copy constructor";
#endif
  // elementwise copy
  memberCopy(rhs);
}

// Destructor
LegendPlot::~LegendPlot(){
#ifdef DEBUGPRINT
  cerr << "++ LegendPlot::Destructor" << endl;
#endif
}

// Assignment operator
LegendPlot& LegendPlot::operator=(const LegendPlot &rhs){
#ifdef DEBUGPRINT
  cerr << "++ LegendPlot::Assignment operator" << endl;
#endif
  if (this == &rhs) return *this;

  // elementwise copy
  memberCopy(rhs);

  return *this;
}

// Equality operator
bool LegendPlot::operator==(const LegendPlot &rhs) const{
#ifdef DEBUGPRINT
  cerr << "++ LegendPlot::Equality operator" << endl;
#endif
  return false;
}

void LegendPlot::memberCopy(const LegendPlot& rhs){
#ifdef DEBUGPRINT
  cerr << "++ LegendPlot::MemberCopy" << endl;
#endif
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

void LegendPlot::getStringSize(miString str, float& width, float& height)
{

  //Bugfix
  //The postscript size of "-" are underestimated
  if (hardcopy){
    int n = str.countChar('-');
    for(int i=0;i<n;i++) str+="-";
  }

  fp->getStringSize(str.cStr(), width, height);

  // fontsizeScale != 1 when postscript font size != X font size
  if (hardcopy){
    float fontsizeScale = fp->getSizeDiv();
    width*=fontsizeScale;
    height*=fontsizeScale;
  }

  height *= 1.2;
}

bool LegendPlot::plot(float x, float y)
{
#ifdef DEBUGPRINT
  cerr << "++ LegendPlot::plot" << endl;
#endif
  // fill the table with colours and textstrings from palette information

  int ncolours = colourcodes.size();
  if(!ncolours) return false;

  float width,height,maxwidth=0,maxheight=0,titlewidth=0;

  //colour code strings
  for (int i=0; i<ncolours; i++){
    colourcodes[i].colourstr += suffix;
    getStringSize(colourcodes[i].colourstr, width, height);
    if (width>maxwidth) maxwidth= width;
    if (height>maxheight) maxheight= height;
  }

  //title
  int ntitle = 0;
  vector<miString> vtitlestring;
  if(titlestring.exists()){
    getStringSize(titlestring, titlewidth, height);
    if(titlewidth>maxwidth){
      vector<miString> vs = titlestring.split(" ");
      if (vs.size()>=5) {
	// handle field difference...
	miString smove;
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
      getStringSize(vtitlestring[i], width, height);
      if (width>titlewidth) titlewidth = width;
      if (height>maxheight)   maxheight= height;
    }
  }

  // position table

  float xborder;
  float yborder;
  getStringSize("c",xborder,yborder);
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
    glColor3ubv(poptions.fillcolour.RGB());
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBegin(GL_POLYGON);
      glVertex2f(x1title,y2title);
      glVertex2f(x1title,y1title);
      glVertex2f(x2title,y1title);
      glVertex2f(x2title,y2title);
    glEnd();

    //draw title
    glColor4ubv(poptions.textcolour.RGBA());
    float titley1 = y2title-yborder-maxheight/2;
    for (int i=0;i<ntitle;i++){
      fp->drawStr(vtitlestring[i].cStr(),(x1title+xborder),titley1);
      titley1 -= maxheight;
    }
  }

  // draw table
  if (showplot){

    //draw table background
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4ubv(poptions.fillcolour.RGBA());
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBegin(GL_POLYGON);
    glVertex2f(x1table,y1table);
    glVertex2f(x1table,y1title);
    glVertex2f(x2table,y1title);
    glVertex2f(x2table,y1table);
    glEnd();

    glDisable(GL_BLEND);

    // draw table
    float x1box = x1table + xborder;
    float x2box = x1box   + 4*xborder;
    float y2box = y1title - yborder;
    float y1box = y2box   - maxheight;
    ImageGallery ig;
    glEnable(GL_POLYGON_STIPPLE);
    for (int i=0;i<ncolours;i++){
      if(colourcodes[i].plotBox){
	//draw colour/pattern box
	// draw background of colour/pattern boxes
	glColor3ubv(poptions.fillcolour.RGB());
	glBegin(GL_POLYGON);
          glVertex2f(x1box,y1box);
          glVertex2f(x1box,y2box);
          glVertex2f(x2box,y2box);
          glVertex2f(x2box,y1box);
        glEnd();
	if(colourcodes[i].pattern.exists()){
	  GLubyte* p=ig.getPattern(colourcodes[i].pattern);
	  if(p==0)
	    glPolygonStipple(solid);
	  else
	    glPolygonStipple(p);
	}else{
	  glPolygonStipple(solid);
	}
	glColor4ubv(colourcodes[i].colour.RGBA());
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBegin(GL_POLYGON);
	  glVertex2f(x1box,y1box);
	  glVertex2f(x1box,y2box);
	  glVertex2f(x2box,y2box);
	  glVertex2f(x2box,y1box);
	glEnd();

	// draw border of colour/pattern box
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glBegin(GL_POLYGON);
  	  glVertex2f(x1box,y1box);
          glVertex2f(x1box,y2box);
          glVertex2f(x2box,y2box);
	  glVertex2f(x2box,y1box);
        glEnd();
      }
      //draw textstring
      glColor4ubv(poptions.textcolour.RGBA());
      miString cstring = colourcodes[i].colourstr;
      fp->drawStr(cstring.cStr(),(x2box+xborder),(y1box+0.8*yborder));
      y2box -= maxheight;
      y1box -= maxheight;
      UpdateOutput();
    }
      glDisable(GL_POLYGON_STIPPLE);
  }

#ifdef DEBUGPRINT
  cerr << "++ Returning from Legend::plot() ++" << endl;
#endif

  return true;
}


void LegendPlot::showSatTable(int x,int y)
{
  // if x,y inside title bar, then the table should be hidden or shown

  //convert x and y...
  float xpos=x*fullrect.width()/pwidth + fullrect.x1;
  float ypos=y*fullrect.height()/pheight + fullrect.y1;

  //now check if it's inside title bar
  if (xpos>x1title && xpos<x2title && ypos>y1title && ypos<y2title)
    showplot = !showplot;


}



bool LegendPlot::inSatTable(int x,int y)
{

  //convert x and y...
  float xpos=x*fullrect.width()/pwidth + fullrect.x1;
  float ypos=y*fullrect.height()/pheight + fullrect.y1;

  //now check if it's inside title bar
  if (xpos>x1title && xpos<x2title && ypos>y1title && ypos<y2title)
    return true;
  else return false;

}


void LegendPlot::moveSatTable(int x1,int y1, int x2, int y2)
{

  float deltax = (x2-x1)/pwidth;
  float deltay = (y2-y1)/pheight;

  xRatio = xRatio - deltax;
  yRatio = yRatio - deltay;
  //check that table doesn't disappear out of picture
  if (xRatio < 0.0) xRatio = 0.0;
  if (yRatio < 0.02) yRatio = 0.02;
  if (xRatio > 0.98) xRatio = 0.98;
  if (yRatio > 0.99) yRatio = 0.99;


}

float LegendPlot::height()
{
  int ncolours = colourcodes.size();
  if(!ncolours) return 0.0;

  fp->set(poptions.fontname,poptions.fontface,poptions.fontsize);
  float width,height,maxwidth=0,maxheight=0,titlewidth=0;

  //colour code strings
  for (int i=0; i<ncolours; i++){
    miString cstring;
    cstring = colourcodes[i].colourstr;
    getStringSize(cstring, width, height);
    if (height>maxheight) maxheight= height;
    if (width>maxwidth) maxwidth= width;
  }

  //title
  int ntitle=0;
  if(titlestring.exists()){
    getStringSize(titlestring, titlewidth, height);
    vector<miString> vtitlestring;
    if(titlewidth>maxwidth){
      vtitlestring = titlestring.split(" ");
    } else {
      vtitlestring.push_back(titlestring);
    }
    ntitle = vtitlestring.size();
    titlewidth = 0;
    for (int i=0; i<ntitle; i++){
      getStringSize(vtitlestring[i], width, height);
      if (height>maxheight)   maxheight= height;
    }
  }

  float xborder;
  float yborder;
  getStringSize("c",xborder,yborder);
  yborder /=4;
  float titleheight = maxheight*ntitle;
  float tableheight = maxheight*ncolours + 2*yborder;

  return (tableheight + titleheight);
}

float LegendPlot::width()
{

  int ncolours = colourcodes.size();
  if(!ncolours) return 0.0;

  fp->set(poptions.fontname,poptions.fontface,poptions.fontsize);
  float width,height,maxwidth=0,titlewidth=0;

  //colour code strings
  for (int i=0; i<ncolours; i++){
    miString cstring;
    cstring = colourcodes[i].colourstr;
    getStringSize(cstring, width, height);
    if (width>maxwidth) maxwidth= width;
  }

  //title
  getStringSize(titlestring, titlewidth, height);
  vector<miString> vtitlestring;
  if(titlewidth>maxwidth){
    vtitlestring = titlestring.split(" ");
  } else {
    vtitlestring.push_back(titlestring);
  }
  int ntitle = vtitlestring.size();
  titlewidth = 0;
  for (int i=0; i<ntitle; i++){
    getStringSize(vtitlestring[i], width, height);
    if (width>titlewidth) titlewidth = width;
  }

  float xborder;
  float yborder;
  getStringSize("c",xborder,yborder);
  xborder /=2;
  titlewidth  = titlewidth + 2*xborder;
  float tablewidth  = maxwidth + 6*xborder;
  if(titlewidth < tablewidth ) titlewidth = tablewidth;
  return titlewidth;
}
