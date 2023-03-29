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

#include "diObjectPlot.h"

#include "diGLPainter.h"
#include "diStaticPlot.h"
#include "util/misc_util.h"

#include <mi_fieldcalc/math_util.h>

#include <puTools/miStringFunctions.h>

#include <math.h>
#include <sstream>

//#define DEBUGPRINT
#define MILOGGER_CATEGORY "diana.ObjectPlot"
#include <miLogger/miLogging.h>

using namespace::miutil;

const int DIV_SPLINE = 5;

static const float FLOAT_MAX = std::numeric_limits<float>::max();
static const float FLOAT_LOW = std::numeric_limits<float>::lowest();

static const Colour BLACK03 = Colour::fromF(0, 0, 0, 0.3);
static const Colour CYAN05(0, 255, 255, 128);

// static
const int ObjectPlot::siglinewidth = 2;
// static
std::map <std::string,std::string> ObjectPlot::editTranslations;

ObjectPlot::~ObjectPlot()
{
  // METLIBS_LOG_SCOPE(); must not be enabled -- bdiana will crash
  delete[] x;
  delete[] y;
  delete[] x_s;
  delete[] y_s;
}

ObjectPlot::ObjectPlot(int objTy)
    : addTop(false) // add elements to top instead of bottom
    , window_dw(1)
    , window_dh(1)
    , w(10.0f)
    , h(10.0f)
    , basisColor("black")
    , region("NONE")
    , rubber(false)
    , spline(true)
    , inBoundBox(false)
    , typeOfObject(objTy)
    , stayMarked(false)
    , joinedMarked(false)
    , isVisible(true)
    , isSelected(false)
    , rotation(0.0f)
    , objectColour(Colour("black"))
    , currentState(active) // points can be added
    , fSense(2.5)          // sensitivity to mark rectangle
    , boundBox(FLOAT_MAX, FLOAT_MAX, FLOAT_LOW, FLOAT_LOW)
    , x(nullptr)
    , y(nullptr)
    , x_s(nullptr)
    , y_s(nullptr)
    , s_length(0)
    , scaleToField(1.0)
{
  METLIBS_LOG_SCOPE();
  setWindowInfo();
}

void ObjectPlot::swap(ObjectPlot& o)
{
  using std::swap;
  PlotOptionsPlot::swap(o);

  swap(addTop, o.addTop);
  swap(window_dw, o.window_dw);
  swap(window_dh, o.window_dh);
  swap(w, o.w);
  swap(h, o.h);
  swap(basisColor, o.basisColor);
  swap(region, o.region);
  swap(rubber, o.rubber);
  swap(spline, o.spline);
  swap(inBoundBox, o.inBoundBox);
  swap(typeOfObject, o.typeOfObject);
  swap(stayMarked, o.stayMarked);
  swap(joinedMarked, o.joinedMarked);
  swap(isVisible, o.isVisible);
  swap(isSelected, o.isSelected);
  swap(rotation, o.rotation);
  swap(objectColour, o.objectColour);
  swap(currentState, o.currentState);
  swap(fSense, o.fSense);
  swap(boundBox, o.boundBox);
  swap(x, o.x);
  swap(y, o.y);
  swap(x_s, o.x_s);
  swap(y_s, o.y_s);
  swap(s_length, o.s_length);
  swap(scaleToField, o.scaleToField);

  swap(rubberx, o.rubberx);
  swap(rubbery, o.rubbery);
  swap(objectBorderColour, o.objectBorderColour);
  swap(itsLinetype, o.itsLinetype);
  swap(drawIndex, o.drawIndex);
  swap(nodePoints, o.nodePoints);
  swap(type, o.type);
}

ObjectPlot::ObjectPlot(const ObjectPlot& rhs)
    : addTop(rhs.addTop)
    , window_dw(rhs.window_dw)
    , window_dh(rhs.window_dh)
    , w(rhs.w)
    , h(rhs.h)
    , basisColor(rhs.basisColor)
    , region(rhs.region)
    , rubber(rhs.rubber)
    , spline(rhs.spline)
    , rubberx(rhs.rubberx)
    , rubbery(rhs.rubbery)
    , inBoundBox(rhs.inBoundBox)
    , type(rhs.type)
    , typeOfObject(rhs.typeOfObject)
    , drawIndex(rhs.drawIndex)
    , stayMarked(rhs.stayMarked)
    , joinedMarked(rhs.joinedMarked)
    , isVisible(rhs.isVisible)
    , isSelected(rhs.isSelected)
    , rotation(rhs.rotation)
    , objectColour(rhs.objectColour)
    , objectBorderColour(rhs.objectBorderColour)
    , itsLinetype(rhs.itsLinetype)
    , currentState(rhs.currentState)
    , fSense(rhs.fSense)
    , boundBox(rhs.boundBox)
    , nodePoints(rhs.nodePoints)
    , x(diutil::copy_array(rhs.x, nodePoints.size()))
    , y(diutil::copy_array(rhs.y, nodePoints.size()))
    , x_s(diutil::copy_array(rhs.x_s, rhs.s_length))
    , y_s(diutil::copy_array(rhs.y_s, rhs.s_length))
    , s_length(rhs.s_length)
    , scaleToField(rhs.scaleToField)
{
  METLIBS_LOG_SCOPE();
}

