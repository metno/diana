/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2021 met.no

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

#include "diComplexSymbolPlot.h"

#include "diGLPainter.h"
#include "diGlUtilities.h"
#include "diWeatherSymbol.h"

#include <puTools/miStringFunctions.h>

#include <QPolygonF>

#define MILOGGER_CATEGORY "diana.ComplexSymbolPlot"
#include <miLogger/miLogging.h>

using namespace::miutil;

//Define symbols from font
#define SIG1SYMBOL 248
#define SIG2SYMBOL 249
#define SIG3SYMBOL 250
#define SIG4SYMBOL 251
#define RIGHTARROW 247
#define LEFTARROW  246
#define HIGHSYMBOL 243
#define LOWSYMBOL  242
#define THUNDERSYMBOL  119
#define CROSS  252
#define NEW_CROSS  255
#define MOUNTAINWAVESYMBOL 130
#define VULCANOSYMBOL 226
#define FOGSYMBOL 62
#define SNOWSYMBOL 254
#define WIDESPREADBRSYMBOL 136
#define MOUNTAINOBSCURATIONSYMBOL 106
#define HAILSYMBOL 238
#define SNOWSHOWERSYMBOL 114
#define SHOWERSYMBOL 109
#define FZRASYMBOL 93

namespace {

struct ChangeColour
{
  ChangeColour(DiGLPainter* gl, const Colour& c)
      : gl_(gl)
      , restore_(gl_->getColour())
  {
    gl_->setColour(c);
  }
  ~ChangeColour() { gl_->setColour(restore_); }

  DiGLPainter* gl_;
  const Colour restore_;
};

} // namespace

//static variables
// text used in new complex symbols

std::vector <std::string> ComplexSymbolPlot::currentSymbolStrings; //symbolstrings
std::vector <std::string> ComplexSymbolPlot::currentXStrings; //xtext
std::set <std::string> ComplexSymbolPlot::clist; //texts used in combobox
float ComplexSymbolPlot::textShrink=1.5;

ComplexSymbolPlot::ComplexSymbolPlot(int drawIndex)
{
  METLIBS_LOG_SCOPE();
  xvisible=false;
  nstringsvisible=0;
  symbolStrings=currentSymbolStrings;
  xstrings=currentXStrings;
  //rightarrow and thunder - no white box
//  if (drawIndex==1018 || drawIndex==1021)
//    whiteBox=false;
//  else
//    whiteBox=true;
  whiteBox = isComplexText(drawIndex);
}


void ComplexSymbolPlot::initStrings(int drawIndex)
{
  METLIBS_LOG_SCOPE();
  switch (drawIndex){
  case 1000:
    if (symbolStrings.size()==0)
      symbolStrings.push_back("");
    break;
  case 1007:
    if (xstrings.size()==0)
      xstrings.push_back("");
    if (xstrings.size()==1)
      xstrings.push_back(xstrings[0]);
    break;
  case 900:
    if (symbolStrings.size()==0)
      symbolStrings.push_back("");
    if (xstrings.size()==0)
      xstrings.push_back("");
    break;
  case 2000:
    if (symbolStrings.size()==0)
      symbolStrings.push_back("");
    if (symbolStrings.size()==1)
      symbolStrings.push_back(symbolStrings[0]);
    break;
  case 3000:
    if (symbolStrings.size()==0)
      symbolStrings.push_back("");
    break;
  case 3001:
    if (symbolStrings.size()==0)
      symbolStrings.push_back("");
    break;
  case 1033:
    if (symbolStrings.size()==0)
      currentSymbolStrings.push_back("");
    break;
  }
}


void ComplexSymbolPlot::initCurrentStrings(int drawIndex)
{
  METLIBS_LOG_SCOPE();
  currentSymbolStrings.clear();
  currentXStrings.clear();
  switch (drawIndex){
  case 1000:
    currentSymbolStrings.push_back("");
    break;
  case 1007:
    currentXStrings.push_back("");
    currentXStrings.push_back("");
    break;
  case 1008:
    currentXStrings.push_back("");
    currentXStrings.push_back("");
    break;
  case 1009:
    currentXStrings.push_back("");
    currentXStrings.push_back("");
    break;
  case 1010:
    currentXStrings.push_back("");
    currentXStrings.push_back("");
    break;
  case 1011:
    currentXStrings.push_back("");
    currentXStrings.push_back("");
    break;
  case 1012:
    currentXStrings.push_back("");
    currentXStrings.push_back("");
    break;
  case 1013:
    currentXStrings.push_back("");
    currentXStrings.push_back("");
    break;
  case 1014:
    currentXStrings.push_back("");
    currentXStrings.push_back("");
    currentSymbolStrings.push_back("");
    break;
  case 1015:
    currentXStrings.push_back("");
    currentXStrings.push_back("");
    currentSymbolStrings.push_back("");
    break;
  case 1016:
    currentXStrings.push_back("");
    currentXStrings.push_back("");
    currentSymbolStrings.push_back("");
    currentSymbolStrings.push_back("");
    break;
  case 1018:
    //currentSymbolStrings.push_back("");
    break;
  case 1019:
    //currentSymbolStrings.push_back("");
    break;
  case 1020:
    //currentSymbolStrings.push_back("9xx");
    break;
  case 1021:
    //currentSymbolStrings.push_back("1xxx");
    break;
  case 1022:
    //currentSymbolStrings.push_back("1xxx");
    break;
  case 1023:
    currentXStrings.push_back("");
    currentXStrings.push_back("");
    currentSymbolStrings.push_back("BKN/OVC STF");
    break;
  case 1024:
    currentXStrings.push_back("");
    currentXStrings.push_back("");
    currentSymbolStrings.push_back("BKN/OVC STF");
    break;
  case 1027:
    currentSymbolStrings.push_back("");
    break;
  case 1028:
    currentSymbolStrings.push_back("");
    break;
  case 1029:
    currentSymbolStrings.push_back("");
    break;
  case 1033:
    currentSymbolStrings.push_back("");
    break;
  case 1034:
    currentSymbolStrings.push_back("");
    currentSymbolStrings.push_back("");
    break;
  case 1036:
    currentSymbolStrings.push_back("0°:x"); // was "0<deg>:x";
    currentSymbolStrings.push_back("");
    break;
 case 1040:
    currentXStrings.push_back("");
    currentXStrings.push_back("");
    currentSymbolStrings.push_back("");
    currentSymbolStrings.push_back("");
    break;
  case 900:
    currentXStrings.push_back("");
    currentSymbolStrings.push_back("");
    break;
  case 2000:
    currentSymbolStrings.push_back("");
    currentSymbolStrings.push_back("");
    break;
  case 3000:
    currentSymbolStrings.push_back("");
    break;
  case 3001:
    currentSymbolStrings.push_back("");
    break;
  }
}


