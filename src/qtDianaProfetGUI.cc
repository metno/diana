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
#include <QCoreApplication>
#include <QMessageBox>

DianaProfetGUI::DianaProfetGUI(Profet::ProfetController & pc,
PaintToolBar * ptb, GridAreaManager * gam, QWidget* p) : 
QObject(), paintToolBar(ptb), areaManager(gam), 
Profet::ProfetGUI(pc),sessionDialog(p), 
objectDialog(p), objectFactory(),
userModel(p), sessionModel(p),
objectModel(p), tableModel(p), activeTimeSmooth(false)
{
  parent=p;
#ifndef NOLOG4CXX
  logger = log4cxx::LoggerPtr(log4cxx::Logger::getLogger("diana.DianaProfetGUI"));
#endif
  sessionDialog.setUserModel(&userModel);
  sessionDialog.setSessionModel(&sessionModel);
  sessionDialog.setObjectModel(&objectModel);
  sessionDialog.setTableModel(&tableModel);
  user=getenv("USER");
  connectSignals();
  showPaintToolBar = true;
  showObjectDialog = false;
  emit setPaintMode(true);
  paintToolBar->enableButtons(PaintToolBar::SELECT_ONLY);
}

void DianaProfetGUI::connectSignals(){
  connect(&sessionDialog,SIGNAL(objectSelected(const QModelIndex &)),
      this,SLOT(objectSelected(const QModelIndex &)));
  connect(&sessionDialog,SIGNAL(paramAndTimeChanged(const QModelIndex &)),
      this,SLOT(paramAndTimeSelected(const QModelIndex &)));
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
  connect(&sessionDialog,SIGNAL(sessionSelected(int)),
      this,SLOT(sessionSelected(int)));
  connect(&objectDialog,SIGNAL(saveObjectClicked()),
      this,SLOT(saveObject()));
  connect(&objectDialog,SIGNAL(cancelObjectDialog()),
      this,SLOT(cancelObjectDialog()));
  connect(&objectDialog,SIGNAL(timesmoothClicked()),
      this,SLOT(startTimesmooth())); 
  connect(&objectDialog,SIGNAL(baseObjectSelected(miString)),
      this,SLOT(baseObjectSelected(miString)));
  connect(&objectDialog,SIGNAL(dynamicGuiChanged()),
      this,SLOT(dynamicGuiChanged()));
}

DianaProfetGUI::~DianaProfetGUI(){ 
}


void DianaProfetGUI::setCurrentSession(const fetSession & session){
  //Changing current session must be done in QT event queue 
  //to be performed in the correct order...
  Profet::CurrentSessionEvent * cse = new Profet::CurrentSessionEvent();
  cse->refTime = session.referencetime();
  QCoreApplication::postEvent(this, cse);
  tableModel.initTable(session.times(),session.parameters());
  sessionDialog.selectDefault();
  currentSession=session;
}


void DianaProfetGUI::setBaseObjects(vector<fetBaseObject> obj){
  LOG4CXX_INFO(logger,"setBaseObjects");
  baseObjects = obj;
}

// THREAD SAFE!
void DianaProfetGUI::showMessage(const Profet::InstantMessage & msg){
  QCoreApplication::postEvent(this,new Profet::MessageEvent(msg));//thread-safe
}

void DianaProfetGUI::setSession(const fetSession & session, bool remove){
  Profet::SessionListEvent * sle = new Profet::SessionListEvent();
  sle->remove = remove;
  sle->session = session;
  QCoreApplication::postEvent(this, sle);//thread-safe
}

void DianaProfetGUI::setUser(const Profet::PodsUser & user){
  LOG4CXX_INFO(logger,"setUser " << user.name);
  Profet::UserListEvent * cle = new Profet::UserListEvent();
  cle->type = Profet::UserListEvent::SET_USER;
  cle->user = user;
  QCoreApplication::postEvent(this, cle);//thread-safe
}

void DianaProfetGUI::removeUser(const Profet::PodsUser & user){
  LOG4CXX_INFO(logger,"removeUser " << user.name);
  Profet::UserListEvent * cle = new Profet::UserListEvent();
  cle->type = Profet::UserListEvent::REMOVE_USER;
  cle->user = user;
  QCoreApplication::postEvent(this, cle);//thread-safe
}

