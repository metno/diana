/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: qtEditTimeDialog.h,v 2.16 2007/06/19 06:28:24 lisbethb Exp $

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

#include "qtDianaProfetGUI.h"
#include "qtPaintToolBar.h"
#include <qstring.h>
#include <qapplication.h>
#include <qeventloop.h>
//Added by qt3to4:
#include <QCustomEvent>

DianaProfetGUI::DianaProfetGUI(Profet::ProfetController & pc,
PaintToolBar * ptb, GridAreaManager * gam, QWidget* parent) : 
QObject(), paintToolBar(ptb), areaManager(gam), 
Profet::ProfetGUI(pc), sessionDialog(parent), 
objectDialog(parent), objectFactory(){
#ifndef NOLOG4CXX
  logger = log4cxx::LoggerPtr(log4cxx::Logger::getLogger("diana.DianaProfetGUI"));
#endif
  user=getenv("USER");
  connectSignals();
  showPaintToolBar = true;
  showObjectDialog = false;
  emit setPaintMode(true);
  paintToolBar->enableButtons(PaintToolBar::SELECT_ONLY);
}

void DianaProfetGUI::connectSignals(){
  connect(&sessionDialog,SIGNAL(objectSelected(miString)),
      this,SLOT(objectSelected(miString)));
  connect(&sessionDialog,SIGNAL(paramAndTimeChanged(miString,miTime)),
      this,SLOT(paramAndTimeSelected(miString,miTime)));
  connect(paintToolBar,SIGNAL(paintModeChanged(GridAreaManager::PaintMode)),
      this,SLOT(paintModeChanged(GridAreaManager::PaintMode)));
  connect(&sessionDialog,SIGNAL(newObjectPerformed()),
      this,SLOT(createNewObject()));
  connect(&sessionDialog,SIGNAL(editObjectPerformed()),
      this,SLOT(editObject()));
  connect(&sessionDialog,SIGNAL(deleteObjectPerformed()),
      this,SLOT(deleteObject()));
  connect(&sessionDialog,SIGNAL(closePerformed()),
      this,SIGNAL(toggleProfetGui())); // only signaled when gui is visible
  connect(&sessionDialog,SIGNAL(sendMessage(const QString &)),
      this,SLOT(sendMessage(const QString &)));
  connect(&objectDialog,SIGNAL(saveObjectClicked()),
      this,SLOT(saveObject()));
  connect(&objectDialog,SIGNAL(cancelObjectDialog()),
      this,SLOT(cancelObjectDialog()));
  connect(&objectDialog,SIGNAL(baseObjectSelected(miString)),
      this,SLOT(baseObjectSelected(miString)));
  connect(&objectDialog,SIGNAL(dynamicGuiChanged()),
      this,SLOT(dynamicGuiChanged()));
}

DianaProfetGUI::~DianaProfetGUI(){ 
}

void DianaProfetGUI::setSessionInfo(fetModel model, vector<fetParameter> parameters,
      fetSession session){
  LOG4CXX_INFO(logger,"setSessionInfo");
  sessionDialog.setModel(model);
  sessionDialog.initializeTable(parameters,session);
}

void DianaProfetGUI::setBaseObjects(vector<fetBaseObject> obj){
  LOG4CXX_INFO(logger,"setBaseObjects");
  baseObjects = obj;
}

// THREAD SAFE!
void DianaProfetGUI::showMessage(const Profet::InstantMessage & msg){
  QApplication::postEvent(this, new Profet::MessageEvent(msg));//thread-safe
  QApplication::eventLoop()->wakeUp();//thread-safe
}

// THREAD SAFE!
void DianaProfetGUI::setUsers(vector<Profet::PodsUser> users){
  LOG4CXX_INFO(logger,"setUsers " << users.size());
  Profet::UserListEvent * cle = new Profet::UserListEvent();
  for(int i=0; i<users.size(); i++){
    cle->users.push_back(users[i]); // or just clients=users?
  }
  QApplication::postEvent(this, cle);//thread-safe
  QApplication::eventLoop()->wakeUp();//thread-safe
}

