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
#include "qtProfetEvents.h"
#include "qtProfetWaitDialog.h"
#include <qstring.h>
#include <QCoreApplication>
#include <QMessageBox>
#include <QApplication>


DianaProfetGUI::DianaProfetGUI(Profet::ProfetController & pc,
PaintToolBar * ptb, GridAreaManager * gam, QWidget* p) :
QObject(), paintToolBar(ptb), areaManager(gam),
Profet::ProfetGUI(pc),sessionDialog(p),
viewObjectDialog(p,ProfetObjectDialog::VIEW_OBJECT_MODE),
editObjectDialog(p), objectFactory(),
userModel(p), sessionModel(p),
objectModel(p), tableModel(p), activeTimeSmooth(false),
enableNewbutton_(true), enableModifyButtons_(false),
enableTable_(true), overviewactive(false), ignoreSynchProblems(false)
{
  parent=p;
#ifndef NOLOG4CXX
  logger = log4cxx::LoggerPtr(log4cxx::Logger::getLogger("diana.DianaProfetGUI"));
#endif
  sessionDialog.setUserModel(&userModel);
  sessionDialog.setSessionModel(&sessionModel);
  sessionDialog.setObjectModel(&objectModel);
  sessionDialog.setTableModel(&tableModel);
  connectSignals();
  showPaintToolBar = true;
  showEditObjectDialog = false;
  emit setPaintMode(true);
  paintToolBar->enableButtons(PaintToolBar::SELECT_ONLY);
}

void DianaProfetGUI::connectSignals(){
  connect(&sessionDialog,SIGNAL(objectSelected(const QModelIndex &)),
      this,SLOT(objectSelected(const QModelIndex &)));
  connect(&sessionDialog,SIGNAL(paramAndTimeChanged(const QModelIndex &)),
      this,SLOT(paramAndTimeSelected(const QModelIndex &)));
  connect(&sessionDialog,SIGNAL(showObjectOverview(const QList<QModelIndex> &)),
	  this,SLOT(showObjectOverview(const QList<QModelIndex> &)));

  connect(paintToolBar,SIGNAL(paintModeChanged(GridAreaManager::PaintMode)),
      this,SLOT(paintModeChanged(GridAreaManager::PaintMode)));
  connect(paintToolBar,SIGNAL(undoPressed()),
      this,SLOT(undoCurrentArea()));
  connect(paintToolBar,SIGNAL(redoPressed()),
      this,SLOT(redoCurrentArea()));
  connect(&sessionDialog,SIGNAL(viewObjectToggled(bool)),
      this,SLOT(toggleViewObject(bool)));
  connect(&sessionDialog,SIGNAL(newObjectPerformed()),
      this,SLOT(createNewObject()));
  connect(&sessionDialog,SIGNAL(editObjectPerformed()),
      this,SLOT(editObject()));
  connect(&sessionDialog,SIGNAL(startTimesmooth()),
        this,SLOT(startTimesmooth()));
  connect(&sessionDialog,SIGNAL(deleteObjectPerformed()),
      this,SLOT(deleteObject()));
  connect(&sessionDialog,SIGNAL(closePerformed()),
      this,SIGNAL(toggleProfetGui())); // only signaled when gui is visible 
  connect(&sessionDialog,SIGNAL(forcedClosePerformed(bool)),
      this,SIGNAL(forceDisconnect(bool)));
  connect(&sessionDialog,SIGNAL(sendMessage(const QString &)),
      this,SLOT(sendMessage(const QString &)));
  connect(&sessionDialog,SIGNAL(sessionSelected(int)),
      this,SLOT(sessionSelected(int)));
  connect(&sessionDialog, SIGNAL(doReconnect()),
	  this,SLOT(doReconnect()));
//  connect(&sessionDialog, SIGNAL(updateActionPerformed()),
//	  this,SLOT(doUpdate()));
  
  connect(&viewObjectDialog,SIGNAL(cancelObjectDialog()),
      &sessionDialog,SLOT(hideViewObjectDialog()));
  
  connect(&editObjectDialog,SIGNAL(saveObjectClicked()),
      this,SLOT(saveObject()));
  connect(&editObjectDialog,SIGNAL(cancelObjectDialog()),
      this,SLOT(cancelEditObjectDialog()));
  connect(&editObjectDialog,SIGNAL(baseObjectSelected(miString)),
      this,SLOT(baseObjectSelected(miString)));
  connect(&editObjectDialog,SIGNAL(dynamicGuiChanged()),
      this,SLOT(dynamicGuiChanged()));

  connect(&sessionModel,SIGNAL(dataChanged(const QModelIndex &, const QModelIndex & )),
	  this,SLOT(sessionModified(const QModelIndex & , const QModelIndex & )));

  connect(&editObjectDialog,SIGNAL(copyPolygon(miString,miString,bool)),
      this,SLOT(copyPolygon(miString,miString,bool)));
  connect(&editObjectDialog,SIGNAL(selectPolygon(miString)),
      this,SLOT(selectPolygon(miString)));
  connect(&editObjectDialog,SIGNAL(requestPolygonList()),
        this,SLOT(requestPolygonList()));
}