void ComplexSymbolPlot::draw(DiGLPainter* gl, int drawIndex, float x,float y,int size,float rot)
{
  METLIBS_LOG_SCOPE(LOGVAL(drawIndex) << LOGVAL(x) << LOGVAL(y));
  symbolSizeToPlot=size;
  xvisible=false;
  nstringsvisible=0;
  diutil::GlMatrixPushPop pushpop(gl);
  gl->Translatef(x, y, 0.0);
  gl->Rotatef(rot,0.0,0.0,1.0);
  //scale linewidth to symbolsize
  float linewidth=symbolSizeToPlot/50+1;
  gl->LineWidth(linewidth);
  switch (drawIndex){
  case 1000:
    symbolSizeToPlot=int (symbolSizeToPlot/textShrink);
    drawSigText(gl, 0,0,whiteBox);
    break;
  case 1001:
    drawSig1(gl, 0,0,SIG1SYMBOL);
    break;
  case 1002:
    drawSig1(gl, 0,0,SIG2SYMBOL);
    break;
  case 1003:
    drawSig1(gl, 0,0,SIG3SYMBOL);
    break;
  case 1004:
    drawSig1(gl, 0,0,SIG4SYMBOL);
    break;
  case 1005:
    drawSig5(gl, 0,0);
    break;
  case 1006:
    drawSig6(gl, 0,0);
    break;
  case 1007:
    symbolSizeToPlot=int (symbolSizeToPlot/textShrink);
    drawSig7(gl, 0,0);
    break;
  case 1008:
    drawSig8(gl, 0,0);
    break;
  case 1009:
    drawSig9(gl, 0,0);
    break;
  case 1010:
    drawSig10(gl, 0,0);
    break;
  case 1011:
    drawSig11(gl, 0,0);
    break;
  case 1012:
    drawSig12(gl, 0,0);
    break;
  case 1013:
    drawSig13(gl, 0,0);
    break;
  case 1014:
    drawSig14(gl, 0,0);
    break;
  case 1015:
    drawSig15(gl, 0,0);
    break;
  case 1016:
    drawSig16(gl, 0,0);
    break;
  case 1018:
    symbolSizeToPlot=int (symbolSizeToPlot);
    drawSig1(gl, 0,0,RIGHTARROW);
    break;
  case 1019:
    symbolSizeToPlot=int (symbolSizeToPlot);
    drawSig1(gl, 0,0,LOWSYMBOL);
    break;
  case 1020:
    symbolSizeToPlot=int (symbolSizeToPlot);
    drawSig1(gl, 0,0,HIGHSYMBOL);
    break;
  case 1021:
    drawSig1(gl, 0,0,THUNDERSYMBOL);
    break;
  case 1022:
    drawSig1(gl, 0,0,CROSS);
    break;
  case 1023:
    drawSig14(gl, 0,0);
    break;
  case 1024:
    drawSig15(gl, 0,0);
    break;
  case 1025:
    drawSig1(gl, 0,0,MOUNTAINWAVESYMBOL);
    break;
  case 1026:
    drawSig1(gl, 0,0,VULCANOSYMBOL);
    break;
  case 1027:
    drawSig27(gl, 0,0);
    break;
  case 1028:
    drawSig28(gl, 0,0);
    break;
  case 1029:
    drawSig29(gl, 0,0);
    break;
  case 1030:
    drawSig30(gl, 0,0);
    break;
  case 1031:
    drawSig31(gl, 0,0);
    break;
  case 1032:
    drawSig32(gl, 0,0);
    break;
  case 1033:
    drawSig33(gl, 0,0);
    break;
  case 1034:
    drawSig34(gl, 0,0);
    break;
  case 1035:
    drawSig1(gl, 0,0,NEW_CROSS);
    break;
  case 1036:
    drawSig36(gl, 0,0);
    break;
  case 1037:
    drawSig1(gl, 0,0,WIDESPREADBRSYMBOL);
    break;
  case 1038:
    drawSig1(gl, 0,0,MOUNTAINOBSCURATIONSYMBOL);
    break;
  case 1039:
    drawSig1(gl, 0,0,HAILSYMBOL);
    break;
  case 1040:
    drawSig40(gl, 0,0);
    break;
  case 1041:
    drawSig1(gl, 0,0,FOGSYMBOL);
    break;
  case 1042:
    drawSig1(gl, 0,0,SNOWSYMBOL);
    break;
  case 1043:
    symbolSizeToPlot=int (symbolSizeToPlot);
    drawSig1(gl, 0,0,SNOWSHOWERSYMBOL);
    break;
  case 1044:
    symbolSizeToPlot=int (symbolSizeToPlot);
    drawSig1(gl, 0,0,SHOWERSYMBOL);
    break;
  case 1045:
    drawSig1(gl, 0,0,FZRASYMBOL);
    break;
  case 900:
    symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
    drawColoredSigText(gl, 0,0);
    break;
  case 2000:
    symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
    drawDoubleSigText(gl, 0,0);
    break;
  case 3000:
    symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
    drawSigEditText(gl, 0,0,whiteBox);
    break;
 // case 3001:
   // symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
   // drawSigTextBox(gl);
   // break;
  default:
    METLIBS_LOG_WARN("ComplexSymbolPlot::drawComplexSymbols - Index " <<
      drawIndex << " not defined ");
    return;
  }
}

void ComplexSymbolPlot::drawSymbol(DiGLPainter* gl, int index,float x,float y)
{
  METLIBS_LOG_SCOPE();
  float cw,ch;
  gl->setFont(diutil::METSYMBOLFONT, poptions.fontface, symbolSizeToPlot);
  gl->getCharSize(index,cw,ch);
  gl->drawChar(index,x-cw/2,y-ch/2,0.0);
}


void ComplexSymbolPlot::drawSigString(DiGLPainter* gl, float x,float y, bool whitebox)
{
  METLIBS_LOG_SCOPE();
  if (whitebox){
    drawBox(gl, 1999,x,y);
  }
  float cw,ch;
  getComplexSize(gl, 1999,cw,ch);
  gl->setFont(poptions.fontname,poptions.fontface,symbolSizeToPlot);
  gl->drawText(sigString,x-0.45*cw,y-0.4*ch,0.0);
}


void ComplexSymbolPlot::drawSigTextBoxString(DiGLPainter* gl, float& x,float& y, bool whitebox)
{
  METLIBS_LOG_SCOPE();
  float cw,ch;
  getComplexSize(gl, 1999,cw,ch);
  gl->setFont(diutil::SCALEFONT, diutil::F_BOLD, 12);
  gl->drawText(sigString,x+0.45 ,y-0.4,0.0);
  METLIBS_LOG_DEBUG(LOGVAL(sigString));
  nstringsvisible=10;
}

void ComplexSymbolPlot::drawSigEditString(DiGLPainter* gl, float& x,float& y, bool whitebox)
{
  METLIBS_LOG_SCOPE(LOGVAL(x) << LOGVAL(y));
  float cw,ch;
  getComplexSize(gl, 1999,cw,ch);
  gl->setFont(poptions.fontname,poptions.fontface,symbolSizeToPlot);
  gl->drawText(sigString,x+0.45 ,y-0.4,0.0);
  METLIBS_LOG_DEBUG(LOGVAL(sigString));
  nstringsvisible=10;
}

void ComplexSymbolPlot::drawSigText(DiGLPainter* gl, float x,float y, bool whitebox)
{
  METLIBS_LOG_SCOPE();
  initStrings(1000);
  if (symbolStrings.size()>0)
    sigString=symbolStrings[0];
  drawSigString(gl, x,y,whitebox);
  nstringsvisible=1;
}