void ObjectPlot::defineTranslations(){
  //set map to translate from norwegian object names (used in old files)
  editTranslations["Kaldfront"]="Cold front";
  editTranslations["Varmfront"]="Warm front";
  editTranslations["Okklusjon"]="Occlusion";
  editTranslations["KaldOkklusjon"]="Cold occlusion";
  editTranslations["VarmOkklusjon"]="Warm occlusion";
  editTranslations["Stasjon�r front"]="Stationary front";
  editTranslations["Tr�g"]="Trough";
  editTranslations["Bygelinje"]="Squall line";
  editTranslations["Sig.v�r"]="Significant weather";

  editTranslations["Lavtrykk"]="Low pressure";
  editTranslations["H�ytrykk"]="High pressure";
  editTranslations["Kald"]="Cold";
  editTranslations["Varm"]="Warm";
  editTranslations["T�ke"]="Fog";
  editTranslations["Yr"]="Drizzle";
  editTranslations["Yr som fryser"]="Freezing drizzle";
  editTranslations["Regn som fryser"]="Freezing rain";
  editTranslations["Byger"]="Showers";
  editTranslations["Regnbyger"]="Rain showers";
  editTranslations["Sluddbyger"]="Sleet showers";
  editTranslations["Haglbyger"]="Hail showers";
  editTranslations["Sn�byger"]="Snow showers";
  editTranslations["Tordenv�r"]="Thunderstorm";
  editTranslations["Tordenv�r m/hagl"]="Thunderstorm with hail";
  editTranslations["Sn�stjerne"]="Snow";
  editTranslations["Tropisk orkan"]="Hurricane";
  editTranslations["Disk"]="Disk";
  editTranslations["Sirkel"]="Circle";
  editTranslations["Kryss"]="Cross";
  editTranslations["Tekster"]="Text";

  editTranslations["Nedb�r"]="Precipitation";
  editTranslations["Byger"]="Showers";
  editTranslations["Skyer"]="Clouds";
  editTranslations["T�ke"]="Fog";
  editTranslations["Is"]="Ice";
  editTranslations["Sig.v�r"]="Significant weather";
  editTranslations["Generisk omr�de"]="Generic area";
}


int ObjectPlot::getXYZsize() const
{
  return nodePoints.size();
}


XY ObjectPlot::getXY(int idx) const
{
  return nodePoints[idx].xy();
}


std::vector<XY> ObjectPlot::getXY() const
{
  std::vector<XY> xy;
  xy.reserve(nodePoints.size());
  for (const ObjectPoint& np : nodePoints)
    xy.push_back(np.xy());
  return xy;
}


std::vector<XY> ObjectPlot::getXYjoined() const
{
  std::vector<XY> xy;
  xy.reserve(nodePoints.size());
  for (const ObjectPoint& np : nodePoints)
    if (np.joined())
      xy.push_back(np.xy());
  return xy;
}


std::vector<XY> ObjectPlot::getXYmarked() const
{
  std::vector<XY> xy;
  xy.reserve(nodePoints.size());
  for (const ObjectPoint& np : nodePoints)
    if (np.marked())
      xy.push_back(np.xy());
  return xy;
}


std::vector<XY> ObjectPlot::getXYmarkedJoined() const
{
  std::vector<XY> xy;
  xy.reserve(nodePoints.size());
  for (const ObjectPoint& np : nodePoints)
    if (np.marked() && np.joined())
      xy.push_back(np.xy());
  return xy;
}


void ObjectPlot::setXY(const std::vector<float>& x, const std::vector <float>& y)
{
  const size_t n = std::min(x.size(), y.size());
  const size_t end = std::min(n, nodePoints.size());
  size_t i = 0;
  for (; i < end; i++)
    nodePoints[i].setXY(x[i], y[i]);
  for (; i < n; i++)
    nodePoints.push_back(ObjectPoint(x[i], y[i]));
  updateBoundBox();
}

void ObjectPlot::recalculate()
{
}

void ObjectPlot::addPoint(float x, float y)
{
  switch (currentState){
  case active:
    int n=nodePoints.size();
    // avoid points at same position
    if (n==0 || !nodePoints[n-1].isInRectangle(x,y,0)){
      ObjectPoint pxy(x,y);
      if (addTop)
        nodePoints.push_front(pxy);
      else
        nodePoints.push_back(pxy);
      //borders - first points always joined
      if (objectIs(Border))
        nodePoints[0].setJoined(true);
    }
    recalculate();
    changeBoundBox(x,y);
    break;
  }
}