DianaProfetGUI::~DianaProfetGUI(){
}

void DianaProfetGUI::showObjectOverview(const QList<QModelIndex> & selected){

  QList<QModelIndex>::const_iterator i = selected.begin();
  set<miString> parameters;
  set<miTime> times;
  for (; i != selected.end(); i++){
    miTime t = tableModel.getTime(*i);
    miString p = tableModel.getParameter(*i);
    parameters.insert(p);
    times.insert(t);
  }
  miString param;
  miTime time;
  if ( parameters.size() == 1 && times.size() != 1 ){
    param = (*parameters.begin());
  } else if ( parameters.size() != 1 && times.size() == 1){
    time = (*times.begin());
  }
  toggleObjectOverview(true,param,time);

  enableObjectButtons(enableNewbutton_,enableModifyButtons_,enableTable_);
  emit repaintMap(true);
}

// void DianaProfetGUI::showObjectOverview(int row, int col){
//   miString p;
//   miTime t;
//   if ( row >= 0 ){
//     p = getCurrentParameter();
//   }
//   if ( col >= 0 ){
//     t = getCurrentTime();
//   }
//   toggleObjectOverview(true,p,t);
// }


void DianaProfetGUI::toggleObjectOverview(bool turnon, miString par, miTime time){

  if ( turnon ){
    vector<fetObject> objects = controller.getOverviewObjects(par,time);
    cerr << "Got " << objects.size() << " objects for par:" << par << endl;

    areaManager->clearTemporaryAreas();
    for ( int i=0; i<objects.size(); i++ ){
      Colour colour = Colour(128, 128, 128, 100);
      if ( objects[i].parameter() == "MSLP" ){
	colour = Colour(0,0,180,100);
      } else if ( objects[i].parameter() == "T.2M" ){
	colour = Colour(180,0,0,100);
      } else if ( objects[i].parameter() == "VIND.10M" ){
	colour = Colour(0,180,0,100);
      } else if ( objects[i].parameter() == "TOTALT.SKYDEKKE" ){
	colour = Colour(180,180,0,100);
      } else if ( objects[i].parameter() == "FOG-INDEX" ){
	colour = Colour(128,128,128,100);
      } else if ( objects[i].parameter() == "THUNDER-INDEX" ){
	colour = Colour(0,180,180,100);
      } else if ( objects[i].parameter() == "NEDBØR.1T.PROFF" ){
	colour = Colour(180,0,180,100);
      } else if ( objects[i].parameter() == "Significant_Wave_Height" ){
	colour = Colour(128,128,70,100);
      }

      areaManager->addTemporaryArea(objects[i].id(),objects[i].polygon(),colour);
    }
  } else {
    areaManager->clearTemporaryAreas();
  }
  overviewactive = turnon;
}



void DianaProfetGUI::sessionModified(const QModelIndex & topLeft, const QModelIndex & bottomRight)
{
  if ( topLeft == bottomRight ){
    fetSession s = sessionModel.getSession(topLeft);
    if ( s == currentSession ){
      cerr << "CURRENTSESSION CHANGED!!!!!" << endl;
      // current session has been modified
      currentSession = s;
      enableObjectButtons(enableNewbutton_,enableModifyButtons_,enableTable_);
    }
  }
}


fetSession DianaProfetGUI::getCurrentSession(){
  return currentSession;
}

void DianaProfetGUI::enableObjectButtons(bool enableNewbutton,
					 bool enableModifyButtons,
					 bool enableTable)
{
//   cerr << "DianaProfetGUI::enableObjectButtons (" << enableNewbutton
//        << "," << enableModifyButtons << "," << enableTable << ")" << endl;

  fetSession s = getCurrentSession();
  bool isopen = ( !s.referencetime().undef() &&
		  s.editstatus() == fetSession::editable &&
		  !overviewactive);

  sessionDialog.enableObjectButtons(enableNewbutton && isopen,
				    enableModifyButtons && isopen,
				    enableTable);

  enableNewbutton_ = enableNewbutton;
  enableModifyButtons_ = enableModifyButtons;
  enableTable_ = enableTable;
}

void DianaProfetGUI::setCurrentParam(const miString & p){
  currentParamTimeMutex.lock();
  currentParam = p;
  currentParamTimeMutex.unlock();
}

void DianaProfetGUI::setCurrentTime(const miTime & t){
  currentParamTimeMutex.lock();
  currentTime = t;
  currentParamTimeMutex.unlock();
}

void DianaProfetGUI::resetStatus(){
  userModel.clearModel();
  sessionModel.clearModel();
  objectModel.clearModel();
  tableModel.clearModel();
}

