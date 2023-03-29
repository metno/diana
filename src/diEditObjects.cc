/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2019 met.no

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

#include "diana_config.h"

#include "diEditObjects.h"
#include "diWeatherArea.h"
#include "diWeatherFront.h"
#include "diWeatherSymbol.h"
#include "util/string_util.h"

#include <cmath>

#define MILOGGER_CATEGORY "diana.EditObjects"
#include <miLogger/miLogging.h>

using namespace::miutil;

std::map<int,object_modes> EditObjects::objectModes;
std::map<int,combine_modes> EditObjects::combineModes;

EditObjects::EditObjects()
{
  METLIBS_LOG_SCOPE();

  //undo variables
  undoCurrent = new UndoFront( );
  undoHead = undoCurrent;
  undoTemp=0;

  init();
}

void EditObjects::init()
{
  createobject= false;
  inDrawing= false;
  filename= std::string();
  itsComments = std::string();
  commentsChanged = false;
  commentsSaved = true;
  labelsSaved=true;
  prefix= std::string();
  mapmode = normal_mode;
  clear();
}

void EditObjects::defineModes(std::map<int,object_modes> objModes,
    std::map<int,combine_modes> combModes){
  objectModes=objModes;
  combineModes=combModes;
}


void EditObjects::setEditMode(const mapMode mmode, const int emode, const std::string etool)
{
  //called when new edit mode/tool selected in gui (EditDIalog)
  METLIBS_LOG_SCOPE();
  mapmode= mmode;
  editmode= emode;
  drawingtool= etool; //draw tool string
  if (mapmode==draw_mode){
    objectmode= objectModes[emode];
  }  else if (mapmode==combine_mode){
    combinemode=combineModes[emode];
  }
}


/************************************************************
 *  Edit methods called by user via EditManager             *
 ************************************************************/
void EditObjects::setMouseCoordinates(const float x,const float y)
{
  newx = x;
  newy = y;
}

void EditObjects::createNewObject()
{
  METLIBS_LOG_SCOPE();
  createobject=true;
  inDrawing=true;
  if (mapmode==draw_mode){
    objectmode= objectModes[editmode];
  } else if (mapmode==combine_mode) {
    combinemode=combineModes[editmode];
  }
}


void EditObjects::editStayMarked()
{
  int edsize=objects.size();
  for (int i =0; i < edsize; i++){
    // mark objects
    if  (objects[i]->ismarkSomePoint())
      objects[i]->setStayMarked(true);
  }
}

void EditObjects::editNotMarked()
{
  int edsize=objects.size();
  for (int i =0; i < edsize; i++){
    // unmark objects
    objects[i]->setStayMarked(false);
    objects[i]->unmarkAllPoints();
    objects[i]->setInBoundBox(false);
  }
  setAllPassive();
}


bool EditObjects::editResumeDrawing(const float x, const float y)
{
  METLIBS_LOG_SCOPE("mapmode = " << mapmode << "editmode = " << editmode);

  bool ok = false;

  //resumeDrawing of marked front or insert point

  std::vector <ObjectPlot*>::iterator p = objects.begin();
  while (p!=objects.end()) {
    ObjectPlot * pobject = *p;
    if( pobject->ismarkAllPoints()){
      if (pobject->insertPoint(x,y)) ok = true;
    } else if (pobject->ismarkEndPoint() || pobject->ismarkBeginPoint()) {
      inDrawing = true;
      if (pobject->objectIs(wFront))
        objectmode= front_drawing;
      if (pobject->objectIs(wArea))
        objectmode= area_drawing;
      if (pobject->objectIs(Border))
        combinemode= set_borders;
      removeObject(p); //remove from editobjects
      pobject->resumeDrawing();
      addObject(pobject); //put at the end
      ok = true;
      break;
    }
    p++;
  }
  return ok;
}


bool EditObjects::editDeleteMarkedPoints()
{
  bool ok=false;

  if (mapmode==draw_mode){

    std::vector <ObjectPlot*>::iterator p1 = objects.begin();
    while (p1!=objects.end()){
      ObjectPlot * pobject = *p1;
      if (pobject->deleteMarkPoints() &&  pobject->isEmpty()){
        p1 = removeObject(p1); //remove from editobjects
        delete pobject;
      } else p1++;
    }

    ok=true;
    //Check if this affected the join points
    checkJoinPoints();

  } else if (mapmode == combine_mode){
    std::vector <ObjectPlot*>::iterator q1 = objects.begin();
    while (q1!=objects.end()){
      ObjectPlot * qobject = *q1;
      //not allowed to remove the whole border
      if (!(qobject->ismarkAllPoints() && qobject->objectIs(Border) ) )
        //not allowed to remove joined point
        if (!qobject->ismarkJoinPoint())
          if (qobject->deleteMarkPoints()) ok = true;
      if( qobject->isEmpty()){
        q1 = removeObject(q1);
        delete qobject;
      } else q1++;
    }

  }

  setAllPassive();

  return ok;
}



bool EditObjects::editAddPoint(const float x, const float y)
{
  METLIBS_LOG_SCOPE("mapmode = " << mapmode << "editmode = " << editmode << "drawingtool =" << drawingtool);

  bool ok = false;

  if (mapmode==draw_mode){
    if (createobject) {
      if (objectmode == front_drawing){
        addObject(new WeatherFront(drawingtool));
      } else if (objectmode == area_drawing){
        addObject(new WeatherArea(drawingtool));
      }
      createobject= false;
    }

    if (objectmode==symbol_drawing){
      //make a new vector for every new symbol
      addObject(new WeatherSymbol(drawingtool,wSymbol));
      if (objects.size()){
        objects.back()->addPoint(x,y);
        ok = true;
      }
    } else if (objectmode==front_drawing || objectmode==area_drawing){
      //fronts or areas
      if (objects.size()){
        objects.back()->addPoint(x,y);
        ok = true;
      }
    }

  } else if (mapmode==combine_mode){

    if (combinemode == set_borders){
      if (objects.size()) {
        objects.back()->addPoint(x,y);
        ok = true;
      }
    } else if (combinemode == set_region){
      //make a new vector for every new Region (symbol)
      addObject(new WeatherSymbol(drawingtool,RegionName));
      if (objects.size()){
        objects.back()->addPoint(x,y);
        ok = true;
      }
    }
  }
  return ok;
}