// Drawing texbox on right low corner of the frame
//void ComplexSymbolPlot::drawSigTextBox(){
void ComplexSymbolPlot::drawTextBox(DiGLPainter* gl, int drawIndex,int size, float rot)
{
  METLIBS_LOG_SCOPE();
  float x, y;
  x = 5.57101e+06;
  y = 3.40754e+06;
  METLIBS_LOG_DEBUG(LOGVAL(drawIndex) << LOGVAL(x) << LOGVAL(y));
  symbolSizeToPlot=size;
  xvisible=false;
  diutil::GlMatrixPushPop pushpop(gl);
  gl->Translatef(x, y, 0.0);
  gl->Rotatef(rot,0.0,0.0,1.0);
  //scale linewidth to symbolsize
  float linewidth=symbolSizeToPlot/50+1;
  gl->LineWidth(linewidth);
  drawBox(gl, drawIndex,0,0,false);
  gl->setColour(Colour::BLACK);

  float sx, sy, cw, ch;
  sx = 0.0; sy = 0.0;
  bool whitebox = false;
  initStrings(3000);
  for (unsigned int j= 0 ; j < symbolStrings.size(); j++) {
    sigString=symbolStrings[j];
    METLIBS_LOG_DEBUG(LOGVAL(sigString));
    getComplexSize(gl, 1999,cw,ch);
    drawSigTextBoxString(gl, sx,sy,whitebox);
    sy -= 0.5*ch;
  }
}

void ComplexSymbolPlot::drawColoredSigText(DiGLPainter* gl, float x,float y, bool whitebox)
{
  METLIBS_LOG_SCOPE(LOGVAL(x) << LOGVAL(y));

  float sw,sh;
  initStrings(1000);
  if (symbolStrings.size()>0)
    sigString=symbolStrings[0];
  getComplexSize(gl, 1999,sw,sh);
  drawSigText(gl, x-sw/2,y,whiteBox);
  symbolSizeToPlot= int(symbolSizeToPlot/textShrink);
  initStrings(900);
  getComplexSize(gl, 900,sw,sh);
  drawSigNumber(gl, x+sw/2,y);
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
  nstringsvisible=1;
}


// Plotting multiline text
void ComplexSymbolPlot::drawSigEditText(DiGLPainter* gl, float x,float y, bool whitebox)
{
  METLIBS_LOG_SCOPE(LOGVAL(x) << LOGVAL(y));
  float cw,ch;
  initStrings(3000);
  if (symbolStrings.size()>0)
    for (unsigned int j= 0 ; j < symbolStrings.size(); j++)
    {
       sigString=symbolStrings[j];
       METLIBS_LOG_DEBUG(LOGVAL(sigString));
       getComplexSize(gl, 1999,cw,ch);
       drawSigEditString(gl, x,y,whitebox);
       y -= 0.9*ch;
    }
  nstringsvisible=10;
}

void ComplexSymbolPlot::drawDoubleSigText(DiGLPainter* gl, float x,float y, bool whitebox)
{
  METLIBS_LOG_SCOPE();
  float cw1,ch1;
  float cw2,ch2;
  initStrings(2000);
  if (symbolStrings.size()>0)
    sigString=symbolStrings[0];
  getComplexSize(gl, 1999,cw1,ch1);
  drawSigString(gl, x,y+ch1/2,whitebox);
  if (symbolStrings.size()>1)
    sigString=symbolStrings[1];
  getComplexSize(gl, 1999,cw2,ch2);
  drawSigString(gl, x,y-ch2/2,whitebox);
  nstringsvisible=2;
}

void ComplexSymbolPlot::drawDoubleSigTextAndSymbol(DiGLPainter* gl, int symbol, float x,float y)
{
  METLIBS_LOG_SCOPE();
  float cw1,ch1;
  float cw2,ch2;
  float sw,sh;
  initStrings(2000);
  if (symbolStrings.size()>0)
    sigString=symbolStrings[0];
  getComplexSize(gl, symbol,sw,sh);
  getComplexSize(gl, 1999,cw1,ch1);
  drawSymbol(gl, symbol,x-cw1/2,y+ch1/2);
  drawSigString(gl, x+sw/2,y+ch1/2);
  if (symbolStrings.size()>1)
    sigString=symbolStrings[1];
  getComplexSize(gl, 1999,cw2,ch2);
  drawSigString(gl, x,y-ch2/2);
  nstringsvisible=2;
}

void ComplexSymbolPlot::drawSigNumber(DiGLPainter* gl, float x,float y)
{
  if (xstrings.size()<1)
    initStrings(900);
  if(whiteBox){
    drawBox(gl, 900,x,y);
  }
  gl->setFont(poptions.fontname,poptions.fontface,symbolSizeToPlot);
  float cw1,ch1;
  gl->getTextSize(xstrings[0],cw1,ch1);
  gl->drawText(xstrings[0],x-cw1/2,y-1.1*ch1,0.0);
  float sw,sh;
  getComplexSize(gl, 900,sw,sh);
}



void ComplexSymbolPlot::drawSig1(DiGLPainter* gl, float x,float y, int metSymbol)
{
  if (whiteBox) {
    drawBox(gl, metSymbol,0,0);
  }
  drawSymbol(gl, metSymbol,x,y);
}

void ComplexSymbolPlot::drawSig5(DiGLPainter* gl, float x,float y)
{
  if (whiteBox) {
    drawBox(gl, 1005,x,y);
  }
  float cw,ch;
  getComplexSize(gl, 1001,cw,ch);
  drawSymbol(gl, SIG1SYMBOL,x-0.4*cw,y+0.3*ch);
  getComplexSize(gl, 1002,cw,ch);
  drawSymbol(gl, SIG2SYMBOL,x+0.4*cw,y-0.5*ch);

  getComplexSize(gl, 1005,cw,ch);
  gl->drawLine(x-0.3*cw, y-0.3*ch, x+0.3*cw, y+0.3*ch);
}


void ComplexSymbolPlot::drawSig6(DiGLPainter* gl, float x,float y)
{
  if (whiteBox) {
    drawBox(gl, 1006,x,y);
  }
  float cw,ch;
  getComplexSize(gl, 1003,cw,ch);
  drawSymbol(gl, SIG3SYMBOL,x-0.3*cw,y+1.05*ch);
  getComplexSize(gl, 1004,cw,ch);
  drawSymbol(gl, SIG4SYMBOL,x+0.45*cw,y-0.5*ch);
  getComplexSize(gl, 1006,cw,ch);
  gl->drawLine(x-0.35*cw, y-0.35*ch, x+0.35*cw, y+0.35*ch);
}


void ComplexSymbolPlot::drawSig7(DiGLPainter* gl, float x,float y)
{
  if (xstrings.size()<2)
    initStrings(1007);
  if(whiteBox){
    drawBox(gl, 1007,x,y);
  }
  gl->setFont(poptions.fontname,poptions.fontface,symbolSizeToPlot);
  float cw1,ch1;
  float cw2,ch2;
  gl->getTextSize(xstrings[0],cw1,ch1);
  gl->getTextSize(xstrings[1],cw2,ch2);
  gl->drawText(xstrings[0],x-cw1/2,y+0.1*ch1,0.0);
  gl->drawText(xstrings[1],x-cw2/2,y-1.1*ch2,0.0);
  float sw,sh;
  getComplexSize(gl, 1007,sw,sh);
  gl->drawLine(x-sw/2, y, x+sw/2, y);
  xvisible=true;
}


void ComplexSymbolPlot::drawSig8(DiGLPainter* gl, float x,float y)
{
  float sw,sh;
  getComplexSize(gl, 1001,sw,sh);
  drawSig1(gl, x-sw/2,y,SIG1SYMBOL);
  initStrings(1007);
  symbolSizeToPlot= int(symbolSizeToPlot/textShrink);
  getComplexSize(gl, 1007,sw,sh);
  drawSig7(gl, x+sw/2,y);
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}