bool ObjectPlot::insertPoint(float x,float y){
  //find out between which points we should insert something
  //insert at x,y
  if (onLine(x,y))
  {
    ObjectPoint pxy(x,y);
    nodePoints.insert(nodePoints.begin()+insert,pxy);
    updateBoundBox();
    unmarkAllPoints();
    markPoint(x,y);
    return true;
  }
  else{
    unmarkAllPoints();
    return false;
  }
}


void ObjectPlot::changeBoundBox(float x, float y)
{
  if (!boundBox.isinside(x,y)) {
    miutil::minimaximize(boundBox.x1, boundBox.x2, x);
    miutil::minimaximize(boundBox.y1, boundBox.y2, y);
  }
}

bool ObjectPlot::markPoint(float x, float y)
{
  bool found=false;
  const float fdeltaw = fSense * window_dw * w * 0.5;
  for (ObjectPoint& np : nodePoints) {
    if (np.isInRectangle(x, y, fdeltaw)) {
      if (!np.marked())
        markedChanged=true;
      np.setMarked(true);
      found=true;
    } else if (!stayMarked && !joinedMarked) {
      if (np.marked())
        markedChanged=true;
      np.setMarked(false);
    }
  }
  return found;
}

void ObjectPlot::markAllPoints()
{
  for (ObjectPoint& np : nodePoints) {
    if (!np.marked())
      markedChanged=true;
    np.setMarked(true);
  }
}


void ObjectPlot::unmarkAllPoints()
{
  if (stayMarked)
    return;
  for (ObjectPoint& np : nodePoints) {
    if (np.marked())
      markedChanged=true;
    np.setMarked(false);
  }
}


bool ObjectPlot::deleteMarkPoints()
{
  std::deque <ObjectPoint>::iterator p=nodePoints.begin();
  while (p!= nodePoints.end()){
    if (p->marked())
      p=nodePoints.erase(p);
    else
      p++;
  }
  unmarkAllPoints();
  updateBoundBox();

  return true;
}


bool ObjectPlot::ismarkPoint(float x, float y)
{
  const float fdeltaw = fSense * window_dw * w * 0.5;
  for (const ObjectPoint& np : nodePoints)
    if (np.marked() && np.isInRectangle(x, y, fdeltaw))
      return true;
  return false;
}

bool ObjectPlot::ismarkAllPoints() const
{
  for (const ObjectPoint& np : nodePoints)
    if (!np.marked())
      return false;
  return true;
}

bool ObjectPlot::ismarkSomePoint() const
{
  for (const ObjectPoint& np : nodePoints)
    if (np.marked())
      return true;
  return false;
}

bool ObjectPlot::ismarkEndPoint() const
{
  return !nodePoints.empty() && nodePoints.back().marked();
}

bool ObjectPlot::ismarkBeginPoint() const
{
  return !nodePoints.empty() && nodePoints.front().marked();
}


bool ObjectPlot::joinPoint(float x, float y)
{
  float dist;
  int iJoin = 0;
  //distmax = a large number
  float distmax = 100000;
  bool join = false;
  if (isJoinPoint(x, y)) {
    return false;
  }
  float fdeltaw = fSense * window_dw * w * 0.5;
  int end = nodePoints.size();
  for (int i = 0; i < end; i++) {
    if (nodePoints[i].isInRectangle(x, y, fdeltaw)) {
      dist = nodePoints[i].distSquared(x, y);
      if (dist < distmax) {
        iJoin = i;
        distmax = dist;
        join = true;
      }
    }
  }
  if (join) {
    //join to the closest point
    nodePoints[iJoin].setJoined(true);
    return true;
  } else
    return false;
}

bool ObjectPlot::isJoinPoint( float x , float y, float &xjoin, float &yjoin){
  float fdeltaw=fSense*window_dw*w*0.5;
  int end = nodePoints.size();
  for (int i=0; i < end; i++)
    if (nodePoints[i].joined() && nodePoints[i].isInRectangle(x,y,fdeltaw)){
      xjoin=nodePoints[i].x();
      yjoin=nodePoints[i].y();
      return true;
    }
  return false;
}

bool ObjectPlot::isJoinPoint(float x, float y) const
{
  const float fdeltaw = fSense * window_dw * w * 0.5;
  for (const ObjectPoint& np : nodePoints)
    if (np.joined() && np.isInRectangle(x, y, fdeltaw))
      return true;
  return false;
}

bool ObjectPlot::ismarkJoinPoint() const
{
  //function to check whether a joined point is marked
  for (const ObjectPoint& np : nodePoints)
    if (np.joined() && np.marked())
      return true;
  return false;
}