bool EditObjects::editMergeFronts(bool mergeAll)
{
  //merge two fronts of same type into one
  METLIBS_LOG_SCOPE(LOGVAL(mergeAll));

  bool frontsChanged= false;
  std::vector <ObjectPlot*>::iterator p = objects.begin();
  int pcount=0;//for undo
  for (;p!=objects.end();p++){
    ObjectPlot * pfront = *p;
    if (pfront->oktoMerge(mergeAll,pfront->getIndex())){
      int end = pfront->endPoint();
      float xend[2],yend[2],xnew,ynew;
      xend[0] = pfront->getXY(0).x();
      yend[0] = pfront->getXY(0).y();
      xend[1] = pfront->getXY(end).x();
      yend[1] = pfront->getXY(end).y();
      std::vector <ObjectPlot*>::iterator q = objects.begin();
      int qcount=0; //for undo
      while (q!=objects.end()){
        bool merged = false;
        ObjectPlot * qfront = *q;
        //for each front, check if it is separate front of same type
        if (pfront !=qfront && qfront->oktoMerge(true,pfront->getIndex())){
          for (int i=0;i<2;i++){ //loop over endpoints
            if(   qfront->isBeginPoint(xend[i],yend[i],xnew,ynew) ||
                qfront->isEndPoint(xend[i],yend[i],xnew,ynew)){
              pfront->addTop=(i==0);
              if (!merged){
                ObjectPlot * pold
                = new WeatherFront(*((WeatherFront*)(pfront)));
                ObjectPlot * qold
                = new WeatherFront(*((WeatherFront*)(qfront)));
                if (pfront->addFront(qfront)){
                  merged=true;
                  q=objects.erase(q);
                  delete qfront;
                  //after merging, pfront has changed
                  end = pfront->endPoint();
                  xend[0] = pfront->getXY(0).x();
                  yend[0] = pfront->getXY(0).y();
                  xend[1] = pfront->getXY(end).x();
                  yend[1] = pfront->getXY(end).y();
                  if (pcount<qcount){
                    undoTemp->undoAdd(Replace,pold,pcount);
                    undoTemp->undoAdd(Insert,qold,qcount);
                  } else{
                    undoTemp->undoAdd(Insert,qold,qcount);
                    undoTemp->undoAdd(Replace,pold,pcount);
                  }
                  frontsChanged=true;
                }
                delete pold;
                delete qold;
              }
            }
            if (merged){
              pfront->markAllPoints();
              break;
            }

          } // end loop over endpoints
        } // end of test qfront
        qcount++;
        if (!merged)
          q++;
        else{
          checkJoinPoints();
          return frontsChanged;
        }
        //	  break;
      } //end of qloop
    }  //end of test pfront
    pcount++;
  } //end of ploop
  //after all fronts are merged...
  checkJoinPoints();
  return frontsChanged;
}


bool EditObjects::editJoinFronts(bool joinAll,bool movePoints,bool joinOnLine)
{
  //input parameters
  //joinAll = true ->all fronts are joined
  //        = false->only marked or active fronts are joined
  //movePoints = true->points moved to join
  //           = false->points not moved to join
  //joinOnLine = true->join front to line not just end points
  //           = false->join front only to join- and endpoints...

  METLIBS_LOG_SCOPE(LOGVAL(joinAll) << LOGVAL(movePoints) << LOGVAL(joinOnLine));

  std::vector <ObjectPlot*>::iterator p = objects.begin();
  for (;p!=objects.end();p++){
    ObjectPlot * pfront = *p;
    if (pfront->oktoJoin(joinAll)){
      bool endPointJoined[2] = {false,false};
      int end = pfront->endPoint();
      float xend[2],yend[2],xnew,ynew;
      xend[0] = pfront->getXY(0).x();
      yend[0] = pfront->getXY(0).y();
      xend[1] = pfront->getXY(end).x();
      yend[1] = pfront->getXY(end).y();
      std::vector <ObjectPlot*>::iterator q = objects.begin();
      for (;q!=objects.end();q++){
        ObjectPlot * qfront = *q;
        if (pfront !=qfront && qfront->oktoJoin(true)){
          for (int i=0;i<2;i++){
            if (!endPointJoined[i]){
              // check if endpoints of front close to join point or endpoints
              if (qfront->isJoinPoint(xend[i],yend[i],xnew,ynew)  ||
                  qfront->isBeginPoint(xend[i],yend[i],xnew,ynew) ||
                  qfront->isEndPoint(xend[i],yend[i],xnew,ynew)){
                // if we found a point, move front
                if (movePoints) {
                  pfront->movePoint(xend[i],yend[i],xnew,ynew);
                  pfront->joinPoint(xnew,ynew);
                  qfront->joinPoint(xnew,ynew);
                } else {
                  pfront->joinPoint(xend[i],yend[i]);
                }
                endPointJoined[i] = true;
              }
            }
          }
        }
      }
      if (joinOnLine && !(endPointJoined[0] && endPointJoined[1])){
        //if no end/join points close to front join on line
        std::vector <ObjectPlot*>::iterator q = objects.begin();
        for (;q!=objects.end();q++){
          ObjectPlot * qfront = *q;
          if (pfront !=qfront && qfront->oktoJoin(true)){
            for (int i=0;i<2;i++){
              if (!endPointJoined[i]){
                //check if end points close to node points
                if (qfront->isInside(xend[i],yend[i],xnew,ynew)){
                  pfront->movePoint(xend[i],yend[i],xnew,ynew);
                  pfront->joinPoint(xnew,ynew);
                  qfront->joinPoint(xnew,ynew);
                  endPointJoined[i] = true;
                }
              }
            }
          }
        }
        if (!(endPointJoined[0] && endPointJoined[1])){
          q = objects.begin();
          for (;q!=objects.end();q++){
            ObjectPlot * qfront = *q;
            if (pfront !=qfront && qfront->oktoJoin(true)){
              for (int i=0;i<2;i++){
                //check if end points close to line
                if (!endPointJoined[i] && qfront->onLine(xend[i],yend[i])){
                  float distx = qfront->getDistX();
                  float disty = qfront->getDistY();
                  xnew = xend[i]+distx; ynew = yend[i]+disty;
                  pfront->movePoint(xend[i],yend[i],xnew,ynew);
                  pfront->joinPoint(xnew,ynew);
                  qfront->insertPoint(xnew,ynew);
                  qfront->joinPoint(xnew,ynew);
                }
              }
            }
          }
        }
      }
    }
  }
  return false;
}


