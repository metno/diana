/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2022 met.no

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

#include "diWeatherObjects.h"

#include "diWeatherFront.h"
#include "diWeatherSymbol.h"
#include "diWeatherArea.h"
#include "diShapeObject.h"
#include "diLabelPlotCommand.h"
#include "diUtilities.h"

#include "util/charsets.h"
#include "util/string_util.h"
#include "util/time_util.h"

#include <puTools/miStringFunctions.h>

#include <fstream>
#include <iomanip>
#include <sstream>

#define MILOGGER_CATEGORY "diana.WeatherObjects"
#include <miLogger/miLogging.h>

using namespace::miutil;

namespace {
const std::string ObjectTypeNames[] = {"edittool", "front", "symbol", "area", "anno"};

const Area& geoArea()
{
  static const Area ga(Projection::geographic(), Rectangle());
  return ga;
}

bool convertProjection(const Projection& from, const Projection& to, int n, float* x, float* y)
{
  if (from == to)
    return true;
  else if (from == geoArea().P())
    return to.convertFromGeographic(n, x, y);
  else if (to == geoArea().P())
    return from.convertToGeographic(n, x, y);
  else
    return to.convertPoints(from, n, x, y);
}

} // namespace

WeatherObjects::WeatherObjects()
    : xcopy(0)
    , ycopy(0)
    , enabled(true)
{
  METLIBS_LOG_SCOPE();

  itsArea = geoArea();

  //use all objects if nothing else specified
  for (const std::string& otn : ObjectTypeNames)
    useobject[otn] = true;
}

WeatherObjects::~WeatherObjects()
{
}

// static
const Area& WeatherObjects::getGeoArea()
{
  return geoArea();
}

const miutil::miTime& WeatherObjects::getTime() const
{
  return itsTime;
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

bool WeatherObjects::empty() const
{
  return (objects.empty() && itsLabels.empty());
}

/*********************************************/

void WeatherObjects::plot(DiGLPainter* gl, PlotOrder porder)
{
  METLIBS_LOG_SCOPE();

  if (!enabled || (porder != PO_LINES && porder != PO_OVERLAY) || empty())
    return;

  if (mapArea.P() != itsArea.P()) {
    switchProjection(mapArea);
  }

  const objectType order[] = {wArea, wFront, wSymbol, wText, Border, RegionName, ShapeXXX};
  for (int ord : order) {
    for (ObjectPlot* op : objects) {
      if (op->objectIs(ord))
        op->plot(gl, porder);
    }
  }
}

/*********************************************/

void WeatherObjects::changeProjection(const Area& newArea, const Rectangle& /*plotSize*/)
{
  mapArea = newArea;
}

bool WeatherObjects::convertFromProjection(const Area& pointProjection, int n, float* x, float* y)
{
  return convertProjection(pointProjection.P(), itsArea.P(), n, x, y);
}

bool WeatherObjects::convertToProjection(const Area& pointProjection, int n, float* x, float* y)
{
  return convertProjection(itsArea.P(), pointProjection.P(), n, x, y);
}

bool WeatherObjects::switchProjection(const Area& newArea)
{
  METLIBS_LOG_SCOPE("Change projection from " << itsArea <<" to " << newArea);

  if (itsArea.P() == newArea.P() || !newArea.P().isDefined())
    return false;

  if (empty()) {
    itsArea= newArea;
    return false;
  }

  //npos = number of points to be transformed = all object points
  //plus one copy point(xcopy,ycopy)
  int npos = 1;
  for (ObjectPlot* op : objects)
    npos += op->getXYZsize();

  std::unique_ptr<float[]> xpos(new float[npos]);
  std::unique_ptr<float[]> ypos(new float[npos]);

  int n = 0;
  for (ObjectPlot* op : objects) {
    const int m = op->getXYZsize();
    for (int j=0; j<m; ++j) {
      op->getXY(j).unpack(xpos[n], ypos[n]);
      n++;
    }
  }
  xpos[n]=xcopy;
  ypos[n]=ycopy;

  if (!convertToProjection(newArea, npos, xpos.get(), ypos.get())) {
    METLIBS_LOG_ERROR("coordinate conversion error");
    return false;
  }

  xcopy=xpos[n];
  ycopy=ypos[n];
  n= 0;
  for (ObjectPlot* op : objects) {
    const int m = op->getXYZsize();
    const std::vector<float> x(&xpos[n], &xpos[n+m]);
    const std::vector<float> y(&ypos[n], &ypos[n+m]);
    op->setXY(x, y);
    n += m;
  }

  itsArea= newArea;

  updateObjects();

  return true;
}


/*********************************************/

void WeatherObjects::updateObjects()
{
  METLIBS_LOG_SCOPE();
  for (ObjectPlot* op : objects)
    op->updateBoundBox();
}


/*********************************************/

bool WeatherObjects::readEditDrawFile(const std::string& fn)
{
  METLIBS_LOG_SCOPE("filename" << fn);

  //if *.shp read shapefile
  if (diutil::endswith(fn, ".shp")) {
    // shape file expected to be in geographical projection; convert all other objects to this projection
    // such that all objects have the same projection after adding the shape file
    switchProjection(geoArea());

    METLIBS_LOG_INFO("This is a shapefile");
    std::unique_ptr<ShapeObject> shape(new ShapeObject());
    shape->read(fn); // FIXME ShapeObject::read automatically reprojects to map projection
    addObject(shape.release());

    return true;
  }

  diutil::CharsetConverter_p converter = diutil::findConverter( diutil::CHARSET_READ(), diutil::CHARSET_INTERNAL());

  // open filestream
  std::ifstream file(fn.c_str());
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
  str = converter->convert(str);
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
    str = converter->convert(str);

    // The font Helvetica is not supported if X-fonts are not enabled, use BITMAPFONT defined in setup
    if (miutil::contains(str, "Helvetica")) {
      miutil::replace(str, "Helvetica", diutil::BITMAPFONT);
    }
    // check if this is a LABEL string
    if (diutil::startswith(str, "LABEL")) {
      if (useobject["anno"]) {
        itsOldLabels.push_back(LabelPlotCommand::fromString(str.substr(5)));
      }
    } else {
      fileString += str;
    }
  }
  file.close();
  return readEditDrawString(fileString);
}