void DianaProfetGUI::setCurrentSession(const fetSession & session){
  //cerr << "DianaProfetGUI::setCurrentSession:" << session << endl;

  //Changing current session must be done in QT event queue
  //to be performed in the correct order...
  Profet::CurrentSessionEvent * cse = new Profet::CurrentSessionEvent();
  cse->refTime = session.referencetime();
  QCoreApplication::postEvent(this, cse);

  currentSession=session;
  tableModel.setCurrentSessionRefTime(currentSession.referencetime());
  tableModel.initTable(session.times(),session.parameters());
  sessionDialog.selectDefault();
  enableObjectButtons(true,false,true);
  QCoreApplication::flush();
}


void DianaProfetGUI::setBaseObjects(vector<fetBaseObject> obj){
  LOG4CXX_DEBUG(logger,"setBaseObjects");
  baseObjects = obj;
}

// THREAD SAFE!,
void DianaProfetGUI::showMessage(const Profet::InstantMessage & msg){
  QCoreApplication::postEvent(&sessionDialog,new Profet::MessageEvent(msg));//thread-safe
  QCoreApplication::flush();
}

void DianaProfetGUI::setSession(const fetSession & session, bool remove){
  Profet::SessionListEvent * sle = new Profet::SessionListEvent();
  sle->remove = remove;
  sle->session = session;
  QCoreApplication::postEvent(&sessionModel, sle);//thread-safe
  QCoreApplication::flush();
}

void DianaProfetGUI::setUser(const Profet::PodsUser & user){
  LOG4CXX_DEBUG(logger,"setUser " << user.name);
  Profet::UserListEvent * cle1 = new Profet::UserListEvent();
  cle1->type = Profet::UserListEvent::SET_USER;
  cle1->user = user;
  QCoreApplication::postEvent(this, cle1);//thread-safe
  QCoreApplication::flush();
}

void DianaProfetGUI::removeUser(const Profet::PodsUser & user){
  LOG4CXX_DEBUG(logger,"removeUser " << user.name);
  Profet::UserListEvent * cle = new Profet::UserListEvent();
  cle->type = Profet::UserListEvent::REMOVE_USER;
  cle->user = user;
  QCoreApplication::postEvent(this, cle);//thread-safe
  QCoreApplication::flush();
}

// THREAD SAFE!
void DianaProfetGUI::setUsers(const vector<Profet::PodsUser> & users){
  LOG4CXX_DEBUG(logger,"setUsers " << users.size());
  Profet::UserListEvent * cle = new Profet::UserListEvent();
  cle->type = Profet::UserListEvent::REPLACE_LIST;
  cle->users = users;
  QCoreApplication::postEvent(this, cle);//thread-safe
  QCoreApplication::flush();
}

void DianaProfetGUI::customEvent(QEvent * e){
  if(e->type() == Profet::UPDATE_MAP_EVENT){
    emit repaintMap(true);
  }else if(e->type() == Profet::OBJECT_LIST_UPDATE_EVENT){
    Profet::ObjectListUpdateEvent * oue = (Profet::ObjectListUpdateEvent*) e;
    objectModel.setObjects(oue->objects);
    bool removeAreas = (areaManager->getAreaCount() > 0);
    areaManager->clear();
    int nObjects = oue->objects.size();
    for(int i=0;i<nObjects;i++){
      areaManager->addArea(oue->objects[i].id(),oue->objects[i].polygon(),false);
    }
  }else if(e->type() == Profet::OBJECT_UPDATE_EVENT){
    Profet::ObjectUpdateEvent * oue = (Profet::ObjectUpdateEvent*) e;
    if(oue->remove) {
      bool r = objectModel.removeObject(oue->object.id());
      if(r) areaManager->removeArea(oue->object.id());
    }
    else {
      objectModel.setObject(oue->object);
      areaManager->updateArea(oue->object.id(),oue->object.polygon());
    }
  }else if(e->type() == Profet::CURRENT_SESSION_UPDATE_EVENT){
    Profet::CurrentSessionEvent * cse = (Profet::CurrentSessionEvent*) e;
    sessionDialog.setCurrentSession(
        sessionModel.getIndexByRefTime(cse->refTime));
    // updates FieldDialog
    emit updateModelDefinitions();
  }else if(e->type() == Profet::USER_LIST_EVENT){
    Profet::UserListEvent * cle = (Profet::UserListEvent*) e;
    if(cle->type == Profet::UserListEvent::REPLACE_LIST) {
      userModel.setUsers(cle->users);
    } else if(cle->type == Profet::UserListEvent::SET_USER) {
      PodsUser podsUser = userModel.setUser(cle->user);
      tableModel.setUserLocation(podsUser);
    } else if(cle->type == Profet::UserListEvent::REMOVE_USER) {
      userModel.removeUser(cle->user);
      tableModel.removeUserLocation(cle->user);
    }
  }

}
// THREAD SAFE!
void DianaProfetGUI::updateObjects(const vector<fetObject> & objects){
  Profet::ObjectListUpdateEvent * oue = new Profet::ObjectListUpdateEvent(objects);
  QCoreApplication::postEvent(this, oue);//thread-safe
  QCoreApplication::flush();
  cerr << "DianaProfetGUI:: updateObjects" << endl;
}