void ComplexSymbolPlot::drawSig9(DiGLPainter* gl, float x,float y)
{
  float sw,sh;
  getComplexSize(gl, 1002,sw,sh);
  drawSig1(gl, x-sw/2,y,SIG2SYMBOL);
  initStrings(1007);
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  getComplexSize(gl, 1007,sw,sh);
  drawSig7(gl, x+sw/2,y);
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}


void ComplexSymbolPlot::drawSig10(DiGLPainter* gl, float x,float y)
{
  float sw,sh;
  getComplexSize(gl, 1005,sw,sh);
  drawSig5(gl, x-sw/2,y);
  initStrings(1007);
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  getComplexSize(gl, 1007,sw,sh);
  drawSig7(gl, x+sw/2,y);
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}

void ComplexSymbolPlot::drawSig11(DiGLPainter* gl, float x, float y)
{
  float sw,sh;
  getComplexSize(gl, 1003,sw,sh);
  drawSig1(gl, x-sw/2,y,SIG3SYMBOL);
  initStrings(1007);
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  getComplexSize(gl, 1007,sw,sh);
  drawSig7(gl, x+sw/2,y);
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}

void ComplexSymbolPlot::drawSig12(DiGLPainter* gl, float x, float y)
{
  float sw,sh;
  getComplexSize(gl, 1004,sw,sh);
  drawSig1(gl, x-sw/2,y,SIG4SYMBOL);
  initStrings(1007);
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  getComplexSize(gl, 1007,sw,sh);
  drawSig7(gl, x+sw/2,y);
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}

void ComplexSymbolPlot::drawSig13(DiGLPainter* gl, float x, float y)
{
  float sw,sh;
  getComplexSize(gl, 1006,sw,sh);
  drawSig6(gl, x-sw/2,y);
  initStrings(1007);
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  getComplexSize(gl, 1007,sw,sh);
  drawSig7(gl, x+sw/2,y);
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}

void ComplexSymbolPlot::drawSig14(DiGLPainter* gl, float x,float y)
{
  float sw,sh;
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  initStrings(1000);
  getComplexSize(gl, 1000,sw,sh);
  drawSigText(gl, x-sw/2,y,whiteBox);
  initStrings(1007);
  getComplexSize(gl, 1007,sw,sh);
  drawSig7(gl, x+sw/2,y);
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}


void ComplexSymbolPlot::drawSig15(DiGLPainter* gl, float x, float y)
{
  float sw,sh;
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  initStrings(1000);
  getComplexSize(gl, 1000,sw,sh);
  drawSigText(gl, x,y+sh/2,whiteBox);
  initStrings(1007);
  getComplexSize(gl, 1007,sw,sh);
  drawSig7(gl, x,y-sh/2);
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}

void ComplexSymbolPlot::drawSig40(DiGLPainter* gl, float x, float y)
{
  float sw,sh;
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  initStrings(2000);
  getComplexSize(gl, 2000,sw,sh);
  drawDoubleSigText(gl, x,y+sh/1.75,whiteBox);
  initStrings(1007);
  getComplexSize(gl, 1007,sw,sh);
  drawSig7(gl, x,y-sh/1.75);
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}

void ComplexSymbolPlot::drawSig16(DiGLPainter* gl, float x, float y)
{
  //draw two texts
  float sw,sh;
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  initStrings(2000);
  getComplexSize(gl, 2000,sw,sh);
  drawDoubleSigText(gl, x-sw/2,y,whiteBox);
  initStrings(1007);
  getComplexSize(gl, 1007,sw,sh);
  drawSig7(gl, x+sw/2,y);
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}

void ComplexSymbolPlot::drawSig17(DiGLPainter* gl, float x,float y)
{
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  if (symbolStrings.size()>0)
    sigString=symbolStrings[0];
  if(whiteBox) {
    drawBox(gl, 1000,x,y);
  }
  drawSigString(gl, x,y);
  nstringsvisible=1;
  float sw,sh;
  getComplexSize(gl, 1000,sw,sh);
  gl->drawRect(false, x-0.5*sw, y-0.5*sh, x+0.5*sw, y+0.5*sh);
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}

void ComplexSymbolPlot::drawSig22(DiGLPainter* gl, float x,float y)
{
  drawBox(gl, 1022,x,y);
  drawSymbol(gl, CROSS,x,y);
}

//Sea temp, blue circle
void ComplexSymbolPlot::drawSig27(DiGLPainter* gl, float x, float y)
{
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  if (symbolStrings.size()>0)
    sigString=symbolStrings[0];
  drawCircle(gl, 1000,x,y,true);
  drawSigString(gl, x,y,false);
  nstringsvisible=1;
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}

//Mean SFC wind, red diamond
void ComplexSymbolPlot::drawSig28(DiGLPainter* gl, float x, float y)
{
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  if (symbolStrings.size()>0)
    sigString=symbolStrings[0];
  drawDiamond(gl, 1000,x,y);
  drawSigString(gl, x,y,false);
  nstringsvisible=1;
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}

// Sea state, black flag
void ComplexSymbolPlot::drawSig29(DiGLPainter* gl, float x, float y)
{
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink/2);
  if (symbolStrings.size()>0)
    sigString=symbolStrings[0];
  drawFlag(gl, 1000, x, y);
  drawSigString(gl, x,y,false);
  nstringsvisible=1;
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink*2);
}

// Freezing fog
void ComplexSymbolPlot::drawSig30(DiGLPainter* gl, float x, float y)
{
  if(whiteBox) {
    drawBox(gl, 1030,x,y);
  }
  drawSymbol(gl, FOGSYMBOL,x,y);
  ChangeColour cc(gl, borderColour);
  float cw,ch;
  gl->setFont(poptions.fontname,poptions.fontface,symbolSizeToPlot/2);
  gl->getTextSize("V",cw,ch);
  gl->drawText("V",x-cw/2,y-ch/2,0.0);
}

//Nuclear
void ComplexSymbolPlot::drawSig31(DiGLPainter* gl, float x, float y)
{
  drawNuclear(gl, x,y);
}

//precipitation, green lines
void ComplexSymbolPlot::drawSig32(DiGLPainter* gl, float x, float y)
{
  drawPrecipitation(gl, x,y);
}

//Visibility, black rectangular box
void ComplexSymbolPlot::drawSig33(DiGLPainter* gl, float x, float y)
{
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  sigString=symbolStrings[0];
  //  if (whiteBox) drawBox(gl, 1000,x,y,true);
  drawBox(gl, 1000,x,y,false);

  poptions.fontface = diutil::F_ITALIC;
  drawSigString(gl, x,y);
  nstringsvisible=1;
}

//Vulcano box,
void ComplexSymbolPlot::drawSig34(DiGLPainter* gl, float x, float y){
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  if (whiteBox) drawBox(gl, 1034,x,y,true);
  drawBox(gl, 1034,x,y,false);
  drawDoubleSigTextAndSymbol(gl, VULCANOSYMBOL,x,y);
}

//Freezing level (new)
void ComplexSymbolPlot::drawSig36(DiGLPainter* gl, float x, float y)
{
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  drawCircle(gl, 1036,x,y,false);
  if (symbolStrings.size()==1  ||
      (symbolStrings.size()==2 && symbolStrings[1]=="")){
    sigString=symbolStrings[0];
    drawSigString(gl, x,y,false);
    nstringsvisible=1;
  } else if (symbolStrings.size()==2){
    drawDoubleSigText(gl, x,y,false);
  }
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}