void ObjectPlot::unjoinAllPoints()
{
  for (ObjectPoint& np : nodePoints)
    np.setJoined(false);
}

void ObjectPlot::unJoinPoint(float x, float y)
{
  const float fdeltaw = fSense * window_dw * w * 0.5;
  for (ObjectPoint& np : nodePoints)
    if (np.joined() && np.isInRectangle(x, y, fdeltaw))
      np.setJoined(false);
}

bool ObjectPlot::isEmpty()
{
  return nodePoints.empty();
}

bool ObjectPlot::isSinglePoint() const
{
  return nodePoints.size() == 1;
}

bool ObjectPlot::movePoint(float x, float y, float new_x, float new_y)
{
  for (ObjectPoint& np : nodePoints) {
    if (np.isInRectangle(x, y, 0)) {
      np.setXY(new_x, new_y);
      updateBoundBox();
      return true;
    }
  }
  return false;
}

bool ObjectPlot::moveMarkedPoints(float d_x, float d_y)
{
  if (isEmpty())
    return false;
  for (ObjectPoint& np : nodePoints)
    if (np.marked()) {
      np.rx() += d_x;
      np.ry() += d_y;
    }
  updateBoundBox();
  return true;
}


bool ObjectPlot::rotateLine(float d_x, float d_y)
{
  //for now, only rotate fronts...
  if (!(objectIs(wFront)|| objectIs(Border)))
    return false;
  if (nodePoints.size()<2 || getXYmarked().size() != 1)
    return false;

  int i, n= nodePoints.size();
  float *s = new float[n];
  float dx, dy, smax, weight;
  s[0]=0.;
  for (i=1; i<n; i++) {
    dx= nodePoints[i].x()-nodePoints[i-1].x();
    dy= nodePoints[i].y()-nodePoints[i-1].y();
    s[i] = s[i - 1] + miutil::absval(dx, dy);
  }
  for (int m=0; m < n; m++){
    if (!nodePoints[m].marked())
      continue;
    if (m==0) {
      smax= s[n-1];
      for (i=0; i<n-1; i++) {
        weight= (smax-s[i])/smax;
        nodePoints[i].rx()+=(d_x*weight);
        nodePoints[i].ry()+=(d_y*weight);
      }
      break;
    } else if (m==n-1) {
      smax= s[n-1];
      for (i=1; i<n; i++) {
        weight= s[i]/smax;
        nodePoints[i].rx()+=(d_x*weight);
        nodePoints[i].ry()+=(d_y*weight);
      }
    } else {
      smax= s[m];
      for (i=1; i<m; i++) {
        weight= s[i]/smax;
        nodePoints[i].rx()+=(d_x*weight);
        nodePoints[i].ry()+=(d_y*weight);
      }
      smax= s[n-1]-s[m];
      for (i=m; i<n-1; i++) {
        weight= (s[n-1]-s[i])/smax;
        nodePoints[i].rx()+=(d_x*weight);
        nodePoints[i].ry()+=(d_y*weight);
      }
    }
    break;
  }
  delete[] s;
  updateBoundBox();
  return true;
}


bool ObjectPlot::isInside(float x , float y)
{
  const float fdeltaw = fSense * window_dw * w * 0.5;
  for (const ObjectPoint& np : nodePoints)
    if (np.isInRectangle(x, y, fdeltaw))
      return true;
  return false;
}

bool ObjectPlot::isInside( float x , float y, float &xin, float &yin)
{
  const float fdeltaw = fSense * window_dw * w * 0.5;
  for (ObjectPoint& np : nodePoints) {
    if (np.isInRectangle(x, y, fdeltaw)) {
      xin = np.x();
      yin = np.y();
      return true;
    }
  }
  return false;
}


bool ObjectPlot::isBeginPoint( float x , float y, float &xin, float &yin)
{
  const float fdeltaw = fSense * window_dw * w * 0.5;
  if (nodePoints.front().isInRectangle(x,y,fdeltaw)){
    xin = nodePoints.front().x();
    yin = nodePoints.front().y();
    return true;
  }
  return false;
}


bool ObjectPlot::isEndPoint( float x , float y, float &xin, float &yin)
{
  const float fdeltaw = fSense * window_dw * w * 0.5;
  if (nodePoints.back().isInRectangle(x,y,fdeltaw)){
    xin = nodePoints.back().x();
    yin = nodePoints.back().y();
    return true;
  }
  return false;
}


void ObjectPlot::updateBoundBox()
{
  METLIBS_LOG_SCOPE();

  boundBox.x1= FLOAT_MAX;   // makes impossible box
  boundBox.x2= FLOAT_LOW;
  boundBox.y1= FLOAT_MAX;
  boundBox.y2= FLOAT_LOW;

  for (const ObjectPoint& np : nodePoints)
    changeBoundBox(np.x(), np.y());
  recalculate();
}