void DianaProfetGUI:: updateObject(const fetObject & object, bool remove){
  Profet::ObjectUpdateEvent * oue = new Profet::ObjectUpdateEvent(object,remove);
  QCoreApplication::postEvent(this, oue);//thread-safe
  QCoreApplication::flush();
  cerr << "DianaProfetGUI:: updateObject" << endl;
}

/**
 * Called by multiple threads
 */
void DianaProfetGUI::updateObjectSignatures(const vector<fetObject::Signature> & s){
  Profet::SignatureListUpdateEvent * sue = new Profet::SignatureListUpdateEvent(s);
  QCoreApplication::postEvent(&tableModel, sue);//thread-safe
  QCoreApplication::flush();
}

void DianaProfetGUI::updateObjectSignature(
    const fetObject::Signature & s, bool remove){
  Profet::SignatureUpdateEvent * sue = new Profet::SignatureUpdateEvent(s,remove);
  QCoreApplication::postEvent(&tableModel, sue);//thread-safe
  QCoreApplication::flush();
}

void DianaProfetGUI::baseObjectSelected(miString id){
  int i = getBaseObjectIndex(id);
  if(i != -1){
    LOG4CXX_WARN(logger,"base object selected " << baseObjects[i].name());
    editObjectDialog.addDymanicGui(objectFactory.getGuiComponents(baseObjects[i]));
    if(areaManager->isAreaSelected()){
      miTime refTime;
      try{
        fetSession s = getCurrentSession();
        refTime = s.referencetime();
      }catch(InvalidIndexException & iie){
        cerr << "DianaProfetGUI::baseObjectSelected invalid session index" << endl;
      }

      // TODO: fetch parent from somewhere...
      miString parent_ = "";
      miString username = controller.getCurrentUser().name;
      currentObjectMutex.lock();
      currentObject = objectFactory.makeObject(baseObjects[i],
          areaManager->getCurrentPolygon(),
          getCurrentParameter(), getCurrentTime(),
					       editObjectDialog.getReason(),username,refTime,parent_);
      currentObjectMutex.unlock();
      LOG4CXX_DEBUG(logger,"calling controller.objectChanged");
      controller.objectChanged(currentObject);
    }
  }
  else{
    LOG4CXX_ERROR(logger,"Selected base object not found");
  }
  if(areaManager->isAreaSelected()){
    editObjectDialog.setAreaStatus(ProfetObjectDialog::AREA_OK);
  }
  else{
    editObjectDialog.setAreaStatus(ProfetObjectDialog::AREA_NOT_SELECTED);
  }
}

void DianaProfetGUI::objectSelected(const QModelIndex & index){
  LOG4CXX_DEBUG(logger,"objectSelected");
  try{
    fetObject fo = objectModel.getObject(sessionDialog.getCurrentObjectIndex());
    areaManager->setCurrentArea(fo.id());
    viewObjectDialog.showObject(fo,objectFactory.getGuiComponents(fo));
    updateMap();
  }catch(InvalidIndexException & iie){
    LOG4CXX_ERROR(logger,"editObject:" << iie.what());
    return;
  }
  enableObjectButtons(true,true,true);
}

void DianaProfetGUI::saveObject(){
  // Validate current object
  if(!areaManager->isAreaSelected()){
    editObjectDialog.setAreaStatus(ProfetObjectDialog::AREA_NOT_VALID);
    LOG4CXX_WARN(logger,"Save Object: No Area Selected");
    return;
  }
  currentObjectMutex.lock();
  bool coExist = currentObject.exist();
  currentObjectMutex.unlock();
  if(!coExist){
    LOG4CXX_ERROR(logger,"Save Object: currentObject does not exist");
    setEditObjectDialogVisible(false);
    return;
  }
  //Save current object
  currentObjectMutex.lock();
  fetObject safeCopy = currentObject;
  currentObjectMutex.unlock();

  if(editObjectDialog.showingNewObject()){
    areaManager->changeAreaId("newArea",safeCopy.id());
  }
  currentObject.setReason(editObjectDialog.getReason());
  try{
    controller.saveObject(safeCopy);
    currentObjectMutex.lock();
    currentObject=fetObject();
    currentObjectMutex.unlock();
    setEditObjectDialogVisible(false);
    enableObjectButtons(true,false,true);
  }catch(Profet::ServerException & se){
    // reset id in areaManager
    if(editObjectDialog.showingNewObject()){
      areaManager->changeAreaId(safeCopy.id(),"newArea");
    }
    handleServerException(se);
  }
}

