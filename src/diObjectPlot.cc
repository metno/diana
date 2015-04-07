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

#include <diObjectPlot.h>

#include <puTools/miStringFunctions.h>

#include <math.h>
#include <sstream>

//#define DEBUGPRINT
#define MILOGGER_CATEGORY "diana.ObjectPlot"
#include <miLogger/miLogging.h>

using namespace::miutil;
using namespace std;

// static members
int ObjectPlot::siglinewidth=2;
map <std::string,std::string> ObjectPlot::editTranslations;

ObjectPlot::ObjectPlot()
{
  initVariables();
}

ObjectPlot::ObjectPlot(int objTy)
  : typeOfObject(objTy)
{
  METLIBS_LOG_SCOPE();
  initVariables();
}

ObjectPlot::ObjectPlot(const ObjectPlot &rhs)
{
  METLIBS_LOG_SCOPE();
  memberCopy(rhs);
}

ObjectPlot::~ObjectPlot()
{
  delete[] x;
  delete[] y;
  delete[] x_s;
  delete[] y_s;
}

void ObjectPlot::initVariables()
{
  isVisible=  true;
  isSelected=  false;
  spline=true;
  rotation=   0.0f;
  basisColor= "black";
  objectColour=Colour("black");

  currentState = active;  // points can be added
  addTop=false; //add elements to top instead of bottom
  stayMarked=false;
  joinedMarked=false;
  inBoundBox=false;

  boundBox.x1= +INT_MAX;
  boundBox.x2= -INT_MAX;
  boundBox.y1= +INT_MAX;
  boundBox.y2= -INT_MAX;

  w=10.0f;
  h=10.0f;
  window_dw=1;
  window_dh=1;
  setWindowInfo();

  //sensitivity to mark rectangle
  fSense= 2.5;
  test = false;
  nodePoints.clear();

  // Spline curve variables
  s_length = 0;
  x = NULL;
  y = NULL;
  x_s = NULL;
  y_s = NULL;

  rubber = false;

  region ="NONE";
  scaleToField= 1.0;
}