void DianaProfetGUI::customEvent(QCustomEvent * e){
  if(e->type() == Profet::MESSAGE_EVENT){
    Profet::MessageEvent * me = (Profet::MessageEvent*) e;
    sessionDialog.showMessage(me->message);
  }else if(e->type() == Profet::USER_LIST_EVENT){
    Profet::UserListEvent * cle = (Profet::UserListEvent*) e;
    sessionDialog.setUserList(cle->users);
  }else if(e->type() == Profet::UPDATE_MAP_EVENT){
    emit repaintMap(true);
  }else if(e->type() == Profet::OBJECT_UPDATE_EVENT){
    Profet::ObjectUpdateEvent * oue = (Profet::ObjectUpdateEvent*) e;
    objects = oue->objects;
    bool removeAreas = (areaManager->getAreaCount() > 0);
    areaManager->clear();  
    for(int i=0;i<objects.size();i++){
      areaManager->addArea(objects[i].id(),objects[i].polygon(),false);
    }
    sessionDialog.setObjectList(objects);
    //TODO Select previous object. And ignore replot signal!
//    if(objects.size()) objectSelected(objects[0].id()); // will update map
//    else if(removeAreas) updateMap(); // plot map without objects
  }else if(e->type() == Profet::SIGNATURE_UPDATE_EVENT){
    Profet::SignatureUpdateEvent * sue = (Profet::SignatureUpdateEvent*) e;
    sessionDialog.setObjectSignatures(sue->objects);
  }
  
}
// THREAD SAFE!
void DianaProfetGUI::setObjects(vector<fetObject> obj){
  Profet::ObjectUpdateEvent * oue = new Profet::ObjectUpdateEvent(obj);
  QApplication::postEvent(this, oue);//thread-safe
  QApplication::eventLoop()->wakeUp();//thread-safe
}


void DianaProfetGUI::setObjectSignatures( vector<fetObject::Signature> s){ 
  Profet::SignatureUpdateEvent * sue = new Profet::SignatureUpdateEvent(s);
  QApplication::postEvent(this, sue);//thread-safe
  QApplication::eventLoop()->wakeUp();//thread-safe
}

void DianaProfetGUI::baseObjectSelected(miString id){
  int i = getBaseObjectIndex(id);
  if(i != -1){
    LOG4CXX_INFO(logger,"base object selected " << baseObjects[i].name());
    objectDialog.addDymanicGui(objectFactory.getGuiComponents(baseObjects[i]));
    if(areaManager->isAreaSelected()){
      currentObject = objectFactory.makeObject(baseObjects[i],
          areaManager->getCurrentPolygon(),
          sessionDialog.getSelectedParameter(),
          sessionDialog.getSelectedTime(),
          objectDialog.getReason(),user);
      LOG4CXX_INFO(logger,"calling controller.objectChanged");
      controller.objectChanged(currentObject);
    }
  }
  else{
    LOG4CXX_ERROR(logger,"Selected base object not found");
  }
  if(areaManager->isAreaSelected()){
    objectDialog.setAreaStatus(ProfetObjectDialog::AREA_OK);
  }
  else{
    objectDialog.setAreaStatus(ProfetObjectDialog::AREA_NOT_SELECTED);
  }
}

void DianaProfetGUI::objectSelected(miString id){
  LOG4CXX_INFO(logger,"objectSelected "<<id);
  int i = getObjectIndex(id);
  if(i!=-1){ // object found with id
    sessionDialog.setCurrentObject(objects[i]);
    areaManager->setCurrentArea(id);
    updateMap();
  }
}

void DianaProfetGUI::saveObject(){
  // Validate current object
  if(!areaManager->isAreaSelected()){
    objectDialog.setAreaStatus(ProfetObjectDialog::AREA_NOT_VALID);
    LOG4CXX_WARN(logger,"Save Object: No Area Selected");
    return;
  }else if(!currentObject.exist()){
    LOG4CXX_ERROR(logger,"Save Object: currentObject does not exist");
    setObjectDialogVisible(false);
    return;
  }
  //Save current object
  if(objectDialog.showingNewObject()){
    areaManager->changeAreaId("newArea",currentObject.id());
  }
  currentObject.setReason(objectDialog.getReason());
  controller.saveObject(currentObject);
  currentObject=fetObject();
  setObjectDialogVisible(false);
}

void DianaProfetGUI::dynamicGuiChanged(){
  LOG4CXX_INFO(logger,"Dynamic GUI changed");
  // Check if current object is ok ...
  if(areaManager->isAreaSelected()){
    objectFactory.setGuiValues(currentObject,
        objectDialog.getCurrentGuiComponents());
    controller.objectChanged(currentObject);
  }
}

