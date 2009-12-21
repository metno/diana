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
#include <diEditObjects.h>
#include <diWeatherFront.h>
#include <diWeatherSymbol.h>
#include <diWeatherArea.h>
#include <math.h>

using namespace::miutil;

map<int,object_modes> EditObjects::objectModes;
map<int,combine_modes> EditObjects::combineModes;

EditObjects::EditObjects(){
#ifdef DEBUGPRINT
  cerr << "EditObjects constructor" << endl;
#endif
  //undo variables
  undoCurrent = new UndoFront( );
  undoHead = undoCurrent;
  undoTemp=0;

  init();

}

void EditObjects::init(){
  createobject= false;
  inDrawing= false;
  filename= miString();
  itsComments = miString();
  commentsChanged = false;
  commentsSaved = true;
  labelsSaved=true;
  prefix= miString();
  mapmode = normal_mode;
  clear();
}

void EditObjects::defineModes(map<int,object_modes> objModes,
    map<int,combine_modes> combModes){
  objectModes=objModes;
  combineModes=combModes;
}


void EditObjects::setEditMode(const mapMode mmode,
    const int emode,
    const miString etool){
  //called when new edit mode/tool selected in gui (EditDIalog)
#ifdef DEBUGPRINT
  cerr << "EditObjects::setEditMode" << endl;
#endif
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


void EditObjects::setMouseCoordinates(const float x,const float y){
  newx = x;
  newy = y;
}

void EditObjects::createNewObject(){
#ifdef DEBUGPRINT
  cerr << "EditObjects::createNewObject" << endl;
#endif
  createobject=true;
  inDrawing=true;
  if (mapmode==draw_mode){
    objectmode= objectModes[editmode];
  }  else if (mapmode==combine_mode){
    combinemode=combineModes[editmode];
  }
}


void EditObjects::editStayMarked(){
  int edsize=objects.size();
  for (int i =0; i < edsize; i++){
    // mark objects
    if  (objects[i]->ismarkSomePoint())
      objects[i]->setStayMarked(true);
  }
}

void EditObjects::editNotMarked(){

  int edsize=objects.size();
  for (int i =0; i < edsize; i++){
    // unmark objects
    objects[i]->setStayMarked(false);
    objects[i]->unmarkAllPoints();
    objects[i]->setInBoundBox(false);
  }
  setAllPassive();
}


bool EditObjects::editResumeDrawing(const float x, const float y) {
#ifdef DEBUGPRINT
  cerr <<"EditObjects::Edit resume drawing" << endl;
  cerr << "mapmode = " << mapmode << "editmode = " << editmode << endl;
#endif

  bool ok = false;

  //resumeDrawing of marked front or insert point

  vector <ObjectPlot*>::iterator p = objects.begin();
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



bool EditObjects::editDeleteMarkedPoints(){

  bool ok=false;

  if (mapmode==draw_mode){

    vector <ObjectPlot*>::iterator p1 = objects.begin();
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
    vector <ObjectPlot*>::iterator q1 = objects.begin();
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



bool EditObjects::editAddPoint(const float x, const float y){
#ifdef DEBUGPRINT
  cerr << "EditObjects::editAddPoint" << endl;
  cerr << "mapmode = " << mapmode << "editmode = " << editmode << endl;
  cerr << "drawingtool =" << drawingtool << endl;
#endif


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


bool EditObjects::editMergeFronts(bool mergeAll){
  //merge two fronts of same type into one
#ifdef DEBUGPRINT
  cerr << "+++EditObjects::editMergeFronts" << endl;
  if (mergeAll) cerr << "mergeAll = true" << endl;
#endif
  bool frontsChanged= false;
  vector <ObjectPlot*>::iterator p = objects.begin();
  int pcount=0;//for undo
  for (;p!=objects.end();p++){
    ObjectPlot * pfront = *p;
    if (pfront->oktoMerge(mergeAll,pfront->getIndex())){
      int end = pfront->endPoint();
      float xend[2],yend[2],xnew,ynew;
      vector<float> x=pfront->getX();
      vector<float> y=pfront->getY();
      xend[0] = x[0];  yend[0] =y[0];
      xend[1] = x[end]; yend[1] =y[end];
      vector <ObjectPlot*>::iterator q = objects.begin();
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
                  vector<float> x=pfront->getX();
                  vector<float> y=pfront->getY();
                  xend[0] = x[0];  yend[0] =y[0];
                  xend[1] = x[end]; yend[1] =y[end];
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



bool
EditObjects::editJoinFronts(bool joinAll,bool movePoints,bool joinOnLine){
  //input parameters
  //joinAll = true ->all fronts are joined
  //        = false->only marked or active fronts are joined
  //movePoints = true->points moved to join
  //           = false->points not moved to join
  //joinOnLine = true->join front to line not just end points
  //           = false->join front only to join- and endpoints...
#ifdef DEBUGPRINT
  cerr << "+++EditObjects::editJoinFronts" << endl;
  if (joinAll) cerr << "joinAll = true" << endl;
  if (movePoints) cerr << "movePoints = true" << endl;
  if (joinOnLine) cerr << "joinOnLine = true" << endl;
#endif
  vector <ObjectPlot*>::iterator p = objects.begin();
  for (;p!=objects.end();p++){
    ObjectPlot * pfront = *p;
    if (pfront->oktoJoin(joinAll)){
      bool endPointJoined[2] = {false,false};
      int end = pfront->endPoint();
      float xend[2],yend[2],xnew,ynew;
      vector<float> x=pfront->getX();
      vector<float> y=pfront->getY();
      xend[0] = x[0];  yend[0] =y[0];
      xend[1] = x[end]; yend[1] =y[end];
      vector <ObjectPlot*>::iterator q = objects.begin();
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
        vector <ObjectPlot*>::iterator q = objects.begin();
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


bool EditObjects::editMoveMarkedPoints(const float x, const float y){
  bool changed=false;
  int edsize = objects.size();
  for (int i =0; i < edsize; i++) {
    if (objects[i]->moveMarkedPoints(x,y))
      changed = true;
  }
  return changed;
}

bool EditObjects::editRotateLine(const float x, const float y){
  bool changed = false;
  int edsize = objects.size();
  for (int i =0; i < edsize; i++){
    if (objects[i]->rotateLine(x,y))
      changed= true;
  }
  return changed;
}


void EditObjects::editCopyObjects(){
#ifdef DEBUGPRINT
  cerr << "EditObjects::editCopyObjects !" << endl;
#endif
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
  copyObjects.changeProjection(geoArea);
}



void EditObjects::editPasteObjects(){
#ifdef DEBUGPRINT
  cerr << "EditObjects::editPasteObjects !" << endl;
#endif
  float diffx,diffy;
  int csize = copyObjects.objects.size();

  if (csize){
    unmarkAllPoints();
    copyObjects.changeProjection(itsArea);
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
      ObjectPlot * paste;
      ObjectPlot * copy;
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

void EditObjects::editFlipObjects(){
  int edsize = objects.size();
  for (int i =0; i< edsize;i++){
    if (objects[i]->ismarkAllPoints())
      objects[i]->flip();
  }
}

void EditObjects::editUnJoinPoints(){

  int edsize = objects.size();
  for (int i=0; i<edsize; i++){
    //unjoin all points on marked fronts
    if  (objects[i]->ismarkAllPoints())
      objects[i]->unjoinAllPoints();
  }
}


bool EditObjects::editChangeObjectType(int val){
#ifdef DEBUGPRINT
  cerr <<"EditObjects::editChangeObjectType" << endl;
#endif
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

void EditObjects::editTestFront(){
#ifdef DEBUGPRINT
  cerr << "EditObjects::editTestFront()"<< endl;
#endif
  vector <ObjectPlot*>::iterator p = objects.begin();
  while (p!=objects.end())
  {
    ObjectPlot * pobject = *p;
    if( pobject->ismarkAllPoints()){
      pobject->test = !pobject->test;
    }
    p++;
  }
}


bool EditObjects::editSplitFront(const float x, const float y){
  bool ok=false;
  vector <ObjectPlot*>::iterator p = objects.begin();
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


void EditObjects::unmarkAllPoints(){
#ifdef DEBUGPRINT
  cerr << "EditObjects::unmarkAllPoints " << endl;
#endif
  //unmark all points
  int edsize=objects.size();
  for (int i = 0;i<edsize;i++){
    objects[i]->setJoinedMarked(false);
    objects[i]->unmarkAllPoints();
  }
}


bool EditObjects::editCheckPosition(const float x, const float y){
#ifdef DEBUGPRINT
  cerr << "EditObjects::editCheckPosition" << endl;
#endif
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
    if (foundJoined) found = true;
  }

  return changed;
}

bool EditObjects::editIncreaseSize(float val){
  bool doCombine=false;
  int edsize = objects.size();
  for (int i = 0; i< edsize;i++){
    if (objects[i]->ismarkAllPoints()){
      objects[i]->increaseSize(val);
      if (objects[i]->objectIs(Border))
        doCombine= true;
    }
  }
  return doCombine;
}

void EditObjects::editRotateObjects(float val){
  //only rotates complex objects !!!
  int edsize = objects.size();
  for (int i = 0; i< edsize;i++){
    if (objects[i]->isComplex() && objects[i]->ismarkAllPoints()){
      objects[i]->rotateObject(val);
    }
  }
}


void EditObjects::editHideBox(){
  int edsize = objects.size();
  for (int i = 0; i< edsize;i++){
    if (objects[i]->isComplex() && objects[i]->ismarkAllPoints()){
      objects[i]->hideBox();
    }
  }
}

void EditObjects::editDefaultSize(){
  int edsize = objects.size();
  for (int i =0; i< edsize;i++){
    if (objects[i]->ismarkAllPoints()){
      objects[i]->setDefaultSize();
    }
  }
}

void EditObjects::editDefaultSizeAll(){
  int edsize = objects.size();
  for (int i =0; i< edsize;i++){
    objects[i]->setDefaultSize();
  }
}


void EditObjects::editHideAll(){
  int edsize = objects.size();
  for (int i =0; i< edsize;i++){
    objects[i]->setVisible(false);
    objects[i]->unmarkAllPoints();
  }

}


void EditObjects::editUnHideAll(){
  //cerr << "editUnHideAll" << endl;
  int edsize = objects.size();
  for (int i =0; i< edsize;i++){
    objects[i]->setVisible(true);
  }
}

void EditObjects::editHideCombineObjects(miString region){
#ifdef DEBUGPRINT
  cerr << "editHideCombineObject from " << region << endl;
#endif
  int edsize = objects.size();
  for (int i =0; i< edsize;i++){
    if(objects[i]->getRegion() != region)
      objects[i]->setVisible(false);
  }
}


void EditObjects::editHideCombineObjects(int ir ){
#ifdef DEBUGPRINT
  cerr << "editHideCombineObject from " << ir << endl;
#endif
  miString region = WeatherSymbol::getAllRegions(ir);
  if (!region.empty()){
    int edsize = objects.size();
    for (int i =0; i< edsize;i++){
      if(objects[i]->getRegion() != region)
        objects[i]->setVisible(false);
    }
  }
}


/************************************************************
 *  Various methods                                         *
 ************************************************************/

bool EditObjects::setRubber(bool rubber, const float x, const float y){
#ifdef DEBUGPRINT
  //cerr <<"EditObjects::setRubber called" << endl;
#endif
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


void EditObjects::setAllPassive(){
#ifdef DEBUGPRINT
  cerr << "EditObjects:.setAllPassive" << endl;
#endif
  //Sets current status of the objects to passive
  int edsize=objects.size();
  for (int i=0; i< edsize;i++){
    objects[i]->setState(ObjectPlot::passive);
  }
  createobject= false;
  inDrawing=false;

}


void EditObjects::cleanUp(){
  //remove any front with only one point
  vector <ObjectPlot*>::iterator p1 = objects.begin();
  while (p1!=objects.end()){
    ObjectPlot * pobject = *p1;
    if(pobject->objectIs(wFront) && pobject->isSinglePoint()){
      p1 = removeObject(p1); //remove objects
      delete pobject;
    } else p1++;
  }
}



void EditObjects::checkJoinPoints(){
  //cerr <<"EditObjects::checkJoinPoints !" << endl;
  int edsize=objects.size();
  // check all fronts for join points...
  for (int i =0; i <edsize; i++) {
    if (!objects[i]->objectIs(wFront)) continue;
    vector <float> xjoin=objects[i]->getXjoined();
    vector <float> yjoin=objects[i]->getYjoined();
    unsigned int jsize=xjoin.size();
    if (yjoin.size()<jsize) jsize=yjoin.size();
    //loop over all the join rectangles
    for (unsigned int k = 0;k<jsize;k++){
      float x1=xjoin[k];
      float y1=yjoin[k];
      int njoin=0;
      for (int j =0; j < edsize; j++)
      {
        if (!objects[i]->objectIs(wFront)) continue;
        if (i!=j && objects[j]->isJoinPoint(x1,y1))
          njoin=njoin+1;
      }
      if (njoin < 1)
        objects[i]->unJoinPoint(x1,y1);
    }
  }
}

void EditObjects::changeDefaultSize(){
  //set symbol default size to size of last read object !
  vector <ObjectPlot*>::iterator p = objects.begin();
  while (p!=objects.end()){
    ObjectPlot * pobject = *p;
    if (pobject->objectIs(wSymbol)) pobject->changeDefaultSize();
    p++;
  }
}


/************************************************************
 *  Undo and redo methods                                   *
 ************************************************************/

void EditObjects::newUndoCurrent(UndoFront * undoNew){
  //a new UndoFront is created and becomes undoCurrent
  // last points back to the previous undoCurrent
  //next points to NULL
#ifdef DEBUGPRINT
  cerr << "newUndoCurrent called" << endl;
#endif

  if (undoNew == NULL){
    cerr << "EditObjects	::newUndoCurrent" << endl;
    cerr << " Warning ! undoNew = 0 !" << endl;
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
  if (undoCurrent->Next !=0) cerr << "***********undoCurrent->Next !=0\n";
}



bool EditObjects::redofront(){
#ifdef DEBUGPRINT
  cerr << "EditObjects::redofront" << endl;
#endif
  if (undoCurrent->Next!=NULL){
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
    cerr << "Not possible to redo any more!" << endl;
    return false;
  }
}

bool EditObjects::undofront(){
#ifdef DEBUGPRINT
  cerr << "EditObjects::undofront" << endl;
#endif
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
    cerr << "Not possible to undo any more ! " << endl;
    return false;
  }
}

void EditObjects::undofrontClear(){
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


bool EditObjects::saveCurrentFronts(operation iop, UndoFront * undo){

#ifdef DEBUGPRINT
  cerr << "EditObjects	::saveCurrentFronts" << endl;
#endif

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
      if (objects[i]->ismarkSomePoint()){
        if (!objects[i]->objectIs(wSymbol)) continue;
        // now check if previous operation was IncreaseSize and the
        //same symbols were affected
        bool saveThis = true;
        if (undoCurrent->iop == IncreaseSize){
          vector<saveObject>::iterator p2 = undoCurrent->saveobjects.begin();
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
          vector<saveObject>::iterator p2 = undoCurrent->saveobjects.begin();
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
    cerr << "Error in saveCurrentFronts" << endl;
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


bool EditObjects::changeCurrentFronts(){
#ifdef DEBUGPRINT
  cerr << "changeCurrentFronts" << endl;
  int edsize = objects.size();
  cerr << "Number of objects " << edsize << endl;
  switch (undoCurrent->iop){
  case AddPoint:
    cerr << "Type of operation;AddPoint " << endl;
    break;
  case MoveMarkedPoints:
    cerr << "Type of operation;MoveMarkedPoints " << endl;
    break;
  case DeleteMarkedPoints:
    cerr << "Type of operation;DeleteMarkedPoints " << endl;
    break;
  case FlipObjects:
    cerr << "Type of operation;FlipObjects " << endl;
    break;
  case IncreaseSize:
    cerr << "Type of operation;IncreaseSize " << endl;
    break;
  case ChangeObjectType:
    cerr << "Type of operation;ChangeObjectType " << endl;
    break;
  case RotateLine:
    cerr << "Type of operation;RotateLine " << endl;
    break;
  case ResumeDrawing:
    cerr << "Type of operation;ResumeDrawing " << endl;
    break;
  case SplitFronts:
    cerr << "Type of operation;SplitFronts " << endl;
    break;
  case JoinFronts:
    cerr << "Type of operation;JoinFronts " << endl;
    break;
  case ReadObjects:
    cerr << "Type of operation;ReadObjects " << endl;
    break;
  case CleanUp:
    cerr << "Type of operation;CleanUp " << endl;
    break;
  case PasteObjects:
    cerr << "Type of operation;PasteObjects " << endl;
    break;
  default:
    cerr << "Type of operation;Unknown " << endl;
  }
#endif
  //reads the current undoBuffer, and updates fronts
  Area oldarea= itsArea;
  changeProjection(undoCurrent->oldArea);
  vector<saveObject>::iterator p1 = undoCurrent->saveobjects.begin();
  int nFrontsDeleted = 0; // number of symbols deleted
  while (p1!=undoCurrent->saveobjects.end()){
    vector <ObjectPlot*>::iterator q;
    ObjectPlot * pold;
    q = objects.begin()+p1->place;
    q-=nFrontsDeleted;

    unsigned int test = p1->place-nFrontsDeleted;
    if (p1->todo!=Insert &&
        (test<0 || test>=objects.size())){
      //HK ??? Something "strange" happened
      cerr << "Warning! diEditObjects	::changeCurrentFronts " << endl
      <<  "****************************************** "<< endl
      <<"undoCurrent->saveObjects.size()= "
      << undoCurrent->saveobjects.size() << endl;
      cerr << "p1->place = " <<p1->place << endl;
      cerr << "nFrontsDeleted" << nFrontsDeleted << endl;
      cerr << "objects.size()=" <<
      objects.size() << endl;
      cerr << "****************************************** "<<endl;
      return false;
    }

    switch (p1->todo){
    case Replace:
      //  cerr << "Replace front ! " << endl;
      //erase front at p->place
      pold = *q;
      q=objects.erase(q);
      objects.insert(q,p1->object);
      //now we have to keep the current front, for when we
      // want to redo  (replace also for redo)!
      p1->object = pold;
      break;
    case Erase:
      //  cerr << "Erase front ! " << endl;
      pold = *q;
      //erase front at p->place
      objects.erase(q);
      p1->object = pold;
      p1->todo=Insert;
      nFrontsDeleted++;
      break;
    case Insert:
      //  cerr << "Insert front !" << endl;
      objects.insert(q,p1->object);
      p1->object=NULL;
      p1->todo=Erase;
      break;
    default:
      cerr << "Error in undofront ! " << endl;
    }
    p1++;
  }

  // Changed/Added by ADC - 23.02.01
  if (!changeProjection(oldarea)){
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

bool EditObjects::findAllJoined(const float x,const float y,objectType wType){
  //cerr << "find all joined" << endl;
  // recursive routine to find all joined fronts belonging to point x,y
  int frsize=objects.size();
  bool found = false;
  for (int i=0; i< frsize;i++){
    ObjectPlot * pfront = objects[i];
    if (!pfront->visible() || !pfront->objectIs(wType)) continue;
    if (pfront->getJoinedMarked()) continue;
    if (pfront->isJoinPoint(x,y)){
      pfront->markAllPoints();
      pfront->setJoinedMarked(true);
      found = true;
      vector <float> xjoin=pfront->getXjoined();
      vector <float> yjoin=pfront->getYjoined();
      unsigned int jsize=xjoin.size();
      if (yjoin.size()<jsize) jsize=yjoin.size();
      for (unsigned int j=0; j<jsize; j++){
        findAllJoined(xjoin[j],yjoin[j],wType);
      }
    }
    else pfront->setJoinedMarked(false);
  }
  return found;
}

void EditObjects::findJoinedFronts(ObjectPlot * pfront,objectType wType){
  //cerr << "find JoinedFronts" << endl;
  //routine to find fronts joined to pfront
  vector <float> xjoin=pfront->getXmarkedJoined();
  vector <float> yjoin=pfront->getYmarkedJoined();
  unsigned int jsize=xjoin.size();
  if (yjoin.size()<jsize) jsize=yjoin.size();
  for (unsigned int j=0; j<jsize; j++){
    findAllJoined(xjoin[j],yjoin[j],wType);
  }
}


bool EditObjects::findJoinedPoints(const float x,const float y,
    objectType wType){
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

void EditObjects::drawJoinPoints(){
  int n= objects.size();
  for (int i=0; i<n; i++){
    objects[i]->drawJoinPoints();
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

void EditObjects::putCommentStartLines(miString name,miString prefix){
  //return the startline of the comments file to read
  miString startline = prefix + miString(" ") + name +
  miString(" ") + itsTime.isoTime()+ miString("\n");
  itsComments+=
    "*************************************************\n";
  itsComments+=startline;
  itsComments+=
    "*************************************************\n";
}

miString EditObjects::getComments(){
  //return the comments
  commentsChanged = false;
  //HK ???
  commentsSaved = true;
  return itsComments;
}

void EditObjects::putComments(const miString & comments){
  itsComments = comments;
  commentsChanged = true;
  commentsSaved = false;
}


/************************************************************
 *  Methods for reading and writing labels                *
 ************************************************************/
void EditObjects::saveEditLabels(vector <miString> labels){
  itsLabels=labels;
  labelsSaved=false;
}

/************************************************************
 *  Methods for reading and writing text                    *
 ************************************************************/

miString EditObjects::getMarkedText(){
  if (mapmode==draw_mode){
    int edsize = objects.size();
    for (int i =0; i< edsize;i++){
      if (objects[i]->isText() && objects[i]->ismarkAllPoints()){
        return objects[i]->getString();
      }
    }

  }
  return miString();
}


Colour::ColourInfo EditObjects::getMarkedColour(){
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

void EditObjects::changeMarkedText(const miString & newText){
  if (mapmode==draw_mode){
    int edsize = objects.size();
    for (int i =0; i< edsize;i++){
      if (objects[i]->isText() && objects[i]->ismarkAllPoints()){
        objects[i]->setString(newText);
      }
    }
  }
}

void EditObjects::changeMarkedColour(const Colour::ColourInfo & newColour){
  if (mapmode==draw_mode){
    int edsize = objects.size();
    for (int i =0; i< edsize;i++){
      if (objects[i]->isText() && objects[i]->ismarkAllPoints()){
        objects[i]->setObjectColor(newColour);
      }
    }
  }
}


void EditObjects::getMarkedComplexText(vector <miString> & symbolText, vector <miString> & xText){
#ifdef DEBUGPRINT
  cerr << "EditObjects::getMarkedComplex called" << endl;
#endif
  vector <miString> xString;
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


void EditObjects::changeMarkedComplexText(const vector <miString> & symbolText, const vector <miString> & xText){
#ifdef DEBUGPRINT
  cerr << "EditObjects::changeMarkedComplex called" << endl;
#endif
  if (mapmode==draw_mode){
    int edsize = objects.size();
    for (int i =0; i< edsize;i++){
      if (objects[i]->isComplex() && objects[i]->ismarkAllPoints()){
        objects[i]->changeComplexText(symbolText,xText);
      }
    }
  }
}



bool EditObjects::inTextMode(){
  if (objectmode==symbol_drawing)
    return WeatherSymbol::isSimpleText(drawingtool);
  else
    return false;
}


bool EditObjects::inComplexTextMode(){
  if (objectmode==symbol_drawing)
    return WeatherSymbol::isComplexText(drawingtool);
  else
    return false;
}

void EditObjects::initCurrentComplexText(){
  if (objectmode==symbol_drawing)
    WeatherSymbol::initCurrentComplexText(drawingtool);
}



/************************************************************/