void ObjectPlot::memberCopy(const ObjectPlot &rhs)
{
  METLIBS_LOG_SCOPE();
  s_length = 0;
  x = NULL;
  y = NULL;
  x_s = NULL;
  y_s = NULL;

  isVisible= rhs.isVisible;
  isSelected= rhs.isSelected;
  spline=rhs.spline;
  rubber=false;
  rubberx=rhs.rubberx;
  rubbery=rhs.rubbery;
  rotation= rhs.rotation;
  basisColor = rhs.basisColor;
  objectColour = rhs.objectColour;
  objectBorderColour = rhs.objectBorderColour;
  itsLinetype = rhs.itsLinetype;
  drawIndex = rhs.drawIndex;
  currentState = rhs.currentState;
  addTop = rhs.addTop;
  boundBox = rhs.boundBox;
  w = rhs.w;
  h = rhs.h;
  window_dw = rhs.window_dw;
  window_dh = rhs.window_dh;
  fSense = rhs.fSense;
  nodePoints=rhs.nodePoints;
  test = rhs.test;
  stayMarked = false;
  joinedMarked=false;
  inBoundBox=false;
  type = rhs.type;
  typeOfObject = rhs.typeOfObject;
  scaleToField= rhs.scaleToField;
  region = rhs.region;
  // more is to be added here
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


vector<float> ObjectPlot::getX(){
  int n=nodePoints.size();
  vector<float> x;
  for (int i=0;i<n;i++)
    x.push_back(nodePoints[i].x);
  return x;
}


vector<float> ObjectPlot::getY(){
  int n=nodePoints.size();
  vector<float> y;
  for (int i=0;i<n;i++)
    y.push_back(nodePoints[i].y);
  return y;
}


vector<float> ObjectPlot::getXjoined(){
  int n=nodePoints.size();
  vector<float> x;
  for (int i=0;i<n;i++)
    if (nodePoints[i].joined)
      x.push_back(nodePoints[i].x);
  return x;

}


vector<float> ObjectPlot::getYjoined(){
  int n=nodePoints.size();
  vector<float> y;
  for (int i=0;i<n;i++)
    if (nodePoints[i].joined)
      y.push_back(nodePoints[i].y);
  return y;
}


vector<float> ObjectPlot::getXmarked(){
  int n=nodePoints.size();
  vector<float> x;
  for (int i=0;i<n;i++)
    if (nodePoints[i].marked)
      x.push_back(nodePoints[i].x);
  return x;
}


vector<float> ObjectPlot::getYmarked(){
  int n=nodePoints.size();
  vector<float> y;
  for (int i=0;i<n;i++)
    if (nodePoints[i].marked)
      y.push_back(nodePoints[i].y);
  return y;
}



vector<float> ObjectPlot::getXmarkedJoined(){
  int n=nodePoints.size();
  vector<float> x;
  for (int i=0;i<n;i++)
    if (nodePoints[i].marked && nodePoints[i].joined)
      x.push_back(nodePoints[i].x);
  return x;
}


vector<float> ObjectPlot::getYmarkedJoined(){
  int n=nodePoints.size();
  vector<float> y;
  for (int i=0;i<n;i++)
    if (nodePoints[i].marked && nodePoints[i].joined)
      y.push_back(nodePoints[i].y);
  return y;
}


void ObjectPlot::setXY(const vector<float>& x, const vector <float>& y)
{
  unsigned int n=x.size();
  unsigned int end=nodePoints.size();
  if (y.size()<n) n=y.size();
  for (unsigned int i =0;i<n;i++){
    if(i<end){
      nodePoints[i].x=x[i];
      nodePoints[i].y=y[i];
    } else {
      ObjectPoint pxy(x[i],y[i]);
      nodePoints.push_back(pxy);
    }
  }
  updateBoundBox();
}

void ObjectPlot::recalculate()
{
  //METLIBS_LOG_DEBUG("------------ ObjectPlot::recalculate");
}

void ObjectPlot::addPoint( float x , float y){
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
      if (objectIs(Border)) nodePoints[0].joined=true;
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
  // Changes boundBox
  if (!boundBox.isinside(x,y)) {
    if (x < boundBox.x1){ boundBox.x1=x; }
    if (x > boundBox.x2){ boundBox.x2=x; }
    if (y < boundBox.y1){ boundBox.y1=y; }
    if (y > boundBox.y2){ boundBox.y2=y; }
  }
}



bool ObjectPlot::markPoint( float x , float y){
  bool found=false;
  float fdeltaw=fSense*window_dw*w*0.5;
  int end = nodePoints.size();
  for (int i=0; i < end; i++){
    if (nodePoints[i].isInRectangle(x,y,fdeltaw)){
      if (nodePoints[i].marked==false) markedChanged=true;
      nodePoints[i].marked=true;
      found=true;
    } else if (!stayMarked && !joinedMarked) {
      if (nodePoints[i].marked==true) markedChanged=true;
      nodePoints[i].marked=false;
    }
  }
  return found;
}

void ObjectPlot::markAllPoints(){
  int end = nodePoints.size();
  for (int i=0; i < end; i++){
    if (nodePoints[i].marked==false) markedChanged=true;
    nodePoints[i].marked=true;
  }
}


void ObjectPlot::unmarkAllPoints(){
  if (stayMarked) return;
  int end = nodePoints.size();
  for (int i=0; i < end; i++){
    if (nodePoints[i].marked==true) markedChanged=true;
    nodePoints[i].marked=false;
  }
}


bool ObjectPlot::deleteMarkPoints(){
  deque <ObjectPoint>::iterator p=nodePoints.begin();
  while (p!= nodePoints.end()){
    if (p->marked)
      p=nodePoints.erase(p);
    else
      p++;
  }
  unmarkAllPoints();
  updateBoundBox();

  return true;
}


bool ObjectPlot::ismarkPoint( float x , float y){
  float fdeltaw=fSense*window_dw*w*0.5;
  int end = nodePoints.size();
  for (int i=0; i < end; i++)
    if (nodePoints[i].marked && nodePoints[i].isInRectangle(x,y,fdeltaw))
      return true;
  return false;
}


bool ObjectPlot::ismarkAllPoints(){
  int end = nodePoints.size();
  for (int i=0; i < end; i++)
    if (!nodePoints[i].marked)
      return false;
  return true;
}


bool ObjectPlot::ismarkSomePoint(){
  int end = nodePoints.size();
  for (int i=0; i < end; i++)
    if (nodePoints[i].marked)
      return true;
  return false;
}


bool ObjectPlot::ismarkEndPoint(){
  if (nodePoints.back().marked)
    return true;
  return false;
}

bool ObjectPlot::ismarkBeginPoint(){
  if (nodePoints.front().marked)
    return true;
  return false;
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
    nodePoints[iJoin].joined = true;
    return true;
  } else
    return false;
}

