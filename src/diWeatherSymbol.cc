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
#include <diWeatherSymbol.h>
#include <diFontManager.h>
#include <sstream>
#include <diColour.h>
#include <math.h>

//static variables
vector<editToolInfo> WeatherSymbol::allSymbols;
vector<editToolInfo> WeatherSymbol::allRegions;
map<miString,int> WeatherSymbol::symbolTypes;     //finds symbol type number
map<miString,int> WeatherSymbol::regionTypes;     //finds region type number
map<int,int> WeatherSymbol::indexTypes;           //finds symbol type number
map<int,int> WeatherSymbol::next;           //finds next type number
map<int,int> WeatherSymbol::last;           //finds last type number
float WeatherSymbol::defaultSize=60.;
float WeatherSymbol::defaultComplexSize=6.;
miString WeatherSymbol::currentText;
Colour::ColourInfo WeatherSymbol::currentColour; //text colour
set <miString> WeatherSymbol::textlist; //texts used in combobox

//Constructor
WeatherSymbol::WeatherSymbol() : ObjectPlot(wSymbol),complexSymbol(0){
#ifdef DEBUGPRINT
  cerr << "Weather symbol- default constructor" << endl;
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
  cerr << "Weather symbol - int constructor" << endl;
#endif
  setType(ty);
  if (drawIndex < 1000)
    setSymbolSize(defaultSize);
  else
    setSymbolSize(defaultComplexSize);
}


//constructor taking symboltype and type of object as argument
WeatherSymbol::WeatherSymbol(miString tystring,int objTy) : ObjectPlot(objTy),
  symbolSize(defaultSize),complexSymbol(0){
#ifdef DEBUGPRINT
  cerr << "Weather symbol(miString,int) constructor" << endl;
#endif
  // set correct symboltype
  if (tystring.empty())
    setType(0);
  else if (!setType(tystring))
    cerr << "WeatherSymbol constructor error, type " << tystring << " not found !!!" << endl;  
  if (drawIndex < 1000)
    setSymbolSize(defaultSize);
  else
    setSymbolSize(defaultComplexSize);

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
  cerr << "WeatherSymbol destructor" << endl;
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
    for (int i=0; i<regions.size(); i++)
      regionTypes[regions[i].name] = i;
  }
  
}



void WeatherSymbol::setCurrentText(const miString & newText){
  currentText=newText;
}


set <miString> WeatherSymbol::getTextList(){
  return textlist;
}


void WeatherSymbol::setCurrentColour(const Colour::ColourInfo & newColour){
  currentColour=newColour;
}


miString WeatherSymbol::getCurrentText(){
  return currentText;
}

Colour::ColourInfo WeatherSymbol::getCurrentColour(){
  return currentColour;
}

void WeatherSymbol::initComplexList(){
  ComplexSymbolPlot::initComplexList();
}

set <miString> WeatherSymbol::getComplexList(){
  return ComplexSymbolPlot::getComplexList();
}



bool WeatherSymbol::isSimpleText(miString edittool){
  //return true if editool corresponds to simple text, type 0
  if (allSymbols[symbolTypes[edittool]].index == 0)
    return true;
  else
    return false;
}


bool WeatherSymbol::isComplexText(miString edittool){
  //return true if editool corresponds to simple text, type 0
  //cerr << "WeatherSymbol::isComplexText - edittool=" << edittool << endl;
  int edIndex=allSymbols[symbolTypes[edittool]].index;
  if (edIndex >=1000){
    return ComplexSymbolPlot::isComplexText(edIndex);
  }
  else
    return false;
}



void WeatherSymbol::getCurrentComplexText(vector <miString> & symbolText, 
					  vector <miString> & xText){
  ComplexSymbolPlot::getCurrentComplexText(symbolText,xText);
}




void WeatherSymbol::setCurrentComplexText(const vector <miString> &
symbolText, const vector <miString> & xText){
  ComplexSymbolPlot::setCurrentComplexText(symbolText,xText);
}


void WeatherSymbol::initCurrentComplexText(miString edittool){
  int edIndex=allSymbols[symbolTypes[edittool]].index;
  if (edIndex >=1000){
    ComplexSymbolPlot::initCurrentStrings(edIndex);
  }
}



