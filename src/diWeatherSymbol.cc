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

#include <diWeatherSymbol.h>
#include <diFontManager.h>
#include <diColour.h>

#include <puTools/miStringFunctions.h>

//#define DEBUGPRINT
#define MILOGGER_CATEGORY "diana.WeatherSymbol"
#include <miLogger/miLogging.h>

using namespace::miutil;
using namespace std;

//static variables
vector<editToolInfo> WeatherSymbol::allSymbols;
vector<editToolInfo> WeatherSymbol::allRegions;
map<std::string,int> WeatherSymbol::symbolTypes;     //finds symbol type number
map<std::string,int> WeatherSymbol::regionTypes;     //finds region type number
map<int,int> WeatherSymbol::indexTypes;           //finds symbol type number
map<int,int> WeatherSymbol::next;           //finds next type number
map<int,int> WeatherSymbol::last;           //finds last type number
float WeatherSymbol::defaultSize=60.;
float WeatherSymbol::defaultComplexSize=6.;
std::string WeatherSymbol::currentText;
Colour::ColourInfo WeatherSymbol::currentColour; //text colour
set <std::string> WeatherSymbol::textlist; //texts used in combobox

//Constructor
WeatherSymbol::WeatherSymbol() : ObjectPlot(wSymbol),complexSymbol(0) {
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("Weather symbol- default constructor");
#endif
  setType(0);
  if (drawIndex < 1000)
    setSymbolSize(defaultSize);
  else
    setSymbolSize(defaultComplexSize);
}


//constructor taking symboltype as argument
WeatherSymbol::WeatherSymbol(int ty) : ObjectPlot(wSymbol),complexSymbol(0){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("Weather symbol - int constructor");
#endif

  setType(ty);

}


//constructor taking symboltype and type of object as argument
WeatherSymbol::WeatherSymbol(std::string tystring,int objTy) : ObjectPlot(objTy),
symbolSize(defaultSize),complexSymbol(0){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("Weather symbol(std::string,int) constructor");
#endif

  // set correct symboltype
  if (tystring.empty())
    setType(0);
  else if (!setType(tystring))
    METLIBS_LOG_ERROR("WeatherSymbol constructor error, type " << tystring << " not found !!!");
}


WeatherSymbol::WeatherSymbol(const WeatherSymbol &rhs):ObjectPlot(rhs){
  symbolSize=rhs.symbolSize;
  symbolString = rhs.symbolString;
  if (rhs.complexSymbol!=0)
    complexSymbol= new ComplexSymbolPlot(*rhs.complexSymbol);
  else complexSymbol=0;
}


// destructor
WeatherSymbol::~WeatherSymbol(){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("WeatherSymbol destructor");
#endif
  if (complexSymbol!=0) delete complexSymbol;
}



void WeatherSymbol::defineSymbols(vector<editToolInfo> symbols){
  // static function setting static private members
  int nStart = allSymbols.size();
  int n = symbols.size();
  for (int i=0; i<n; i++){
    allSymbols.push_back(symbols[i]);
    int index=allSymbols.size()-1;
    // map from name to type
    symbolTypes[symbols[i].name] = index;
    // map from index to type (for compatibility with old drawfiles)
    indexTypes[symbols[i].index] = index;
    //indices to go up and down in same symbol set(with t (shift-t))
    if (i==n-1)
      next[index] = nStart;
    else
      next[index]=index+1;
    if (i==0)
      last[index] = index+n-1;
    else
      last[index]=index-1;
  }
  currentColour.name="black";
  currentColour.rgb[0]=0;
  currentColour.rgb[1]=0;
  currentColour.rgb[2]=0;
}


void WeatherSymbol::defineRegions(vector<editToolInfo> regions){
  if (regions.size()) {
    allRegions = regions;
    for (unsigned int i=0; i<regions.size(); i++)
      regionTypes[regions[i].name] = i;
  }

}



