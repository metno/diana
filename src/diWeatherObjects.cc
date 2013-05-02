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
//#define DEBUGPRINT

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diWeatherObjects.h>
#include <diDrawingTypes.h>
#include <diWeatherFront.h>
#include <diWeatherSymbol.h>
#include <diWeatherArea.h>
#include <diShapeObject.h>

#include <fstream>
#include <iomanip>

using namespace::miutil;

//static
miTime WeatherObjects::ztime = miTime(1970,1,1,0,0,0);


/*********************************************/

WeatherObjects::WeatherObjects()
: xcopy(0), ycopy(0)
{
#ifdef DEBUGPRINT
  cerr << "WeatherObjects constructor" << endl;
#endif
  //zero time = 00:00:00 UTC Jan 1 1970
  itsTime=ztime;

  // correct spec. when making Projection for long/lat coordinates
  Projection geop;
  geop.setGeographic();
  geop.setUsingLatLonValues(true);
  geoArea.setP(geop);

  useobject.clear();
  //use all objects if nothing else specified
  for (int i=0; i<numObjectTypes; i++)
    useobject[ObjectTypeNames[i]]= true;
  enabled=true;
}

/*********************************************/

void WeatherObjects::clear()
{
#ifdef DEBUGPRINT
  cerr << "WeatherObjects::clear" << endl;
#endif
  int no = objects.size();
  for (int i=0; i<no; i++)
    delete objects[i];
  objects.clear();
  prefix = miString();
  filename= miString();
  itsOldComments = miString();
  itsLabels.clear();
  itsOldLabels.clear();

}


/*********************************************/

bool WeatherObjects::empty(){
  return (objects.size() == 0 && itsLabels.size()==0);
}

/*********************************************/

void WeatherObjects::plot(){
#ifdef DEBUGPRINT
  cerr << "WeatherObjects::plot\n";
#endif
  if (!enabled) return;
  // draw objects
  int n= objects.size();
  //draw areas, then fronts, then symbols
  for (int i=0; i<n; i++){
    if (objects[i]->objectIs(wArea)){
#ifdef DEBUGPRINT
  cerr << "WeatherObjects:wArea plotted  ";
#endif
      objects[i]->plot();}
  }

  for (int i=0; i<n; i++){
    if (objects[i]->objectIs(wFront)){
#ifdef DEBUGPRINT
  cerr << "WeatherObjects:wFront plotted  ";
#endif

      objects[i]->plot();}
  }

  for (int i=0; i<n; i++){
    if (objects[i]->objectIs(wSymbol)){
#ifdef DEBUGPRINT
  cerr << "WeatherObjects:wSymbol plotted  ";
#endif

      objects[i]->plot();}
  }


  for (int i=0; i<n; i++){
    if (objects[i]->objectIs(Border)){
#ifdef DEBUGPRINT
  cerr << "WeatherObjects:wBorder plotted  ";
#endif

      objects[i]->plot();}
  }


  for (int i=0; i<n; i++){
    if (objects[i]->objectIs(RegionName)){
#ifdef DEBUGPRINT
  cerr << "WeatherObjects:wRegionName plotted  ";
#endif

      objects[i]->plot();}
  }


  for (int i=0; i<n; i++){
    if (objects[i]->objectIs(ShapeXXX)){
#ifdef DEBUGPRINT
  cerr << "WeatherObjects:wShape plotted  ";
#endif

      objects[i]->plot();}
  }



}

/*********************************************/

