/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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

#include "diWeatherObjects.h"
#include "diDrawingTypes.h"
#include "diWeatherFront.h"
#include "diWeatherSymbol.h"
#include "diWeatherArea.h"
#include "diShapeObject.h"
#include "diUtilities.h"

#include <puTools/miStringFunctions.h>

#include <fstream>
#include <iomanip>

#define MILOGGER_CATEGORY "diana.WeatherObjects"
#include <miLogger/miLogging.h>

using namespace::miutil;
using namespace std;

//static
const miTime WeatherObjects::ztime = miTime(1970,1,1,0,0,0);

/*********************************************/

WeatherObjects::WeatherObjects()
: xcopy(0), ycopy(0)
{
  METLIBS_LOG_SCOPE();

  //zero time = 00:00:00 UTC Jan 1 1970
  itsTime=ztime;

  // correct spec. when making Projection for long/lat coordinates
  geoArea.setP(Projection::geographic());

  useobject.clear();
  //use all objects if nothing else specified
  for (int i=0; i<numObjectTypes; i++)
    useobject[ObjectTypeNames[i]]= true;
  enabled=true;
}

/*********************************************/

void WeatherObjects::clear()
{
  METLIBS_LOG_SCOPE();

  diutil::delete_all_and_clear(objects);
  prefix.clear();
  filename.clear();
  itsOldComments.clear();
  itsLabels.clear();
  itsOldLabels.clear();
}

/*********************************************/

bool WeatherObjects::empty()
{
  return (objects.empty() && itsLabels.empty());
}

/*********************************************/

void WeatherObjects::plot(DiGLPainter* gl, Plot::PlotOrder porder)
{
  METLIBS_LOG_SCOPE();

  if (!enabled || ( porder != Plot::LINES && porder != Plot::OVERLAY) )
    return;

  const int n = objects.size();
  const int NORDER = 7;
  const objectType order[NORDER] = {
    wArea, wFront, wSymbol, wText, Border, RegionName, ShapeXXX
  };

  for (int o=0; o<NORDER; ++o) {
    for (int i=0; i<n; i++){
      if (objects[i]->objectIs(order[o]))
        objects[i]->plot(gl, porder);
    }
  }
}

/*********************************************/