bool EditObjects::editMoveMarkedPoints(const float x, const float y)
{
  bool changed=false;
  int edsize = objects.size();
  for (int i =0; i < edsize; i++) {
    if (objects[i]->moveMarkedPoints(x,y))
      changed = true;
  }
  return changed;
}

bool EditObjects::editRotateLine(const float x, const float y)
{
  bool changed = false;
  int edsize = objects.size();
  for (int i =0; i < edsize; i++){
    if (objects[i]->rotateLine(x,y))
      changed= true;
  }
  return changed;
}


void EditObjects::editCopyObjects()
{
  METLIBS_LOG_SCOPE();

  int edsize = objects.size();
  copyObjects.clear();
  //position of objects
  copyObjects.xcopy=newx;
  copyObjects.ycopy=newy;
  for (int i =0; i< edsize;i++){
    if (objects[i]->ismarkAllPoints()){
      ObjectPlot * copy;
      if (objects[i]->objectIs(wFront))
        copy = new WeatherFront(*((WeatherFront*)(objects[i])));
      else if (objects[i]->objectIs(wSymbol))
        copy = new WeatherSymbol(*((WeatherSymbol*)(objects[i])));
      else if (objects[i]->objectIs(wArea))
        copy = new WeatherArea(*((WeatherArea*)(objects[i])));
      copyObjects.objects.push_back(copy);
    }
  }
  copyObjects.setArea(itsArea);
  copyObjects.switchProjection(getGeoArea());
}


void EditObjects::editPasteObjects()
{
  METLIBS_LOG_SCOPE();

  float diffx,diffy;
  int csize = copyObjects.objects.size();

  if (csize){
    unmarkAllPoints();
    copyObjects.switchProjection(itsArea);
    diffx=newx-copyObjects.xcopy;
    diffy=newy-copyObjects.ycopy;
    // AC: changed offset-check to absolute values
    if (fabs(diffx) < fabs(objects[0]->getFdeltaw()) &&
        fabs(diffy) < fabs(objects[0]->getFdeltaw())){
      diffx=objects[0]->getFdeltaw();
      diffy=objects[0]->getFdeltaw();
    }
    for (int i =0; i< csize;i++){
      copyObjects.objects[i]->markAllPoints();
      ObjectPlot * paste = 0;
      ObjectPlot * copy = 0;
      copy=copyObjects.objects[i];
      if (copy->objectIs(wFront))
        paste = new WeatherFront(*((WeatherFront*)(copy)));
      else if (copy->objectIs(wSymbol))
        paste = new WeatherSymbol(*((WeatherSymbol*)(copy)));
      else if (copy->objectIs(wArea))
        paste = new WeatherArea(*((WeatherArea*)(copy)));
      addObject(paste);
    }
    editMoveMarkedPoints(diffx,diffy);
    unmarkAllPoints();
  }
}

void EditObjects::editFlipObjects()
{
  int edsize = objects.size();
  for (int i =0; i< edsize;i++){
    if (objects[i]->ismarkAllPoints())
      objects[i]->flip();
  }
}

void EditObjects::editUnJoinPoints()
{
  int edsize = objects.size();
  for (int i=0; i<edsize; i++){
    //unjoin all points on marked fronts
    if  (objects[i]->ismarkAllPoints())
      objects[i]->unjoinAllPoints();
  }
}


bool EditObjects::editChangeObjectType(int val)
{
  METLIBS_LOG_SCOPE();

  bool ok=false;
  int edsize=objects.size();
  for (int i =0; i< edsize;i++){
    if (objects[i]->ismarkAllPoints()){
      objects[i]->increaseType(val);
      ok=true;
    }
  }
  return ok;
}

bool EditObjects::editSplitFront(const float x, const float y)
{
  bool ok=false;
  std::vector <ObjectPlot*>::iterator p = objects.begin();
  while (p!= objects.end()){
    ObjectPlot * pobject = *p;
    if( pobject->ismarkAllPoints()){
      //split front - get pointer to new front
      ObjectPlot * newobject = pobject->splitFront(x,y);
      if (newobject !=NULL){
        objects.push_back(newobject);
        ok= true;
        break;
      }
    }
    p++;
  }
  return ok;
}


void EditObjects::unmarkAllPoints()
{
  METLIBS_LOG_SCOPE();

  //unmark all points
  int edsize=objects.size();
  for (int i = 0;i<edsize;i++){
    objects[i]->setJoinedMarked(false);
    objects[i]->unmarkAllPoints();
  }
}


