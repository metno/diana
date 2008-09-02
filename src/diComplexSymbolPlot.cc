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

#include <diComplexSymbolPlot.h>
#include <diFontManager.h>
#include <diWeatherSymbol.h>

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
#define MOUNTAINWAVESYMBOL 130
#define VULCANOSYMBOL 226

//static variables
// text used in new complex symbols 
vector <miString> ComplexSymbolPlot::currentSymbolStrings; //symbolstrings
vector <miString> ComplexSymbolPlot::currentXStrings; //xtext
set <miString> ComplexSymbolPlot::clist; //texts used in combobox
float ComplexSymbolPlot::textShrink=1.5;

ComplexSymbolPlot::ComplexSymbolPlot() : Plot(){
#ifdef DEBUGPRINT
  cerr << "ComplexSymbolPlot::ComplexSymbolPlot()"<< endl;
#endif
  xvisible=false;
  nstringsvisible=0;
  symbolStrings=currentSymbolStrings;
  xstrings=currentXStrings;
  whiteBox=true;
}


ComplexSymbolPlot::ComplexSymbolPlot(int drawIndex) : Plot(){
#ifdef DEBUGPRINT
  cerr << "ComplexSymbolPlot::ComplexSymbolPlot()"<< endl;
#endif
  xvisible=false;
  nstringsvisible=0;
  symbolStrings=currentSymbolStrings;
  xstrings=currentXStrings;
  //rightarrow and thunder - no white box
  if (drawIndex==1018 || drawIndex==1021)
    whiteBox=false;
  else
    whiteBox=true;
}


void ComplexSymbolPlot::initStrings(int drawIndex){
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
  case 2000:
    if (symbolStrings.size()==0)
      symbolStrings.push_back("");
    if (symbolStrings.size()==1)
      symbolStrings.push_back(symbolStrings[0]);
    break;
  }
}


void ComplexSymbolPlot::initCurrentStrings(int drawIndex){
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
  case 1017:
    currentSymbolStrings.push_back("0°:x");
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
    currentSymbolStrings.push_back("BKN/OVC");
    break;
  case 1024:
    currentXStrings.push_back("");
    currentXStrings.push_back("");
    currentSymbolStrings.push_back("BKN/OVC");
    break;
  case 2000:
    currentSymbolStrings.push_back("");
    currentSymbolStrings.push_back("");
    break;
  }
}


void ComplexSymbolPlot::draw(int drawIndex, float x,float y,int size,float rot){
  symbolSizeToPlot=size;
  xvisible=false;
  nstringsvisible=0;
  glPushMatrix();
  glTranslatef(x, y, 0.0);
  glRotatef(rot,0.0,0.0,1.0); 
  //scale linewidth to symbolsize
  float linewidth=symbolSizeToPlot/50+1;  
  glLineWidth(linewidth);
  switch (drawIndex){
  case 1000:
    symbolSizeToPlot=int (symbolSizeToPlot/textShrink);
    drawSigText(0,0);	
    break;
  case 1001:
    drawSig1(0,0);	
    break;
  case 1002:
    drawSig2(0,0);	
    break;
  case 1003:
    drawSig3(0,0);	
    break;
  case 1004:
    drawSig4(0,0);	
    break;
  case 1005:
    drawSig5(0,0);	
    break;
  case 1006:
    drawSig6(0,0);	
    break;
  case 1007:
    symbolSizeToPlot=int (symbolSizeToPlot/textShrink);
    drawSig7(0,0);	
    break;       
  case 1008:
    drawSig8(0,0);	
    break;       
  case 1009:
    drawSig9(0,0);	
    break;       
  case 1010:
    drawSig10(0,0);	
    break;   
  case 1011:
    drawSig11(0,0);	
    break;       
  case 1012:
    drawSig12(0,0);	
    break;       
  case 1013:
    drawSig13(0,0);	
    break;       
  case 1014:
    drawSig14(0,0);	
    break;       
  case 1015:
    drawSig15(0,0);	
    break;       
  case 1016:
    drawSig16(0,0);	
    break;       
  case 1017:
    drawSig17(0,0);	
    break;       
  case 1018:
    drawSig18(0,0);	
    break;       
  case 1019:
    drawSig19(0,0);	
    break;       
  case 1020:
    drawSig20(0,0);	
    break;       
  case 1021:
    drawSig21(0,0);	
    break;
  case 1022:
    drawSig22(0,0);	
    break;       
  case 1023:
    drawSig14(0,0);	
    break; 
  case 1024:
    drawSig15(0,0);	
    break;             
  case 1025:
    drawSig25(0,0);	
    break;             
  case 1026:
    drawSig26(0,0);	
    break;             
  case 2000:
    symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
    drawDoubleSigText(0,0);	
    break;       
  default:
    cerr << "ComplexSymbolPlot::drawComplexSymbols - Index " << 
      drawIndex << " not defined " << endl;
    return;
  }
  glPopMatrix();
}