void ObjectPlot::drawJoinPoints(DiGLPainter* gl)
{
  if (!isVisible)
    return;
  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
  gl->LineWidth(3);
  if (inBoundBox){
    const std::vector<XY> xy = getXYjoined();
    gl->Enable(DiGLPainter::gl_BLEND);
    gl->BlendFunc(DiGLPainter::gl_SRC_ALPHA, DiGLPainter::gl_ONE_MINUS_SRC_ALPHA);
    gl->setColour(BLACK03);
    //draw all points in grey here (if cursor inside bounding box)
    drawPoints(gl, xy,true);
    gl->Disable(DiGLPainter::gl_BLEND);
  }
  gl->setColour(CYAN05);
  const std::vector<XY> xymark = getXYmarkedJoined();
  gl->setColour(Colour::CYAN);
  //draw marked points here
  drawPoints(gl, xymark,true);
}


void ObjectPlot::drawNodePoints(DiGLPainter* gl)
{
  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_LINE);
  gl->EdgeFlag(DiGLPainter::gl_TRUE);
  gl->ShadeModel(DiGLPainter::gl_FLAT);
  if (inBoundBox){
    const std::vector<XY> xy = getXY();
    gl->Enable(DiGLPainter::gl_BLEND);
    gl->BlendFunc(DiGLPainter::gl_SRC_ALPHA, DiGLPainter::gl_ONE_MINUS_SRC_ALPHA);
    gl->setColour(BLACK03);
    //draw all points in grey here (if cursor inside bounding box)
    drawPoints(gl, xy);
    gl->Disable(DiGLPainter::gl_BLEND);
  }
  const std::vector<XY> xymark = getXYmarked();

  gl->setColour(Colour::CYAN);
  //draw marked points here
  drawPoints(gl, xymark);
}


void ObjectPlot::drawPoints(DiGLPainter* gl, const std::vector<XY>& xydraw, bool fill)
{
  gl->LineWidth(2);
  const float deltaw=window_dw*w*0.5;
  unsigned int msize = xydraw.size();
  for (unsigned int i=0; i<msize; i++){
    if (objectIs(wFront) || objectIs(Border)) {
      if (fill)
        gl->drawRect(true, xydraw[i].x() - deltaw, xydraw[i].y()- deltaw,
            xydraw[i].x() + deltaw, xydraw[i].y() + deltaw);
      else
        gl->drawRect(false, xydraw[i].x() - deltaw, xydraw[i].y()- deltaw,
            xydraw[i].x() + deltaw, xydraw[i].y() + deltaw);
    } else if (objectIs(wArea)){
      gl->drawCircle(false, xydraw[i].x(), xydraw[i].y(), deltaw);
    } else if (objectIs(wSymbol) || objectIs(RegionName)) {
      gl->Begin(DiGLPainter::gl_POLYGON);
      gl->Vertex2f(xydraw[i].x() - deltaw, xydraw[i].y() - deltaw);
      gl->Vertex2f(xydraw[i].x() + deltaw, xydraw[i].y() - deltaw);
      gl->Vertex2f(xydraw[i].x(),          xydraw[i].y() + deltaw);
      gl->End();
    }
  }
}


void ObjectPlot::plotRubber(DiGLPainter* gl)
{
  gl->Begin(DiGLPainter::gl_LINE_STRIP);        // Draws line from end of front to cursor
  if (addTop)
    gl->Vertex2f(nodePoints.front().x(), nodePoints.front().y());
  else
    gl->Vertex2f(nodePoints.back().x(), nodePoints.back().y());
  gl->Vertex2f(rubberx,rubbery);
  gl->End();
}


void ObjectPlot::setWindowInfo()
{
  window_dw= getStaticPlot()->getPhysToMapScaleX();
  window_dh= getStaticPlot()->getPhysToMapScaleY();
}

void ObjectPlot::setBasisColor(const std::string& colour)
{
  // sets basis color of object
  basisColor = colour;
  objectColour = Colour(colour);
}

void ObjectPlot::setObjectColor(const std::string& colour)
{
  objectColour = Colour(colour);
}

void ObjectPlot::setObjectBorderColor(const std::string& colour)
{
  objectBorderColour = Colour(colour);
}

void ObjectPlot::setObjectColor(const Colour::ColourInfo& colour)
{
  objectColour = Colour(colour.rgb[0],colour.rgb[1],colour.rgb[2]);
}

void ObjectPlot::setObjectRGBColor(const std::string& rgbstring)
{
  //METLIBS_LOG_DEBUG("rgba value is " << rgbstring);
  std::vector<std::string> colours2add=miutil::split(rgbstring, ",");
  int nColours = colours2add.size()/4;
  for (int cc=0; cc < nColours; cc++){
    unsigned char cadd[4];
    for (int i = 0;i<4;i++){
      cadd[i] = atoi(colours2add[cc*4+i].c_str());
    }
    objectColour = Colour(cadd[0],cadd[1],cadd[2],cadd[3]);
  }
}