bool EditObjects::editCheckPosition(const float x, const float y)
{
  METLIBS_LOG_SCOPE();

  //check to see if pointer over any symbols,fronts,areas
  bool found=false;
  bool changed=false;
  bool foundJoined=false;

  int edsize=objects.size();

  if (mapmode == draw_mode){
    //symbols
    for (int i=0; i< edsize;i++){
      ObjectPlot * pfront = objects[i];
      if (!pfront->objectIs(wSymbol)) continue;
      if (found || !pfront->visible())
        pfront->unmarkAllPoints();
      else if (pfront->isOnObject(x,y))
        found=true;
      if (pfront->getMarkedChanged()) changed=true ;
    }
    //fronts
    foundJoined = findJoinedPoints(x,y,wFront);
    for (int i=0; i< edsize;i++){
      ObjectPlot * pfront = objects[i];
      if (!pfront->objectIs(wFront)) continue;
      pfront->setMarkedChanged(false);
      if (found || !pfront->visible()){
        if (!pfront->getJoinedMarked()) pfront->unmarkAllPoints();
      }
      else if (foundJoined && pfront->isJoinPoint(x,y)){
        pfront->markPoint(x,y);
        if (!autoJoinOn)
          findJoinedFronts(pfront,wFront);
      }
      else if (!foundJoined && pfront->isOnObject(x,y)){
        found=true;
        if (autoJoinOn)
          findJoinedFronts(pfront,wFront);
      } else
        pfront->unmarkAllPoints();
      if (pfront->getMarkedChanged()) changed=true ;
    }
    if (foundJoined) found = true;
    //areas
    for (int i=0; i< edsize;i++){
      ObjectPlot * pfront = objects[i];
      if (!pfront->objectIs(wArea)) continue;
      if (found || !pfront->visible())
        pfront->unmarkAllPoints();
      else if (pfront->isOnObject(x,y))
        found=true;
      if (pfront->getMarkedChanged()) changed=true ;
    }
  } else if (mapmode == combine_mode){
    for (int i=0; i< edsize;i++){
      //region names
      ObjectPlot * pfront = objects[i];
      if (!pfront->objectIs(RegionName)) continue;
      if (found || !pfront->visible())
        pfront->unmarkAllPoints();
      else if (pfront->isOnObject(x,y))
        found=true;
      if (pfront->getMarkedChanged()) changed=true ;
    }
    //borders
    foundJoined = findJoinedPoints(x,y,Border);
    for (int i=0; i< edsize;i++){
      ObjectPlot * pfront = objects[i];
      if (!pfront->objectIs(Border)) continue;
      pfront->setMarkedChanged(false);
      if (found || !pfront->visible())
        pfront->unmarkAllPoints();
      else if (foundJoined && pfront->isJoinPoint(x,y)){
        pfront->markPoint(x,y);
      }
      else if (!foundJoined && pfront->isOnObject(x,y)){
        found=true;
        findJoinedFronts(pfront,Border);
      } else
        pfront->unmarkAllPoints();
      if (pfront->getMarkedChanged()) changed=true ;
    }
  }

  return changed;
}

bool EditObjects::editIncreaseSize(float val)
{
  bool doCombine=false;
  for (ObjectPlot* op : objects) {
    if (op->ismarkAllPoints()) {
      op->increaseSize(val);
      if (op->objectIs(Border))
        doCombine= true;
    }
  }
  return doCombine;
}

void EditObjects::editRotateObjects(float val)
{
  //only rotates complex objects !!!
  for (ObjectPlot* op : objects) {
    if (op->isComplex() && op->ismarkAllPoints()) {
      op->rotateObject(val);
    }
  }
}


void EditObjects::editHideBox()
{
  for (ObjectPlot* op : objects) {
    if (op->isComplex() && op->ismarkAllPoints()) {
      op->hideBox();
    }
  }
}

void EditObjects::editDefaultSize()
{
  for (ObjectPlot* op : objects) {
    if (op->ismarkAllPoints()) {
      op->setDefaultSize();
    }
  }
}

void EditObjects::editDefaultSizeAll()
{
  for (ObjectPlot* op : objects) {
    op->setDefaultSize();
  }
}

void EditObjects::editHideAll()
{
  for (ObjectPlot* op : objects) {
    op->setVisible(false);
    op->unmarkAllPoints();
  }
}

void EditObjects::editUnHideAll()
{
  for (ObjectPlot* op : objects) {
    op->setVisible(true);
  }
}

void EditObjects::editHideCombineObjects(std::string region)
{
  METLIBS_LOG_SCOPE(LOGVAL(region));

  for (ObjectPlot* op : objects) {
    if (op->getRegion() != region)
      op->setVisible(false);
  }
}


void EditObjects::editHideCombineObjects(int ir)
{
  METLIBS_LOG_SCOPE(LOGVAL(ir));

  std::string region = WeatherSymbol::getAllRegions(ir);
  if (!region.empty()){
    for (ObjectPlot* op : objects) {
      if (op->getRegion() != region)
        op->setVisible(false);
    }
  }
}

/************************************************************
 *  Various methods                                         *
 ************************************************************/

bool EditObjects::setRubber(bool rubber, const float x, const float y)
{
  bool changed=false;
  if(!createobject){
    if (mapmode==draw_mode){
      if (objects.size()){
        objects.back()->setRubber(rubber,x,y);
        changed=true;
      }
    }
  }
  return changed;
}


void EditObjects::setAllPassive()
{
  METLIBS_LOG_SCOPE();

  //Sets current status of the objects to passive
  for (ObjectPlot* op : objects) {
    op->setState(ObjectPlot::passive);
  }
  createobject= false;
  inDrawing=false;
}


void EditObjects::cleanUp()
{
  //remove any front with only one point
  std::vector <ObjectPlot*>::iterator p1 = objects.begin();
  while (p1!=objects.end()){
    ObjectPlot * pobject = *p1;
    if(pobject->objectIs(wFront) && pobject->isSinglePoint()){
      p1 = removeObject(p1); //remove objects
      delete pobject;
    } else
      p1++;
  }
}


void EditObjects::checkJoinPoints()
{
  int edsize=objects.size();
  // check all fronts for join points...
  for (int i =0; i <edsize; i++) {
    if (!objects[i]->objectIs(wFront))
      continue;

    const std::vector<XY> xyjoin = objects[i]->getXYjoined();
    const int jsize = xyjoin.size();
    //loop over all the join rectangles
    for (int k=0; k<jsize; k++) {
      const XY& xy1 = xyjoin[k];
      int njoin=0;
      for (int j = 0; j < edsize; j++) {
        if (!objects[i]->objectIs(wFront))
          continue;
        if (i!=j && objects[j]->isJoinPoint(xy1.x(), xy1.y()))
          njoin=njoin+1;
      }
      if (njoin < 1)
        objects[i]->unJoinPoint(xy1.x(), xy1.y());
    }
  }
}