bool ObjectPlot::isJoinPoint( float x , float y, float &xjoin, float &yjoin){
  float fdeltaw=fSense*window_dw*w*0.5;
  int end = nodePoints.size();
  for (int i=0; i < end; i++)
    if (nodePoints[i].joined && nodePoints[i].isInRectangle(x,y,fdeltaw)){
      xjoin=nodePoints[i].x;
      yjoin=nodePoints[i].y;
      return true;
    }
  return false;
}


bool ObjectPlot::isJoinPoint( float x , float y){
  float fdeltaw=fSense*window_dw*w*0.5;
  int end = nodePoints.size();
  for (int i=0; i < end; i++)
    if (nodePoints[i].joined && nodePoints[i].isInRectangle(x,y,fdeltaw))
      return true;
  return false;
}

bool ObjectPlot::ismarkJoinPoint(){
  //function to check whether a joined point is marked
  int end = nodePoints.size();
  for (int i=0; i < end; i++)
    if (nodePoints[i].joined && nodePoints[i].marked)
      return true;
  return false;
}


void ObjectPlot::unjoinAllPoints(){
  int end = nodePoints.size();
  for (int i=0; i < end; i++)
    nodePoints[i].joined=false;
}


void ObjectPlot::unJoinPoint( float x , float y){
  float fdeltaw=fSense*window_dw*w*0.5;
  int end = nodePoints.size();
  for (int i=0; i < end; i++)
    if (nodePoints[i].joined && nodePoints[i].isInRectangle(x,y,fdeltaw))
      nodePoints[i].joined=false;
}

bool ObjectPlot::isEmpty(){
  if (nodePoints.size()>0) return false;
  else return true;
}


bool ObjectPlot::isSinglePoint(){
  if (nodePoints.size()==1) return true;
  else return false;
}

bool ObjectPlot::movePoint( float x , float y,float new_x , float new_y){
  int end = nodePoints.size();
  for (int i=0; i<end; i++){
    if (nodePoints[i].isInRectangle(x,y,0)){
      nodePoints[i].x=new_x;
      nodePoints[i].y=new_y;
      updateBoundBox();
      return true;
    }
  }
  return false;
}

bool  ObjectPlot::moveMarkedPoints(float d_x , float d_y){
  int end = nodePoints.size();
  if (end==0) return false;
  for (int i=0; i < end; i++)
    if (nodePoints[i].marked){
      nodePoints[i].x+=d_x;
      nodePoints[i].y+=d_y;
    }
  updateBoundBox();
  return true;
}