miString WeatherSymbol::getAllRegions(int ir){
  miString region;
  if (allRegions.size()>ir) region=allRegions[ir].name;
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
  if (!enabled) return false;

  if (isVisible){
    
    setWindowInfo();
 
    //change the symbolsize to plot according to great circle distance
    float scalefactor = gcd/7000000;
    int symbolSizeToPlot = int(symbolSize/scalefactor);
    
    //also scale according to windowheight and width (standard is 500)
    scalefactor = sqrtf(pheight*pheight+pwidth*pwidth)/500;
    symbolSizeToPlot = int(symbolSizeToPlot*scalefactor); 

    fSense = symbolSizeToPlot/12;  //  sensitivity to mark object 
    if (fSense < 1.0) fSense = 1.0; //HK ???

    glPushMatrix();
  
    //enable blending and set colour
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(col->R(),col->G(),col->B(),col->A());

    int end = nodePoints.size();      
    for (int i=0; i<end; i++) {
      float cw,ch;
      float x=nodePoints[i].x;
      float y=nodePoints[i].y;
      if (drawIndex>=1000){
	// this is a complex symbol 
	if (complexSymbol!=0){
	  complexSymbol->draw(drawIndex,x,y,symbolSizeToPlot,rotation);
	  complexSymbol->getComplexBoundingBox(drawIndex,cw,ch,x,y);
	}

      } else if (drawIndex>0) {
	// this is a normal symbol 
	fp->set("METSYMBOLFONT",poptions.fontface,symbolSizeToPlot);      
	fp->getCharSize(drawIndex,cw,ch);
	fp->drawChar(drawIndex,x-cw/2,y-ch/2,0.0);

      } else if (drawIndex==0){
	// this is a normal text
	fp->set(poptions.fontname,poptions.fontface,symbolSizeToPlot);
	fp->getStringSize(symbolString.c_str(),cw,ch);
	fp->drawStr(symbolString.c_str(),x-cw/2,y-ch/2,0.0);   
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
	
    glPopMatrix();
    glDisable(GL_BLEND);  

    drawNodePoints();
  }
  // for PostScript generation
  UpdateOutput();

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
  if (drawIndex < 1000)
    setSymbolSize(defaultSize);
  else
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
  cerr << "WeatherSymbol::setType(int)" << ty << endl;
#endif
  if (typeOfObject ==wSymbol){
    if (-1<ty && ty<allSymbols.size())
      type= ty;
    else if (ty==allSymbols.size())
      type=0;
    else if (ty==-1)
      type =allSymbols.size()-1;
    else 
      return;
    setIndex(allSymbols[type].index);
    setBasisColor(allSymbols[type].colour);
    if (drawIndex>=1000 && complexSymbol==0) {
      complexSymbol= new ComplexSymbolPlot(drawIndex);
      complexSymbol->setBorderColour(allSymbols[type].borderColour);
    }
    if (isText()){
      if (currentText.empty()) 
	setString("Text");
      else
	setString(currentText);
      setObjectColor(currentColour);
    }
  } else if (typeOfObject ==RegionName){
    if (-1<ty && ty<allRegions.size())
      type= ty;
   else if (ty==allRegions.size())
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


bool WeatherSymbol::setType(miString tystring){
#ifdef DEBUGPRINT
  cerr << "WeatherSymbol::setType(miString)=" << tystring <<  endl;
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


miString WeatherSymbol::writeTypeString()
{
#ifdef DEBUGPRINT
  cerr <<"WeatherSymbol::writeTypeString" << endl;
#endif
  miString ret,tstring ;
  ostringstream cs;
  if (objectIs(wSymbol) || objectIs(RegionName)){
    if (objectIs(wSymbol)){
      ret ="Object=Symbol;\n";
      tstring = "Type=" + allSymbols[type].name + ";\n";
    }
    if (objectIs(RegionName)){
      ret ="Object=RegionName;\n";    
      tstring = "Type=" + allRegions[type].name + ";\n";
    }
    cs << "Size=" << symbolSize<< ";\n";
    if (drawIndex==0){
      //text 
      miString tempString=symbolString;
      //replace !:
      replaceText(tempString,true);
      tstring+= "Text=" +tempString+ ";\n";
    }      
    else if (drawIndex>=1000){
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
  cerr << "ret=" << ret << endl;
#endif
  return ret;
}


void WeatherSymbol::setString(miString s){
  miString tempString=s;
  replaceText(tempString,false);
  symbolString = tempString;
  textlist.insert(symbolString);
}


void WeatherSymbol::applyFilters(vector <miString> symbolfilter){
  for (int i=0;i<symbolfilter.size();i++){
    if (allSymbols[type].name==symbolfilter[i]){
      isVisible=false;
      return;
    }
  }
}

/************************************************************
 *  Methods for editing complex symbols                     *
 ************************************************************/
void WeatherSymbol::getComplexText(vector <miString> & symbolText, vector <miString> & xText){
#ifdef DEBUGPRINT
  cerr << "WeatherSymbol::getComplexText" << endl;
#endif
  if (complexSymbol!=0)
    complexSymbol->getComplexText(symbolText,xText);
}

void WeatherSymbol::readComplexText(miString s){
#ifdef DEBUGPRINT
  cerr << "WeatherSymbol::readComplexText " <<  s << endl;
#endif
  if (complexSymbol!=0)
    complexSymbol->readComplexText(s);
}


void WeatherSymbol::changeComplexText(const vector <miString> & symbolText, const vector <miString> & xText){
#ifdef DEBUGPRINT
cerr << "WeatherSymbol::changeComplexText" << endl;
#endif
  if (complexSymbol!=0)
    complexSymbol->changeComplexText(symbolText,xText);
}


void WeatherSymbol::rotateObject(float val){
 //only works for complex objects
#ifdef DEBUGPRINT
  cerr << "WeatherSymbol::rotateObject" << endl;
#endif
  rotation+=val;
}


void WeatherSymbol::hideBox(){
 //only works for complex objects
#ifdef DEBUGPRINT
  cerr << "WeatherSymbol::hideBox" << endl;
#endif
  if (complexSymbol!=0)
    complexSymbol->hideBox();
}


void WeatherSymbol::setWhiteBox(int on){
//only works for complex objects
  if (complexSymbol!=0)
    complexSymbol->setWhiteBox(on);
} 





void WeatherSymbol::replaceText(miString & tempString,bool writestring){
  //replace !;=#,:
  if (writestring){
    tempString.replace("!","{exclamationmark}");
    tempString.replace(";","{semicolon}");
    tempString.replace("=","{equals}");
    tempString.replace("#","{hash}");
    tempString.replace(",","{comma}");
    tempString.replace(":","{colon}");
    tempString.replace("/","{slash}");
  } else {
    tempString.replace("{exclamationmark}","!");
    tempString.replace("{semicolon}",";");
    tempString.replace("{equals}","=");
    tempString.replace("{hash}","#");
    tempString.replace("{comma}",",");
    tempString.replace("{colon}",":");
    tempString.replace("{slash}","/");
  }
}