void ComplexSymbolPlot::drawBox(DiGLPainter* gl, int index,float x, float y,
    bool fill)
{
  METLIBS_LOG_SCOPE(LOGVAL(x) << LOGVAL(y) << LOGVAL(fill));

  float sw,sh;
  getComplexSize(gl, index,sw,sh);
  METLIBS_LOG_DEBUG(LOGVAL(sw) << LOGVAL(sh));

  ChangeColour cc(gl, fill ? Colour::WHITE : borderColour);
  if (index == 3001) {
    gl->drawRect(fill, x-25000, y-295000, x+1100000, y+37000);
  } else {
    gl->drawRect(fill, x-0.5*sw, y-0.5*sh, x+0.6*sw, y+0.5*sh);
  }
}

void ComplexSymbolPlot::drawFlag(DiGLPainter* gl, int index, float x, float y)
{
  float sw,sh;
  std::string s = "10";
  gl->setFont(poptions.fontname,poptions.fontface,symbolSizeToPlot);
  gl->getTextSize(s,sw,sh);
  sw *= 1.1*1.5;
  sh *= 1.2*1.5;
  const int NSTEP = 10;
  const float radius = sw/2, y2 = y+0.5*sh, y3 = y-0.5*sh, tiltEnd = sh/3, tiltStep = tiltEnd / NSTEP;

  float rcos[NSTEP+1], rsin2[NSTEP+1];
  for (int i=0; i<=NSTEP; ++i) {
    rcos[i]  = cos(i*M_PI/NSTEP) * radius;
    rsin2[i] = sin(i*M_PI/NSTEP) * radius * 0.5;
  }

  QPolygonF polygon;
  polygon << QPointF(x-sw, y3 + tiltEnd)
          << QPointF(x-sw, y2 + tiltEnd);

  for (int i=0; i<=NSTEP; ++i)
    polygon << QPointF(x - radius - rcos[i], y2 - rsin2[i] + (NSTEP-i)*tiltStep);
  for (int i=0; i<NSTEP; ++i)
    polygon << QPointF(x + radius - rcos[i], y2 + rsin2[i] - i*tiltStep);

  polygon << QPointF(x+sw, y2 - tiltEnd)
          << QPointF(x+sw, y3 - tiltEnd);

  for (int i=0; i<=NSTEP; ++i)
    polygon << QPointF(x + radius + rcos[i], y3 + rsin2[i] - (NSTEP-i)*tiltStep);
  for(int i=0; i<=NSTEP; ++i)
    polygon << QPointF(x - radius + rcos[i], y3 - rsin2[i] + i*tiltStep);

  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
  ChangeColour cc(gl, Colour::WHITE);
  gl->drawPolygon(polygon);

  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_LINE_LOOP);
  gl->setLineStyle(borderColour, 1);
  gl->drawPolyline(polygon);
}

void ComplexSymbolPlot::drawCircle(DiGLPainter* gl, int index,
    float x, float y, bool circle)
{
  float sw,sh;
  if(circle) {
    std::string s = "10";
    gl->setFont(poptions.fontname,poptions.fontface,symbolSizeToPlot);
    gl->getTextSize(s,sw,sh);
    sw=1.1*sw; sh=1.2*sh;
  } else {
    getComplexSize(gl, index,sw,sh);
  }

  //  if(symbolStrings.size()==2&& symbolStrings[1]!="") sh*=2;
  DiGLPainter::GLfloat xc,yc;
  DiGLPainter::GLfloat radius;
  if( circle) {
    radius = sw/1.5;
    sw = 0.0;
  } else {
    radius = sh/1.5;
    sw = sw/1.8;
  }

  //draw white background
  ChangeColour cc(gl, Colour::WHITE);
  if (whiteBox) {
    gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
    gl->Begin(DiGLPainter::gl_POLYGON);
    for(int i=0;i<50;i++){
      xc = sw/2+radius*sin(i*2*M_PI/100.0);
      yc = radius*cos(i*2*M_PI/100.0);
      gl->Vertex2f(xc,yc);
    }
    for(int i=50;i<100;i++){
      xc = radius*sin(i*2*M_PI/100.0)-sw/2;
      yc = radius*cos(i*2*M_PI/100.0);
      gl->Vertex2f(xc,yc);
    }
    gl->End();
  }

  //draw circle
  gl->setLineStyle(borderColour, 2);
  gl->Begin(DiGLPainter::gl_LINE_LOOP);
    for(int i=0;i<50;i++){
      yc = sw/2+radius*sin(i*2*M_PI/100.0);
      xc = radius*cos(i*2*M_PI/100.0);
      gl->Vertex2f(yc,xc);
    }
    for(int i=50;i<100;i++){
      yc = radius*sin(i*2*M_PI/100.0)-sw/2;
      xc = radius*cos(i*2*M_PI/100.0);
      gl->Vertex2f(yc,xc);
    }
  gl->End();
}

void ComplexSymbolPlot::drawDiamond(DiGLPainter* gl, int index,float x, float y)
{
  float sw,sh;
  std::string s = "10";
  gl->setFont(poptions.fontname,poptions.fontface,symbolSizeToPlot);
  gl->getTextSize(s,sw,sh);
  sw=1.1*sw; sh=1.2*sh;

  //draw white background
  ChangeColour cc(gl, Colour::WHITE);
  if (whiteBox) {
    gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
    gl->Begin(DiGLPainter::gl_POLYGON);
    gl->Vertex2f(0,y-sh);
    gl->Vertex2f(x-sw,0);
    gl->Vertex2f(0,y+sh);
    gl->Vertex2f(x+sw,0);
    gl->End();
  }

  //draw diamond
  gl->setLineStyle(borderColour, 2);
  gl->Begin(DiGLPainter::gl_LINE_LOOP);
  gl->Vertex2f(0, y - sh);
  gl->Vertex2f(x - sw, 0);
  gl->Vertex2f(0, y + sh);
  gl->Vertex2f(x + sw, 0);
  gl->End();
}

void ComplexSymbolPlot::drawNuclear(DiGLPainter* gl, float x, float y)
{
  float sw,sh;
  getComplexSize(gl, 1031,sw,sh);
  DiGLPainter::GLfloat xc,yc;
  DiGLPainter::GLfloat radius = sh/1.5;

  ChangeColour cc(gl, borderColour);
  gl->drawCircle(false, 0, 0, radius);

  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
  gl->Begin(DiGLPainter::gl_POLYGON);
    gl->Vertex2f(x,y);
    for(int i=0;i<100/6;i++){
      xc = radius*cos(i*2*M_PI/100.0);
      yc = radius*sin(i*2*M_PI/100.0);
      gl->Vertex2f(xc,yc);
    }
    gl->Vertex2f(x,y);
    for(int i=200/6;i<300/6;i++){
      xc = radius*cos(i*2*M_PI/100.0);
      yc = radius*sin(i*2*M_PI/100.0);
      gl->Vertex2f(xc,yc);
    }
    gl->Vertex2f(x,y);
    for(int i=400/6;i<500/6;i++){
      xc = radius*cos(i*2*M_PI/100.0);
      yc = radius*sin(i*2*M_PI/100.0);
      gl->Vertex2f(xc,yc);
    }
    gl->Vertex2f(x,y);
    gl->End();

    gl->setColour(Colour::WHITE);
    gl->Begin(DiGLPainter::gl_POLYGON);
    gl->Vertex2f(x,y);
    for(int i=100/6;i<200/6;i++){
      xc = radius*cos(i*2*M_PI/100.0);
      yc = radius*sin(i*2*M_PI/100.0);
      gl->Vertex2f(xc,yc);
    }
    gl->Vertex2f(x,y);
    for(int i=300/6;i<400/6;i++){
      xc = radius*cos(i*2*M_PI/100.0);
      yc = radius*sin(i*2*M_PI/100.0);
      gl->Vertex2f(xc,yc);
    }
    gl->Vertex2f(x,y);
    for(int i=500/6;i<600/6;i++){
      xc = radius*cos(i*2*M_PI/100.0);
      yc = radius*sin(i*2*M_PI/100.0);
      gl->Vertex2f(xc,yc);
    }

    gl->End();
}