bool  ObjectPlot::rotateLine(float d_x , float d_y){
  //for now, only rotate fronts...
  if (!(objectIs(wFront)|| objectIs(Border))) return false;
  if (nodePoints.size()<2 || getXmarked().size()!=1) return false;

  int i, n= nodePoints.size();
  float *s = new float[n];
  float dx, dy, smax, weight;
  s[0]=0.;
  for (i=1; i<n; i++) {
    dx= nodePoints[i].x-nodePoints[i-1].x;
    dy= nodePoints[i].y-nodePoints[i-1].y;
    s[i]= s[i-1] + sqrtf(dx*dx+dy*dy);
  }
  for (int m=0; m < n; m++){
    if (!nodePoints[m].marked) continue;
    if (m==0) {
      smax= s[n-1];
      for (i=0; i<n-1; i++) {
        weight= (smax-s[i])/smax;
        nodePoints[i].x+=(d_x*weight);
        nodePoints[i].y+=(d_y*weight);
      }
      break;
    } else if (m==n-1) {
      smax= s[n-1];
      for (i=1; i<n; i++) {
        weight= s[i]/smax;
        nodePoints[i].x+=(d_x*weight);
        nodePoints[i].y+=(d_y*weight);
      }
    } else {
      smax= s[m];
      for (i=1; i<m; i++) {
        weight= s[i]/smax;
        nodePoints[i].x+=(d_x*weight);
        nodePoints[i].y+=(d_y*weight);
      }
      smax= s[n-1]-s[m];
      for (i=m; i<n-1; i++) {
        weight= (s[n-1]-s[i])/smax;
        nodePoints[i].x+=(d_x*weight);
        nodePoints[i].y+=(d_y*weight);
      }
    }
    break;
  }
  delete[] s;
  updateBoundBox();
  return true;
}



bool ObjectPlot::isInside( float x , float y){
  float fdeltaw=fSense*window_dw*w*0.5;
  int end = nodePoints.size();
  for (int i=0; i < end; i++){
    if (nodePoints[i].isInRectangle(x,y,fdeltaw))
      return true;
  }
  return false;
}

bool ObjectPlot::isInside( float x , float y, float &xin, float &yin){
  float fdeltaw=fSense*window_dw*w*0.5;
  int end = nodePoints.size();
  for (int i=0; i < end; i++){
    if (nodePoints[i].isInRectangle(x,y,fdeltaw)){
      xin = nodePoints[i].x;
      yin = nodePoints[i].y;
      return true;
    }
  }
  return false;
}


bool ObjectPlot::isBeginPoint( float x , float y, float &xin, float &yin){
  float fdeltaw=fSense*window_dw*w*0.5;
  if (nodePoints.front().isInRectangle(x,y,fdeltaw)){
    xin = nodePoints.front().x;
    yin = nodePoints.front().y;
    return true;
  }
  return false;
}



bool ObjectPlot::isEndPoint( float x , float y, float &xin, float &yin){
  float fdeltaw=fSense*window_dw*w*0.5;
  if (nodePoints.back().isInRectangle(x,y,fdeltaw)){
    xin = nodePoints.back().x;
    yin = nodePoints.back().y;
    return true;
  }
  return false;
}




void ObjectPlot::updateBoundBox()
{
  METLIBS_LOG_SCOPE();

  boundBox.x1= +INT_MAX;   // makes impossible box
  boundBox.x2= -INT_MAX;
  boundBox.y1= +INT_MAX;
  boundBox.y2= -INT_MAX;
  int end = nodePoints.size();
  for (int i=0; i<end; i++){
    float x=nodePoints[i].x;
    float y=nodePoints[i].y;
    changeBoundBox(x,y);
  }
  recalculate();
}

void ObjectPlot::drawJoinPoints(){
  if (!isVisible) return;
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glLineWidth(3);
  if (inBoundBox){
    vector<float> x = getXjoined();
    vector<float> y = getYjoined();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0,0,0,0.3);
    //draw all points in grey here (if cursor inside bounding box)
    drawPoints(x,y);
    glDisable(GL_BLEND);
  }
  glColor4f(0,1,1,0.5);
  vector<float> xmark = getXmarkedJoined();
  vector<float> ymark = getYmarkedJoined();
  glColor4f(0,1,1,1.0);
  //draw marked points here
  drawPoints(xmark,ymark);
}


void ObjectPlot::drawNodePoints(){
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glEdgeFlag(GL_TRUE);
  glShadeModel(GL_FLAT);
  if (inBoundBox){
    vector<float> x = getX();
    vector<float> y = getY();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0,0,0,0.3);
    //draw all points in grey here (if cursor inside bounding box)
    drawPoints(x,y);
    glDisable(GL_BLEND);
  }
  vector<float> xmark = getXmarked();
  vector<float> ymark = getYmarked();
  glColor4f(0,1,1,1.0);
  //draw marked points here
  drawPoints(xmark,ymark);
  //###################################################################
  //test= true; //test to show boundingbox and sensitive areas>
  if (test) drawTest();
  //###################################################################
}