void DianaProfetGUI::handleServerException(Profet::ServerException & se){
  if (se.getType() == Profet::ServerException::OUT_OF_SYNC && ignoreSynchProblems)
    return;
  if (se.isDisconnectRecommanded()) {
    int i = QMessageBox::warning(0,"Disconnect from Profet?",
        se.getHtmlMessage(true).c_str(), QMessageBox::Ok, QMessageBox::Ignore);
    if (i == QMessageBox::Ok) emit forceDisconnect(false);
    else if(Profet::ServerException::OUT_OF_SYNC) ignoreSynchProblems = true;
  }
  QMessageBox::critical(0,QString("Profet Warning"),se.getHtmlMessage(true).c_str());
}

void DianaProfetGUI::startTimesmooth()
{
  if(activeTimeSmooth) return;

  miString id_;
  try{
     fetObject fo = objectModel.getObject(sessionDialog.getCurrentObjectIndex());
     id_=fo.id();
  }catch(InvalidIndexException & iie){
     LOG4CXX_ERROR(logger,"DianaProfetGUI::startTimesmooth :" << iie.what());
     return;
  }

  vector<miTime> tim;
  try{
    fetSession s = getCurrentSession();
    tim = s.times();
  }catch(InvalidIndexException & iie){
    cerr << "DianaProfetGUI::startTimesmooth invalid session index" << endl;
    return;
  }
  vector<fetObject::TimeValues> obj;
  try {
    obj=controller.getTimeValues(id_);
  } catch (Profet::ServerException & se){
    handleServerException(se);
    return;
  }

  if(obj.empty()) return;

  ProfetTimeSmoothDialog *timesmoothdialog= new ProfetTimeSmoothDialog(parent,obj,tim);

  connect(timesmoothdialog,SIGNAL(runObjects(vector<fetObject::TimeValues>)),
      this,SLOT(processTimesmooth(vector<fetObject::TimeValues>)));

  connect(this,SIGNAL(timesmoothProcessed(miTime, miString)),
      timesmoothdialog,SLOT(processed(miTime, miString)));

  connect(timesmoothdialog,SIGNAL(endTimesmooth(vector<fetObject::TimeValues>)),
        this,SLOT(endTimesmooth(vector<fetObject::TimeValues>)));

  activeTimeSmooth=true;
  enableObjectButtons(false,false,false);
}

void DianaProfetGUI::processTimesmooth(vector<fetObject::TimeValues> tv)
{
  vector<fetObject> obj;

  currentObjectMutex.lock();
  if(currentObject.exists())
      obj.push_back(currentObject);
  currentObjectMutex.unlock();

  set<miString>     deletion_ids;
  controller.getTimeValueObjects(obj,tv,deletion_ids);
  // deleting objects without effect
  try{
    for(int i=0;i<tv.size();i++) {
      if(deletion_ids.count(tv[i].id)){
        controller.deleteObject(tv[i].id);
        emit timesmoothProcessed(tv[i].validTime,"");
      }
    }
  }catch(Profet::ServerException & se){
    cerr << "DianaProfetGUI::processTimesmooth:  Failed to delete " <<
      "depricated object. (Not significant)" << endl;
  }


  for(int i=0;i<obj.size();i++) {
    miTime tim      = obj[i].validTime();
    miString obj_id = obj[i].id();
    try {
      if(objectFactory.processTimeValuesOnObject(obj[i])) {
        controller.saveObject(obj[i],true);
        emit timesmoothProcessed(tim,obj[i].id());
      } else {
        emit timesmoothProcessed(tim,"");
        cerr << "could not process" << obj[i].id() << " at " << tim << endl;
      }
    } catch (Profet::ServerException & se) {
      handleServerException(se);
    }
  }

}
void DianaProfetGUI::endTimesmooth(vector<fetObject::TimeValues> tv)
{
  activeTimeSmooth=false;
  try{
    controller.unlockObjectsByTimeValues(tv);
  }catch(Profet::ServerException & se){
    handleServerException(se);
  }
  enableObjectButtons(true,true,true);
}



void DianaProfetGUI::copyPolygon(miString fromPoly,miString toPoly,bool move)
{
  try{
    controller.copyPolygon(fromPoly,toPoly,move);
  }catch(Profet::ServerException & se){
    handleServerException(se);
  }
}


void DianaProfetGUI::selectPolygon(miString polyname)
{
  fetPolygon fpoly;
  try{
    fpoly=controller.getPolygon(polyname);
  }catch(Profet::ServerException & se){
    handleServerException(se);
    return;
  }
  if(areaManager) {
    miString id = "newArea";
    currentObjectMutex.lock();
    if(currentObject.exists()) {
      id = currentObject.id();
      // If new object but reselected polygon
      if(areaManager->getCurrentId() == "newArea")
        areaManager->changeAreaId("newArea",id);
    }
    currentObject.setPolygon(fpoly.polygon());
    currentObjectMutex.unlock();
    areaManager->addArea(id,fpoly.polygon(),true);
    gridAreaChanged();
  }
}