void DianaProfetGUI::sendMessage(const QString & m){
  Profet::InstantMessage message(miTime::nowTime(),0,user,"all",m.latin1());
  bool sent = controller.sendMessage(message);
  if(!sent) sessionDialog.showMessage(Profet::InstantMessage(miTime::nowTime(),0,
      "system","all","Error sending message"));
}

void DianaProfetGUI::paramAndTimeSelected(miString p, miTime t){
  LOG4CXX_INFO(logger,"paramAndTimeSelected "<<p<<", "<<t);
	controller.parameterAndTimeChanged(p,t);
}

void DianaProfetGUI::createNewObject(){
  LOG4CXX_INFO(logger,"createNewObject");
  bool ok = areaManager->addArea("newArea");
  if(!ok){
    LOG4CXX_WARN(logger,"Previous temp. area not removed!");
  }
  paintToolBar->enableButtons(PaintToolBar::PAINT_ONLY);
  objectDialog.setSession(getCurrentTime());
  objectDialog.setParameter(getCurrentParameter());
  objectDialog.setBaseObjects(baseObjects);
  objectDialog.newObjectMode();
  objectDialog.selectDefault(); // selects first base-object
  setObjectDialogVisible(true);
}

void DianaProfetGUI::editObject(){
  LOG4CXX_INFO(logger,"editObject");
  int index = getObjectIndex(sessionDialog.getSelectedObject());
  if(index!=-1){
    if(!objects[index].is_locked()){
    objectDialog.setSession(getCurrentTime());
    objectDialog.setParameter(getCurrentParameter());
    objectDialog.setBaseObjects(baseObjects);
    objectDialog.editObjectMode(objects[index],
        objectFactory.getGuiComponents(objects[index]));
      currentObject=objects[index];
      controller.openObject(currentObject);
    setObjectDialogVisible(true);
    
    paintToolBar->enableButtons(PaintToolBar::PAINT_AND_MODIFY);
    // Area always ok for a saved object(?)
    objectDialog.setAreaStatus(ProfetObjectDialog::AREA_OK);
    }
    else{
      LOG4CXX_WARN(logger,"editObject: object is locked ("<<
		    sessionDialog.getSelectedObject()<<")");
    }
  }
  else{
    LOG4CXX_ERROR(logger,"editObject: object not found ("<<
        sessionDialog.getSelectedObject()<<")");
  }
}

void DianaProfetGUI::deleteObject(){
  LOG4CXX_INFO(logger,"deleteObject");
  int index = getObjectIndex(sessionDialog.getSelectedObject());
  if(index!=-1){
    if(!objects[index].is_locked()){
      controller.deleteObject(sessionDialog.getSelectedObject());
    }
    else{
      LOG4CXX_WARN(logger,"deleteObject():  Locked object " <<
          sessionDialog.getSelectedObject() << " could not be deleted");
    }
  }
}

void DianaProfetGUI::hideProfetPerformed(){
  emit toggleProfetGui();
}

void DianaProfetGUI::showField(miString param, miTime time){
  LOG4CXX_INFO(logger,"show field "<<param<<" "<<time);
  if ( param != currentParam ){ // has parameter changed?
    LOG4CXX_INFO(logger,"show field parameter changed from"<<currentParam << " to " <<
		  param);
    miString plotString;
    
    // First, remove previous PROFET fieldPlot (if any)
    if ( currentParam.length() ){
      plotString = "REMOVE ";
      plotString += "FIELD ";
      plotString += "profet ";
      plotString += currentParam;
      emit showProfetField(plotString); //FieldManager->addField
    }
    
    // make plot string for new PROFET fieldPlot
    plotString = "FIELD ";
    plotString += "profet ";
    plotString += param;
    plotString += " overlay=1";
    LOG4CXX_INFO(logger,"showField: "<<plotString);
    emit showProfetField(plotString); //FieldManager->addField
  }
  
  currentParam = param;
  
  emit setTime(time);
//  emit repaintMap(false);
}


void DianaProfetGUI::setBaseProjection(Area a, int size_x, int size_y){
  if ( size_x==0 || size_y==0 || a.R().width()==0 ){
    LOG4CXX_ERROR(logger,"Unvalid base projection set");
  }
  else{
    objectFactory.initFactory(a,size_x,size_y);
    areaManager->setBaseProjection(a);
  }
}