bool WeatherObjects::changeProjection(const Area& newArea)
{
  METLIBS_LOG_SCOPE("Change projection from " << itsArea <<" to " << newArea);

  if (itsArea.P() == newArea.P())
    return false;

  if (empty()) {
    itsArea= newArea;
    return false;
  }

  const int obsize = objects.size();

  //npos = number of points to be transformed = all object points
  //plus one copy point(xcopy,ycopy)
  int npos = 1;
  for (int i=0; i<obsize; i++)
    npos += objects[i]->getXYZsize();

  float *xpos = new float[npos];
  float *ypos = new float[npos];
  int n= 0;

  for (int i=0; i<obsize; i++){
    const int m= objects[i]->getXYZsize();
    for (int j=0; j<m; ++j) {
      const XY& xy = objects[i]->getXY(j);
      xpos[n]= xy.x();
      ypos[n]= xy.y();
      n++;
    }
  }

  xpos[n]=xcopy;
  ypos[n]=ycopy;

  bool converted;
  if (itsArea.P() == geoArea.P()) {
    converted = (newArea.P().convertFromGeographic(npos,xpos,ypos) == 0);
  } else if (newArea.P() == geoArea.P()) {
    converted = (itsArea.P().convertToGeographic(npos,xpos,ypos) == 0);
  } else {
    converted = gc.getPoints(itsArea.P(),newArea.P(),npos,xpos,ypos);
  }

  if (!converted) {
    METLIBS_LOG_ERROR("coordinate conversion error");
    delete[] xpos;
    delete[] ypos;
    return false;
  }

  xcopy=xpos[n];
  ycopy=ypos[n];

  n= 0;
  for (int i=0; i<obsize; i++){
    const int m= objects[i]->getXYZsize();
    const vector<float> x(&xpos[n], &xpos[n+m]);
    const vector<float> y(&ypos[n], &ypos[n+m]);
    objects[i]->setXY(x,y);
    n += m;
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
  METLIBS_LOG_SCOPE();

  const int obsize = objects.size();
  for (int i=0; i<obsize; i++)
    objects[i]->updateBoundBox();
}


/*********************************************/

bool WeatherObjects::readEditDrawFile(const std::string& fn, const Area& newArea)
{
  METLIBS_LOG_SCOPE("filename" << fn);

  //if *.shp read shapefile
  if (diutil::endswith(fn, ".shp")) {
    if (newArea.P()!=itsArea.P()){
      changeProjection(newArea);
    }
    changeProjection(geoArea);

    METLIBS_LOG_INFO("This is a shapefile");
    ShapeObject * shape = new ShapeObject();
    addObject(shape);
    shape->read(fn);

    changeProjection(newArea);

    return true;
  }

  // open filestream
  ifstream file(fn.c_str());
  if (!file){
    METLIBS_LOG_ERROR("ERROR OPEN (READ) '" << fn << "'");
    return false;
  }


  /* ---------------------------------------------------------------------
 -----------------------start to read new format--------------------------
  -----------------------------------------------------------------------*/
  // read the first line check if it contains "date"
  std::string str;
  getline(file, str);
  //METLIBS_LOG_DEBUG("The first line read is " << str);
  const std::vector<std::string> stokens = miutil::split(str, 0, "=");
  std::string value,key;
  if (stokens.size()==2) {
    key = miutil::to_lower(stokens[0]);
    value = stokens[1];
  }
  //check if the line contains keyword date ?
  if (key != "date") {
    METLIBS_LOG_ERROR("This file is not in the new format ");
    file.close();
    return false;
  }

  std::string fileString;
  // read file
  while (getline(file,str) && !file.eof()){
    if (str.empty() || str[0]=='#')
      continue;

    // The font Helvetica is not supported if X-fonts are not enabled, use BITMAPFONT defined in setup
    if (miutil::contains(str, "Helvetica")) {
      miutil::replace(str, "Helvetica","BITMAPFONT");
    }
    // check if this is a LABEL string
    if (diutil::startswith(str, "LABEL")) {
      if (useobject["anno"])
        itsOldLabels.push_back(str);
    } else {
      fileString += str;
    }
  }
  file.close();
  return readEditDrawString(fileString, newArea);
}

bool WeatherObjects::readEditDrawString(const std::string& inputString,
    const Area& newArea, bool replace)
{
  METLIBS_LOG_SCOPE(LOGVAL(inputString));

  std::string key,value,objectString;

  // nb ! if useobject not true for an objecttype, no objects used(read) for
  // this object type. Useobject is set in WeatherObjects constructor and
  // also in DisplayObjects::define and in EditManager::startEdit

  // first convert existing objects to geographic coordinates
  if (newArea.P()!=itsArea.P()){
    changeProjection(newArea);
  }
  changeProjection(geoArea);


  //split inputString into one string for each object
  vector <std::string> objectStrings = miutil::split(inputString, 0, "!");

  for (unsigned int i = 0;i<objectStrings.size();i++){
    //split objectString and check which type of new
    //object should be created from first keyword and value
    vector <std::string> tokens = miutil::split(objectStrings[i], 0, ";");
    vector <std::string> stokens = miutil::split(tokens[0], 0, "=");
    if ( stokens.size()==2) {
      key = miutil::to_lower(stokens[0]);
      value = stokens[1];
    } else {
      METLIBS_LOG_WARN("WeatherObjects::readEditDrawString - Warning !");
      METLIBS_LOG_ERROR("Error in objectString " << objectStrings[i]);
      continue;
    }
    if (key == "date"){
      //METLIBS_LOG_DEBUG("date of object file = " << timeFromString(value));
    } else if (key == "object"){
      ObjectPlot * tObject;
      if (value == "Front") {
        if (useobject["front"])
          tObject = new WeatherFront();
        else
          continue;
      } else if (value == "Symbol") {
        if (useobject["symbol"])
          tObject = new WeatherSymbol();
        else
          continue;
      } else if (value == "Area") {
        if (useobject["area"])
          tObject = new WeatherArea();
        else
          continue;
      } else if (value == "Border")
        tObject = new AreaBorder();
      else if (value == "RegionName")
        tObject = new WeatherSymbol("",RegionName);
      else {
        METLIBS_LOG_ERROR("Unknown object: '" << value << "'");
        continue;
      }
      if (tObject->readObjectString(objectStrings[i]))
        //add a new object
        addObject(tObject,replace);
      else
        delete tObject;
    } else {
      METLIBS_LOG_ERROR("Error! Object key '" << key << "'not found !");
    }
  }

  changeProjection(newArea);
  return true;
}

std::string WeatherObjects::writeEditDrawString(const miTime& t)
{
  METLIBS_LOG_SCOPE();

  if (empty())
    return std::string();

  const Area oldarea = itsArea;
  changeProjection(geoArea);

  ostringstream ostr;
  ostr << "Date=" << stringFromTime(t, true) << ';' << endl << endl;

  for (vector <ObjectPlot*>::iterator p = objects.begin(); p!=objects.end(); ++p)
    ostr << (*p)->writeObjectString();

  changeProjection(oldarea);

  const int n = itsLabels.size();
  for (int i=0; i<n; i++)
    ostr << itsLabels[i] << "\n";
  return ostr.str();
}

/************************************************
 *  Methods for reading comments  ****************
 *************************************************/

bool WeatherObjects::readEditCommentFile(const std::string fn)
{
  METLIBS_LOG_SCOPE(LOGVAL(fn));

  std::ifstream file(fn.c_str());
  if (!file) {
    METLIBS_LOG_DEBUG("not found " << fn);
    return false;
  }

  std::string str, fileString;
  while (getline(file,str) && !file.eof())
    fileString += str + "\n";

  file.close();

  itsOldComments += fileString;

  METLIBS_LOG_DEBUG("itsOldComments" << itsOldComments);

  return true;
}

std::string WeatherObjects::readComments()
{
  METLIBS_LOG_SCOPE();

  //read the old comments
  if (itsOldComments.empty())
    return "No comments";
  else
    return itsOldComments;
}

/************************************************
 *  Methods for reading and writing labels *******
 *************************************************/

vector <string> WeatherObjects::getObjectLabels()
{
  METLIBS_LOG_SCOPE();
  //oldLabels from object file
  return itsOldLabels;
}

vector<string> WeatherObjects::getEditLabels()
{
  METLIBS_LOG_SCOPE();
  //new edited labels
  return itsLabels;
}

/************************************************
 *  Methods for reading and writing areaBorders  *
 *************************************************/

bool WeatherObjects::readAreaBorders(const std::string fn, const Area& newArea)
{
  METLIBS_LOG_SCOPE("filename = " << fn);

  std::ifstream file(fn.c_str());
  if (!file){
    METLIBS_LOG_ERROR("ERROR OPEN (READ) " << fn);
    return false;
  }

  std::string str,fileString;
  // read file
  while (getline(file,str) && !file.eof())
    fileString += str + "\n";

  file.close();

  return readEditDrawString(fileString,newArea);
}


bool WeatherObjects::writeAreaBorders(const std::string& fn)
{
  if (empty())
    return false;

  // open filestream
  std::ofstream file(fn.c_str());
  if (!file){
    METLIBS_LOG_ERROR("ERROR OPEN (WRITE) " << fn);
    return false;
  }

  const Area oldarea = itsArea;
  changeProjection(geoArea);

  for (vector <ObjectPlot*>::iterator p = objects.begin(); p!=objects.end(); ++p) {
    ObjectPlot* pobject = *p;
    if (pobject->objectIs(Border))
      file << pobject->writeObjectString();
  }

  file.close();

  changeProjection(oldarea);
  return true;
}

// ---------------------------------------------------------------
// ---------------------------------------------------------------
// ---------------------------------------------------------------

int WeatherObjects::objectCount(int type)
{
  int ncount= 0;
  for (std::vector <ObjectPlot*>::const_iterator p = objects.begin(); p!=objects.end(); ++p) {
    if ((*p)->objectIs(type))
      ncount++;
  }
  return ncount;
}

void WeatherObjects::addObject(ObjectPlot* object, bool replace)
{
  METLIBS_LOG_SCOPE();
  if (!object)
    return;

  if (replace) { // remove old object
    std::vector<ObjectPlot*>::iterator p = objects.begin();
    while (p!=objects.end() && (*p)->getName() != object->getName())
      p++;
    if (p!=objects.end())
      objects.erase(p);
  }

  objects.push_back(object);
  object->setRegion(prefix);
}

vector<ObjectPlot*>::iterator WeatherObjects::removeObject(vector<ObjectPlot*>::iterator p)
{
  return objects.erase(p);
}

/*********************************************/

// static
std::string WeatherObjects::stringFromTime(const miTime& t, bool addMinutes)
{
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

  return ostr.str();
}

/*********************************************/

// static
miTime WeatherObjects::timeFromString(const std::string& timeString)
{
  if (timeString.length()<10)
    return ztime;
  //get time from a string with yyyymmddhhmm
  int year= atoi(timeString.substr(0,4).c_str());
  int mon=  atoi(timeString.substr(4,2).c_str());
  int day=  atoi(timeString.substr(6,2).c_str());
  int hour= atoi(timeString.substr(8,2).c_str());
  int min= 0;
  if (timeString.length() >= 12)
    min= atoi(timeString.substr(10,2).c_str());
  if (year<0 || mon <0 || day<0 || hour<0 || min < 0)
    return ztime;
  return miTime(year,mon,day,hour,min,0);
}

/*********************************************/

// static
map<std::string,bool> WeatherObjects::decodeTypeString(const std::string& token)
{
  map<std::string,bool> use;
  for (int i=0; i<numObjectTypes; i++)
    use[ObjectTypeNames[i]]= false;
  //types of objects to plot
  vector<std::string> stokens = miutil::split(token.substr(6,token.size()-6), 0, ",");
  int m= stokens.size();
  for (int j=0; j<m; j++){
    if (stokens[j] == "all"){
      for (int k=0; k<numObjectTypes; k++)
        use[ObjectTypeNames[k]]= true;
      break;
    }
    use[stokens[j]]= true;
  }
  return use;
}

void WeatherObjects::enable(bool b)
{
  enabled = b;
}

bool WeatherObjects::isEnabled()
{
  return enabled;
}