bool WeatherObjects::changeProjection(const Area& newArea)
{

#ifdef DEBUGPRINT
  cerr << "WeatherObjects::changeProjection" << endl;
  cerr << "Change projection from " << itsArea <<endl<<" to " <<
  newArea << endl;
#endif


  if (itsArea.P() == newArea.P()) return false;

  if (empty())
  {
    itsArea= newArea;
    return false;
  }

  int i,j,npos= 0;
  int obsize = objects.size();

  //npos = number of points to be transformed = all object points
  for (i=0; i<obsize; i++) npos+= objects[i]->getXYZsize();
  //plus one copy point(xopy,ycopy)
  npos++;

  float *xpos = new float[npos];
  float *ypos = new float[npos];
  int m,n= 0;

  for (i=0; i<obsize; i++){
    m= objects[i]->getXYZsize();
    vector<float> x=objects[i]->getX();
    vector<float> y=objects[i]->getY();
    for (j=0; j<m; ++j) {
      xpos[n]= x[j];
      ypos[n]= y[j];
      n++;
    }
  }

  xpos[n]=xcopy;
  ypos[n]=ycopy;

  int ierror=0;
  bool err=true;
  if ( itsArea.P().getUsingLatLonValues()) {
    ierror = newArea.P().convertFromGeographic(npos,xpos,ypos);
  } else if ( newArea.P().getUsingLatLonValues()) {
      ierror = itsArea.P().convertToGeographic(npos,xpos,ypos);
  } else {
    err = gc.getPoints(itsArea.P(),newArea.P(),npos,xpos,ypos);
  }

  if(!err || ierror !=0 ) {
    cerr << "WeatherObjects::changeProjection: getPoints error" << endl;
    delete[] xpos;
    delete[] ypos;
    return false;
  }

  xcopy=xpos[n];
  ycopy=ypos[n];

  n= 0;
  for (i=0; i<obsize; i++){
    m= objects[i]->getXYZsize();
    vector<float> x;
    vector<float> y;
    for (j=0; j<m; ++j) {
      x.push_back(xpos[n]);
      y.push_back(ypos[n]);
      n++;
    }
    objects[i]->setXY(x,y);
  }

  delete[] xpos;
  delete[] ypos;

  itsArea= newArea;

  updateObjects();

  return true;
}


/*********************************************/

void WeatherObjects::updateObjects()
{
#ifdef DEBUGPRINT
  cerr << "WeatherObjects::updateObjects" << endl;
#endif

  int obsize = objects.size();

  for (int i=0; i<obsize; i++) objects[i]->updateBoundBox();
}


/*********************************************/

bool
WeatherObjects::readEditDrawFile(const miString fn,const Area& newArea){
#ifdef DEBUGPRINT
  cerr << "WeatherObjects::readEditDrawFile(2)" << endl;
  cerr << "filename" << fn << endl;
#endif


  //if *.shp read shapefile
  vector <miString> parts= fn.split('.');
  miString ext = parts.back();
  if (ext=="shp"){
    if (newArea.P()!=itsArea.P()){
      changeProjection(newArea);
    }
    changeProjection(geoArea);

    cerr << "This is a shapefile" << endl;
    ShapeObject * shape = new ShapeObject();
    addObject(shape);
    shape->read(fn);

    changeProjection(newArea);

    return true;
  }

  // open filestream
  ifstream file(fn.c_str());
  if (!file){
    cerr << "ERROR OPEN (READ) " << fn << endl;
    return false;
  }


  /* ---------------------------------------------------------------------
 -----------------------start to read new format--------------------------
  -----------------------------------------------------------------------*/
  miString str,value,key,fileString;

  // read the first line check if it contains "date"
  getline(file,str);
  //cerr << "The first line read is " << str << endl;
  vector<miString> stokens = str.split('=');
  if ( stokens.size()==2) {
    key = stokens[0].downcase();
    value = stokens[1];
  }
  //check if the line contains keyword date ?
  if (key == "date")
  {
    fileString = miString();
    // read file
    while (getline(file,str) && !file.eof()){
      if (str.exists() && str[0]!='#'){
        // The font Helvetica is not supported if X-fonts are not enabled, use BITMAPFONT defined in setup
        if (str.contains("Helvetica")) {
          str.replace("Helvetica","BITMAPFONT");
        }
        //check if this is a LABEL string
        if (str.substr(0,5)=="LABEL"){
          if (useobject["anno"])
            itsOldLabels.push_back(str);
        } else {
          fileString+=str;
        }
      }
    }
    file.close();

    return readEditDrawString(fileString,newArea);
  }
  else{
    cerr << "This file is not in the new format " << endl;
    file.close();
  }

  return false;

}