// THREAD SAFE!
void DianaProfetGUI::setUsers(const vector<Profet::PodsUser> & users){
  LOG4CXX_INFO(logger,"setUsers " << users.size());
  Profet::UserListEvent * cle = new Profet::UserListEvent();
  cle->type = Profet::UserListEvent::REPLACE_LIST;
  cle->users = users;
  QCoreApplication::postEvent(this, cle);//thread-safe
}

void DianaProfetGUI::customEvent(QEvent * e){
  if(e->type() == Profet::MESSAGE_EVENT){
    Profet::MessageEvent * me = (Profet::MessageEvent*) e;
    if(me->message.type == Profet::InstantMessage::WARNING_MESSAGE){
      QString qs = me->message.message.cStr();
      QMessageBox::warning(0, tr("Profet Warning"),qs,
          QMessageBox::Ok,  QMessageBox::NoButton);
    }else {
      sessionDialog.showMessage(me->message);
    }
  }else if(e->type() == Profet::USER_LIST_EVENT){
    Profet::UserListEvent * cle = (Profet::UserListEvent*) e;
    if(cle->type == Profet::UserListEvent::REPLACE_LIST)
      userModel.setUsers(cle->users);
    else if(cle->type == Profet::UserListEvent::SET_USER)
      userModel.setUser(cle->user);
    else if(cle->type == Profet::UserListEvent::REMOVE_USER)
      userModel.removeUser(cle->user);
  }else if(e->type() == Profet::UPDATE_MAP_EVENT){
    emit repaintMap(true);
  }else if(e->type() == Profet::OBJECT_UPDATE_EVENT){
    Profet::ObjectUpdateEvent * oue = (Profet::ObjectUpdateEvent*) e;
    objectModel.setObjects(oue->objects);
    bool removeAreas = (areaManager->getAreaCount() > 0);
    areaManager->clear();
    int nObjects = oue->objects.size();
    for(int i=0;i<nObjects;i++){
      areaManager->addArea(oue->objects[i].id(),oue->objects[i].polygon(),false);
    }
  }else if(e->type() == Profet::SIGNATURE_UPDATE_EVENT){
    Profet::SignatureUpdateEvent * sue = (Profet::SignatureUpdateEvent*) e;
    tableModel.setObjectSignatures(sue->objects);
  }else if(e->type() == Profet::SESSION_LIST_EVENT){
    Profet::SessionListEvent * sle = (Profet::SessionListEvent*) e;
    if(sle->remove) sessionModel.removeSession(sle->session);
    else sessionModel.setSession(sle->session);
  }else if(e->type() == Profet::CURRENT_SESSION_UPDATE_EVENT){
    Profet::CurrentSessionEvent * cse = (Profet::CurrentSessionEvent*) e;
    sessionDialog.setCurrentSession(
        sessionModel.getIndexByRefTime(cse->refTime));
    // updates FieldDialog
    emit updateModelDefinitions();
  }
  
}
// THREAD SAFE!
void DianaProfetGUI::setObjects(vector<fetObject> obj){
  Profet::ObjectUpdateEvent * oue = new Profet::ObjectUpdateEvent(obj);
  QCoreApplication::postEvent(this, oue);//thread-safe
}

/**
 * Called by multiple threads
 */
void DianaProfetGUI::setObjectSignatures( vector<fetObject::Signature> s){ 
  Profet::SignatureUpdateEvent * sue = new Profet::SignatureUpdateEvent(s);
  QCoreApplication::postEvent(this, sue);//thread-safe
}