void EditObjects::changeDefaultSize()
{
  //set symbol default size to size of last read object !
  for (ObjectPlot* op : objects) {
    if (op->objectIs(wSymbol))
      op->changeDefaultSize();
  }
}

/************************************************************
 *  Undo and redo methods                                   *
 ************************************************************/

void EditObjects::newUndoCurrent(UndoFront * undoNew)
{
  //a new UndoFront is created and becomes undoCurrent
  // last points back to the previous undoCurrent
  //next points to NULL
  METLIBS_LOG_SCOPE();

  if (undoNew == NULL){
    METLIBS_LOG_WARN("undoNew == 0 !");
    return;
  }

  //Remove obsolete entries
  UndoFront * undoObsolete = undoCurrent->Next;
  while (undoObsolete != NULL) {
    if (undoObsolete->Next!=NULL){
      undoObsolete = undoObsolete->Next;
      delete undoObsolete->Last;
    }else{
      delete undoObsolete;
      undoObsolete = NULL;
    }
  }

  //Update next pointers
  undoNew->Last = undoCurrent;
  undoNew->Next = NULL;
  undoCurrent->Next = undoNew;
  undoCurrent =  undoNew;
  if (undoCurrent->Next !=0)
    METLIBS_LOG_WARN("undoCurrent->Next !=0");
}


bool EditObjects::redofront()
{
  METLIBS_LOG_SCOPE();

  if (undoCurrent->Next != 0) {
    undoCurrent = undoCurrent->Next;
    if (!changeCurrentFronts()){
      //something went wrong
      undofrontClear();
      return false;
    }
    if (undoCurrent->Next ==NULL)
      return false;
    else return true;
  }
  else{
    METLIBS_LOG_ERROR("Not possible to redo any more!");
    return false;
  }
}

bool EditObjects::undofront()
{
  METLIBS_LOG_SCOPE();

  if (undoCurrent->Last!= NULL){
    if (!changeCurrentFronts()){
      //something went wrong
      undofrontClear();
      return false;
    }
    undoCurrent = undoCurrent->Last;
    if (undoCurrent->Last ==NULL)
      return false;
    else return true;
  }
  else {
    METLIBS_LOG_ERROR("Not possible to undo any more ! ");
    return false;
  }
}

void EditObjects::undofrontClear()
{
  //Removes everything from undoFront buffers
  UndoFront* undoObsolete = undoHead->Next;
  while (undoObsolete != NULL) {
    if (undoObsolete->Next!=NULL){
      undoObsolete = undoObsolete->Next;
      delete undoObsolete->Last;
    }else{
      delete undoObsolete;
      undoObsolete = NULL;
    }
  }

  undoCurrent = undoHead;
  undoCurrent->Next = NULL;
}