void ObjectPlot::drawPoints(vector <float> xdraw, vector <float> ydraw){
  unsigned int msize=xdraw.size();
  if (ydraw.size()<msize) msize=ydraw.size();
  glLineWidth(2);
  float deltaw=window_dw*w*0.5;
  for (unsigned int i=0; i<msize; i++){
    glBegin(GL_POLYGON);
    if (objectIs(wFront) || objectIs(Border)){
      glVertex2f(xdraw[i]- deltaw,ydraw[i]- deltaw);
      glVertex2f(xdraw[i]+ deltaw,ydraw[i] - deltaw);
      glVertex2f(xdraw[i]+ deltaw,ydraw[i] + deltaw);
      glVertex2f(xdraw[i]- deltaw,ydraw[i] + deltaw);
    } else if (objectIs(wArea)){
      //Circle
      GLfloat xc,yc;
      GLfloat radius=deltaw;
      for(int j=0;j<100;j++){
        xc = radius*cos(j*2*M_PI/100.0);
        yc = radius*sin(j*2*M_PI/100.0);
        glVertex2f(xdraw[i]+xc,ydraw[i]+yc);
      }
    } else if (objectIs(wSymbol) || objectIs(RegionName)){
      float deltaw=window_dw*w*0.5;
      glVertex2f(xdraw[i]- deltaw,ydraw[i]- deltaw);
      glVertex2f(xdraw[i]+ deltaw,ydraw[i] - deltaw);
      glVertex2f(xdraw[i],ydraw[i] + deltaw);
    }
    glEnd();
  }


}


void ObjectPlot::drawTest(){
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  if (test){
    glColor4f(0.8,0.8,0,0.8);
    glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    glVertex2f(boundBox.x1,boundBox.y1);
    glVertex2f(boundBox.x2,boundBox.y1);
    glVertex2f(boundBox.x2,boundBox.y2);
    glVertex2f(boundBox.x1,boundBox.y2);
    glEnd();
  }
  int size = nodePoints.size()-1;
  if (size > 1 && test){
    for (int i = 0; i < s_length-1; i++){
      if (x_s[i+1]!=x_s[i])
      {
        float x1,x2,x3,x4,y1,y2,y3,y4;
        float dwidth = 16*getDwidth();
        float  deltay = y_s[i+1]-y_s[i];
        float  deltax = x_s[i+1]-x_s[i];
        float hyp = sqrtf(deltay*deltay+deltax*deltax);
        float dx = dwidth*deltay/hyp;
        float dy = dwidth*deltax/hyp;
        x1=x_s[i]-dx;
        y1=y_s[i]+dy;
        x2=x_s[i]+dx;
        y2=y_s[i]-dy;
        x3=x_s[i+1]+dx;
        y3=y_s[i+1]-dy;
        x4=x_s[i+1]-dx;
        y4=y_s[i+1]+dy;
        glColor4f(0.8,0.8,0.0,0.8);
        glLineWidth(2);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x1,y1);
        glVertex2f(x2,y2);
        glVertex2f(x3,y3);
        glVertex2f(x4,y4);
        glEnd();
      }
    }
  }
}


void ObjectPlot::plotRubber(){
  int size = nodePoints.size();
  glBegin(GL_LINE_STRIP);        // Draws line from end of front to cursor
  if (addTop)
    glVertex2f(nodePoints[0].x,nodePoints[0].y);
  else
    glVertex2f(nodePoints[size-1].x,nodePoints[size-1].y);
  glVertex2f(rubberx,rubbery);
  glEnd();
}


void ObjectPlot::setWindowInfo()
{
  window_dw= getStaticPlot()->getPhysToMapScaleX();
  window_dh= getStaticPlot()->getPhysToMapScaleY();
}


void  ObjectPlot::setBasisColor(std::string colour) {
  // sets basis color of object
  basisColor = colour;
  objectColour = Colour(colour);
}

void  ObjectPlot::setObjectColor(std::string colour) {
  objectColour = Colour(colour);
}