void DianaProfetGUI::baseObjectSelected(miString id){
  int i = getBaseObjectIndex(id);
  if(i != -1){
    LOG4CXX_INFO(logger,"base object selected " << baseObjects[i].name());
    objectDialog.addDymanicGui(objectFactory.getGuiComponents(baseObjects[i]));
    if(areaManager->isAreaSelected()){
      miTime refTime;
      try{
        fetSession s = sessionModel.getSession(
            sessionDialog.getCurrentSessionIndex());
        refTime = s.referencetime();
      }catch(InvalidIndexException & iie){
        cerr << "DianaProfetGUI::baseObjectSelected invalid session index" << endl;
      }
      
      // TODO: fetch parent from somewhere...
      miString parent_ = "";

      currentObject = objectFactory.makeObject(baseObjects[i],
          areaManager->getCurrentPolygon(),
          getCurrentParameter(), getCurrentTime(),
					       objectDialog.getReason(),user,refTime,parent_);
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

void DianaProfetGUI::objectSelected(const QModelIndex & index){
  LOG4CXX_INFO(logger,"objectSelected");
  try{
    fetObject fo = objectModel.getObject(sessionDialog.getCurrentObjectIndex());
// Lock is not distributed    
//    sessionDialog.lockedObjectSelected(fo.is_locked());
    areaManager->setCurrentArea(fo.id());
    updateMap();
  }catch(InvalidIndexException & iie){
    LOG4CXX_ERROR(logger,"editObject:" << iie.what());
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

void DianaProfetGUI::startTimesmooth()
{
  if(activeTimeSmooth) return;
  
  if(!currentObject.exist()){
      LOG4CXX_ERROR(logger,"startTimeSmooth Object: currentObject does not exist");
      return;
  }
  
  vector<miTime> tim;
  try{
    fetSession s = sessionModel.getSession( sessionDialog.getCurrentSessionIndex() );
    tim = s.times();
  }catch(InvalidIndexException & iie){
    cerr << "DianaProfetGUI::startTimesmooth invalid session index" << endl;
  }
  
  vector<fetObject::TimeValues> obj=controller.getTimeValues(currentObject.timeValues());
  
  if(obj.empty()) return;
  
  ProfetTimeSmoothDialog *timesmoothdialog= new ProfetTimeSmoothDialog(parent,obj,tim);
  
  connect(timesmoothdialog,SIGNAL(runObjects(vector<fetObject::TimeValues>)), 
      this,SLOT(processTimesmooth(vector<fetObject::TimeValues>)));
  
  connect(this,SIGNAL(timesmoothProcessed(miTime, miString)),
      timesmoothdialog,SLOT(processed(miTime, miString))); 
  
  connect(timesmoothdialog,SIGNAL(endTimesmooth(vector<fetObject::TimeValues>)), 
        this,SLOT(endTimesmooth(vector<fetObject::TimeValues>)));
    
  activeTimeSmooth=true;
  timesmoothdialog->show();
}

void DianaProfetGUI::processTimesmooth(vector<fetObject::TimeValues> tv)
{
  vector<fetObject> obj;
  set<miString>     deletion_ids;
  controller.getTimeValueObjects(obj,tv,deletion_ids);
  
  for(int i=0;i<tv.size();i++) {
    if(deletion_ids.count(tv[i].id)){
      controller.deleteObject(tv[i].id);
      emit timesmoothProcessed(tv[i].validTime,"");
    }
  }
  
  
   for(int i=0;i<obj.size();i++) { 
     miTime tim      = obj[i].validTime();
     miString obj_id = obj[i].id();

    
     if(objectFactory.processTimeValuesOnObject(obj[i])) {
       controller.saveObject(obj[i]);
       emit timesmoothProcessed(tim,obj[i].id());
     } else {
       emit timesmoothProcessed(tim,"");
       cerr << "could not process" << obj[i].id() << " at " << tim << endl;
     }
   }

}
void DianaProfetGUI::endTimesmooth(vector<fetObject::TimeValues> tv)
{
  activeTimeSmooth=false;
  controller.unlockObjectsByTimeValues(tv);
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

void DianaProfetGUI::sessionSelected(int index){
  try{
    controller.currentSessionChanged(sessionModel.getSession(index));
  }catch(InvalidIndexException & iie){
    cerr << "DianaProfetGUI::sessionSelected invalid index" << endl;
  }
}

void DianaProfetGUI::sendMessage(const QString & m){
  Profet::InstantMessage message(miTime::nowTime(),0,user,"all",m.latin1());
  bool sent = controller.sendMessage(message);
  if(!sent) sessionDialog.showMessage(Profet::InstantMessage(miTime::nowTime(),0,
      "system","all","Error sending message"));
}

void DianaProfetGUI::paramAndTimeSelected(const QModelIndex & index){
  tableModel.setLastSelectedIndex(index);
  bool tableInited = tableModel.inited();
  if(tableInited) {
    try{
      currentParam = tableModel.getParameter(index);
      currentTime = tableModel.getTime(index);
      controller.parameterAndTimeChanged(currentParam,currentTime);
    }catch(InvalidIndexException & iie){
      LOG4CXX_ERROR(logger,"Invalid time/param index");
    }
  }
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
  try{
//    controller.openObject(currentObject);
    fetObject fo = objectModel.getObject(sessionDialog.getCurrentObjectIndex());
// Lock info not distributed
//    if(!fo.is_locked()){
    objectDialog.setSession(getCurrentTime());
    objectDialog.setParameter(getCurrentParameter());
    objectDialog.setBaseObjects(baseObjects);
    objectDialog.editObjectMode(fo,objectFactory.getGuiComponents(fo));
    currentObject=fo;
    controller.openObject(currentObject);
    setObjectDialogVisible(true);
    paintToolBar->enableButtons(PaintToolBar::PAINT_AND_MODIFY);
    // Area always ok for a saved object(?)
    objectDialog.setAreaStatus(ProfetObjectDialog::AREA_OK);
  }catch(InvalidIndexException & iie){
    LOG4CXX_ERROR(logger,"editObject:" << iie.what());
  }catch(ObjectLockedException &){
    InstantMessage m(miTime::nowTime(), InstantMessage::WARNING_MESSAGE,
                "","","Unable to open object: locked by other user.");
    showMessage(m);
    //        return;
  }
}

void DianaProfetGUI::deleteObject(){
  LOG4CXX_INFO(logger,"deleteObject");
  try{
    fetObject fo = objectModel.getObject(sessionDialog.getCurrentObjectIndex());
    controller.deleteObject(fo.id());
  }catch(InvalidIndexException & iie){
    LOG4CXX_ERROR(logger,"deleteObject:" << iie.what());
  }
}

void DianaProfetGUI::hideProfetPerformed(){
  emit toggleProfetGui();
}

void DianaProfetGUI::showField(miString param, miTime time){
  LOG4CXX_INFO(logger,"show field "<<param<<" "<<time);
  if ( param != prevParam ){ // has parameter changed?
    LOG4CXX_INFO(logger,"show field parameter changed from "<< prevParam << " to " <<
		  param);
    miString plotString;
    
    // First, remove previous PROFET fieldPlot (if any)
    if ( prevParam.length() ){
      plotString = "REMOVE ";
      plotString += "FIELD ";
      plotString += "profet ";
      plotString += prevParam;
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
  
  prevParam = param;
  cerr << "*** DianaProfetGUI::showField setting time: " << time.isoTime() << endl;
  emit setTime(time);
//  emit repaintMap(false);
}


void DianaProfetGUI::setBaseProjection(Area a, int size_x, int size_y){
  if ( size_x==0 || size_y==0 || a.R().width()==0 ){
    LOG4CXX_ERROR(logger,"Unvalid base projection set:" << a);
  }
  else{
    objectFactory.initFactory(a,size_x,size_y);
    areaManager->setBaseProjection(a);
  }
}

void DianaProfetGUI::updateMap(){
  QCoreApplication::postEvent(this, new QEvent(
      QEvent::Type(Profet::UPDATE_MAP_EVENT)));//thread-safe
}
/**
 * Called by multiple threads
 */
miString DianaProfetGUI::getCurrentParameter(){
  return currentParam;
}
/**
 * Called by multiple threads
 */
miTime DianaProfetGUI::getCurrentTime(){
  return currentTime;
}

void DianaProfetGUI::paintModeChanged(GridAreaManager::PaintMode mode){
  areaManager->setPaintMode(mode);
}

void DianaProfetGUI::gridAreaChanged(){
  LOG4CXX_INFO(logger,"gridAreaChanged");
  miString currentId = areaManager->getCurrentId();
  if(currentId != "newArea" && currentId != currentObject.id()){ //use currentObject??
    QModelIndex modelIndex = objectModel.getIndexById(currentId);
    if(modelIndex.isValid()){
      sessionDialog.setSelectedObject(modelIndex);
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
          miTime refTime;
          try{
            fetSession s = sessionModel.getSession(
                sessionDialog.getCurrentSessionIndex());
            refTime = s.referencetime();
          }catch(InvalidIndexException & iie){
            cerr << "DianaProfetGUI::baseObjectSelected invalid session index" << endl;
          }
      	  // TODO: fetch parent from somewhere...
      	  miString parent_ = "";
          currentObject = objectFactory.makeObject(baseObjects[i],
						   areaManager->getCurrentPolygon(),
						   getCurrentParameter(), getCurrentTime(),
						   objectDialog.getReason(),user,
						   refTime,parent_);
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

void  DianaProfetGUI::setStatistics(map<miString,float> m){
  objectDialog.setStatistics(m);
}

void  DianaProfetGUI::setActivePoints(vector<Point>){

}