void DianaProfetGUI::requestPolygonList()
{
  vector <miString> polynames;
  try{
    polynames=controller.getPolygonIndex();
  }catch(Profet::ServerException & se){
    handleServerException(se);
    return;
  }

  if(polynames.size())
    editObjectDialog.startBookmarkDialog(polynames);


}


void DianaProfetGUI::dynamicGuiChanged(){
  LOG4CXX_DEBUG(logger,"Dynamic GUI changed");
  // Check if current object is ok ...
  if(areaManager->isAreaSelected()){
    currentObjectMutex.lock();
    objectFactory.setGuiValues(currentObject,
        editObjectDialog.getCurrentGuiComponents());
    // safe copy needed to unlock mutex before calling controller
    // ... that in turn can call another mutex.lock
    fetObject safeCopy = currentObject;
    currentObjectMutex.unlock();
    controller.objectChanged(safeCopy);
  }
}

void DianaProfetGUI::sessionSelected(int index){
  cerr << "DianaProfetGUI::sessionSelected:" << index << endl;
  try{
    controller.currentSessionChanged(sessionModel.getSession(index));
  }catch(Profet::ServerException & se){
    handleServerException(se);
    return;
  }catch(InvalidIndexException & iie){
    cerr << "DianaProfetGUI::sessionSelected invalid index" << endl;
    return;
  }
  //enableObjectButtons(true,false,true);
}

void DianaProfetGUI::sendMessage(const QString & m){
  miString username = controller.getCurrentUser().name;
  Profet::InstantMessage message(miTime::nowTime(),0,username,"all",m.latin1());
  try{
    controller.sendMessage(message);
  }catch (Profet::ServerException & se) {
    handleServerException(se);
  }
}

void DianaProfetGUI::paramAndTimeSelected(const QModelIndex & index){
  toggleObjectOverview(false,miString(),miTime());
  bool newbutton = true;
  bool modbutton = false;
  bool thetable  = true;
  enableObjectButtons(newbutton,modbutton,thetable);

  tableModel.setLastSelectedIndex(index);
  //check if parameters and times are set in model
  bool tableInited = tableModel.inited();
  if(tableInited) {
    try{
      setCurrentParam(tableModel.getParameter(index));
      setCurrentTime(tableModel.getTime(index));
      controller.parameterAndTimeChanged(
          getCurrentParameter(),getCurrentTime());
    }catch(Profet::ServerException & se){
      handleServerException(se);
    }catch(InvalidIndexException & iie){
      LOG4CXX_ERROR(logger,"Invalid time/param index");
    }
  }
}

void DianaProfetGUI::createNewObject(){
  LOG4CXX_DEBUG(logger,"createNewObject");
  bool ok = areaManager->addArea("newArea");
  if(!ok){
    LOG4CXX_WARN(logger,"Previous temp. area not removed!");
  }
  paintToolBar->enableButtons(PaintToolBar::PAINT_ONLY);
  editObjectDialog.setSession(getCurrentTime());
  editObjectDialog.setParameter(getCurrentParameter());
  editObjectDialog.setBaseObjects(baseObjects);
  editObjectDialog.newObjectMode();
  currentObject = fetObject(); // new id
  // select first base object
  // currentObject is reset
  editObjectDialog.selectDefault();
  setEditObjectDialogVisible(true);
  enableObjectButtons(false,false,false);
}

void DianaProfetGUI::toggleViewObject(bool show) 
{
  setViewObjectDialogVisible(show);
}

void DianaProfetGUI::editObject(){
  LOG4CXX_DEBUG(logger,"editObject");
  string error = "";
  try{
    fetObject fo = objectModel.getObject(sessionDialog.getCurrentObjectIndex());
    editObjectDialog.setSession(getCurrentTime());
    editObjectDialog.setParameter(getCurrentParameter());
    editObjectDialog.setBaseObjects(baseObjects);
    editObjectDialog.editObjectMode(fo,objectFactory.getGuiComponents(fo));
    currentObjectMutex.lock();
    currentObject=fo;
    currentObjectMutex.unlock();
    controller.openObject(fo);
    setEditObjectDialogVisible(true);
    enableObjectButtons(false,false,false);
    paintToolBar->enableButtons(PaintToolBar::PAINT_AND_MODIFY);
    // Area always ok for a saved object
    editObjectDialog.setAreaStatus(ProfetObjectDialog::AREA_OK);
  }catch(Profet::ServerException & se){
    handleServerException(se);
  }catch(InvalidIndexException & iie){
    error = "Unable to find selected object.";
  }
  if(!error.empty()){
    InstantMessage m(miTime::nowTime(), InstantMessage::WARNING_MESSAGE,
                "Edit Object Failed","",error);
    showMessage(m);
  }
}