void  ObjectPlot::setObjectBorderColor(std::string colour) {
  objectBorderColour = Colour(colour);
}

void  ObjectPlot::setObjectColor(Colour::ColourInfo colour) {
  objectColour = Colour(colour.rgb[0],colour.rgb[1],colour.rgb[2]);
}

void  ObjectPlot::setObjectRGBColor(std::string rgbstring) {
  //METLIBS_LOG_DEBUG("rgba value is " << rgbstring);
  vector<std::string> colours2add=miutil::split(rgbstring, ",");
  int nColours = colours2add.size()/4;
  for (int cc=0; cc < nColours; cc++){
    //METLIBS_LOG_DEBUG("cc = " << cc);
    //METLIBS_LOG_DEBUG("The colour string to be added is\n ");
    unsigned char cadd[4];
    for (int i = 0;i<4;i++){
      //METLIBS_LOG_DEBUG(colours2add[cc*4+i]);
      cadd[i] = atoi(colours2add[cc*4+i].c_str());
    }
    objectColour = Colour(cadd[0],cadd[1],cadd[2],cadd[3]);
  }

}


Colour::ColourInfo  ObjectPlot::getObjectColor() {
  Colour::ColourInfo colour;
  colour.rgb[0]= (int) objectColour.R();
  colour.rgb[1]= (int) objectColour.G();
  colour.rgb[2]= (int) objectColour.B();

  return colour;
}