bool WeatherObjects::readEditDrawString(const std::string& inputString, bool replace)
{
  METLIBS_LOG_SCOPE(LOGVAL(inputString));

  // nb ! if useobject not true for an objecttype, no objects used(read) for
  // this object type. Useobject is set in WeatherObjects constructor and
  // also in DisplayObjects::define and in EditManager::startEdit

  // first convert existing objects to geographic coordinates
  switchProjection(geoArea());

  //split inputString into one string for each object
  const std::vector<std::string> objectStrings = miutil::split(inputString, 0, "!");
  for (const std::string& objstr : objectStrings) {
    //split objectString and check which type of new
    //object should be created from first keyword and value
    const std::vector<std::string> tokens = miutil::split(objstr, 0, ";");
    const std::vector<std::string> stokens = miutil::split(tokens[0], 0, "="); // never empty
    if (stokens.size() != 2) {
      METLIBS_LOG_ERROR("Error in objectString '" << objstr << "'");
      continue;
    }
    const std::string key = miutil::to_lower(stokens[0]);
    const std::string& value = stokens[1];
    if (key == "date"){
    } else if (key == "object"){
      std::unique_ptr<ObjectPlot> tObject;
      if (value == "Front") {
        if (useobject["front"])
          tObject.reset(new WeatherFront());
        else
          continue;
      } else if (value == "Symbol") {
        if (useobject["symbol"])
          tObject.reset(new WeatherSymbol());
        else
          continue;
      } else if (value == "Area") {
        if (useobject["area"])
          tObject.reset(new WeatherArea());
        else
          continue;
      } else if (value == "Border")
        tObject.reset(new AreaBorder());
      else if (value == "RegionName")
        tObject.reset(new WeatherSymbol("", RegionName));
      else {
        METLIBS_LOG_ERROR("Unknown object: '" << value << "'");
        continue;
      }
      if (tObject->readObjectString(objstr))
        addObject(tObject.release(), replace);
    } else {
      METLIBS_LOG_ERROR("Object key '" << key << "'not found !");
    }
  }

  switchProjection(mapArea);

  return true;
}

std::string WeatherObjects::writeEditDrawString(const miTime& t)
{
  METLIBS_LOG_SCOPE();

  if (empty())
    return std::string();

  const Area oldarea = itsArea;
  switchProjection(geoArea());

  std::ostringstream ostr;
  ostr << "Date=" << miutil::stringFromTime(t, true) << ';' << std::endl << std::endl;

  for (std::vector <ObjectPlot*>::iterator p = objects.begin(); p!=objects.end(); ++p)
    ostr << (*p)->writeObjectString();

  switchProjection(oldarea);

  for (PlotCommand_cp pc : itsLabels)
    ostr << pc->toString() << "\n";
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

  itsOldComments += miutil::from_latin1_to_utf8(fileString);

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

const PlotCommand_cpv& WeatherObjects::getObjectLabels()
{
  METLIBS_LOG_SCOPE();
  //oldLabels from object file
  return itsOldLabels;
}

const PlotCommand_cpv& WeatherObjects::getEditLabels()
{
  METLIBS_LOG_SCOPE();
  //new edited labels
  return itsLabels;
}

/************************************************
 *  Methods for reading and writing areaBorders  *
 *************************************************/

bool WeatherObjects::readAreaBorders(const std::string& fn)
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

  return readEditDrawString(miutil::from_latin1_to_utf8(fileString));
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
  switchProjection(geoArea());

  diutil::CharsetConverter_p converter = diutil::findConverter(diutil::CHARSET_INTERNAL(), diutil::ISO_8859_1);

  for (std::vector <ObjectPlot*>::iterator p = objects.begin(); p!=objects.end(); ++p) {
    ObjectPlot* pobject = *p;
    if (pobject->objectIs(Border))
      file << converter->convert(pobject->writeObjectString());
  }

  file.close();

  switchProjection(oldarea);
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

  if (replace) {
    // remove old object
    objects.erase(std::remove_if(objects.begin(), objects.end(), [&](ObjectPlot* op) { return op->getName() == object->getName(); }), objects.end());
  }

  objects.push_back(object);
  object->setRegion(prefix);
}

std::vector<ObjectPlot*>::iterator WeatherObjects::removeObject(std::vector<ObjectPlot*>::iterator p)
{
  return objects.erase(p);
}

/*********************************************/

// static
std::map<std::string, bool> WeatherObjects::decodeTypeString(const std::vector<std::string>& types)
{
  std::map<std::string, bool> use;
  for (const std::string& otn : ObjectTypeNames)
    use[otn] = false;
  //types of objects to plot
  for (const std::string& tok : types) {
    if (tok == "all") {
      for (const std::string& otn : ObjectTypeNames)
        use[otn] = true;
      break;
    }
    use[tok] = true;
  }
  return use;
}

// static
std::map<std::string, bool> WeatherObjects::decodeTypeString(const std::string& types)
{
  return decodeTypeString(miutil::split(types, ","));
}

void WeatherObjects::enable(bool b)
{
  enabled = b;
}

bool WeatherObjects::isEnabled() const
{
  return enabled;
}