Colour::ColourInfo ObjectPlot::getObjectColor() const
{
  Colour::ColourInfo colour;
  colour.rgb[0]= (int) objectColour.R();
  colour.rgb[1]= (int) objectColour.G();
  colour.rgb[2]= (int) objectColour.B();
  return colour;
}

bool ObjectPlot::readObjectString(const std::string& objectString)
{
  bool objectRead = false;
  bool typeRead = false;
  bool LonLatRead = false;
  METLIBS_LOG_DEBUG("ObjectPlot::readObjectString\n");
  METLIBS_LOG_DEBUG("string is: " << objectString);

  for (const std::string tok : miutil::split(objectString, 0, ";")) {
    const std::vector<std::string> stokens = miutil::split(tok, 0, "=");
    if( stokens.size() != 2 ) {
      METLIBS_LOG_WARN(" readObjectString: key without value: '" << tok << "'");
      return false;
    }
    const std::string key = miutil::to_lower(stokens[0]);
    const std::string& value = stokens[1];
    if (key == "object"){
      METLIBS_LOG_DEBUG("Object value is " << value);
      // typeOfObject is already set in constructor
      objectRead = true;
    }
    else if (key =="type"){
      typeRead=setType(value);
      if (!typeRead){
        //check if value of type can be translated
        if (editTranslations.count(value)){
          typeRead=setType(editTranslations[value]);
        }
      }
      METLIBS_LOG_DEBUG("Type value is " << value);
    }
    else if (key =="name"){
      name=value; //set
#ifdef DEBUGPRINT
      METLIBS_LOG_DEBUG("Name is " << value);

#endif
    }
    else if (key == "latitudelongitude" ||     // old and wrong!
        key == "longitudelatitude") {
      METLIBS_LOG_DEBUG("Lonlat value is " << value);
      LonLatRead = true;
      std::vector<std::string> points2add=miutil::split(value, ",");
      int nPoints = points2add.size()/2;
      for (int pp=0; pp< nPoints; pp++){
        METLIBS_LOG_DEBUG(points2add[pp*2]);
        METLIBS_LOG_DEBUG(points2add[pp*2+1]);
        addPoint( atof(points2add[pp*2].c_str()),
            atof(points2add[pp*2+1].c_str()));
      }
    }
    else if (key == "rgba"){
      setObjectRGBColor(value);
    }
    else if (key == "size"){
      METLIBS_LOG_DEBUG("size value is " << value);
      setSize(atof(value.c_str()));
    }
    else if (key == "linewidth"){
      METLIBS_LOG_DEBUG("lineWidth value is " << value);
      setLineWidth(miutil::to_double(value));
    }
    else if (key == "rotation"){
      METLIBS_LOG_DEBUG("rotation value is " << value);
      setRotation(atof(value.c_str()));
    }
    else if (key == "text"){
      METLIBS_LOG_DEBUG("text value is " << value);
      setString(value);
    }
    else if (key == "complextext"){
      METLIBS_LOG_DEBUG("complexText value is " << value);
      readComplexText(value);
    }
    else if (key == "whitebox"){
      METLIBS_LOG_DEBUG("whitebox value is " << value);
      setWhiteBox(atoi(value.c_str()));
    }
    else
      METLIBS_LOG_WARN("ObjectPlot::readObjectString - Warning !, unknown key = "
          << key);
  }
  //check if type and Latlondefined !
  if (!objectRead || !typeRead || !LonLatRead){
    METLIBS_LOG_WARN("ObjectPlot::readObjectString - Warning !, " <<
        "Input string lacks Object,Type or Longitude/Latitude Input! "
        << objectString);
    return false;
  }
  return true;
}


std::string ObjectPlot::writeObjectString()
{
  std::ostringstream r;
  //write type of object
  r << writeTypeString();

  //write coordinates
  r << "LongitudeLatitude=\n";
  if (!nodePoints.empty()) {
    bool first = true;
    for (const ObjectPoint& np : nodePoints) {
      if (!first)
        r << ",\n";
      r << np.x() << "," << np.y();
      first = false;
    }
    r << ";\n";
  }

  //write colour
  r << "RGBA=" << (int)objectColour.R() << "," << (int)objectColour.G() << "," << (int)objectColour.B() << "," << (int)objectColour.A() << ";\n";

  //write "!" to signal end of object
  r << "!\n";
  return r.str();
}