void DianaProfetGUI::deleteObject(){
  LOG4CXX_DEBUG(logger,"deleteObject");
  enableObjectButtons(true,false,true);
  try{
    fetObject fo = objectModel.getObject(sessionDialog.getCurrentObjectIndex());
    controller.deleteObject(fo.id());
  }catch(Profet::ServerException & se){
    handleServerException(se);
  }catch(InvalidIndexException & iie){
    InstantMessage m(miTime::nowTime(), InstantMessage::WARNING_MESSAGE,
                "Delete Object Failed","","Unable to find selected object.");
    showMessage(m);
  }
}

void DianaProfetGUI::hideProfetPerformed(){
  emit toggleProfetGui();
}

void DianaProfetGUI::doReconnect()
{
  Profet::DataManagerType preferredType = Profet::DISTRIBUTED_MANAGER;
  Profet::PodsUser user = controller.getCurrentUser();
  miString password = controller.getPassword();

  QApplication::setOverrideCursor( Qt::WaitCursor );

  controller.disconnect();

  ProfetWaitDialog * wait = new ProfetWaitDialog(&sessionDialog,3000,200);
  if ( !wait->exec())
    return;

  try {
    Profet::DataManagerType dmt = controller.connect(user,preferredType,password, false);
    QApplication::restoreOverrideCursor();
    if(dmt != preferredType)
      QMessageBox::warning(0,"Running disconnected mode",
			   "Distributed field editing system is not available.");
  } catch(Profet::ServerException & se) {
    controller.disconnect();
    QMessageBox::warning(0,"Error connecting",se.getHtmlMessage(false).c_str());
    QApplication::restoreOverrideCursor();
  }
}
/*
void DianaProfetGUI::doUpdate()
{
  cerr << "doUpdate" << endl;
}
*/

void DianaProfetGUI::showField(const miTime & reftime, const miString & param, const miTime & time){
  LOG4CXX_DEBUG(logger,"show field "<<param<<" "<<time);

  //send time(s) to TimeSlider and set time
  vector<miTime> vtime;
  vtime.push_back(time);
  emit emitTimes("product",vtime);
  emit setTime(time);

  // First, remove previous PROFET fieldPlot (if any)
  emit showProfetField(""); //FieldDialog::fieldEditUpdate

  // make plot string for new PROFET fieldPlot
  miString plotString;
  plotString += ModelNames::profetWork(reftime) + " ";
  plotString += param;
  plotString += " time=";
  plotString += time.isoTime("T");
  plotString += " overlay=1";
  LOG4CXX_DEBUG(logger,"showField: "<<plotString);
  emit showProfetField(plotString); //FieldDialog::fieldEditUpdate

  // will trigger MenuOK in qtMainWindow :-)
  emit prepareAndPlot();
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
  QCoreApplication::flush();
}
/**
 * Called by multiple threads
 */
miString DianaProfetGUI::getCurrentParameter(){
  currentParamTimeMutex.lock();
  miString tmp = currentParam;
  currentParamTimeMutex.unlock();
  return tmp;
}
/**
 * Called by multiple threads
 */
miTime DianaProfetGUI::getCurrentTime(){
  currentParamTimeMutex.lock();
  miTime tmp = currentTime;
  currentParamTimeMutex.unlock();
  return tmp;
}

void DianaProfetGUI::paintModeChanged(GridAreaManager::PaintMode mode){
  areaManager->setPaintMode(mode);
}

void DianaProfetGUI::undoCurrentArea(){
  if(areaManager->isUndoPossible())
    areaManager->undo();
  paintToolBar->enableUndo(areaManager->isUndoPossible());
  paintToolBar->enableRedo(areaManager->isRedoPossible());
  gridAreaChanged();
}

void DianaProfetGUI::redoCurrentArea(){
  if(areaManager->isRedoPossible())
    areaManager->redo();
  paintToolBar->enableUndo(areaManager->isUndoPossible());
  paintToolBar->enableRedo(areaManager->isRedoPossible());
  gridAreaChanged();
}