bool EditObjects::saveCurrentFronts(operation iop, UndoFront * undo)
{
  METLIBS_LOG_SCOPE();

  int edsize=objects.size();
  bool frontsChanged = false;
  undo->oldArea = itsArea;
  undo->iop = iop;
  undoTemp=undo;
  //save objects according to sort of operation
  switch (iop){
  case AddPoint:
    if (createobject || objectmode==symbol_drawing) {
      undo->undoAdd(Erase,0,edsize);
    }
    else {
      ObjectPlot * pold = objects.back();
      undo->undoAdd(Replace,pold,edsize-1);
    }
    frontsChanged = true;
    break;
  case MoveMarkedPoints:
    for (int i =0; i < edsize; i++) {
      if (objects[i]->ismarkSomePoint()){
        ObjectPlot * pold = objects[i];
        undo->undoAdd(Replace,pold,i);
        frontsChanged = true;
      }
    }
    break;
  case DeleteMarkedPoints:
    for (int i =0; i <edsize; i++) {
      if (objects[i]->ismarkAllPoints()){
        ObjectPlot * pold = objects[i];
        undo->undoAdd(Insert,pold,i);
        frontsChanged = true;
      } else if (objects[i]->ismarkSomePoint()){
        ObjectPlot * pold = objects[i];
        undo->undoAdd(Replace,pold,i);
        frontsChanged = true;
      }
    }
    break;
  case FlipObjects:
    for (int i =0; i < edsize; i++) {
      if (objects[i]->ismarkAllPoints() &&
          objects[i]->objectIs(wFront)){
        ObjectPlot * pold = objects[i];
        undo->undoAdd(Replace,pold,i);
        frontsChanged = true;
      }
    }
    break;
  case DefaultSize:
    for (int i =0; i < edsize; i++) {
      if (objects[i]->ismarkAllPoints()){
        ObjectPlot * pold = objects[i];
        undo->undoAdd(Replace,pold,i);
        frontsChanged = true;
      }
    }
    break;
  case DefaultSizeAll:
    for (int i =0; i < edsize; i++) {
      ObjectPlot * pold = objects[i];
      undo->undoAdd(Replace,pold,i);
      frontsChanged = true;
    }
    break;
  case HideAll:
    for (int i =0; i < edsize; i++) {
      ObjectPlot * pold = objects[i];
      undo->undoAdd(Replace,pold,i);
      frontsChanged = true;
    }
    break;
  case IncreaseSize:
    for (int i =0; i < edsize; i++) {
      ObjectPlot* op = objects[i];
      if (op->ismarkSomePoint()) {
        if (!op->objectIs(wSymbol))
          continue;
        // now check if previous operation was IncreaseSize and the
        //same symbols were affected
        bool saveThis = true;
        if (undoCurrent->iop == IncreaseSize){
          std::vector<saveObject>::iterator p2 = undoCurrent->saveobjects.begin();
          while (p2!=undoCurrent->saveobjects.end()){
            if (p2->place ==i){
              saveThis = false;
              break;
            }
            p2++;
          }
        }
        undo->undoAdd(Replace, op, i);
        if (saveThis)
          frontsChanged = true;
      }
    }
    break;
  case ChangeObjectType:
    for (int i =0; i < edsize; i++) {
      if (objects[i]->ismarkAllPoints()){
        ObjectPlot * pold = objects[i];
        undo->undoAdd(Replace,pold,i);
        frontsChanged = true;
      }
    }
    break;
  case RotateLine:
    for (int i =0; i < edsize; i++) {
      if (objects[i]->ismarkSomePoint() &&
          objects[i]->objectIs(wFront)){
        ObjectPlot * pold = objects[i];
        undo->undoAdd(Replace,pold,i);
        frontsChanged = true;
      }
    }
    break;
  case ResumeDrawing:
    for (int i =0; i < edsize; i++) {
      if (objects[i]->ismarkAllPoints()){
        ObjectPlot * pold = objects[i];
        undo->undoAdd(Replace,pold,i);
        frontsChanged = true;
      } else if (objects[i]->ismarkEndPoint() ||
          objects[i]->ismarkBeginPoint()){
        if (i==(edsize-1)){
          frontsChanged = false;
          break;
        } else{
          ObjectPlot * pold = objects[i];
          undo->undoAdd(Insert,pold,i);
          undo->undoAdd(Erase,0,edsize);
          frontsChanged = true;
          break;
        }
      }
    }
    break;
  case SplitFronts:
    for (int i =0; i < edsize; i++) {
      if (objects[i]->ismarkAllPoints() &&
          objects[i]->onLine(newx,newy)){
        ObjectPlot * pold = objects[i];
        undo->undoAdd(Replace,pold,i);
        frontsChanged = true;
        undo->undoAdd(Erase,0,edsize);
        break;
      }
    }
    break;
  case RotateObjects:
    for (int i =0; i < edsize; i++) {
      if (objects[i]->ismarkSomePoint()){
        if (!objects[i]->objectIs(wSymbol)) continue;
        // now check if previous operation was RotateObjects and the
        //same symbols were affected
        bool saveThis = true;
        if (undoCurrent->iop == RotateObjects){
          std::vector<saveObject>::iterator p2 = undoCurrent->saveobjects.begin();
          while (p2!=undoCurrent->saveobjects.end()){
            if (p2->place ==i){
              saveThis = false;
              break;
            }
            p2++;
          }
        }
        ObjectPlot * pold = objects[i];
        undo->undoAdd(Replace,pold,i);
        if (saveThis) frontsChanged = true;
      }
    }
    break;
  case ChangeText:
    for (int i =0; i < edsize; i++) {
      if (objects[i]->ismarkAllPoints()){
        ObjectPlot * pold = objects[i];
        undo->undoAdd(Replace,pold,i);
        frontsChanged = true;
      }
    }
    break;
  case HideBox:
    for (int i =0; i < edsize; i++) {
      if (objects[i]->ismarkAllPoints()){
        ObjectPlot * pold = objects[i];
        undo->undoAdd(Replace,pold,i);
        frontsChanged = true;
      }
    }
    break;
  case PasteObjects:
    for (unsigned int i =0; i< copyObjects.objects.size();i++)
      undo->undoAdd(Erase,0,edsize+i);
    frontsChanged = true;
    break;
  case JoinFronts:
    frontsChanged = false;
    break;
  case CleanUp:
    for (int i =0; i <edsize; i++) {
      if (objects[i]->objectIs(wFront)&&
          objects[i]->isSinglePoint()){
        ObjectPlot * pold = objects[i];
        undo->undoAdd(Insert,pold,i);
        frontsChanged = true;
      }
    }
    break;
  default:
    METLIBS_LOG_ERROR("Error in saveCurrentFronts");
  }
  return frontsChanged;
}



bool EditObjects::saveCurrentFronts(int nObjects, UndoFront *undo){
  //this function is called after nObjects have been added
  //for example when a file has been read
  bool frontsChanged = false;
  int edsize;
  edsize=objects.size()-nObjects;
  undo->oldArea = itsArea;
  undo->iop = ReadObjects;
  for (int i = edsize;i<edsize+nObjects;i++){
    undo->undoAdd(Erase,0,i);
    frontsChanged = true;
  }
  return frontsChanged;
}