void ComplexSymbolPlot::drawSymbol(int index,float x,float y){
  float cw,ch;
  fp->set("METSYMBOLFONT",poptions.fontface,symbolSizeToPlot);       
  fp->getCharSize(index,cw,ch);
  fp->drawChar(index,x-cw/2,y-ch/2,0.0);
}


void ComplexSymbolPlot::drawSigString(float x,float y){
  drawBox(1999,x,y);
  float cw,ch;
  getComplexSize(1999,cw,ch);
  fp->set(poptions.fontname,poptions.fontface,symbolSizeToPlot);
  fp->drawStr(sigString.c_str(),x-0.45*cw,y-0.4*ch,0.0); 
}


void ComplexSymbolPlot::drawSigText(float x,float y){
  initStrings(1000);
  if (symbolStrings.size()>0)
    sigString=symbolStrings[0];
  drawSigString(x,y); 
  nstringsvisible=1;
}

void ComplexSymbolPlot::drawDoubleSigText(float x,float y){
  float cw1,ch1;
  float cw2,ch2;  
  initStrings(2000);
  if (symbolStrings.size()>0)
    sigString=symbolStrings[0];
  getComplexSize(1999,cw1,ch1);
  drawSigString(x,y+ch1/2);
  if (symbolStrings.size()>1)
    sigString=symbolStrings[1];
  getComplexSize(1999,cw2,ch2);
  drawSigString(x,y-ch2/2);
  nstringsvisible=2;
}


void ComplexSymbolPlot::drawSig1(float x,float y){
  drawBox(1001,x,y);
  drawSymbol(SIG1SYMBOL,x,y);
}

void ComplexSymbolPlot::drawSig2(float x,float y){
  drawBox(1002,x,y);
  drawSymbol(SIG2SYMBOL,x,y);
}

void ComplexSymbolPlot::drawSig3(float x,float y){
  drawBox(1003,x,y);
  float cw,ch;
  getComplexSize(1003,cw,ch);
  drawSymbol(SIG3SYMBOL,x,y);
}

void ComplexSymbolPlot::drawSig4(float x,float y){
  drawBox(1004,x,y);
  float cw,ch;
  getComplexSize(1004,cw,ch);
  drawSymbol(SIG4SYMBOL,x,y);
}

void ComplexSymbolPlot::drawSig5(float x,float y){
  drawBox(1005,x,y);
  float cw,ch;
  getComplexSize(1001,cw,ch);
  drawSymbol(SIG1SYMBOL,x-0.4*cw,y+0.3*ch);
  getComplexSize(1002,cw,ch);
  drawSymbol(SIG2SYMBOL,x+0.4*cw,y-0.5*ch);

  getComplexSize(1005,cw,ch);
  glBegin(GL_LINE_STRIP);
  glVertex2f(x-0.3*cw,y-0.3*ch);
  glVertex2f(x+0.3*cw,y+0.3*ch);
  glEnd();
}