bool WeatherObjects::readEditDrawString(const miString inputString,
    const Area& newArea, bool replace){

#ifdef DEBUGPRINT
  cerr << "readEditDrawString\n";
  cerr << "Input string" << inputString << endl;
#endif

  miString key,value,objectString;

  // nb ! if useobject not true for an objecttype, no objects used(read) for
  // this object type. Useobject is set in WeatherObjects constructor and
  // also in DisplayObjects::define and in EditManager::startEdit

  // first convert existing objects to geographic coordinates
  if (newArea.P()!=itsArea.P()){
    changeProjection(newArea);
  }
  changeProjection(geoArea);


  //split inputString into one string for each object
  vector <miString> objectStrings = inputString.split('!');

  for (unsigned int i = 0;i<objectStrings.size();i++){
    //split objectString and check which type of new
    //object should be created from first keyword and value
    vector <miString> tokens = objectStrings[i].split(';');
    vector <miString> stokens = tokens[0].split('=');
    if ( stokens.size()==2) {
      key = stokens[0].downcase();
      value = stokens[1];
    } else {
      cerr << "WeatherObjects::readEditDrawString - Warning !";
      cerr << "Error in objectString " << objectStrings[i] << endl;
      continue;
    }
    if (key == "date"){
      //cerr << "date of object file = " << timeFromString(value) << endl;
    }
    else if (key == "object"){
      ObjectPlot * tObject;
      if (value == "Front")
        if (useobject["front"])
          tObject = new WeatherFront();
        else continue;
      else if (value == "Symbol")
        if (useobject["symbol"])
          tObject = new WeatherSymbol();
        else continue;
      else if (value == "Area")
        if (useobject["area"])
          tObject = new WeatherArea();
        else continue;
      else if (value == "Border")
        tObject = new AreaBorder();
      else if (value == "RegionName")
        tObject = new WeatherSymbol("",RegionName);
      else {
        cerr << "WeatherObjects::readEditDrawString Unknown object:"
        << value << endl;
        continue;
      }
      if (tObject->readObjectString(objectStrings[i]))
        //add a new object
        addObject(tObject,replace);
      else delete tObject;
    }
    else cerr << "Error! Object key not found !" << endl;
  }

  changeProjection(newArea);

  return true;
}

miString WeatherObjects::writeEditDrawString(const miTime& t){

#ifdef DEBUGPRINT
  cerr << "WeatherObjects::writeEditDrawString" << endl;
#endif
  if (empty()) return miString();

  Area oldarea= itsArea;
  changeProjection(geoArea);

  //output stream
  ostringstream ostr;

  //write out the date
  miString date ="Date="+stringFromTime(t,true)+";";
  ostr << date << endl << endl;

  vector <ObjectPlot*>::iterator p = objects.begin();
  while (p!=objects.end()){
    ObjectPlot * pobject = *p;
    ostr << pobject->writeObjectString();
    p++;
  }

  changeProjection(oldarea);

  miString objectString = ostr.str();
  int n=itsLabels.size();
  for  (int i=0;i<n;i++)
    objectString+=itsLabels[i]+"\n";
  return objectString;
}


/************************************************
 *  Methods for reading comments  ****************
 *************************************************/

bool WeatherObjects::readEditCommentFile(const miString fn){
#ifdef DEBUGPRINT
  cerr << "WeatherObjects::readEditCommentFile" << fn << endl;
#endif

  // open filestream
  ifstream file(fn.c_str());
  if (!file){
#ifdef DEBUGPRINT
    cerr << "not found " << fn << endl;
#endif
    return false;
  }

  miString str,fileString;

  fileString = miString();
  // read file
  while (getline(file,str) && !file.eof())
    fileString+=str+"\n";

  file.close();

  itsOldComments += fileString;

#ifdef DEBUGPRINT
  cerr <<"itsOldComments" << itsOldComments << endl;
#endif

  return true;
}


miString WeatherObjects::readComments(){
#ifdef DEBUGPRINT
  cerr << "WeatherObjects::Read comments" << endl;
#endif
  //read the old comments
  if (itsOldComments.empty())
    return "No comments";
  else
    return itsOldComments;
}

/************************************************
 *  Methods for reading and writing labels *******
 *************************************************/

vector <miString> WeatherObjects::getObjectLabels(){
#ifdef DEBUGPRINT
  cerr << "WeatherObjects::getObjectLabels" << endl;
#endif
  //oldLabels from object file
  return itsOldLabels;
}

vector <miString> WeatherObjects::getEditLabels(){
#ifdef DEBUGPRINT
  cerr << "WeatherObjects::getEditLabels" << endl;
#endif
  //new edited labels
  return itsLabels;
}

/************************************************
 *  Methods for reading and writing areaBorders  *
 *************************************************/