void WeatherSymbol::setCurrentText(const std::string & newText){
  currentText=newText;
}


set <std::string> WeatherSymbol::getTextList(){
  return textlist;
}


void WeatherSymbol::setCurrentColour(const Colour::ColourInfo & newColour){
  currentColour=newColour;
}


std::string WeatherSymbol::getCurrentText(){
  return currentText;
}

Colour::ColourInfo WeatherSymbol::getCurrentColour(){
  return currentColour;
}

void WeatherSymbol::initComplexList(){
  ComplexSymbolPlot::initComplexList();
}

set <string> WeatherSymbol::getComplexList(){
  return ComplexSymbolPlot::getComplexList();
}



bool WeatherSymbol::isSimpleText(std::string edittool){
  //return true if editool corresponds to simple text, type 0
  if (allSymbols[symbolTypes[edittool]].index == 0)
    return true;
  else
    return false;
}


bool WeatherSymbol::isComplexText(std::string edittool){
  //return true if editool corresponds to simple text, type 0
  int edIndex=allSymbols[symbolTypes[edittool]].index;
  if (edIndex >=1000 && edIndex<3000){
    return ComplexSymbolPlot::isComplexText(edIndex);
  }
  else
    return false;
}
bool WeatherSymbol::isComplexTextColor(std::string edittool){
  //return true if editool corresponds to simple text, type 0
  int edIndex=allSymbols[symbolTypes[edittool]].index;
  if (edIndex ==900){
    return ComplexSymbolPlot::isComplexTextColor(edIndex);
  }
  else
    return false;
}

bool WeatherSymbol::isTextEdit(std::string edittool){
  //return true if editool corresponds to simple text, type 0
  int edIndex=allSymbols[symbolTypes[edittool]].index;
  if (edIndex >=3000){
    return ComplexSymbolPlot::isTextEdit(edIndex);
  }
  else
    return false;
}


void WeatherSymbol::getCurrentComplexText(vector<std::string> & symbolText, vector <std::string> & xText)
{
  ComplexSymbolPlot::getCurrentComplexText(symbolText,xText);
}


void WeatherSymbol::setCurrentComplexText(const vector <std::string>& symbolText, const vector <std::string> & xText)
{
  ComplexSymbolPlot::setCurrentComplexText(symbolText,xText);
}


void WeatherSymbol::initCurrentComplexText(std::string edittool){
  int edIndex=allSymbols[symbolTypes[edittool]].index;
  if (edIndex == 900 || edIndex >=1000){
    ComplexSymbolPlot::initCurrentStrings(edIndex);
  }
}



std::string WeatherSymbol::getAllRegions(int ir){
  std::string region;
  if (int(allRegions.size())>ir) region=allRegions[ir].name;
  return region;
}


void WeatherSymbol::addPoint( float x , float y){
  switch (currentState){
  case active:
    ObjectPoint pxy(x,y);
    nodePoints.push_back(pxy);
    //     recalculate();
    changeBoundBox(x,y);
    break;
  }
}


/*
  Draws the weather symbol
 */
bool WeatherSymbol::plot()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG(" Weather symbol::plot()");
  METLIBS_LOG_DEBUG(" drawIndex = " << drawIndex);