void DianaProfetGUI::gridAreaChanged(){
  LOG4CXX_DEBUG(logger,"gridAreaChanged");
  miString currentId = areaManager->getCurrentId();
  currentObjectMutex.lock();
  miString obj_id = currentObject.id();
  currentObjectMutex.unlock();
  // Different object selected on map
  if(currentId != "newArea" && currentId != obj_id){
    QModelIndex modelIndex = objectModel.getIndexById(currentId);
    if(modelIndex.isValid()){
      sessionDialog.setSelectedObject(modelIndex);
    }
    return;
  }
  if(areaManager->isAreaSelected()){
    editObjectDialog.setAreaStatus(ProfetObjectDialog::AREA_OK);
    currentObjectMutex.lock();
    bool obj_exist = currentObject.exist();
    if(obj_exist){
      objectFactory.setPolygon(currentObject, areaManager->getCurrentPolygon());
      objectFactory.setGuiValues(currentObject,editObjectDialog.getCurrentGuiComponents());
    }
    currentObjectMutex.unlock();
    if(!obj_exist) {
      int i = getBaseObjectIndex(editObjectDialog.getSelectedBaseObject());
      if(i != -1){
        if(areaManager->isAreaSelected()){
          miTime refTime;
          try{
            fetSession s = getCurrentSession();
            refTime = s.referencetime();
          }catch(InvalidIndexException & iie){
            cerr << "DianaProfetGUI::baseObjectSelected invalid session index" << endl;
          }
      	  // TODO: fetch parent from somewhere...
      	  miString parent_ = "";
      	  miString username = controller.getCurrentUser().name;
      	  currentObjectMutex.lock();
          currentObject = objectFactory.makeObject(baseObjects[i],
						   areaManager->getCurrentPolygon(),
						   getCurrentParameter(), getCurrentTime(),
						   editObjectDialog.getReason(),username,
						   refTime,parent_);
          currentObjectMutex.unlock();
        }
        else{
          LOG4CXX_WARN(logger,"gridAreaChanged: No Area Selected");
        }
      }
    }
    currentObjectMutex.lock();
    fetObject safeCopy = currentObject;
    currentObjectMutex.unlock();
    controller.objectChanged(safeCopy);
    paintToolBar->enableButtons(PaintToolBar::PAINT_AND_MODIFY);
    // select move mode after paint to avoid accidental redefinition
    if(paintToolBar->getPaintMode() == GridAreaManager::DRAW_MODE)
      paintToolBar->setPaintMode(GridAreaManager::MOVE_MODE);
  }
  else{
    editObjectDialog.setAreaStatus(ProfetObjectDialog::AREA_NOT_SELECTED);
  }
  paintToolBar->enableUndo(areaManager->isUndoPossible());
}

void DianaProfetGUI::cancelEditObjectDialog(){
  // Current area added with createNewObject
  if(areaManager->getCurrentId() == "newArea"){
    areaManager->removeCurrentArea();
  }
  if(!editObjectDialog.showingNewObject()){
    while(areaManager->undo()) ;
    gridAreaChanged();
    currentObjectMutex.lock();
    fetObject safeCopy = currentObject;
    currentObjectMutex.unlock();
    try{
      controller.closeObject(safeCopy);
    }catch (Profet::ServerException & se) {
      handleServerException(se);
    }
  }
  setEditObjectDialogVisible(false);
  currentObjectMutex.lock();
  currentObject=fetObject();
  currentObjectMutex.unlock();
  enableObjectButtons(true,false,true);
}

void DianaProfetGUI::setPaintToolBarVisible(bool visible){
  showPaintToolBar = visible;
  emit setPaintMode(showPaintToolBar);
  if(showPaintToolBar) paintToolBar->show();
  else {
    paintToolBar->hide();
  }
}

void DianaProfetGUI::setViewObjectDialogVisible(bool visible){
  cerr << "setViewObjectDialogVisible: " << visible << endl;
  showViewObjectDialog = visible;
  if (showViewObjectDialog) {
    viewObjectDialog.show();
  } else {
    viewObjectDialog.hide();
  }
}

void DianaProfetGUI::setEditObjectDialogVisible(bool visible){
  showEditObjectDialog = visible;
  //sessionDialog.setEditable(!visible); // doesn't do anything...
  if(showEditObjectDialog) {
    editObjectDialog.show();
  }
  else {
    paintToolBar->enableButtons(PaintToolBar::SELECT_ONLY);
    editObjectDialog.hide();
  }
}

bool DianaProfetGUI::isVisible(){
  return sessionDialog.isVisible();
}

void DianaProfetGUI::setVisible(bool visible){
  if(visible){
    ignoreSynchProblems = true;
    setPaintToolBarVisible(showPaintToolBar);
    emit setPaintMode(showPaintToolBar);
    setEditObjectDialogVisible(showEditObjectDialog);
//    sessionDialog.selectDefault(); too early first time?
    sessionDialog.show();
    emit repaintMap(false);
  }
  else{
    // First, remove previous PROFET fieldPlot (if any)
    if ( getCurrentParameter().length() ){
      emit showProfetField("");
      emit prepareAndPlot();
    }
    setCurrentParam("");
    areaManager->clear();
    emit repaintMap(false);
    sessionDialog.hide();
    editObjectDialog.hide();
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
  editObjectDialog.setStatistics(m);
}

void  DianaProfetGUI::setActivePoints(vector<Point> points){
  areaManager->setActivePoints(points);
}