bool WeatherObjects::readAreaBorders(const miString fn,

    const Area& newArea){
#ifdef DEBUGPRINT
  cerr << "WeatherObjects::readAreaBorders" << endl;
  cerr << "filename = " << fn << endl;
#endif

  // open filestream
  ifstream file(fn.c_str());
  if (!file){
    cerr << "ERROR OPEN (READ) " << fn << endl;
    return false;
  }


  miString str,fileString;

  fileString = miString();
  // read file
  while (getline(file,str) && !file.eof())
    fileString+=str+"\n";

  file.close();

  return readEditDrawString(fileString,newArea);

}



bool WeatherObjects::writeAreaBorders(const miString fn){

  if (empty()) return false;

  // open filestream
  ofstream file(fn.c_str());
  if (!file){
    cerr << "ERROR OPEN (WRITE) " << fn << endl;
    return false;
  }

  Area oldarea= itsArea;
  changeProjection(geoArea);

  vector <ObjectPlot*>::iterator p = objects.begin();
  while (p!=objects.end()){
    ObjectPlot * pobject = *p;
    if (pobject->objectIs(Border))
      file << pobject->writeObjectString();
    p++;
  }


  file.close();

  changeProjection(oldarea);
  return true;
}




// ---------------------------------------------------------------
// ---------------------------------------------------------------
// ---------------------------------------------------------------


int WeatherObjects::objectCount(int type ){
  int ncount= 0;
  vector <ObjectPlot*>::iterator p = objects.begin();
  while (p!=objects.end()){
    ObjectPlot * pobject = *p;
    if (pobject->objectIs(type)) ncount++;
    p++;
  }
  return ncount;
}

void WeatherObjects::addObject(ObjectPlot * object, bool replace){
#ifdef DEBUGPRINT
  cerr <<"WeatherObjects::addObject" << endl;
#endif
  if (!object) return;

  if(replace){ // remove old object
    vector <ObjectPlot*>::iterator p = objects.begin();
    while (p!=objects.end() && (*p)->getName()!= object->getName())p++;
    if(p!=objects.end()){
      objects.erase(p);
    }
  }

  objects.push_back(object);
  object->setRegion(prefix);
}

vector<ObjectPlot*>::iterator
WeatherObjects::removeObject(vector<ObjectPlot*>::iterator p){
  return objects.erase(p);
}


/*********************************************/

miString WeatherObjects::stringFromTime(const miTime& t,bool addMinutes){
  int yyyy= t.year();
  int mm  = t.month();
  int dd  = t.day();
  int hh  = t.hour();
  int mn  = t.min();

  ostringstream ostr;
  ostr << setw(4) << setfill('0') << yyyy
  << setw(2) << setfill('0') << mm
  << setw(2) << setfill('0') << dd
  << setw(2) << setfill('0') << hh;
  if (addMinutes)
    ostr << setw(2) << setfill('0') << mn;

  miString timestring = ostr.str();
  return timestring;
}


/*********************************************/

miTime WeatherObjects::timeFromString(miString timeString)
{
  if (timeString.length()<10) return ztime;
  //get time from a string with yyyymmddhhmm
  int year= atoi(timeString.substr(0,4).c_str());
  int mon=  atoi(timeString.substr(4,2).c_str());
  int day=  atoi(timeString.substr(6,2).c_str());
  int hour= atoi(timeString.substr(8,2).c_str());
  int min= 0;
  if (timeString.length() >= 12)
    min= atoi(timeString.substr(10,2).c_str());
  if (year<0 || mon <0 || day<0 || hour<0 || min < 0) return ztime;
  return miTime(year,mon,day,hour,min,0);
}

/*********************************************/

map<miString,bool> WeatherObjects::decodeTypeString( miString token){

  map<miString,bool> use;
  for (int i=0; i<numObjectTypes; i++)
    use[ObjectTypeNames[i]]= false;
  vector<miString> stokens;
  //types of objects to plot
  token= token.substr(6,token.size()-6);
  stokens= token.split(',');
  int m= stokens.size();
  for (int j=0; j<m; j++){
    if (stokens[j]=="all"){
      for (int k=0; k<numObjectTypes; k++)
        use[ObjectTypeNames[k]]= true;
      break;
    }
    use[stokens[j]]= true;
  }
  return use;
}


void WeatherObjects::enable(const bool b)
{
  enabled = b;
}

bool WeatherObjects::isEnabled()
{
  return enabled;
}