void ComplexSymbolPlot::drawSig6(float x,float y){
  drawBox(1006,x,y);
  float cw,ch;
  getComplexSize(1003,cw,ch);
  drawSymbol(SIG3SYMBOL,x-0.3*cw,y+1.05*ch);
  getComplexSize(1004,cw,ch);
  drawSymbol(SIG4SYMBOL,x+0.45,y-0.5*ch);
  getComplexSize(1006,cw,ch);
  glBegin(GL_LINE_STRIP);
  glVertex2f(x-0.35*cw,y-0.35*ch);
  glVertex2f(x+0.35*cw,y+0.35*ch);
  glEnd();
}


void ComplexSymbolPlot::drawSig7(float x,float y){
  if (xstrings.size()<2)
    initStrings(1007);
  drawBox(1007,x,y);
  fp->set(poptions.fontname,poptions.fontface,symbolSizeToPlot);
  float cw1,ch1;
  float cw2,ch2;  
  fp->getStringSize(xstrings[0].c_str(),cw1,ch1);
  fp->getStringSize(xstrings[1].c_str(),cw2,ch2);
  fp->drawStr(xstrings[0].c_str(),x-cw1/2,y+0.1*ch1,0.0);      
  fp->drawStr(xstrings[1].c_str(),x-cw2/2,y-1.1*ch2,0.0); 
  float sw,sh;
  getComplexSize(1007,sw,sh);
  glBegin(GL_LINE_STRIP);
  glVertex2f(x-sw/2,y);
  glVertex2f(x+sw/2,y);
  glEnd();     
  xvisible=true;
}


void ComplexSymbolPlot::drawSig8(float x,float y){
  float sw,sh;
  getComplexSize(1001,sw,sh);
  drawSig1(x-sw/2,y);
  initStrings(1007);
  symbolSizeToPlot= int(symbolSizeToPlot/textShrink);
  getComplexSize(1007,sw,sh);
  drawSig7(x+sw/2,y);
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}


void ComplexSymbolPlot::drawSig9(float x,float y){
  float sw,sh;
  getComplexSize(1002,sw,sh);
  drawSig2(x-sw/2,y);
  initStrings(1007);
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  getComplexSize(1007,sw,sh);
  drawSig7(x+sw/2,y);
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}


void ComplexSymbolPlot::drawSig10(float x,float y){
  float sw,sh;
  getComplexSize(1005,sw,sh);
  drawSig5(x-sw/2,y);
  initStrings(1007);
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  getComplexSize(1007,sw,sh);
  drawSig7(x+sw/2,y);
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}

void ComplexSymbolPlot::drawSig11(float x,float y){
  float sw,sh;
  getComplexSize(1003,sw,sh);
  drawSig3(x-sw/2,y);
  initStrings(1007);
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  getComplexSize(1007,sw,sh);
  drawSig7(x+sw/2,y);
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}

void ComplexSymbolPlot::drawSig12(float x,float y){
  float sw,sh;
  getComplexSize(1004,sw,sh);
  drawSig4(x-sw/2,y);
  initStrings(1007);
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  getComplexSize(1007,sw,sh);
  drawSig7(x+sw/2,y);
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}
void ComplexSymbolPlot::drawSig13(float x,float y){
  float sw,sh;
  getComplexSize(1006,sw,sh);
  drawSig6(x-sw/2,y);
  initStrings(1007);
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  getComplexSize(1007,sw,sh);
  drawSig7(x+sw/2,y);
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}

void ComplexSymbolPlot::drawSig14(float x,float y){
  float sw,sh;
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  initStrings(1000);
  getComplexSize(1000,sw,sh);
  drawSigText(x-sw/2,y);
  initStrings(1007);
  getComplexSize(1007,sw,sh);
  drawSig7(x+sw/2,y);
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}