bool EditObjects::changeCurrentFronts()
{
  METLIBS_LOG_SCOPE();
  if (METLIBS_LOG_DEBUG_ENABLED()) {
    const int edsize = objects.size();
    METLIBS_LOG_DEBUG("Number of objects " << edsize);
    switch (undoCurrent->iop){
    case AddPoint:
      METLIBS_LOG_DEBUG("Type of operation);AddPoint");
      break;
    case MoveMarkedPoints:
      METLIBS_LOG_DEBUG("Type of operation);MoveMarkedPoints");
      break;
    case DeleteMarkedPoints:
      METLIBS_LOG_DEBUG("Type of operation);DeleteMarkedPoints");
      break;
    case FlipObjects:
      METLIBS_LOG_DEBUG("Type of operation);FlipObjects");
      break;
    case IncreaseSize:
      METLIBS_LOG_DEBUG("Type of operation);IncreaseSize");
      break;
    case ChangeObjectType:
      METLIBS_LOG_DEBUG("Type of operation);ChangeObjectType");
      break;
    case RotateLine:
      METLIBS_LOG_DEBUG("Type of operation);RotateLine");
      break;
    case ResumeDrawing:
      METLIBS_LOG_DEBUG("Type of operation);ResumeDrawing");
      break;
    case SplitFronts:
      METLIBS_LOG_DEBUG("Type of operation);SplitFronts");
      break;
    case JoinFronts:
      METLIBS_LOG_DEBUG("Type of operation);JoinFronts");
      break;
    case ReadObjects:
      METLIBS_LOG_DEBUG("Type of operation);ReadObjects");
      break;
    case CleanUp:
      METLIBS_LOG_DEBUG("Type of operation);CleanUp");
      break;
    case PasteObjects:
      METLIBS_LOG_DEBUG("Type of operation);PasteObjects");
      break;
    default:
      METLIBS_LOG_DEBUG("Type of operation);Unknown");
    }
  }

  //reads the current undoBuffer, and updates fronts
  const Area oldarea= itsArea;
  switchProjection(undoCurrent->oldArea);
  std::vector<saveObject>::iterator p1 = undoCurrent->saveobjects.begin();
  int nFrontsDeleted = 0; // number of symbols deleted
  while (p1!=undoCurrent->saveobjects.end()){
    std::vector <ObjectPlot*>::iterator q;
    ObjectPlot * pold = 0;
    q = objects.begin()+p1->place;
    q-=nFrontsDeleted;

    const int test = p1->place-nFrontsDeleted;
    if (p1->todo!=Insert &&
        (test<0 || test>=(int)objects.size())){
      //HK ??? Something "strange" happened
      METLIBS_LOG_WARN("Warning! diEditObjects	::changeCurrentFronts "
      <<  "****************************************** "
      <<"undoCurrent->saveObjects.size()= "
      << undoCurrent->saveobjects.size());
      METLIBS_LOG_WARN("p1->place = " <<p1->place);
      METLIBS_LOG_WARN("nFrontsDeleted" << nFrontsDeleted);
      METLIBS_LOG_WARN("objects.size()=" <<
      objects.size());
      METLIBS_LOG_WARN("****************************************** ");
      return false;
    }

    switch (p1->todo){
    case Replace:
      //  METLIBS_LOG_DEBUG("Replace front ! ");
      //erase front at p->place
      pold = *q;
      q=objects.erase(q);
      objects.insert(q,p1->object);
      //now we have to keep the current front, for when we
      // want to redo  (replace also for redo)!
      p1->object = pold;
      break;
    case Erase:
      //  METLIBS_LOG_DEBUG("Erase front ! ");
      pold = *q;
      //erase front at p->place
      objects.erase(q);
      p1->object = pold;
      p1->todo=Insert;
      nFrontsDeleted++;
      break;
    case Insert:
      //  METLIBS_LOG_DEBUG("Insert front !");
      objects.insert(q,p1->object);
      p1->object=NULL;
      p1->todo=Erase;
      break;
    default:
      METLIBS_LOG_ERROR("Error in undofront ! ");
    }
    p1++;
  }

  // Changed/Added by ADC - 23.02.01
  if (!switchProjection(oldarea)) {
    // projection change not necessary - but still call updateObjects
    updateObjects();
  }
  setAllPassive();
  editNotMarked();
  checkJoinPoints();
  return true;
}

/************************************************************
 *  Methods for finding and marking join points              *
 ************************************************************/

bool EditObjects::findAllJoined(const float x,const float y,objectType wType)
{
  // recursive routine to find all joined fronts belonging to point x,y
  int frsize=objects.size();
  bool found = false;
  for (int i=0; i< frsize;i++){
    ObjectPlot * pfront = objects[i];
    if (!pfront->visible() || !pfront->objectIs(wType)) continue;
    if (pfront->getJoinedMarked()) continue;
    if (pfront->isJoinPoint(x,y)) {
      pfront->markAllPoints();
      pfront->setJoinedMarked(true);
      found = true;

      const std::vector<XY> xyjoin = pfront->getXYjoined();
      const int jsize = xyjoin.size();
      for (int j=0; j<jsize; j++)
        findAllJoined(xyjoin[j].x(), xyjoin[j].y(), wType);
    } else {
      pfront->setJoinedMarked(false);
    }
  }
  return found;
}


void EditObjects::findJoinedFronts(ObjectPlot * pfront,objectType wType)
{
  //routine to find fronts joined to pfront
  const std::vector<XY> xyjoin = pfront->getXYmarkedJoined();
  const int jsize = xyjoin.size();
  for (int j=0; j<jsize; j++)
    findAllJoined(xyjoin[j].x(), xyjoin[j].y(), wType);
}


bool EditObjects::findJoinedPoints(const float x,const float y, objectType wType)
{
  //returns true if on joined point
  int frsize=objects.size();
  bool found = false;
  for (int i=0; i< frsize ;i++){
    ObjectPlot * pfront = objects[i];
    if (!pfront->visible() || !pfront->objectIs(wType)) continue;
    pfront->setJoinedMarked(false);
    if (pfront->isJoinPoint(x,y)) found = true;
  }
  return found;
}

/************************************************************
 *  Methods for plotting                                    *
 ************************************************************/

void EditObjects::drawJoinPoints(DiGLPainter* gl)
{
  int n= objects.size();
  for (int i=0; i<n; i++){
    objects[i]->drawJoinPoints(gl);
  }
}


void EditObjects::setScaleToField(float s){
  int edsize = objects.size();
  for (int i=0; i<edsize; i++)
    if (objects[i]->objectIs(Border))
      objects[i]->setScaleToField(s);
}

/************************************************************
 *  Methods for reading and writing comments                *
 ************************************************************/

void EditObjects::putCommentStartLines(const std::string& name, const std::string& prefix, const std::string& lines)
{
  //return the startline of the comments file to read
  startlines = prefix + std::string(" ") + name;
  if (!itsTime.undef())
    diutil::appendText(startlines, itsTime.isoTime());
  startlines += "\n";
  itsComments+=
    "*************************************************\n";
  itsComments+=startlines;
  itsComments+=
    "*************************************************\n";
  itsComments+=lines;
  startlines = itsComments;
}

bool EditObjects::hasComments()
{
  return ( itsComments != startlines );
}

std::string EditObjects::getComments()
{
  commentsChanged = false;
  commentsSaved = true;

  return itsComments;
}

void EditObjects::putComments(const std::string & comments)
{
  itsComments = comments;
  commentsChanged = true;
  commentsSaved = false;
}


/************************************************************
 *  Methods for reading and writing labels                *
 ************************************************************/
void EditObjects::saveEditLabels(const PlotCommand_cpv& labels)
{
  itsLabels = labels;
  labelsSaved=false;
}

/************************************************************
 *  Methods for reading and writing text                    *
 ************************************************************/