void DianaProfetGUI::updateMap(){
  QApplication::postEvent(this, new QCustomEvent(Profet::UPDATE_MAP_EVENT));//thread-safe
  QApplication::eventLoop()->wakeUp();//thread-safe
}

miString DianaProfetGUI::getCurrentParameter(){
  return sessionDialog.getSelectedParameter();
}

miTime DianaProfetGUI::getCurrentTime(){
  return sessionDialog.getSelectedTime();
}

void DianaProfetGUI::paintModeChanged(GridAreaManager::PaintMode mode){
  areaManager->setPaintMode(mode);
}

void DianaProfetGUI::gridAreaChanged(){
  LOG4CXX_INFO(logger,"gridAreaChanged");
  miString currentId = areaManager->getCurrentId();
  if(currentId != "newArea" && currentId != sessionDialog.getSelectedObject()){
    int index = getObjectIndex(currentId);
    if(index!=-1) {
      bool found = sessionDialog.setCurrentObject(objects[index]);
    }
    return;
  }
  if(areaManager->isAreaSelected()){
    objectDialog.setAreaStatus(ProfetObjectDialog::AREA_OK);
    if(currentObject.exist()){
      objectFactory.setPolygon(currentObject, areaManager->getCurrentPolygon());
      objectFactory.setGuiValues(currentObject,objectDialog.getCurrentGuiComponents());
    }
    else {
      int i = getBaseObjectIndex(objectDialog.getSelectedBaseObject());
      if(i != -1){
        if(areaManager->isAreaSelected()){
          currentObject = objectFactory.makeObject(baseObjects[i],
              areaManager->getCurrentPolygon(),
              sessionDialog.getSelectedParameter(),
              sessionDialog.getSelectedTime(),
              objectDialog.getReason(),user);
        }
        else{
          LOG4CXX_WARN(logger,"gridAreaChanged: No Area Selected");
        }
      }
    }
   
    controller.objectChanged(currentObject);
    paintToolBar->enableButtons(PaintToolBar::PAINT_AND_MODIFY);
  }
  else{
    objectDialog.setAreaStatus(ProfetObjectDialog::AREA_NOT_SELECTED);
  }
}

void DianaProfetGUI::cancelObjectDialog(){
  // Current area added with createNewObject 
  if(areaManager->getCurrentId() == "newArea"){
    areaManager->removeCurrentArea();
  }
  setObjectDialogVisible(false);
  // AC
  controller.closeObject(currentObject);
  currentObject=fetObject();
}

void DianaProfetGUI::setPaintToolBarVisible(bool visible){
  showPaintToolBar = visible;
  emit setPaintMode(showPaintToolBar);
  if(showPaintToolBar) paintToolBar->show();
  else {
    paintToolBar->hide();
  }
}

void DianaProfetGUI::setObjectDialogVisible(bool visible){
  showObjectDialog = visible;
  sessionDialog.setEditable(!visible);
  if(showObjectDialog) {
    objectDialog.show();
  }
  else {
    paintToolBar->enableButtons(PaintToolBar::SELECT_ONLY);
    objectDialog.hide();
  }
}

bool DianaProfetGUI::isVisible(){
  return sessionDialog.isVisible();
}

void DianaProfetGUI::setVisible(bool visible){
  if(visible){
    setPaintToolBarVisible(showPaintToolBar);
    emit setPaintMode(showPaintToolBar);
    setObjectDialogVisible(showObjectDialog);
    sessionDialog.selectDefault();
    sessionDialog.show();
    emit repaintMap(false);
  }
  else{
    // First, remove previous PROFET fieldPlot (if any)
    if ( currentParam.length() )
      emit showProfetField("REMOVE FIELD profet " + currentParam);
    currentParam="";
    areaManager->clear();
    emit repaintMap(false);
    sessionDialog.hide();
    objectDialog.hide();
    paintToolBar->hide();
   
    emit setPaintMode(false);
  }
}

int DianaProfetGUI::getBaseObjectIndex(miString name){
  for(int i=0;i<baseObjects.size();i++)
    if(baseObjects[i].name() == name)
      return i;
  return -1;
}

int DianaProfetGUI::getObjectIndex(miString id){
  for(int i=0;i<objects.size();i++)
    if(objects[i].id() == id)
      return i;
  return -1;
}

void  DianaProfetGUI::setStatistics(map<miString,float> m)
{
  objectDialog.setStatistics(m);
}

void  DianaProfetGUI::setActivePoints(vector<Point>)
{

}