bool ObjectPlot::isInRegion(int region, int matrix_nx, int matrix_ny, double resx, double resy, int* combinematrix)
{
  for (const ObjectPoint& np : nodePoints) {
    float x1 = np.x() / resx;
    float y1 = np.y() / resy;
    if (x1>=0 && x1<=matrix_nx-1 && y1>=0 && y1<=matrix_ny-1) {
      int x= int(x1+0.5);
      int y= int(y1+0.5);
      int index = matrix_nx*y+x;
      if (combinematrix[index] == region)
        return true;
    }
  }
  return false;
}

int ObjectPlot::combIndex(int matrix_nx, int matrix_ny, double resx, double resy, int * combinematrix)
{
  float x1=nodePoints[0].x()/resx;
  float y1=nodePoints[0].y()/resy;
  if (x1>=0 && x1<=matrix_nx-1 && y1>=0 && y1<=matrix_ny-1) {
    int x= int(x1+0.5);
    int y= int(y1+0.5);
    int index = matrix_nx*y+x;
    return combinematrix[index];
  }
  return -1;
}


bool ObjectPlot::resumeDrawing()
{
  setState(active);
  addTop = ismarkBeginPoint();
  unmarkAllPoints();
  return true;
}


bool ObjectPlot::oktoJoin(bool joinAll)
{
  if  (joinAll || ismarkSomePoint() || currentState == active) {
    // only fronts can be joined
    // drawIndex from SigWeatherFront and higher are lines etc. not to be joined
    // empty fronts shouldn't be joined
    return (objectIs(wFront) && drawIndex < SigweatherFront && nodePoints.size());
  }
  return false;
}

bool ObjectPlot::oktoMerge(bool mergeAll,int index)
{
  if  (mergeAll || ismarkSomePoint() || currentState == active) {
    return (objectIs(wFront) && index == drawIndex && nodePoints.size());
  }
  return false;
}

void ObjectPlot::setRubber(bool rub, const float x, const float y)
{
  if (objectIs(wFront) || objectIs(wArea)) {
    rubber = rub;
    rubberx = x;
    rubbery = y;
  } else
    rubber=false;
}


/*
  Algorithm for checking whether a point is on the front.
 HK 15/9-00 - Use spline points, look in a tilted box following
 curve
 */

bool ObjectPlot::onLine(float x, float y)
{
  int size = nodePoints.size();
  if (size > 1){
    if  (boundBox.isinside(x,y)){
      if (spline){
        if (x_s==NULL) {
          METLIBS_LOG_DEBUG("Online::x_s = 0 !\n");
          return false;
        }
        for (int i = 0; i < s_length-1; i++){
          if (isInsideBox(x,y,x_s[i],y_s[i],x_s[i+1],y_s[i+1])){
            //spline point location of point
            insert = i/(DIV_SPLINE+1)+1;
            return true;
          }
        }
      }else{
        for (int i = 0; i < size; i++){
          float x1,x2,y1,y2;
          if (i+1<size){
            x1=nodePoints[i].x();
            x2=nodePoints[i+1].x();
            y1=nodePoints[i].y();
            y2=nodePoints[i+1].y();
          } else if (i+1==size && objectIs(wArea)){
            x1=nodePoints[i].x();
            x2=nodePoints[0].x();
            y1=nodePoints[i].y();
            y2=nodePoints[0].y();
          } else
            continue;
          if (isInsideBox(x,y,x1,y1,x2,y2)){
            //spline point location of point
            insert = i+1;
            return true;
          }
        }
      }
    }
  }
  return false;
}

//called from onLine-checks if x,y inside the tilted rectangle made by x1,y1,x2,y2
bool ObjectPlot::isInsideBox(float x, float y, float x1, float y1, float x2, float y2)
{
  float salpha,calpha;
  float dwidth = 2*getLineWidth()*getDwidth();
  Rectangle box(0, 0, 0, 0);
  if (x2!=x1){
    float  dy = y2-y1;
    float  dx = x2-x1;
    float hyp = miutil::absval(dy, dx);
    salpha = fabsf(dy) / hyp;
    calpha = fabsf(dx) / hyp;
    if ((dy < 0) != (dx < 0))
      salpha *= -1;
  }else{
    salpha=1.0;
    calpha = 0.0;
  }
  //calculate x and y in transf. system
  float xprime = x*calpha+y*salpha;
  float yprime = y*calpha-x*salpha;
  if (x1<x2){
    box.x1 = x1 * calpha + y1 * salpha;
    box.x2 = x2 * calpha + y2 * salpha;
  }else if (x1>x2){
    box.x1 = x2 * calpha + y2 * salpha;
    box.x2 = x1 * calpha + y1 * salpha;
  }
  box.y1 = y1 * calpha - x1 * salpha - dwidth;
  box.y2 = box.y1 + 2 * dwidth;
  if (box.isinside(xprime, yprime)) {
    //x,y distance to line
    if (x2!=x1){
      float a=(y2-y1)/(x2-x1); // gradient
      float b= y1 - x1*a;
      float dist = (a/fabs(a))*(y-a*x-b)/sqrtf(1.+a*a);
      distX= dist*calpha;
      distY=-dist*salpha;}
    else{
      distX=x1-x;
      distY=0;
    }
    return true;
  }
  return false;
}