void ComplexSymbolPlot::drawSig15(float x,float y){
  float sw,sh;
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  initStrings(1000);
  getComplexSize(1000,sw,sh);
  drawSigText(x,y+sh/2);
  initStrings(1007);
  getComplexSize(1007,sw,sh);
  drawSig7(x,y-sh/2);
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}

void ComplexSymbolPlot::drawSig16(float x,float y){
  //draw two texts
  float sw,sh;
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  initStrings(2000);
  getComplexSize(2000,sw,sh);
  drawDoubleSigText(x-sw/2,y);
  initStrings(1007);
  getComplexSize(1007,sw,sh);
  drawSig7(x+sw/2,y);
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}

void ComplexSymbolPlot::drawSig17(float x,float y){
  symbolSizeToPlot=int(symbolSizeToPlot/textShrink);
  if (symbolStrings.size()>0)
    sigString=symbolStrings[0];
  drawBox(1000,x,y);
  drawSigString(x,y);
  nstringsvisible=1;
  float sw,sh;
  getComplexSize(1000,sw,sh);
  glBegin(GL_LINE_LOOP);
  glVertex2f(x-0.5*sw,y-0.5*sh);
  glVertex2f(x-0.5*sw,y+0.5*sh);
  glVertex2f(x+0.5*sw,y+0.5*sh);
  glVertex2f(x+0.5*sw,y-0.5*sh);
  glEnd();
  symbolSizeToPlot=int(symbolSizeToPlot*textShrink);
}

void ComplexSymbolPlot::drawSig18(float x,float y){
  float sw1,sh1;
  getComplexSize(1018,sw1,sh1);
  drawBox(1018,0,0);
  drawSymbol(RIGHTARROW,0,0);
}

void ComplexSymbolPlot::drawSig19(float x,float y){
  //twice as big as other symbols
  symbolSizeToPlot = 2*symbolSizeToPlot;
  drawBox(1019,x,y);
  drawSymbol(LOWSYMBOL,x,y);
}


void ComplexSymbolPlot::drawSig20(float x,float y){
  symbolSizeToPlot = 2*symbolSizeToPlot;
  drawBox(1020,x,y);
  drawSymbol(HIGHSYMBOL,x,y);
}


void ComplexSymbolPlot::drawSig21(float x,float y){
  symbolSizeToPlot = 2*symbolSizeToPlot;
  drawBox(1021,x,y);
  drawSymbol(THUNDERSYMBOL,x,y);
}


void ComplexSymbolPlot::drawSig22(float x,float y){
  drawBox(1022,x,y);
  drawSymbol(CROSS,x,y);
}



void ComplexSymbolPlot::drawSig25(float x,float y){
  drawBox(1025,x,y);
  drawSymbol(MOUNTAINWAVESYMBOL,x,y);
}

void ComplexSymbolPlot::drawSig26(float x,float y){
  drawBox(1026,x,y);
  drawSymbol(VULCANOSYMBOL,x,y);
}



void ComplexSymbolPlot::drawBox(int index,float x, float y){
  if (!whiteBox) return;
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  GLfloat currentColor[4];
  glGetFloatv(GL_CURRENT_COLOR,currentColor);
  glColor4f(1.0,1.0,1.0,1.0);
  float sw,sh;
  getComplexSize(index,sw,sh);
  glBegin(GL_POLYGON);
  glVertex2f(x-0.5*sw,y-0.5*sh);
  glVertex2f(x-0.5*sw,y+0.5*sh);
  glVertex2f(x+0.5*sw,y+0.5*sh);
  glVertex2f(x+0.5*sw,y-0.5*sh);
  glEnd();
  glColor4fv(currentColor);
}