bool ObjectPlot::readObjectString(std::string objectString)
{
  std::string key,value;
  bool objectRead = false;
  bool typeRead = false;
  bool LonLatRead = false;
  METLIBS_LOG_DEBUG("ObjectPlot::readObjectString\n");
  METLIBS_LOG_DEBUG("string is: " << objectString);

  vector <std::string> tokens = miutil::split(objectString, 0, ";");
  for (unsigned int i = 0; i<tokens.size();i++){
    vector <std::string> stokens = miutil::split(tokens[i], 0, "=");
    if( stokens.size() != 2 ) {
      METLIBS_LOG_WARN(" readObjectString: key without value: "<<tokens[i]);
      return false;
    }
    key = miutil::to_lower(stokens[0]);
    value = stokens[1];
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
      vector<std::string> points2add=miutil::split(value, ",");
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
  //if (objectIs(wFront)) METLIBS_LOG_DEBUG("Object is front");
  //else if (objectIs(wSymbol)) METLIBS_LOG_DEBUG("Object is symbol");
  //else if (objectIs(wArea)) METLIBS_LOG_DEBUG("Object is area");
  //else METLIBS_LOG_DEBUG("Unknown object type "<< typeOfObject);
  //METLIBS_LOG_DEBUG("Type = " << type);
  //METLIBS_LOG_DEBUG("Number of points = " << nodePoints.size());
  return true;
}




std::string ObjectPlot::writeObjectString(){
  //write type of object
  std::string ret=writeTypeString();
  //ret+="LatitudeLongitude=\n";    // old and wrong!
  ret+="LongitudeLatitude=\n";
  ostringstream cs;
  //write coordinates
  if (nodePoints.size()){
    for (unsigned int i=0; i < nodePoints.size(); i++)
    {
      cs << nodePoints[i].x<<","<< nodePoints[i].y;
      if (i<nodePoints.size()-1) cs <<",\n";
      else cs <<";\n";
    }
    ret+=cs.str();
  }

  ostringstream rs;
  ret+="RGBA=";
  //write colour
  rs << (int) objectColour.R() << "," << (int) objectColour.G()
          << "," << (int) objectColour.B() <<"," << (int) objectColour.A();
  rs <<";\n";
  ret+=rs.str();
  //write "!" to signal end of object
  ret+="!\n";
  return ret;
}




bool ObjectPlot::isInRegion(int region,int matrix_nx,int matrix_ny,double resx,double resy,
    int * combinematrix){
  int end = nodePoints.size();
  for (int i=0; i < end; i++){
    float x1=nodePoints[i].x/resx;
    float y1=nodePoints[i].y/resy;
    if (x1>=0. && x1<=matrix_nx-1. &&
        y1>=0. && y1<=matrix_ny-1.) {
      int x= int(x1+0.5);
      int y= int(y1+0.5);
      int index = matrix_nx*y+x;
      if (combinematrix[index] == region) return true;
    }
  }

  return false;
}

int ObjectPlot::combIndex(int matrix_nx, int matrix_ny, double resx, double resy, int * combinematrix){

  float x1=nodePoints[0].x/resx;
  float y1=nodePoints[0].y/resy;
  if (x1>=0. && x1<=matrix_nx-1. &&
      y1>=0. && y1<=matrix_ny-1.) {
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


bool ObjectPlot::oktoJoin(bool joinAll){
  if  (joinAll || ismarkSomePoint() || currentState == active){
    // only fronts can be joined
    // drawIndex from SigWeatherFront and higher are lines etc. not to be joined
    // empty fronts shouldn't be joined
    if (objectIs(wFront) && drawIndex<SigweatherFront && nodePoints.size())
      return true;
    else
      return false;
  }
  return false;
}



bool ObjectPlot::oktoMerge(bool mergeAll,int index){
  if  (mergeAll || ismarkSomePoint() || currentState == active){
    if (objectIs(wFront) && index==drawIndex && nodePoints.size())
      return true;
    else
      return false;
  }
  return false;
}



void ObjectPlot::setRubber(bool rub, const float x, const float y){
  if (objectIs(wFront) || objectIs(wArea)){
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

bool ObjectPlot::onLine(float x, float y){
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
            insert = i/(divSpline+1)+1;
            return true;
          }
        }
      }else{
        for (int i = 0; i < size; i++){
          float x1,x2,y1,y2;
          if (i+1<size){
            x1=nodePoints[i].x;
            x2=nodePoints[i+1].x;
            y1=nodePoints[i].y;
            y2=nodePoints[i+1].y;
          } else if (i+1==size && objectIs(wArea)){
            x1=nodePoints[i].x;
            x2=nodePoints[0].x;
            y1=nodePoints[i].y;
            y2=nodePoints[0].y;
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
bool ObjectPlot::isInsideBox(float x, float y,float x1,float y1,float x2,float y2){
  float salpha,calpha;
  float dwidth = 2*getLineWidth()*getDwidth();
  Rectangle* box= new Rectangle(0,0,0,0);
  if (x2!=x1){
    float  dy = y2-y1;
    float  dx = x2-x1;
    float hyp = sqrtf(dy*dy+dx*dx);
    int sign=1;
    if (dy*dx < 0) sign = -1;
    salpha = sign*fabsf(dy)/hyp;
    calpha = fabsf(dx)/hyp;
  }else{
    salpha=1.0;
    calpha = 0.0;
  }
  //calculate x and y in transf. system
  float xprime = x*calpha+y*salpha;
  float yprime = y*calpha-x*salpha;
  if (x1<x2){
    box->x1=x1*calpha+y1*salpha;
    box->x2=x2*calpha+y2*salpha;
  }else if (x1>x2){
    box->x1=x2*calpha+y2*salpha;
    box->x2=x1*calpha+y1*salpha;
  }
  box->y1 = y1*calpha-x1*salpha-dwidth;
  box->y2 = box->y1 +2*dwidth;
  if (box->isinside(xprime,yprime)){
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
    delete box;
    return true;
  }
  delete box;
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
    s1  = sqrtf(xl1*xl1+yl1*yl1);
    xl2 = x[n+1]-x[n];
    yl2 = y[n+1]-y[n];
    s2  = sqrtf(xl2*xl2+yl2*yl2);
    dx2 = (xl1*(s2/s1)+xl2*(s1/s2))/(s1+s2);
    dy2 = (yl1*(s2/s1)+yl2*(s1/s2))/(s1+s2);
  }
  else
  {
    xl2 = x[n+1]-x[n];
    yl2 = y[n+1]-y[n];
    s2  = sqrtf(xl2*xl2+yl2*yl2);
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
      s2  = sqrtf(xl2*xl2+yl2*yl2);
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