#endif
  if (!isEnabled()) return false;

  if (isVisible){

    setWindowInfo();

    //change the symbolsize to plot according to great circle distance
    float scalefactor = getStaticPlot()->getGcd()/7000000;
    int symbolSizeToPlot = int(symbolSize/scalefactor);

    //also scale according to windowheight and width (standard is 500)
    scalefactor = sqrtf(getStaticPlot()->getPhysHeight()*getStaticPlot()->getPhysHeight()+getStaticPlot()->getPhysWidth()*getStaticPlot()->getPhysWidth())/500;
    symbolSizeToPlot = int(symbolSizeToPlot*scalefactor);

    fSense = symbolSizeToPlot/12;  //  sensitivity to mark object
    if (fSense < 1.0) fSense = 1.0; //HK ???

    glPushMatrix();

    //enable blending and set colour
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // SMHI
    //if (drawIndex == 900 ) setObjectColor(currentColour);
    glColor4ub(objectColour.R(),objectColour.G(),objectColour.B(),objectColour.A());

    if (drawIndex == 3001){
        if (complexSymbol!=0){
          complexSymbol->drawTextBox(drawIndex,symbolSizeToPlot,rotation);
        }
    }
    else
    {

      int end = nodePoints.size();
      for (int i=0; i<end; i++) {
        float cw=0,ch=0;
        float x=nodePoints[i].x;
        float y=nodePoints[i].y;
        if (drawIndex == 900 || (drawIndex >=1000 && drawIndex<=3000)){
          // this is a complex symbol
          if (complexSymbol!=0){
            complexSymbol->draw(drawIndex,x,y,symbolSizeToPlot,rotation);
            complexSymbol->getComplexBoundingBox(drawIndex,cw,ch,x,y);
          }

        } else if (drawIndex>0) {
          // this is a normal symbol
          getStaticPlot()->getFontPack()->set("METSYMBOLFONT",poptions.fontface,symbolSizeToPlot);
          getStaticPlot()->getFontPack()->getCharSize(drawIndex,cw,ch);
          getStaticPlot()->getFontPack()->drawChar(drawIndex,x-cw/2,y-ch/2,0.0);

        } else if (drawIndex==0){
          // this is a normal text
          getStaticPlot()->getFontPack()->set(poptions.fontname,poptions.fontface,symbolSizeToPlot);
          getStaticPlot()->getFontPack()->getStringSize(symbolString.c_str(),cw,ch);
          getStaticPlot()->getFontPack()->drawStr(symbolString.c_str(),x-cw/2,y-ch/2,0.0);
        }

        // update boundBox according to symbolSizeToPlot
        float PI       = acosf(-1.0);
        float angle = PI*rotation/180.;
        float cwr= fabsf(cos(angle))*cw+fabsf(sin(angle))*ch;
        float chr= fabsf(cos(angle))*ch+fabsf(sin(angle))*cw;
        boundBox.x1=x-cwr/2;
        boundBox.x2=x+cwr/2;
        boundBox.y1=y-chr/2;
        boundBox.y2=y+chr/2;
      }
    }

    glPopMatrix();
    glDisable(GL_BLEND);

    drawNodePoints();
  }
  // for PostScript generation
  getStaticPlot()->UpdateOutput();

  return true;
}


void WeatherSymbol::setSymbolSize(float si){
  if (si <0.1) symbolSize = 0.1;
  else if (si >= 201) symbolSize = 200;
  else if(si > 0 && si < 201) symbolSize=si;
  //fSense = symbolSize/12;  //  sensitivity to mark object
  //if (fSense < 1.0) fSense = 1.0;
}

void WeatherSymbol::increaseSize(float val){
  if(symbolSize<2.0) val/=10.;
  setSymbolSize(symbolSize+val);
  //non-complex symbol- new defaultsize
  if (drawIndex < 1000) defaultSize = symbolSize;
}


void WeatherSymbol::setDefaultSize( ){
  if (drawIndex < 1000) {
    setSymbolSize(defaultSize);
 } else
    setSymbolSize(defaultComplexSize);
}


void WeatherSymbol::changeDefaultSize( ){
  if (drawIndex < 1000 ) defaultSize = symbolSize;
}

void WeatherSymbol::setStandardSize(int size1,int size2){
  defaultSize=size1;
  defaultComplexSize=size2;
}