/*
  B-spline smooth of front
 */
int ObjectPlot::smoothline(int npos, float x[], float y[], int nfirst, int nlast,
    int ismooth, float xsmooth[], float ysmooth[])
{
  // Smooth line, make and return spline through points.
  //
  //  input:
  //     x(n),y(n), n=1,npos:   x and y in "window" coordinates
  //     x(nfrst),y(nfsrt):     first point
  //     x(nlast),y(nlast):     last  point
  //     ismooth:               number of points spline-interpolated
  //                            between each pair of input points
  //
  //  method: 'hermit interpolation'
  //     nfirst=0:      starting condition for spline = relaxed
  //     nfirst>0:      starting condition for spline = clamped
  //     nlast<npos-1:  ending   condition for spline = clamped
  //     nlast=npos-1:  ending   condition for spline = relaxed
  //        relaxed  -  second derivative is zero
  //        clamped  -  derivatives computed from nearest points

  int   ndivs, n, ns, i;
  float rdivs, xl1, yl1, s1, xl2, yl2, s2, dx1, dy1, dx2, dy2;
  float c32, c42, c31, c41, fx1, fx2, fx3, fx4, fy1, fy2, fy3, fy4;
  float tstep, t, t2, t3;

  if (npos<3 || nfirst<0 || nfirst>=nlast
      || nlast>npos-1 || ismooth<1) {
    nfirst = (nfirst>0)     ? nfirst : 0;
    nlast  = (nlast<npos-1) ? nlast  : npos-1;
    ns = 0;
    for (n=nfirst; n<=nlast; ++n) {
      xsmooth[ns] = x[n];
      ysmooth[ns] = y[n];
      ++ns;
    }
    return ns;
  }

  ndivs = ismooth;
  rdivs = 1./float(ismooth+1);

  n = nfirst;
  if (n > 0)
  {
    xl1 = x[n]-x[n-1];
    yl1 = y[n]-y[n-1];
    s1 = miutil::absval(xl1, yl1);
    xl2 = x[n+1]-x[n];
    yl2 = y[n+1]-y[n];
    s2 = miutil::absval(xl2, yl2);
    dx2 = (xl1*(s2/s1)+xl2*(s1/s2))/(s1+s2);
    dy2 = (yl1*(s2/s1)+yl2*(s1/s2))/(s1+s2);
  }
  else
  {
    xl2 = x[n+1]-x[n];
    yl2 = y[n+1]-y[n];
    s2 = miutil::absval(xl2, yl2);
    dx2 = xl2/s2;
    dy2 = yl2/s2;
  }

  xsmooth[0] = x[nfirst];
  ysmooth[0] = y[nfirst];
  ns = 0;

  for (n=nfirst+1; n<=nlast; ++n)
  {
    xl1 = xl2;
    yl1 = yl2;
    s1  = s2;
    dx1 = dx2;
    dy1 = dy2;

    if (n < npos-1) {
      xl2 = x[n+1]-x[n];
      yl2 = y[n+1]-y[n];
      s2 = miutil::absval(xl2, yl2);
      dx2 = (xl1*(s2/s1)+xl2*(s1/s2))/(s1+s2);
      dy2 = (yl1*(s2/s1)+yl2*(s1/s2))/(s1+s2);
    }
    else {
      dx2 = xl1/s1;
      dy2 = yl1/s1;
    }

    // four spline coefficients for x and y
    c32 =  1./s1;
    c42 =  c32*c32;
    c31 =  c42*3.;
    c41 =  c42*c32*2.;
    fx1 =  x[n-1];
    fx2 =  dx1;
    fx3 =  c31*xl1-c32*(2.*dx1+dx2);
    fx4 = -c41*xl1+c42*(dx1+dx2);
    fy1 =  y[n-1];
    fy2 =  dy1;
    fy3 =  c31*yl1-c32*(2.*dy1+dy2);
    fy4 = -c41*yl1+c42*(dy1+dy2);

    // make 'ismooth' straight lines, from point 'n-1' to point 'n'

    tstep = s1*rdivs;
    t = 0.;

    for (i=0; i<ndivs; ++i) {
      t += tstep;
      t2 = t*t;
      t3 = t2*t;
      ns++;
      xsmooth[ns] = fx1 + fx2*t + fx3*t2 + fx4*t3;
      ysmooth[ns] = fy1 + fy2*t + fy3*t2 + fy4*t3;
    }

    ns++;
    xsmooth[ns] = x[n];
    ysmooth[ns] = y[n];
  }

  ns++;

  return ns;
}