void ComplexSymbolPlot::drawPrecipitation(DiGLPainter* gl, float x, float y)
{
  float sw,sh;
  getComplexSize(gl, 1026,sw,sh);

  gl->Rotatef(-45,0.0,0.0,1.0);

  if (whiteBox) {
    ChangeColour cc(gl, Colour::WHITE);
    gl->drawRect(true, -sw, -sh, sw, sh);
  }

  gl->LineWidth(3);
  gl->drawLine( sw/4,  sh/2-sh/4,  sw/4,  -sh/2-sh/4);
  gl->drawLine( sw/12, sh/2,       sw/12, -sh/2);
  gl->drawLine(-sw/12, sh/2-sh/4, -sw/12, -sh/2-sh/4);
  gl->drawLine(-sw/4,  sh/2,      -sw/4,  -sh/2);

  gl->Rotatef(45,0.0,0.0,1.0);
}

void ComplexSymbolPlot::getComplexSize(DiGLPainter* gl, int index, float& sw, float & sh)
{
  METLIBS_LOG_SCOPE();
  float cw,ch;
  float cw1,ch1;
  float cw2,ch2;
  //int size;
  diffx=0;
  diffy=0;
  if (index < 1000){
    gl->setFont(diutil::METSYMBOLFONT, poptions.fontface, symbolSizeToPlot);
    gl->getCharSize(index,cw,ch);
    sw=ch; sh=ch;
  }
  else{
    switch (index){
    case 1999:
      //sigstring
      gl->setFont(poptions.fontname,poptions.fontface,symbolSizeToPlot);
      gl->getTextSize(sigString,cw,ch);
      sw=1.1*cw; sh=1.2*ch;
      break;
    case 3001:
      if (symbolStrings.size()>1){
        gl->setFont(diutil::SCALEFONT, diutil::F_BOLD, 12);
        cw2=ch2=0;
        for (unsigned int i=0; i<symbolStrings.size(); i++){
          METLIBS_LOG_DEBUG(LOGVAL(cw2) << LOGVAL(ch2));
          gl->getTextSize(symbolStrings[i],cw1,ch1);
          if (cw1>cw2)  cw2=cw1;
          ch2 += ch1;
          METLIBS_LOG_DEBUG(LOGVAL(cw2) << LOGVAL(ch2));
        }
        sw = cw2 * 1.1;
        sh = ch2 * 1.1;
      }
      break;
    case 3000:
      //sigeditstring
      gl->setFont(poptions.fontname,poptions.fontface,symbolSizeToPlot);
      gl->getTextSize(sigString,cw,ch);
      sw=1.1*cw; sh=1.2*ch;
      break;
    case 900:
      gl->setFont(poptions.fontname,poptions.fontface,symbolSizeToPlot);
      if (xstrings.size()>0) {
        gl->getTextSize(xstrings[0],cw1,ch1);
        sw=cw1;
        sh=2.0*ch1;
      }
      break;
    case 1000:
      if (symbolStrings.size()>0){
        gl->setFont(poptions.fontname,poptions.fontface,symbolSizeToPlot);
        gl->getTextSize(symbolStrings[0],cw,ch);
        sw=1.1*cw; sh=1.2*ch;
      }
      break;
    case 1001:
      gl->setFont(diutil::METSYMBOLFONT, poptions.fontface, symbolSizeToPlot);
      gl->getCharSize(SIG2SYMBOL,cw,ch);
      sw=1.4*cw; sh=1.8*ch;
      break;
    case 1002:
      gl->setFont(diutil::METSYMBOLFONT, poptions.fontface, symbolSizeToPlot);
      gl->getCharSize(SIG2SYMBOL,cw,ch);
      sw=1.4*cw; sh=1.8*ch;
      break;
    case 1003:
      gl->setFont(diutil::METSYMBOLFONT, poptions.fontface, symbolSizeToPlot);
      gl->getCharSize(SIG3SYMBOL,cw,ch);
      sw=1.2*cw; sh=ch;
      break;
    case 1004:
      gl->setFont(diutil::METSYMBOLFONT, poptions.fontface, symbolSizeToPlot);
      gl->getCharSize(SIG3SYMBOL,cw,ch);
      sw=1.2*cw; sh=1.4*ch;
      break;
    case 1005:
      gl->setFont(diutil::METSYMBOLFONT, poptions.fontface, symbolSizeToPlot);
      getComplexSize(gl, 1001,cw,ch);
      sw=1.8*cw; sh=1.5*ch;
      break;
    case 1006:
      gl->setFont(diutil::METSYMBOLFONT, poptions.fontface, symbolSizeToPlot);
      getComplexSize(gl, 1003,cw,ch);
      sw=1.8*cw; sh=2.8*ch;
      break;
    case 1007:
      gl->setFont(poptions.fontname,poptions.fontface,symbolSizeToPlot);
      if (xstrings.size()>0) {
        gl->getTextSize(xstrings[0],cw1,ch1);
        sw=cw1;
        sh=2.0*ch1;
        if (xstrings.size()>1) {
          gl->getTextSize(xstrings[1],cw2,ch2);
          if (cw1<cw2)
            sw=cw2;
          if (ch1<ch2)
            sh=2.0*ch2;
        }
      }
      break;
    case 1008:
      getComplexSize(gl, 1001,cw1,ch1);
      symbolSizeToPlot=int (symbolSizeToPlot/textShrink);
      getComplexSize(gl, 1007,cw2,ch2);
      symbolSizeToPlot=int (symbolSizeToPlot*textShrink);
      sw=cw1+cw2;
      diffx=(cw2-cw1)*0.5;
      if (ch1>ch2)
        sh=ch1;
      else
        sh=ch2;
      break;
    case 1009:
      getComplexSize(gl, 1002,cw1,ch1);
      symbolSizeToPlot=int (symbolSizeToPlot/textShrink);
      getComplexSize(gl, 1007,cw2,ch2);
      symbolSizeToPlot=int (symbolSizeToPlot*textShrink);
      sw=cw1+cw2;
      diffx=(cw2-cw1)*0.5;
      if (ch1>ch2)
        sh=ch1;
      else
        sh=ch2;
      break;
    case 1010:
      getComplexSize(gl, 1005,cw1,ch1);
      symbolSizeToPlot=int (symbolSizeToPlot/textShrink);
      getComplexSize(gl, 1007,cw2,ch2);
      symbolSizeToPlot=int (symbolSizeToPlot*textShrink);
      sw=cw1+cw2;
      diffx=(cw2-cw1)*0.5;
      if (ch1>ch2)
        sh=ch1;
      else
        sh=ch2;
      break;
    case 1011:
      getComplexSize(gl, 1003,cw1,ch1);
      symbolSizeToPlot=int (symbolSizeToPlot/textShrink);
      getComplexSize(gl, 1007,cw2,ch2);
      symbolSizeToPlot=int (symbolSizeToPlot*textShrink);
      sw=cw1+cw2;
      if (ch1>ch2)
        sh=ch1;
      else
        sh=ch2;
      break;
    case 1012:
      getComplexSize(gl, 1004,cw1,ch1);
      symbolSizeToPlot=int (symbolSizeToPlot/textShrink);
      getComplexSize(gl, 1007,cw2,ch2);
      symbolSizeToPlot=int (symbolSizeToPlot*textShrink);
      sw=cw1+cw2;
      diffx=(cw2-cw1)*0.5;
      if (ch1>ch2)
        sh=ch1;
      else
        sh=ch2;
      break;
    case 1013:
      getComplexSize(gl, 1006,cw1,ch1);
      symbolSizeToPlot=int (symbolSizeToPlot/textShrink);
      getComplexSize(gl, 1007,cw2,ch2);
      symbolSizeToPlot=int (symbolSizeToPlot*textShrink);
      sw=cw1+cw2;
      diffx=(cw2-cw1)*0.5;
      if (ch1>ch2)
        sh=ch1;
      else
        sh=ch2;
      break;
    case 1014:
      symbolSizeToPlot=int (symbolSizeToPlot/textShrink);
      getComplexSize(gl, 1000,cw1,ch1);
      getComplexSize(gl, 1007,cw2,ch2);
      symbolSizeToPlot=int (symbolSizeToPlot*textShrink);
      sw=cw1+cw2;
      diffx=(cw2-cw1)*0.5;
      if (ch1>ch2)
        sh=ch1;
      else
        sh=ch2;
      break;
    case 1015:
      symbolSizeToPlot=int (symbolSizeToPlot/textShrink);
      getComplexSize(gl, 1007,cw1,ch1);
      getComplexSize(gl, 1000,cw2,ch2);
      symbolSizeToPlot=int (symbolSizeToPlot*textShrink);
      sh=ch1+ch2;
      diffy=(ch2-ch1)*0.5;
      if (cw1>cw2)
        sw=cw1;
      else
        sw=cw2;
      break;
    case 1016:
      symbolSizeToPlot=int (symbolSizeToPlot/textShrink);
      getComplexSize(gl, 2000,cw1,ch1);
      getComplexSize(gl, 1007,cw2,ch2);
      symbolSizeToPlot=int (symbolSizeToPlot*textShrink);
      sw=cw1+cw2;
      diffx=(cw2-cw1)*0.5;
      if (ch1>ch2)
        sh=ch1;
      else
        sh=ch2;
      break;
    case 1018:
      gl->setFont(diutil::METSYMBOLFONT, poptions.fontface, symbolSizeToPlot);
      gl->getCharSize(RIGHTARROW,cw,ch);
      sw=cw/2; sh=ch/2;
      break;
    case 1019:
      gl->setFont(diutil::METSYMBOLFONT, poptions.fontface, symbolSizeToPlot);
      gl->getCharSize(LOWSYMBOL,cw,ch);
      sw=cw; sh=ch;
      break;
    case 1020:
      gl->setFont(diutil::METSYMBOLFONT, poptions.fontface, symbolSizeToPlot);
      gl->getCharSize(HIGHSYMBOL,cw,ch);
      sw=cw; sh=ch;
      break;
    case 1021:
      gl->setFont(diutil::METSYMBOLFONT, poptions.fontface, symbolSizeToPlot);
      gl->getCharSize(THUNDERSYMBOL,cw,ch);
      sw=cw; sh=ch;
      break;
    case 1022:
      gl->setFont(diutil::METSYMBOLFONT, poptions.fontface, symbolSizeToPlot);
      gl->getCharSize(CROSS,cw,ch);
      sw=cw; sh=0.5*ch;
      break;
    case 1025:
      gl->setFont(diutil::METSYMBOLFONT, poptions.fontface, symbolSizeToPlot);
      gl->getCharSize(MOUNTAINWAVESYMBOL,cw,ch);
      sw=cw; sh=ch;
      break;
    case 1026:
      gl->setFont(diutil::METSYMBOLFONT, poptions.fontface, symbolSizeToPlot);
      gl->getCharSize(VULCANOSYMBOL,cw,ch);
      sw=cw; sh=ch;
      break;
    case 1027:
      getComplexSize(gl, 1000,sw,sh);
      break;
    case 1028:
      getComplexSize(gl, 1000,sw,sh);
      break;
    case 1029:
      getComplexSize(gl, 1000,sw,sh);
      break;
    case 1030:
      gl->setFont(diutil::METSYMBOLFONT, poptions.fontface, symbolSizeToPlot);
      gl->getCharSize(FOGSYMBOL,cw,ch);
      sw=cw; sh=ch;
      break;
    case 1031:
      getComplexSize(gl, 1030,sw,sh);
      break;
    case 1032:
      getComplexSize(gl, 1030,sw,sh);
      break;
    case 1033:
      getComplexSize(gl, 1000,sw,sh);
      break;
    case 1034:
      getComplexSize(gl, 1026,cw2,ch2);
        gl->setFont(poptions.fontname,poptions.fontface,symbolSizeToPlot);
      if (symbolStrings.size()>0){
        gl->getTextSize(symbolStrings[0],cw1,ch1);
        cw2 += cw1;
      }
      if (symbolStrings.size()>1){
        gl->getTextSize(symbolStrings[1],cw1,ch1);
        if (cw1>cw2)
          cw2=cw1;
        ch2 += ch1;
      }
      sw = cw2 * 1.3;
      sh = ch2 * 1.3;
      break;
    case 1035:
      gl->setFont(diutil::METSYMBOLFONT, poptions.fontface, symbolSizeToPlot);
      gl->getCharSize(NEW_CROSS,cw,ch);
      sw=cw; sh=0.5*ch;
      break;
    case 1036:
      gl->setFont(poptions.fontname,poptions.fontface,symbolSizeToPlot);
      if (symbolStrings.size()>0) {
        gl->getTextSize(symbolStrings[0],sw,sh);
        if (symbolStrings.size()>1 && not symbolStrings[1].empty()){
          gl->getTextSize(symbolStrings[1],cw2,ch2);
          if (cw2>sh)  sw=cw2;
          sh += ch2;
        }
        sw *= 1.7;
        sh *= 1.1;
      }
      break;
    case 1037:
      gl->setFont(diutil::METSYMBOLFONT, poptions.fontface, symbolSizeToPlot);
      gl->getCharSize(WIDESPREADBRSYMBOL,cw,ch);
      sh =ch;
      sw=cw;
      sw=cw; sh=ch;
      break;
    case 1040:
      symbolSizeToPlot=int (symbolSizeToPlot/textShrink);
      getComplexSize(gl, 1007,cw1,ch1);
      getComplexSize(gl, 2000,cw2,ch2);
      symbolSizeToPlot=int (symbolSizeToPlot*textShrink);
      sh=ch1+ch2;
      diffy=(ch2-ch1)*0.5;
      if (cw1>cw2)
        sw=cw1;
      else
        sw=cw2;
      break;
    case 2000:
      if (symbolStrings.size()>1){
        float cw1,ch1;
        float cw2,ch2;
        gl->setFont(poptions.fontname,poptions.fontface,symbolSizeToPlot);
        gl->getTextSize(symbolStrings[0],cw1,ch1);
        gl->getTextSize(symbolStrings[1],cw2,ch2);
        if (cw1>cw2)
          sw=cw1;
        else
          sw=cw2;
        if (ch1>ch2)
          sh=1.8*ch1;
        else
          sh=1.8*ch2;
      }
      break;
    }
  }
}