void WeatherSymbol::setType(int ty){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("WeatherSymbol::setType(int)" << ty);
#endif
  if (typeOfObject ==wSymbol){
    if (-1<ty && ty<int(allSymbols.size()))
      type= ty;
    else if (ty==int(allSymbols.size()))
      type=0;
    else if (ty==-1)
      type =allSymbols.size()-1;
    else
      return;
    setIndex(allSymbols[type].index);
    setBasisColor(allSymbols[type].colour);

    if (drawIndex < 1000){
    setSymbolSize(defaultSize);
    }else
      setSymbolSize(defaultComplexSize);

    if ((drawIndex == 900 || drawIndex>=1000) && complexSymbol==0) {
      complexSymbol= new ComplexSymbolPlot(drawIndex);
      complexSymbol->setBorderColour(allSymbols[type].borderColour);
      setSymbolSize(defaultComplexSize + allSymbols[type].sizeIncrement);
      if (drawIndex == 900 ) setObjectColor(currentColour);
    }
    if (isText()){
      if (currentText.empty())
        setString("Text");
      else
        setString(currentText);
      setObjectColor(currentColour);
    }
  } else if (typeOfObject ==RegionName){
    if (-1<ty && ty<int(allRegions.size()))
      type= ty;
    else if (ty==int(allRegions.size()))
      type=0;
    else if (ty==-1)
      type =allRegions.size()-1;
    else
      return;
    setIndex(allRegions[type].index);
    setString(allRegions[type].name);
    setBasisColor(allRegions[type].colour);
  }
}


void WeatherSymbol::increaseType(int val){
  if (typeOfObject ==wSymbol){
    int ty;
    if (val==1)
      ty = next[type];
    else if (val==-1)
      ty = last[type];
    else
      return;
    setType(ty);
  } else if (typeOfObject==RegionName)
    setType(type+val);
}


bool WeatherSymbol::setType(std::string tystring){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("WeatherSymbol::setType(std::string)=" << tystring <<  endl);
#endif
  if (objectIs(wSymbol) && symbolTypes.find(tystring)!=symbolTypes.end()){
    setType(symbolTypes[tystring]);
    return true;
  }
  else if (objectIs(RegionName) && regionTypes.find(tystring)!= regionTypes.end()){
    setType(regionTypes[tystring]);
    return true;
  }
  return false;
}



bool WeatherSymbol::isOnObject(float x,float y){
  markedChanged=false;

  if (boundBox.isinside(x,y)) {
    if (inBoundBox==false) markedChanged=true;
    inBoundBox=true;
  }
  else{
    if (inBoundBox==true) markedChanged=true;
    inBoundBox=false;
  }

  if (isInside(x,y)){
    markPoint(x,y);
    return true;
  }
  else{
    unmarkAllPoints();
    return false;
  }
  return false;
}


string WeatherSymbol::writeTypeString()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("WeatherSymbol::writeTypeString");
#endif
  std::string ret,tstring ;
  ostringstream cs;
  if (objectIs(wSymbol) || objectIs(RegionName)){
    if (objectIs(wSymbol)){
      ret ="Object=Symbol;\n";
      //HACK: bdiana do not know Sig_snow, Sig_showers, Sig_snow_showers yet
      if( allSymbols[type].name == "Sig_snow" ) {
        tstring = "Type=Snow;\n";
      } else if( allSymbols[type].name == "Sig_showers" ) {
          tstring = "Type=Showers;\n";
      } else if( allSymbols[type].name == "Sig_snow_showers" ) {
        tstring = "Type=Snow showers;\n";
      } else {
        tstring = "Type=" + allSymbols[type].name + ";\n";
      }
    }
    if (objectIs(RegionName)){
      ret ="Object=RegionName;\n";
      tstring = "Type=" + allRegions[type].name + ";\n";
    }
    cs << "Size=" << symbolSize<< ";\n";
    if (drawIndex==0){
      //text
      std::string tempString=symbolString;
      //replace !:
      replaceText(tempString,true);
      tstring+= "Text=" +tempString+ ";\n";
    }
    else if (drawIndex==900 || drawIndex>=1000){
      if (complexSymbol!=0){
        tstring+=complexSymbol->writeComplexText();
        cs << "Rotation=" << rotation <<";\n";
        cs << "Whitebox=" << complexSymbol->hasWhiteBox()<< ";\n";
      }
    }
  }
  ret+=tstring;
  ret+=cs.str();
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("ret=" << ret);
#endif
  return ret;
}