std::string EditObjects::getMarkedText(){
  if (mapmode==draw_mode){
    int edsize = objects.size();
    for (int i =0; i< edsize;i++){
      if (objects[i]->isText() && objects[i]->ismarkAllPoints()){
        return objects[i]->getString();
      }
    }
  }
  return std::string();
}

Colour::ColourInfo EditObjects::getMarkedTextColour()
{
  Colour::ColourInfo cinfo;
  if (mapmode==draw_mode){
    int edsize = objects.size();
    for (int i =0; i< edsize;i++){
      if (objects[i]->isTextColored() && objects[i]->ismarkAllPoints()){
        return objects[i]->getObjectColor();
      }
    }
  }
  return cinfo;
}


Colour::ColourInfo EditObjects::getMarkedColour()
{
  Colour::ColourInfo cinfo;
  if (mapmode==draw_mode){
    int edsize = objects.size();
    for (int i =0; i< edsize;i++){
      if (objects[i]->isText() && objects[i]->ismarkAllPoints()){
        return objects[i]->getObjectColor();
      }
    }
  }
  return cinfo;
}

void EditObjects::changeMarkedText(const std::string & newText)
{
  if (mapmode==draw_mode){
    int edsize = objects.size();
    for (int i =0; i< edsize;i++){
      if (objects[i]->isText() && objects[i]->ismarkAllPoints()){
        objects[i]->setString(newText);
      }
    }
  }
}

void EditObjects::changeMarkedTextColour(const Colour::ColourInfo & newColour)
{
  if (mapmode==draw_mode){
    int edsize = objects.size();
    for (int i =0; i< edsize;i++){
      if (objects[i]->isTextColored() && objects[i]->ismarkAllPoints()){
        objects[i]->setObjectColor(newColour);
      }
    }
  }
}

void EditObjects::changeMarkedColour(const Colour::ColourInfo & newColour)
{
  if (mapmode==draw_mode){
    int edsize = objects.size();
    for (int i =0; i< edsize;i++){
      if (objects[i]->isText() && objects[i]->ismarkAllPoints()){
        objects[i]->setObjectColor(newColour);
      }
    }
  }
}

void EditObjects::getMarkedMultilineText(std::vector<std::string>& symbolText)
{
  METLIBS_LOG_SCOPE();
  if (mapmode==draw_mode){
    int edsize = objects.size();
    for (int i =0; i< edsize;i++){
      if (objects[i]->isTextMultiline() && objects[i]->ismarkAllPoints()){
        objects[i]->getMultilineText(symbolText);
        if (symbolText.size()) return;
      }
    }
  }
}

void EditObjects::getMarkedComplexText(std::vector<std::string>& symbolText, std::vector<std::string>& xText)
{
  METLIBS_LOG_SCOPE();
  std::vector <std::string> xString;
  if (mapmode==draw_mode){
    int edsize = objects.size();
    for (int i =0; i< edsize;i++){
      if (objects[i]->isComplex() && objects[i]->ismarkAllPoints()){
        objects[i]->getComplexText(symbolText,xText);
        if (symbolText.size()||xText.size()) return;
      }
    }
  }
}

void EditObjects::getMarkedComplexTextColored(std::vector<std::string> & symbolText, std::vector<std::string> & xText)
{
  METLIBS_LOG_SCOPE();
  if (mapmode==draw_mode){
    int edsize = objects.size();
    for (int i =0; i< edsize;i++){
      if (objects[i]->isTextColored() && objects[i]->ismarkAllPoints()){
        objects[i]->getComplexText(symbolText,xText);
        if (symbolText.size()&& xText.size()) return;
      }
    }
  }
}

void EditObjects::changeMarkedComplexTextColored(const std::vector<std::string>& symbolText, const std::vector<std::string>& xText)
{
  METLIBS_LOG_SCOPE();
  if (mapmode==draw_mode){
    int edsize = objects.size();
    for (int i =0; i< edsize;i++){
      if (objects[i]->isTextColored() && objects[i]->ismarkAllPoints()){
        objects[i]->changeComplexText(symbolText,xText);
      }
    }
  }
}

void EditObjects::changeMarkedMultilineText(const std::vector<std::string>& symbolText)
{
  METLIBS_LOG_SCOPE();
  if (mapmode==draw_mode){
    int edsize = objects.size();
    for (int i =0; i< edsize;i++){
      if (objects[i]->isTextMultiline() && objects[i]->ismarkAllPoints()){
        objects[i]->changeMultilineText(symbolText);
      }
    }
  }
}

void EditObjects::changeMarkedComplexText(const std::vector<std::string>& symbolText, const std::vector<std::string>& xText)
{
  METLIBS_LOG_SCOPE();
  if (mapmode==draw_mode){
    int edsize = objects.size();
    for (int i =0; i< edsize;i++){
      if (objects[i]->isComplex() && objects[i]->ismarkAllPoints()){
        objects[i]->changeComplexText(symbolText,xText);
      }
    }
  }
}



bool EditObjects::inTextMode()
{
  METLIBS_LOG_SCOPE();
  if (objectmode==symbol_drawing)
    return WeatherSymbol::isSimpleText(drawingtool);
  else
    return false;
}


bool EditObjects::inComplexTextMode()
{
  METLIBS_LOG_SCOPE(LOGVAL(drawingtool));
  if (objectmode==symbol_drawing)
    return WeatherSymbol::isComplexText(drawingtool);
  else
    return false;
}

bool EditObjects::inComplexTextColorMode()
{
  METLIBS_LOG_SCOPE(LOGVAL(drawingtool));
  if (objectmode==symbol_drawing)
    return WeatherSymbol::isComplexTextColor(drawingtool);
  else
    return false;
}

bool EditObjects::inEditTextMode()
{
  METLIBS_LOG_SCOPE(LOGVAL(drawingtool));
  if (objectmode==symbol_drawing)
    return WeatherSymbol::isTextEdit(drawingtool);
  else
    return false;
}

void EditObjects::initCurrentComplexText()
{
  METLIBS_LOG_SCOPE();
  if (objectmode==symbol_drawing)
    WeatherSymbol::initCurrentComplexText(drawingtool);
}