void ComplexSymbolPlot::getComplexSize(int index, float& sw, float & sh){
  float cw,ch;
  float cw1,ch1;
  float cw2,ch2;  
  diffx=0;
  diffy=0;
  if (index < 1000){
    fp->set("METSYMBOLFONT",poptions.fontface,symbolSizeToPlot);       
    fp->getCharSize(index,cw,ch);
    sw=ch; sh=ch;
  }
  else{
    switch (index){
    case 1999:
      //sigstring
      fp->set(poptions.fontname,poptions.fontface,symbolSizeToPlot);
      fp->getStringSize(sigString.c_str(),cw,ch);
      sw=1.1*cw; sh=1.2*ch;
      break;
    case 1000:
      if (symbolStrings.size()>0){
	fp->set(poptions.fontname,poptions.fontface,symbolSizeToPlot);
	fp->getStringSize(symbolStrings[0].c_str(),cw,ch);
	sw=1.1*cw; sh=1.2*ch;
      }
      break;
    case 1001:
      fp->set("METSYMBOLFONT",poptions.fontface,symbolSizeToPlot);       
      fp->getCharSize(SIG2SYMBOL,cw,ch);
      sw=1.4*cw; sh=1.8*ch;
      break;
    case 1002:
      fp->set("METSYMBOLFONT",poptions.fontface,symbolSizeToPlot);       
      fp->getCharSize(SIG2SYMBOL,cw,ch);
      sw=1.4*cw; sh=1.8*ch;
      break;
    case 1003:   
      fp->set("METSYMBOLFONT",poptions.fontface,symbolSizeToPlot);       
      fp->getCharSize(SIG3SYMBOL,cw,ch);
      sw=1.2*cw; sh=ch;
      break;
    case 1004:
      fp->set("METSYMBOLFONT",poptions.fontface,symbolSizeToPlot);       
      fp->getCharSize(SIG3SYMBOL,cw,ch);
      sw=1.2*cw; sh=1.4*ch;
      break;
    case 1005:
      fp->set("METSYMBOLFONT",poptions.fontface,symbolSizeToPlot);       
      getComplexSize(1001,cw,ch);
      sw=1.8*cw; sh=1.5*ch;
      break;
    case 1006:
      fp->set("METSYMBOLFONT",poptions.fontface,symbolSizeToPlot);       
      getComplexSize(1003,cw,ch);
      sw=1.8*cw; sh=2.8*ch;
      break;
    case 1007:
      fp->set(poptions.fontname,poptions.fontface,symbolSizeToPlot);
      if (xstrings.size()>0)
	fp->getStringSize(xstrings[0].c_str(),cw1,ch1);
      if (xstrings.size()>1)
	fp->getStringSize(xstrings[1].c_str(),cw2,ch2);
      if (cw1>cw2) 
	sw=cw1;
      else
	sw=cw2;
      if (ch1>ch2) 
	sh=2.0*ch1;
      else
	sh=2.0*ch2;      
      break;
    case 1008:
      getComplexSize(1001,cw1,ch1);
      symbolSizeToPlot=int (symbolSizeToPlot/textShrink);
      getComplexSize(1007,cw2,ch2);
      symbolSizeToPlot=int (symbolSizeToPlot*textShrink);
      sw=cw1+cw2;
      diffx=(cw2-cw1)*0.5;
      if (ch1>ch2) 
	sh=ch1;
      else
	sh=ch2;  
      break;
    case 1009:
      getComplexSize(1002,cw1,ch1);
      symbolSizeToPlot=int (symbolSizeToPlot/textShrink);
      getComplexSize(1007,cw2,ch2);
      symbolSizeToPlot=int (symbolSizeToPlot*textShrink);
      sw=cw1+cw2;
      diffx=(cw2-cw1)*0.5;
      if (ch1>ch2) 
	sh=ch1;
      else
	sh=ch2;      
      break;
    case 1010:
      getComplexSize(1005,cw1,ch1);
      symbolSizeToPlot=int (symbolSizeToPlot/textShrink);
      getComplexSize(1007,cw2,ch2);
      symbolSizeToPlot=int (symbolSizeToPlot*textShrink);
      sw=cw1+cw2;
      diffx=(cw2-cw1)*0.5;
      if (ch1>ch2) 
	sh=ch1;
      else
	sh=ch2;      
      break;
    case 1011:
      getComplexSize(1003,cw1,ch1);
      symbolSizeToPlot=int (symbolSizeToPlot/textShrink);
      getComplexSize(1007,cw2,ch2);
      symbolSizeToPlot=int (symbolSizeToPlot*textShrink);
      sw=cw1+cw2;
      if (ch1>ch2) 
	sh=ch1;
      else
	sh=ch2;      
      break;
    case 1012:
      getComplexSize(1004,cw1,ch1);
      symbolSizeToPlot=int (symbolSizeToPlot/textShrink);
      getComplexSize(1007,cw2,ch2);
      symbolSizeToPlot=int (symbolSizeToPlot*textShrink);
      sw=cw1+cw2;
      diffx=(cw2-cw1)*0.5;
      if (ch1>ch2) 
	sh=ch1;
      else
	sh=ch2;      
      break;
    case 1013:
      getComplexSize(1006,cw1,ch1);
      symbolSizeToPlot=int (symbolSizeToPlot/textShrink);
      getComplexSize(1007,cw2,ch2);
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
      getComplexSize(1000,cw1,ch1);
      getComplexSize(1007,cw2,ch2);
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
      getComplexSize(1007,cw1,ch1);
      getComplexSize(1000,cw2,ch2);
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
      getComplexSize(2000,cw1,ch1);
      getComplexSize(1007,cw2,ch2);
      symbolSizeToPlot=int (symbolSizeToPlot*textShrink);
      sw=cw1+cw2;
      diffx=(cw2-cw1)*0.5;
      if (ch1>ch2) 
	sh=ch1;
      else
	sh=ch2;      
      break;
    case 1017:
      getComplexSize(1000,sw,sh);
      break;
    case 1018:
      fp->set("METSYMBOLFONT",poptions.fontface,symbolSizeToPlot);       
      fp->getCharSize(RIGHTARROW,cw,ch);
      sw=cw; sh=ch;
      break;
    case 1019:
      fp->set("METSYMBOLFONT",poptions.fontface,symbolSizeToPlot);       
      fp->getCharSize(LOWSYMBOL,cw,ch);
      sw=cw; sh=ch;
      break;
    case 1020:
      fp->set("METSYMBOLFONT",poptions.fontface,symbolSizeToPlot);       
      fp->getCharSize(HIGHSYMBOL,cw,ch);
      sw=cw; sh=ch;
      break;
    case 1021:
      fp->set("METSYMBOLFONT",poptions.fontface,symbolSizeToPlot);       
      fp->getCharSize(THUNDERSYMBOL,cw,ch);
      sw=cw; sh=ch;
      break;
    case 1022:
      fp->set("METSYMBOLFONT",poptions.fontface,symbolSizeToPlot);       
      fp->getCharSize(CROSS,cw,ch);
      sw=cw; sh=0.5*ch;
      break;
    case 1025:
      fp->set("METSYMBOLFONT",poptions.fontface,symbolSizeToPlot);            
      fp->getCharSize(MOUNTAINWAVESYMBOL,cw,ch);
      sw=cw; sh=ch;
      break;
    case 1026:
      fp->set("METSYMBOLFONT",poptions.fontface,symbolSizeToPlot);       
      fp->getCharSize(VULCANOSYMBOL,cw,ch);
      sw=cw; sh=ch;
      break;
    case 2000:
      if (symbolStrings.size()>1){
	float cw1,ch1;
	float cw2,ch2;  
	fp->set(poptions.fontname,poptions.fontface,symbolSizeToPlot);
	fp->getStringSize(symbolStrings[0].c_str(),cw1,ch1);
	fp->getStringSize(symbolStrings[1].c_str(),cw2,ch2);
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


void ComplexSymbolPlot::getComplexBoundingBox(int index, float& sw, float & sh, float & x, float & y){
  getComplexSize(index,sw,sh);
  x=x+diffx;
  y=y+diffy;
}

void ComplexSymbolPlot::hideBox(){
  whiteBox=!whiteBox;
}


int ComplexSymbolPlot::hasWhiteBox(){
  return (whiteBox)?1:0;
}


void ComplexSymbolPlot::setWhiteBox(int on){
   (on==1)?whiteBox=true:whiteBox=false;
}




bool ComplexSymbolPlot::isComplexText(int drawIndex){
  //cerr << "complexSymbolPlot::isComplexText " << drawIndex << endl;
  switch (drawIndex){
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
  case 1017:
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
  case 2000:
    return true;	
  default:
    return false;
  }
}



void ComplexSymbolPlot::getCurrentComplexText(vector <miString> & symbolText, 
					  vector <miString> & xText){
  symbolText=currentSymbolStrings;
  xText=currentXStrings;
}

void ComplexSymbolPlot::setCurrentComplexText(const vector <miString> &
symbolText, const vector <miString> & xText){
#ifdef DEBUGPRINT
  cerr << "ComplexSymbolPlot::setCurrentComplexText" << endl;
#endif
  currentSymbolStrings=symbolText;
  //insert into list of texts
  for (int i =0;i<symbolText.size();i++)
    clist.insert(symbolText[i]);
  currentXStrings=xText;
}


void ComplexSymbolPlot::getComplexText(vector <miString> & symbolText, vector <miString> & xText){
#ifdef DEBUGPRINT
  cerr << "ComplexSymbolPlot::getComplexText" << endl;
#endif
  if (xvisible) 
    xText=xstrings;
  symbolText.clear();
  for (int i = 0;i<nstringsvisible && i<symbolStrings.size();i++)
    symbolText.push_back(symbolStrings[i]);
}

void ComplexSymbolPlot::changeComplexText(const vector <miString> & symbolText, const vector <miString> & xText){
#ifdef DEBUGPRINT
  cerr << "ComplexSymbolPlot::changeComplexText" << endl;
#endif
  symbolStrings=symbolText;
  //insert into list of texts
  for (int i =0;i<symbolText.size();i++)
    clist.insert(symbolText[i]);
  xstrings=xText;
}



void ComplexSymbolPlot::readComplexText(miString complexString){
#ifdef DEBUGPRINT
  cerr << "ComplexSymbolPlot::readComplexText" << complexString << endl;
#endif
  miString key,value;
  vector <miString> tokens = complexString.split('/');
  for (int i = 0; i<tokens.size();i++){
    vector <miString> stokens = tokens[i].split(':');
    if (stokens.size()==2){
      key = stokens[0].downcase();
      value = stokens[1];
      vector <miString> texts = value.split(',');
      if (key=="symbolstrings"){
	symbolStrings=texts;
	for (int i=0;i<symbolStrings.size();i++)
	  WeatherSymbol::replaceText(symbolStrings[i],false);
      }
      else if (key=="xstrings"){
	xstrings=texts;
	for (int i=0;i<xstrings.size();i++)
	  WeatherSymbol::replaceText(xstrings[i],false);
      }
    }
  }
}


miString ComplexSymbolPlot::writeComplexText(){
#ifdef DEBUGPRINT
  cerr << "ComplexSymbolPlot::writeComplexText-"  << endl;
#endif
  miString ret;
  int ns=symbolStrings.size();
  int nx=xstrings.size();
  if (ns || nx){
    ret ="ComplexText=";
   if (ns){
      ret+="symbolstrings:";
      for (int i=0;i<ns;i++){
	miString tempString=symbolStrings[i];
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
	miString tempString=xstrings[i];
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


void ComplexSymbolPlot::initComplexList(){
  clist.insert("BKN/OVC");
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



set <miString> ComplexSymbolPlot::getComplexList(){
  return clist;
}