void WeatherSymbol::setString(const std::string& s)
{
  string tempString=s;
  replaceText(tempString,false);
  symbolString = tempString;
  textlist.insert(symbolString);
}


void WeatherSymbol::applyFilters(const std::vector<std::string>& symbolfilter)
{
  for (unsigned int i=0;i<symbolfilter.size();i++){
    if (allSymbols[type].name==symbolfilter[i]){
      isVisible=false;
      return;
    }
  }
}

/************************************************************
 *  Methods for editing complex symbols                     *
 ************************************************************/
void WeatherSymbol::getComplexText(vector<string>& symbolText, vector<string>& xText)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("WeatherSymbol::getComplexText");
#endif
  if (complexSymbol!=0) {
    if (drawIndex==900)
      complexSymbol->getComplexColoredText(symbolText,xText);
    else if (drawIndex<=3000) 
      complexSymbol->getComplexText(symbolText,xText);

  }
}

void WeatherSymbol::getMultilineText(vector<string>& symbolText)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("WeatherSymbol::getMultilineText");
#endif
  if (complexSymbol!=0) {
    if (drawIndex>=3000)
      complexSymbol->getMultilineText(symbolText);
  }
}

void WeatherSymbol::readComplexText(std::string s){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("WeatherSymbol::readComplexText " <<  s);
#endif
  if (complexSymbol!=0)
    complexSymbol->readComplexText(s);
}

void WeatherSymbol::changeMultilineText(const vector <std::string> & symbolText){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("WeatherSymbol::changeMultilineText");
#endif
  if (complexSymbol!=0) {
    if (drawIndex>=3000)
      complexSymbol->changeMultilineText(symbolText);
  }
}

void WeatherSymbol::changeComplexText(const vector <std::string> & symbolText, const vector <std::string> & xText){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("WeatherSymbol::changeComplexText");
#endif
  if (complexSymbol!=0) {
    if (drawIndex<=3000)  
      complexSymbol->changeComplexText(symbolText,xText);
  }
}


void WeatherSymbol::rotateObject(float val){
  //only works for complex objects
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("WeatherSymbol::rotateObject");
#endif
  rotation+=val;
}


void WeatherSymbol::hideBox(){
  //only works for complex objects
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("WeatherSymbol::hideBox");
#endif
  if (complexSymbol!=0)
    complexSymbol->hideBox();
}


void WeatherSymbol::setWhiteBox(int on){
  //only works for complex objects
  if (complexSymbol!=0)
    complexSymbol->setWhiteBox(on);
}





void WeatherSymbol::replaceText(string& tempString, bool writestring)
{
  //replace !;=#,:
  if (writestring){
    miutil::replace(tempString, "!","{exclamationmark}");
    miutil::replace(tempString, ";","{semicolon}");
    miutil::replace(tempString, "=","{equals}");
    miutil::replace(tempString, "#","{hash}");
    miutil::replace(tempString, ",","{comma}");
    miutil::replace(tempString, ":","{colon}");
    miutil::replace(tempString, "/","{slash}");
  } else {
    miutil::replace(tempString, "{exclamationmark}","!");
    miutil::replace(tempString, "{semicolon}",";");
    miutil::replace(tempString, "{equals}","=");
    miutil::replace(tempString, "{hash}","#");
    miutil::replace(tempString, "{comma}",",");
    miutil::replace(tempString, "{colon}",":");
    miutil::replace(tempString, "{slash}","/");
  }
}