void ComplexSymbolPlot::getComplexBoundingBox(DiGLPainter* gl, int index,
    float& sw, float & sh, float & x, float & y)
{
  getComplexSize(gl, index,sw,sh);
  x=x+diffx;
  y=y+diffy;
}

void ComplexSymbolPlot::hideBox()
{
  whiteBox=!whiteBox;
}

int ComplexSymbolPlot::hasWhiteBox()
{
  return (whiteBox)?1:0;
}

void ComplexSymbolPlot::setWhiteBox(int on)
{
  whiteBox = (on==1);
}

bool ComplexSymbolPlot::isTextEdit(int drawIndex)
{
  switch (drawIndex) {
  case 3000:
    return true;
  case 3001:
    return true;
  default:
    return false;
  }
}

bool ComplexSymbolPlot::isComplexTextColor(int drawIndex)
{
  METLIBS_LOG_SCOPE(LOGVAL(drawIndex));
  switch (drawIndex) {
  case 900:
    return true;
  default:
    return false;
  }
}

void ComplexSymbolPlot::setBorderColour(const std::string& colstring)
{
  borderColour = Colour(colstring);
}

bool ComplexSymbolPlot::isComplexText(int drawIndex)
{
  switch (drawIndex) {
  case 1000:
    return true;
  case 1001:
    return false;
  case 1002:
    return false;
  case 1003:
    return false;
  case 1004:
    return false;
  case 1005:
    return false;
  case 1006:
    return false;
  case 1007:
    return true;
  case 1008:
    return true;
  case 1009:
    return true;
  case 1010:
    return true;
  case 1011:
    return true;
  case 1012:
    return true;
  case 1013:
    return true;
  case 1014:
    return true;
  case 1015:
    return true;
  case 1016:
    return true;
  case 1018:
    return false;
  case 1019:
    return false;
  case 1020:
    return false;
  case 1021:
    return false;
  case 1022:
    return false;
  case 1023:
    return true;
  case 1024:
    return true;
  case 1027:
    return true;
  case 1028:
    return true;
  case 1029:
    return true;
  case 1030:
    return false;
  case 1031:
    return false;
  case 1032:
    return false;
  case 1033:
    return true;
  case 1034:
    return true;
  case 1035:
    return false;
  case 1036:
    return true;
  case 1037:
    return false;
  case 1038:
    return false;
  case 1039:
    return false;
  case 1040:
    return true;
  case 1041:
    return false;
  case 1042:
    return false;
  case 1043:
    return false;
  case 1044:
    return false;
  case 1045:
    return false;
  case 2000:
    return true;
  default:
    return false;
  }
}

void ComplexSymbolPlot::getCurrentComplexText(std::vector <std::string> & symbolText,
    std::vector <std::string> & xText)
{
  symbolText=currentSymbolStrings;
  xText=currentXStrings;
}

void ComplexSymbolPlot::setCurrentComplexText(const std::vector <std::string> &
    symbolText, const std::vector <std::string> & xText)
{
  currentSymbolStrings=symbolText;
  //insert into list of texts
  for (unsigned int i =0;i<symbolText.size();i++){
    clist.insert(symbolText[i]);
  }
  currentXStrings=xText;
}

void ComplexSymbolPlot::getComplexColoredText(std::vector <std::string> & symbolText,
    std::vector <std::string> & xText)
{
  xvisible = true;

  if (xvisible)
    xText=xstrings;
  symbolText.clear();
  for (unsigned int i = 0;i<nstringsvisible && i<symbolStrings.size();i++)
    symbolText.push_back(symbolStrings[i]);
}

void ComplexSymbolPlot::getMultilineText(std::vector <std::string> & symbolText){
  symbolText.clear();
    for (unsigned int i = 0;i<symbolStrings.size();i++)
      symbolText.push_back(symbolStrings[i]);
}

void ComplexSymbolPlot::getComplexText(std::vector <std::string> & symbolText, std::vector <std::string> & xText){
  if (xvisible)
    xText=xstrings;
  symbolText.clear();
  for (unsigned int i = 0;i<nstringsvisible && i<symbolStrings.size();i++)
    symbolText.push_back(symbolStrings[i]);
}

void ComplexSymbolPlot::changeMultilineText(const std::vector <std::string> & symbolText){
  symbolStrings=symbolText;
}

void ComplexSymbolPlot::changeComplexText(const std::vector <std::string> & symbolText, const std::vector <std::string> & xText){
  symbolStrings=symbolText;
  //insert into list of texts
  for (unsigned int i =0;i<symbolText.size();i++)
    clist.insert(symbolText[i]);
  xstrings=xText;
}



void ComplexSymbolPlot::readComplexText(const std::string& complexString)
{
  METLIBS_LOG_SCOPE(LOGVAL(complexString));
  std::string key,value;
  std::vector <std::string> tokens = miutil::split(complexString, 0, "/");
  for (unsigned int i = 0; i<tokens.size();i++){
    std::vector <std::string> stokens = miutil::split(tokens[i], 0, ":");
    if (stokens.size()==2){
      key = miutil::to_lower(stokens[0]);
      value = stokens[1];
      std::vector <std::string> texts = miutil::split(value, 0, ",");
      if (key=="symbolstrings"){
        symbolStrings=texts;
        for (unsigned int i=0;i<symbolStrings.size();i++)
          WeatherSymbol::replaceText(symbolStrings[i],false);
      }
      else if (key=="xstrings"){
        xstrings=texts;
        for (unsigned int i=0;i<xstrings.size();i++)
          WeatherSymbol::replaceText(xstrings[i],false);
      }
    }
  }
}


std::string ComplexSymbolPlot::writeComplexText(){
  METLIBS_LOG_SCOPE();
  std::string ret;
  int ns=symbolStrings.size();
  int nx=xstrings.size();
  if (ns || nx){
    ret ="ComplexText=";
   if (ns){
      ret+="symbolstrings:";
      for (int i=0;i<ns;i++){
        std::string tempString=symbolStrings[i];
        WeatherSymbol::replaceText(tempString,true);
        ret+=tempString;
        if (i<ns-1)
          ret+=",";
      }
    }
    if (ns&&nx)ret+="/";
    if (nx){
      ret+="xstrings:";
      for (int i=0;i<nx;i++){
        std::string tempString=xstrings[i];
        WeatherSymbol::replaceText(tempString,true);
        ret+=tempString;
        if (i<nx-1)
          ret+=",";
      }
    }
    ret+=";\n";
  }
  return ret;
}


void ComplexSymbolPlot::initComplexList()
{
  clist.insert("BKN/OVC STF");
  clist.insert("RA");
  clist.insert("RADZ");
  clist.insert("SHRA");
  clist.insert("SN");
  clist.insert("SHSN");
  clist.insert("LOC FG");
  clist.insert("BR");
  clist.insert("NSC");
  clist.insert("RASN");
  clist.insert("SHSNGS");
  clist.insert("SHRASN");
  clist.insert("SHRA");
  clist.insert("BR/FG");
  clist.insert("DZ");
  clist.insert("FZRA");
  clist.insert("SFC VIS");
  clist.insert("LT 5 KM");
  clist.insert("ISOL");
  clist.insert("EMBD CB");
  clist.insert("SCT/BKN");
  clist.insert("CB/TCU");
}



std::set <std::string> ComplexSymbolPlot::getComplexList()
{
  return clist;
}
