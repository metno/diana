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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diCommonTypes.h>
#include "qtDianaProfetGUI.h"
#include "qtPaintToolBar.h"
#include "qtProfetEvents.h"
#include "qtProfetWaitDialog.h"
#include <qstring.h>
#include <QMessageBox>
#include <QApplication>
#include <QMenu>
#include <QAction>


#include <diField/diRectangle.h>

DianaProfetGUI::DianaProfetGUI(Profet::ProfetController & pc,
    PaintToolBar * ptb, GridAreaManager * gam, QWidget* p) :
  QObject(), Profet::ProfetGUI(pc), areaManager(gam), paintToolBar(ptb),
      objectFactory(), objectModel(p), tableModel(p), userModel(p),
      sessionModel(p), activeTimeSmooth(false), overviewactive(false),
      enableNewbutton_(true), enableModifyButtons_(false), enableTable_(true),
      ignoreSynchProblems(false)
{
  parent = p;
#ifndef NOLOG4CXX
  logger = log4cxx::LoggerPtr(
      log4cxx::Logger::getLogger("diana.DianaProfetGUI"));
#endif
  logfile.setSection("PROFET.LOG");

  sessionDialog = new ProfetSessionDialog(parent);
  viewObjectDialog = new ProfetObjectDialog(parent,
      ProfetObjectDialog::VIEW_OBJECT_MODE);
  editObjectDialog = new ProfetObjectDialog(parent);

  logfile.restoreSizeAndPos(sessionDialog, "ProfetSessionDialog");

  sessionDialog->setUserModel(&userModel);
  sessionDialog->setSessionModel(&sessionModel);
  sessionDialog->setObjectModel(&objectModel);
  sessionDialog->setTableModel(&tableModel);
  connectSignals();
  showPaintToolBar = true;
  showEditObjectDialog = false;
  emit setPaintMode(true);
  paintToolBar->enableButtons(PaintToolBar::SELECT_ONLY);

//Popupmenu
  editObjectAction = new QAction(tr("Edit Object"),this);
  editObjectAction->setVisible(false);
  connect(editObjectAction, SIGNAL( triggered() ), SLOT(editObject()));
  deleteObjectAction = new QAction(tr("Delete Object"),this);
  deleteObjectAction->setVisible(false);
  connect(deleteObjectAction, SIGNAL( triggered() ), SLOT(deleteObject()));
  startTimesmoothAction = new QAction(tr("Time smooth"),this);
  startTimesmoothAction->setVisible(false);
  connect(startTimesmoothAction, SIGNAL( triggered() ), SLOT(startTimesmooth()));
  popupMenu = new QMenu(parent);
  popupMenu->addAction(editObjectAction);
  popupMenu->addAction(deleteObjectAction);
  popupMenu->addAction(startTimesmoothAction);

}

void DianaProfetGUI::setParamColours()
{
  vector<fetParameter> parameters = controller.getParameters();
  map<miutil::miString,miutil::miString> plotname2name;
  map<miutil::miString, map<miutil::miString, miutil::miString> > fieldoptions;
  for (size_t i=0; i<parameters.size(); i++){
    fieldoptions[parameters[i].plotname().downcase()]["colour"] = "black";
    plotname2name[parameters[i].plotname().downcase()] = parameters[i].name().downcase();
  }

  emit getFieldPlotOptions(fieldoptions);

  map<miutil::miString, map<miutil::miString, miutil::miString> >::iterator itr = fieldoptions.begin();
  for (; itr != fieldoptions.end(); itr++) {
    map<miutil::miString, miutil::miString>::iterator sitr = itr->second.begin();
    for (; sitr != itr->second.end(); sitr++) {
      if (sitr->first.downcase() == "colour") {
        parameterColours[plotname2name[itr->first.downcase()]] = Colour(sitr->second
            + miutil::miString(":150"));
      }
    }
  }

  tableModel.setParamColours(parameterColours);
}

void DianaProfetGUI::connectSignals()
{
  connect(sessionDialog, SIGNAL(objectSelected(const QModelIndex &)),
  this, SLOT(objectSelected(const QModelIndex &)));
  connect(sessionDialog, SIGNAL(objectDoubleClicked(const QModelIndex &)),
  this, SLOT(objectDoubleClicked(const QModelIndex &)));
  connect(sessionDialog, SIGNAL(paramAndTimeChanged(const QModelIndex &)),
  this, SLOT(paramAndTimeSelected(const QModelIndex &)));
  connect(sessionDialog, SIGNAL(showObjectOverview(const QList<QModelIndex> &)),
  this, SLOT(showObjectOverview(const QList<QModelIndex> &)));

  connect(paintToolBar, SIGNAL(paintModeChanged(GridAreaManager::PaintMode)),
  this, SLOT(paintModeChanged(GridAreaManager::PaintMode)));
  connect(paintToolBar, SIGNAL(undoPressed()),
  this, SLOT(undoCurrentArea()));
  connect(paintToolBar, SIGNAL(redoPressed()),
  this, SLOT(redoCurrentArea()));
  connect(sessionDialog, SIGNAL(viewObjectToggled(bool)),
  this, SLOT(toggleViewObject(bool)));
  connect(sessionDialog, SIGNAL(newObjectPerformed()),
  this, SLOT(createNewObject()));
  connect(sessionDialog, SIGNAL(editObjectPerformed()),
  this, SLOT(editObject()));
  connect(sessionDialog, SIGNAL(startTimesmooth()),
  this, SLOT(startTimesmooth()));
  connect(sessionDialog, SIGNAL(deleteObjectPerformed()),
  this, SLOT(deleteObject()));
  connect(sessionDialog, SIGNAL(closePerformed()),
  this, SLOT(closeSessionDialog())); // only signaled when gui is visible
  connect(sessionDialog, SIGNAL(forcedClosePerformed(bool)),
  this, SIGNAL(forceDisconnect(bool)));
  connect(sessionDialog, SIGNAL(sendMessage(const QString &)),
  this, SLOT(sendMessage(const QString &)));
  connect(sessionDialog, SIGNAL(sessionSelected(int)),
  this, SLOT(sessionSelected(int)));
  connect(sessionDialog, SIGNAL(doReconnect()),
  this, SLOT(doReconnect()));
  connect(sessionDialog, SIGNAL(updateActionPerformed()),
  this, SLOT(doUpdate()));

  connect(viewObjectDialog, SIGNAL(cancelObjectDialog()),
  sessionDialog, SLOT(hideViewObjectDialog()));

  connect(editObjectDialog, SIGNAL(saveObjectClicked()),
  this, SLOT(saveObject()));
  connect(editObjectDialog, SIGNAL(cancelObjectDialog()),
  this, SLOT(cancelEditObjectDialog()));
  connect(editObjectDialog, SIGNAL(baseObjectSelected(miutil::miString)),
  this, SLOT(baseObjectSelected(miutil::miString)));
  connect(editObjectDialog, SIGNAL(dynamicGuiChanged()),
  this, SLOT(dynamicGuiChanged()));

  connect(&sessionModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex & )),
  this, SLOT(sessionModified(const QModelIndex & , const QModelIndex & )));

  connect(editObjectDialog, SIGNAL(copyPolygon(miutil::miString,miutil::miString,bool)),
  this, SLOT(copyPolygon(miutil::miString,miutil::miString,bool)));
  connect(editObjectDialog, SIGNAL(selectPolygon(miutil::miString)),
  this, SLOT(selectPolygon(miutil::miString)));
  connect(editObjectDialog, SIGNAL(requestPolygonList()),
  this, SLOT(requestPolygonList()));

//  connect(popupMenu, SIGNAL(triggered(int)),
//  SLOT(popupMenuActivated(int)));
}

DianaProfetGUI::~DianaProfetGUI()
{

}

void DianaProfetGUI::closeSessionDialog()
{
  logfile.logSizeAndPos(sessionDialog, "ProfetSessionDialog");
  emit toggleProfetGui();
  emit updateModelDefinitions();
}

void DianaProfetGUI::showObjectOverview(const QList<QModelIndex> & selected)
{
  QList<QModelIndex>::const_iterator i = selected.begin();
  set<miutil::miString> parameters;
  set<miutil::miTime> times;
  for (; i != selected.end(); i++) {
    miutil::miTime t = tableModel.getTime(*i);
    miutil::miString p = tableModel.getParameter(*i);
    parameters.insert(p);
    times.insert(t);
  }
  miutil::miString param;
  miutil::miTime time;
  if (parameters.size() == 1 && times.size() != 1) {
    param = (*parameters.begin());
  } else if (parameters.size() != 1 && times.size() == 1) {
    time = (*times.begin());
  }
  toggleObjectOverview(true, param, time);

  enableObjectButtons(enableNewbutton_, enableModifyButtons_, enableTable_);
  emit repaintMap(true);
}

// void DianaProfetGUI::showObjectOverview(int row, int col){
//   miutil::miString p;
//   miutil::miTime t;
//   if ( row >= 0 ){
//     p = getCurrentParameter();
//   }
//   if ( col >= 0 ){
//     t = getCurrentTime();
//   }
//   toggleObjectOverview(true,p,t);
// }


void DianaProfetGUI::toggleObjectOverview(bool turnon, miutil::miString par,
    miutil::miTime time)
{
  if (turnon) {
    vector<fetObject> objects = controller.getOverviewObjects(par, time);
    INFO_ << "Got " << objects.size() << " objects for par:" << par;

    areaManager->clearTemporaryAreas();
    for (size_t i = 0; i < objects.size(); i++) {
      Colour colour = Colour(128, 128, 128, 100);
      if (parameterColours.count(objects[i].parameter().downcase()) > 0) {
        colour = parameterColours[objects[i].parameter().downcase()];
      }
      areaManager->addOverviewArea(objects[i].id(), objects[i].polygon(),
          colour);
    }
    tableModel.setHeaderDisplayMask(FetObjectTableModel::PARAM_COLOUR_RECT);
  } else {
    if (overviewactive) {
      areaManager->clearTemporaryAreas();
      tableModel.setHeaderDisplayMask(FetObjectTableModel::PARAM_ICON);// | FetObjectTableModel::PARAM_COLOUR_GRADIENT);
    }
  }
  overviewactive = turnon;
}

void DianaProfetGUI::sessionModified(const QModelIndex & topLeft,
    const QModelIndex & bottomRight)
{
  if (topLeft == bottomRight) {
    fetSession s = sessionModel.getSession(topLeft);
    if (s == currentSession) {
      INFO_ << "CURRENTSESSION CHANGED!!!!!";
      // current session has been modified
      currentSession = s;
      enableObjectButtons(enableNewbutton_, enableModifyButtons_, enableTable_);
    }
  }
}

void DianaProfetGUI::popupMenuActivated(int i)
{
//  miutil::miString id = popupMenu->text(i).toStdString();
//  id = id.substr(id.find_last_of("|") + 2, id.size() - 1);
//  areaManager->setCurrentArea(id);
//  gridAreaChanged();
}

fetSession DianaProfetGUI::getCurrentSession()
{
  return currentSession;
}

void DianaProfetGUI::enableObjectButtons(bool enableNewbutton,
    bool enableModifyButtons, bool enableTable)
{
  //   DEBUG_ << "DianaProfetGUI::enableObjectButtons (" << enableNewbutton
  //        << "," << enableModifyButtons << "," << enableTable << ")";

  fetSession s = getCurrentSession();
  bool isopen = (!s.referencetime().undef() && s.editstatus()
      == fetSession::editable && !overviewactive);

  sessionDialog->enableObjectButtons(enableNewbutton && isopen,
      enableModifyButtons && isopen, enableTable);

  enableNewbutton_ = enableNewbutton;
  enableModifyButtons_ = enableModifyButtons;
  enableTable_ = enableTable;
}

void DianaProfetGUI::setCurrentParam(const miutil::miString & p)
{
  currentParamTimeMutex.lock();
  currentParam = p;
  currentParamTimeMutex.unlock();
}

void DianaProfetGUI::setCurrentTime(const miutil::miTime & t)
{
  currentParamTimeMutex.lock();
  currentTime = t;
  currentParamTimeMutex.unlock();
}

void DianaProfetGUI::resetStatus()
{
  userModel.clearModel();
  sessionModel.clearModel();
  objectModel.clearModel();
  tableModel.clearModel();
  vector<miutil::miTime> notimes;
  emit emitTimes("product", notimes);

}

void DianaProfetGUI::setParameters(const vector<fetParameter>& vp){
  tableModel.setParameters(vp);
}


void DianaProfetGUI::setCurrentSession(const fetSession & session)
{
  //DEBUG_ << "DianaProfetGUI::setCurrentSession:" << session;

  //Changing current session must be done in QT event queue
  //to be performed in the correct order...
  Profet::CurrentSessionEvent * cse = new Profet::CurrentSessionEvent();
  cse->refTime = session.referencetime();
  QCoreApplication::postEvent(this, cse);

  currentSession = session;
  tableModel.setCurrentSessionRefTime(currentSession.referencetime());
  tableModel.initTable(session.times(), session.parameters());
  sessionDialog->selectDefault();
  enableObjectButtons(true, false, true);
  QCoreApplication::flush();
}

bool DianaProfetGUI::selectTime(miutil::miTime time)
{
  if (!enableTable_)
    return false;
  if (!sessionDialog->animationChecked())
    return false;
  if (time == getCurrentTime())
    return false;
  QModelIndex modelIndex =
      tableModel.getModelIndex(time, getCurrentParameter());
  if (modelIndex.isValid()) {
    sessionDialog->selectParameterAndTime(modelIndex);
    return true;
  }
  return false;
}

void DianaProfetGUI::setBaseObjects(vector<fetBaseObject> obj)
{
  LOG4CXX_DEBUG(logger,"setBaseObjects");
  baseObjects = obj;
}

// THREAD SAFE!,
void DianaProfetGUI::showMessage(const Profet::InstantMessage & msg)
{
  QCoreApplication::postEvent(sessionDialog, new Profet::MessageEvent(msg));//thread-safe
  QCoreApplication::flush();
}

void DianaProfetGUI::setSession(const fetSession & session, bool remove)
{
  Profet::SessionListEvent * sle = new Profet::SessionListEvent();
  sle->remove = remove;
  sle->session = session;
  QCoreApplication::postEvent(&sessionModel, sle);//thread-safe
  QCoreApplication::flush();
}

void DianaProfetGUI::setUser(const Profet::PodsUser & user)
{
  LOG4CXX_DEBUG(logger,"setUser " << user.name);
  Profet::UserListEvent * cle1 = new Profet::UserListEvent();
  cle1->type = Profet::UserListEvent::SET_USER;
  cle1->user = user;
  QCoreApplication::postEvent(this, cle1);//thread-safe
  QCoreApplication::flush();
}

void DianaProfetGUI::removeUser(const Profet::PodsUser & user)
{
  LOG4CXX_DEBUG(logger,"removeUser " << user.name);
  Profet::UserListEvent * cle = new Profet::UserListEvent();
  cle->type = Profet::UserListEvent::REMOVE_USER;
  cle->user = user;
  QCoreApplication::postEvent(this, cle);//thread-safe
  QCoreApplication::flush();
}

// THREAD SAFE!
void DianaProfetGUI::setUsers(const vector<Profet::PodsUser> & users)
{
  LOG4CXX_DEBUG(logger,"setUsers " << users.size());
  Profet::UserListEvent * cle = new Profet::UserListEvent();
  cle->type = Profet::UserListEvent::REPLACE_LIST;
  cle->users = users;
  QCoreApplication::postEvent(this, cle);//thread-safe
  QCoreApplication::flush();
}

void DianaProfetGUI::customEvent(QEvent * e)
{
  if (e->type() == Profet::UPDATE_MAP_EVENT) {
    emit repaintMap(true);
  } else if (e->type() == Profet::OBJECT_LIST_UPDATE_EVENT) {
    Profet::ObjectListUpdateEvent * oue = (Profet::ObjectListUpdateEvent*) e;
    objectModel.setObjects(oue->objects);
    //bool removeAreas = (areaManager->getAreaCount() > 0);
    areaManager->clear();
    int nObjects = oue->objects.size();
    for (int i = 0; i < nObjects; i++) {
      areaManager->addArea(oue->objects[i].id(), oue->objects[i].polygon(),
          false);
    }
  } else if (e->type() == Profet::OBJECT_UPDATE_EVENT) {
    Profet::ObjectUpdateEvent * oue = (Profet::ObjectUpdateEvent*) e;
    if (oue->remove) {
      bool r = objectModel.removeObject(oue->object.id());
      if (r)
        areaManager->removeArea(oue->object.id());
    } else {
      objectModel.setObject(oue->object);
      areaManager->updateArea(oue->object.id(), oue->object.polygon());
    }
  } else if (e->type() == Profet::CURRENT_SESSION_UPDATE_EVENT) {
    Profet::CurrentSessionEvent * cse = (Profet::CurrentSessionEvent*) e;
    sessionDialog->setCurrentSession(sessionModel.getIndexByRefTime(
        cse->refTime));
    // updates FieldDialog
    emit updateModelDefinitions();
  } else if (e->type() == Profet::USER_LIST_EVENT) {
    Profet::UserListEvent * cle = (Profet::UserListEvent*) e;
    if (cle->type == Profet::UserListEvent::REPLACE_LIST) {
      userModel.setUsers(cle->users);
    } else if (cle->type == Profet::UserListEvent::SET_USER) {
      PodsUser podsUser = userModel.setUser(cle->user);
      tableModel.setUserLocation(podsUser);
    } else if (cle->type == Profet::UserListEvent::REMOVE_USER) {
      userModel.removeUser(cle->user);
      tableModel.removeUserLocation(cle->user);
    }
  }

}
// THREAD SAFE!
void DianaProfetGUI::updateObjects(const vector<fetObject> & objects)
{
  Profet::ObjectListUpdateEvent * oue = new Profet::ObjectListUpdateEvent(
      objects);
  QCoreApplication::postEvent(this, oue);//thread-safe
  QCoreApplication::flush();
  DEBUG_ << "DianaProfetGUI:: updateObjects";
}

void DianaProfetGUI::updateObject(const fetObject & object, bool remove)
{
  Profet::ObjectUpdateEvent * oue = new Profet::ObjectUpdateEvent(object,
      remove);
  QCoreApplication::postEvent(this, oue);//thread-safe
  QCoreApplication::flush();
  DEBUG_ << "DianaProfetGUI:: updateObject";
}

/**
 * Called by multiple threads
 */
void DianaProfetGUI::updateObjectSignatures(
    const vector<fetObject::Signature> & s)
{
  Profet::SignatureListUpdateEvent * sue =
      new Profet::SignatureListUpdateEvent(s);
  QCoreApplication::postEvent(&tableModel, sue);//thread-safe
  QCoreApplication::flush();
}

void DianaProfetGUI::updateObjectSignature(const fetObject::Signature & s,
    bool remove)
{
  Profet::SignatureUpdateEvent * sue = new Profet::SignatureUpdateEvent(s,
      remove);
  QCoreApplication::postEvent(&tableModel, sue);//thread-safe
  QCoreApplication::flush();
}

void DianaProfetGUI::baseObjectSelected(miutil::miString id)
{
  int i = getBaseObjectIndex(id);
  if (i != -1) {
    LOG4CXX_DEBUG(logger,"base object selected " << baseObjects[i].name());
    editObjectDialog->addDymanicGui(objectFactory.getGuiComponents(
        baseObjects[i]));
    if (areaManager->isAreaSelected()) {
      miutil::miTime refTime;
      try {
        fetSession s = getCurrentSession();
        refTime = s.referencetime();
      } catch (InvalidIndexException & iie) {
        ERROR_ << "DianaProfetGUI::baseObjectSelected invalid session index";
      }

      // TODO: fetch parent from somewhere...
      miutil::miString parent_ = "";
      miutil::miString username = controller.getCurrentUser().name;
      currentObjectMutex.lock();
      currentObject = objectFactory.makeObject(baseObjects[i],
          areaManager->getCurrentPolygon(), getCurrentParameter(),
          getCurrentTime(), editObjectDialog->getReason(), username, refTime,
          parent_);
      currentObjectMutex.unlock();
      LOG4CXX_DEBUG(logger,"calling controller.objectChanged");
      controller.objectChanged(currentObject);
    }
  } else {
    LOG4CXX_ERROR(logger,"Selected base object not found");
  }
  if (areaManager->isAreaSelected()) {
    editObjectDialog->setAreaStatus(ProfetObjectDialog::AREA_OK);
  } else {
    editObjectDialog->setAreaStatus(ProfetObjectDialog::AREA_NOT_SELECTED);
  }
}

void DianaProfetGUI::objectSelected(const QModelIndex & index)
{
  LOG4CXX_DEBUG(logger,"objectSelected");
  try {
    fetObject fo =
        objectModel.getObject(sessionDialog->getCurrentObjectIndex());
    areaManager->setCurrentArea(fo.id());
    viewObjectDialog->showObject(fo, objectFactory.getGuiComponents(fo));
    updateMap();
    if(sessionDialog->autoZoomEnabled())
      emit zoomToObject(areaManager->getCurrentPolygon());
  } catch (InvalidIndexException & iie) {
    LOG4CXX_ERROR(logger,"editObject:" << iie.what());
    return;
  }
  enableObjectButtons(true, true, true);
}

void DianaProfetGUI::objectDoubleClicked(const QModelIndex & index)
{
  LOG4CXX_DEBUG(logger,"objectDoubleClicked");
  objectSelected(index);
  if (enableModifyButtons_)
    editObject();
}

void DianaProfetGUI::zoomToObject(const ProjectablePolygon & pp) {
  float offsetRatio = 0.3;
  Polygon polygon = pp.getPolygonInCurrentProjection();
  Point minPoint = polygon.minBoundaryPoint();
  Point maxPoint = polygon.maxBoundaryPoint();
  Point offset = (maxPoint - minPoint) * offsetRatio;
  minPoint = minPoint - offset;
  maxPoint = maxPoint + offset;
  Rectangle rectangle(minPoint.get_x(), minPoint.get_y(),
      maxPoint.get_x(), maxPoint.get_y());
  emit zoomTo(rectangle);
}

void DianaProfetGUI::saveObject()
{
  // Validate current object
  if (!areaManager->isAreaSelected()) {
    editObjectDialog->setAreaStatus(ProfetObjectDialog::AREA_NOT_VALID);
    LOG4CXX_WARN(logger,"Save Object: No Area Selected");
    return;
  }
  currentObjectMutex.lock();
  bool coExist = currentObject.exist();
  currentObjectMutex.unlock();
  if (!coExist) {
    LOG4CXX_ERROR(logger,"Save Object: currentObject does not exist");
    setEditObjectDialogVisible(false);
    return;
  }
  currentObject.setReason(editObjectDialog->getReason());
  //Save current object
  currentObjectMutex.lock();
  fetObject safeCopy = currentObject;
  currentObjectMutex.unlock();

  if (editObjectDialog->showingNewObject()) {
    areaManager->changeAreaId("newArea", safeCopy.id());
  }
  try {
    editObjectDialog->setLastSavedPolygonName(safeCopy.polygonName());
    controller.saveObject(safeCopy);
    currentObjectMutex.lock();
    currentObject = fetObject();
    currentObjectMutex.unlock();

    processSpatialsmooth();
    endSpatialsmooth();

    setEditObjectDialogVisible(false);
    enableObjectButtons(true, false, true);
  } catch (Profet::ServerException & se) {
    // reset id in areaManager
    if (editObjectDialog->showingNewObject()) {
      areaManager->changeAreaId(safeCopy.id(), "newArea");
    }
    handleServerException(se);
  }
}

void DianaProfetGUI::handleServerException(Profet::ServerException & se)
{
  if (se.getType() == Profet::ServerException::OUT_OF_SYNC
      && ignoreSynchProblems)
    return;
  if (se.isDisconnectRecommanded()) {
    int i = QMessageBox::warning(0, "Disconnect from Profet?",
        se.getHtmlMessage(true).c_str(), QMessageBox::Ok, QMessageBox::Ignore);
    if (i == QMessageBox::Ok)
      emit forceDisconnect(false);
    else if (Profet::ServerException::OUT_OF_SYNC)
      ignoreSynchProblems = true;
  }
  QMessageBox::critical(0, QString("Profet Warning"),
      se.getHtmlMessage(true).c_str());
}
void DianaProfetGUI::collectRelatedTimeValues(
    vector<fetObject::TimeValues>& obj, miutil::miString id_, bool withPolygon)
{
  if (!id_.exists()) {
    try {
      fetObject fo = objectModel.getObject(
          sessionDialog->getCurrentObjectIndex());
      id_ = fo.id();
    } catch (InvalidIndexException & iie) {
      LOG4CXX_ERROR(logger,"DianaProfetGUI::collectRelatedTimeValue :" << iie.what());
      return;
    }
  }

  try {
    obj = controller.getTimeValues(id_, withPolygon);
  } catch (Profet::ServerException & se) {
    handleServerException(se);
    return;
  }

}

void DianaProfetGUI::startTimesmooth()
{
  if (activeTimeSmooth)
    return;

  vector<miutil::miTime> tim;
  try {
    fetSession s = getCurrentSession();
    tim = s.times();
  } catch (InvalidIndexException & iie) {
    ERROR_ << "DianaProfetGUI::startTimesmooth invalid session index";
    return;
  }
  vector<fetObject::TimeValues> obj;

  collectRelatedTimeValues(obj, "", false);

  if (obj.empty())
    return;

  timesmoothdialog = new ProfetTimeSmoothDialog(parent, obj, tim);

  logfile.restoreSizeAndPos(timesmoothdialog, "ProfetTimesmoothDialog");

  connect(timesmoothdialog, SIGNAL(runObjects(vector<fetObject::TimeValues>)),
  this, SLOT(processTimesmooth(vector<fetObject::TimeValues>)));

  connect(this, SIGNAL(timesmoothProcessed(miutil::miTime, miutil::miString)),
  timesmoothdialog, SLOT(processed(miutil::miTime, miutil::miString)));

  connect(timesmoothdialog, SIGNAL(endTimesmooth(vector<fetObject::TimeValues>)),
  this, SLOT(endTimesmooth(vector<fetObject::TimeValues>)));

  activeTimeSmooth = true;
  enableObjectButtons(false, false, false);
}

void DianaProfetGUI::processTimesmooth(vector<fetObject::TimeValues> tv)
{
  vector<miutil::miString> del_ids;
  processTimeValues(tv, del_ids);
}

void DianaProfetGUI::processTimeValues(vector<fetObject::TimeValues> tv,
    vector<miutil::miString> del_ids)
{
  vector<fetObject> obj;

  currentObjectMutex.lock();
  if (currentObject.exists())
    obj.push_back(currentObject);
  currentObjectMutex.unlock();

  set<std::string> deletion_ids;
  controller.getTimeValueObjects(obj, tv, deletion_ids);
  for (size_t i = 0; i < del_ids.size(); ++i) {
    deletion_ids.insert(del_ids[i]);
  }

  // deleting objects without effect
  try {
    for (size_t i = 0; i < tv.size(); i++) {
      if (deletion_ids.count(tv[i].id)) {
        controller.deleteObject(tv[i].id);
        emit timesmoothProcessed(tv[i].validTime, "");
      }
    }
  } catch (Profet::ServerException & se) {
    ERROR_ << "DianaProfetGUI::processTimesmooth:  Failed to delete "
        << "deprecated object. (Not significant)";
  }

  for (size_t i = 0; i < obj.size(); i++) {
    miutil::miTime tim = obj[i].validTime();
    miutil::miString obj_id = obj[i].id();
    try {
      if (objectFactory.processTimeValuesOnObject(obj[i])) {
        controller.saveObject(obj[i], true);
        emit timesmoothProcessed(tim, obj[i].id());
      } else {
        emit timesmoothProcessed(tim, "");
        ERROR_ << "could not process" << obj[i].id() << " at " << tim;
      }
    } catch (Profet::ServerException & se) {
      handleServerException(se);
    }
  }

}
void DianaProfetGUI::endTimesmooth(vector<fetObject::TimeValues> tv)
{
  activeTimeSmooth = false;
  try {
    controller.unlockObjectsByTimeValues(tv);
  } catch (Profet::ServerException & se) {
    handleServerException(se);
  }
  logfile.logSizeAndPos(timesmoothdialog, "ProfetTimesmoothDialog");
  enableObjectButtons(true, true, true);
}

void DianaProfetGUI::copyPolygon(miutil::miString fromPoly, miutil::miString toPoly, bool move)
{
  try {
    controller.copyPolygon(fromPoly, toPoly, move);
  } catch (Profet::ServerException & se) {
    handleServerException(se);
  }
}

void DianaProfetGUI::selectPolygon(miutil::miString polyname)
{
  fetPolygon fpoly;
  try {
    fpoly = controller.getPolygon(polyname);
  } catch (Profet::ServerException & se) {
    handleServerException(se);
    return;
  }
  if (areaManager) {
    miutil::miString id = "newArea";
    currentObjectMutex.lock();
    if (currentObject.exists()) {
      id = currentObject.id();
      // If new object but reselected polygon
      if (areaManager->getCurrentId() == "newArea")
        areaManager->changeAreaId("newArea", id);
    }
    currentObject.setPolygon(fpoly.polygon());
    currentObjectMutex.unlock();
    areaManager->addArea(id, fpoly.polygon(), true);
    gridAreaChanged();
  }
}

void DianaProfetGUI::requestPolygonList()
{
  vector<std::string> polynames;
  try {
    polynames = controller.getPolygonIndex();
  } catch (Profet::ServerException & se) {
    handleServerException(se);
    return;
  }

  if (not polynames.empty()) {
    vector<miutil::miString> polynames_ms(polynames.begin(), polynames.end());
    editObjectDialog->startBookmarkDialog(polynames_ms);
  }
}

void DianaProfetGUI::dynamicGuiChanged()
{
  LOG4CXX_DEBUG(logger,"Dynamic GUI changed");
  // Check if current object is ok ...
  if (areaManager->isAreaSelected()) {
    currentObjectMutex.lock();
    objectFactory.setGuiValues(currentObject,
        editObjectDialog->getCurrentGuiComponents());
    // safe copy needed to unlock mutex before calling controller
    // ... that in turn can call another mutex.lock
    fetObject safeCopy = currentObject;
    currentObjectMutex.unlock();
    controller.objectChanged(safeCopy);
  }
}

void DianaProfetGUI::sessionSelected(int index)
{
  DEBUG_ << "DianaProfetGUI::sessionSelected:" << index;
  try {
    controller.currentSessionChanged(sessionModel.getSession(index));
  } catch (Profet::ServerException & se) {
    handleServerException(se);
    return;
  } catch (InvalidIndexException & iie) {
    ERROR_ << "DianaProfetGUI::sessionSelected invalid index";
    return;
  }
  //enableObjectButtons(true,false,true);
}

void DianaProfetGUI::sendMessage(const QString & m)
{
  miutil::miString username = controller.getCurrentUser().name;
  Profet::InstantMessage message(miutil::miTime::nowTime(), 0, username, "all",
      m.toStdString());
  try {
    controller.sendMessage(message);
  } catch (Profet::ServerException & se) {
    handleServerException(se);
  }
}

void DianaProfetGUI::paramAndTimeSelected(const QModelIndex & index)
{
  toggleObjectOverview(false, miutil::miString(), miutil::miTime());
  bool newbutton = true;
  bool modbutton = false;
  bool thetable = true;
  enableObjectButtons(newbutton, modbutton, thetable);

  tableModel.setLastSelectedIndex(index);
  //check if parameters and times are set in model
  bool tableInited = tableModel.inited();
  if (tableInited) {
    try {
      setCurrentParam(tableModel.getParameter(index));
      setCurrentTime(tableModel.getTime(index));
      controller.parameterAndTimeChanged(getCurrentParameter(),
          getCurrentTime());
    } catch (Profet::ServerException & se) {
      handleServerException(se);
    } catch (InvalidIndexException & iie) {
      LOG4CXX_ERROR(logger,"Invalid time/param index");
    }
  }
}

void DianaProfetGUI::createNewObject()
{
  LOG4CXX_DEBUG(logger,"createNewObject");
  bool ok = areaManager->addArea("newArea");
  if (!ok) {
    LOG4CXX_WARN(logger,"Previous temp. area not removed!");
  }
  paintToolBar->enableButtons(PaintToolBar::PAINT_ONLY);
  editObjectDialog->setSession(getCurrentTime());
  editObjectDialog->setParameter(getCurrentParameter());
  editObjectDialog->setBaseObjects(baseObjects);
  editObjectDialog->newObjectMode();
  currentObject = fetObject(); // new id
  // select first base object
  // currentObject is reset
  editObjectDialog->selectDefault();
  setEditObjectDialogVisible(true);
  enableObjectButtons(false, false, false);
}

void DianaProfetGUI::toggleViewObject(bool show)
{
  setViewObjectDialogVisible(show);
}

void DianaProfetGUI::editObject()
{
  LOG4CXX_DEBUG(logger,"editObject");
  string error = "";
  try {
    fetObject fo = objectModel.getObject(sessionDialog->getCurrentObjectIndex());
    editObjectDialog->setSession(getCurrentTime());
    editObjectDialog->setParameter(getCurrentParameter());
    editObjectDialog->setBaseObjects(baseObjects);
    editObjectDialog->editObjectMode(fo, objectFactory.getGuiComponents(fo));
    currentObjectMutex.lock();
    currentObject = fo;
    currentObjectMutex.unlock();
    controller.openObject(fo);
    setEditObjectDialogVisible(true);
    enableObjectButtons(false, false, false);
    paintToolBar->enableButtons(PaintToolBar::PAINT_AND_MODIFY);
    // Area always ok for a saved object
    editObjectDialog->setAreaStatus(ProfetObjectDialog::AREA_OK);

  } catch (Profet::ServerException & se) {
    handleServerException(se);
  } catch (InvalidIndexException & iie) {
    error = "Unable to find selected object.";
  }
  if (!error.empty()) {
    InstantMessage m(miutil::miTime::nowTime(), InstantMessage::WARNING_MESSAGE,
        "Edit Object Failed", "", error);
    showMessage(m);
  }
}

void DianaProfetGUI::deleteObject()
{
  LOG4CXX_DEBUG(logger,"deleteObject");
  enableObjectButtons(true, false, true);
  try {
    fetObject fo =
        objectModel.getObject(sessionDialog->getCurrentObjectIndex());
    controller.deleteObject(fo.id());
  } catch (Profet::ServerException & se) {
    handleServerException(se);
  } catch (InvalidIndexException & iie) {
    InstantMessage m(miutil::miTime::nowTime(), InstantMessage::WARNING_MESSAGE,
        "Delete Object Failed", "", "Unable to find selected object.");
    showMessage(m);
  }
}

void DianaProfetGUI::hideProfetPerformed()
{
  emit toggleProfetGui();
}

void DianaProfetGUI::doReconnect()
{
  Profet::DataManagerType preferredType = Profet::DISTRIBUTED_MANAGER;
  Profet::PodsUser user = controller.getCurrentUser();
  miutil::miString password = controller.getPassword();

  QApplication::setOverrideCursor(Qt::WaitCursor);

  controller.disconnect();

  ProfetWaitDialog * wait = new ProfetWaitDialog(sessionDialog, 3000, 200);
  if (!wait->exec())
    return;

  try {
    Profet::DataManagerType dmt = controller.connect(user, preferredType,
        password, false);
    QApplication::restoreOverrideCursor();
    if (dmt != preferredType)
      QMessageBox::warning(0, "Running disconnected mode",
          "Distributed field editing system is not available.");
  } catch (Profet::ServerException & se) {
    controller.disconnect();
    QMessageBox::warning(0, "Error connecting",
        se.getHtmlMessage(false).c_str());
    QApplication::restoreOverrideCursor();
  }
}

void DianaProfetGUI::doUpdate()
{
  setParamColours();
}

void DianaProfetGUI::showField(const miutil::miTime & reftime, const miutil::miString & param,
    const miutil::miTime & time)
{
  LOG4CXX_DEBUG(logger,"show field "<<param<<" "<<time);

  //send time(s) to TimeSlider and set time
  vector<miutil::miTime> vtime;
  vtime.push_back(time);
  emit emitTimes("product", vtime);
  emit setTime(time);

  // First, remove previous PROFET fieldPlot (if any)
  emit showProfetField(""); //FieldDialog::fieldEditUpdate

  // make plot string for new PROFET fieldPlot
  miutil::miString plotString;
  plotString += " model=";
  plotString += ModelNames::profetWork(reftime) + " ";
  plotString += " plot=";
  plotString += param;
  plotString += " time=";
  plotString += time.isoTime("T");
  plotString += " reftime=";
  plotString += reftime.isoTime("T");
  plotString += " overlay=1";
  LOG4CXX_DEBUG(logger,"showField: "<<plotString);
  emit showProfetField(plotString); //FieldDialog::fieldEditUpdate

  // will trigger MenuOK in qtMainWindow :-)
  emit prepareAndPlot();
}

//void DianaProfetGUI::setBaseProjection(Projection p, int size_x, int size_y, const double& gridResolutionX, const double& gridResolutionY)
//{
//  if (size_x == 0 || size_y == 0 || !p.isDefined()) {
//    LOG4CXX_ERROR(logger,"Unvalid base projection set:" << p);
//  } else {
////    objectFactory.initFactory(p, size_x, size_y);
////    areaManager->setBaseProjection(p);
//  }
//}

void DianaProfetGUI::updateMap()
{
  QCoreApplication::postEvent(this, new QEvent(QEvent::Type(
      Profet::UPDATE_MAP_EVENT)));//thread-safe
  QCoreApplication::flush();
}
/**
 * Called by multiple threads
 */
miutil::miString DianaProfetGUI::getCurrentParameter()
{
  currentParamTimeMutex.lock();
  miutil::miString tmp = currentParam;
  currentParamTimeMutex.unlock();
  return tmp;
}
/**
 * Called by multiple threads
 */
miutil::miTime DianaProfetGUI::getCurrentTime()
{
  currentParamTimeMutex.lock();
  miutil::miTime tmp = currentTime;
  currentParamTimeMutex.unlock();
  return tmp;
}

void DianaProfetGUI::startSpatialsmooth()
{
  currentObjectMutex.lock();
  fetObject fo = currentObject;
  currentObjectMutex.unlock();

  if (!fo.parent().exists())
    return;

  // Set child-areas (used for spatial interpolation)
  areaManager->clearSpatialInterpolation();
  collectRelatedTimeValues(spatialsmoothtv, fo.id(), true);
  for (size_t i = 0; i < spatialsmoothtv.size(); i++) {
    areaManager->addSpatialInterpolateArea(spatialsmoothtv[i].id,
        spatialsmoothtv[i].parent, spatialsmoothtv[i].validTime,
        spatialsmoothtv[i].polygon);
  }
}

void DianaProfetGUI::processSpatialsmooth()
{
  if (!areaManager->hasInterpolated()) {
    return;
  }

  QString dtitle = tr("Diana / Profet");
  QString dtext = tr("You have moved a set of objects to new locations. Would you like to save the changes?");
  int ret = QMessageBox::question(parent, dtitle, dtext, QMessageBox::Save
      | QMessageBox::Discard, QMessageBox::Save);
  if (ret != QMessageBox::Save) {
    endSpatialsmooth();
    return;
  }

  vector<GridAreaManager::SpatialInterpolateArea> va =
      areaManager->getSpatialInterpolateAreas();
  int n = va.size();

  for (size_t i = 0; i < spatialsmoothtv.size(); i++) {
    bool foundit = false;
    for (int j = 0; j < n; j++) {
      if (va[j].id == spatialsmoothtv[i].id) {
        foundit = true;
        spatialsmoothtv[i].polygon = va[j].area.getPolygon();
        break;
      }
    }
    if (!foundit) {
      ERROR_ << "processSpatialSmoothing: Unable to find area for object:"
          << spatialsmoothtv[i].id;
    }
  }

  // save them...
  vector<miutil::miString> deletion_ids;
  processTimeValues(spatialsmoothtv, deletion_ids);
}

void DianaProfetGUI::endSpatialsmooth()
{
  try {
    controller.unlockObjectsByTimeValues(spatialsmoothtv);
    areaManager->clearSpatialInterpolation();
  } catch (Profet::ServerException & se) {
    handleServerException(se);
  }
}

void DianaProfetGUI::paintModeChanged(GridAreaManager::PaintMode mode)
{
  GridAreaManager::PaintMode oldmode = areaManager->getPaintMode();
  if (oldmode == GridAreaManager::SPATIAL_INTERPOLATION) {
    processSpatialsmooth();
    endSpatialsmooth();
  } else if (mode == GridAreaManager::SPATIAL_INTERPOLATION) {
    startSpatialsmooth();
  }
  areaManager->setPaintMode(mode);
}

void DianaProfetGUI::undoCurrentArea()
{
  if (areaManager->isUndoPossible())
    areaManager->undo();
  paintToolBar->enableUndo(areaManager->isUndoPossible());
  paintToolBar->enableRedo(areaManager->isRedoPossible());
  gridAreaChanged();
}

void DianaProfetGUI::redoCurrentArea()
{
  if (areaManager->isRedoPossible())
    areaManager->redo();
  paintToolBar->enableUndo(areaManager->isUndoPossible());
  paintToolBar->enableRedo(areaManager->isRedoPossible());
  gridAreaChanged();
}

void DianaProfetGUI::gridAreaChanged()
{
  LOG4CXX_DEBUG(logger,"gridAreaChanged");
  miutil::miString currentId = areaManager->getCurrentId();
  currentObjectMutex.lock();
  miutil::miString obj_id = currentObject.id();
  currentObjectMutex.unlock();
  // Different object selected on map
  if (currentId != "newArea" && currentId != obj_id) {
    QModelIndex modelIndex = objectModel.getIndexById(currentId);
    if (modelIndex.isValid()) {
      sessionDialog->setSelectedObject(modelIndex);
    }
    return;
  }
  if (areaManager->isAreaSelected()) {
    editObjectDialog->setAreaStatus(ProfetObjectDialog::AREA_OK);
    currentObjectMutex.lock();
    bool obj_exist = currentObject.exist();
    if (obj_exist) {
      objectFactory.setPolygon(currentObject, areaManager->getCurrentPolygon());
      objectFactory.setGuiValues(currentObject,
          editObjectDialog->getCurrentGuiComponents());
    }
    currentObjectMutex.unlock();
    if (!obj_exist) {
      int i = getBaseObjectIndex(editObjectDialog->getSelectedBaseObject());
      if (i != -1) {
        if (areaManager->isAreaSelected()) {
          miutil::miTime refTime;
          try {
            fetSession s = getCurrentSession();
            refTime = s.referencetime();
          } catch (InvalidIndexException & iie) {
            ERROR_ << "DianaProfetGUI::baseObjectSelected invalid session index";
          }
          // TODO: fetch parent from somewhere...
          miutil::miString parent_ = "";
          miutil::miString username = controller.getCurrentUser().name;
          currentObjectMutex.lock();
          currentObject = objectFactory.makeObject(baseObjects[i],
              areaManager->getCurrentPolygon(), getCurrentParameter(),
              getCurrentTime(), editObjectDialog->getReason(), username,
              refTime, parent_);
          currentObjectMutex.unlock();
        } else {
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
    if (paintToolBar->getPaintMode() == GridAreaManager::DRAW_MODE)
      paintToolBar->setPaintMode(GridAreaManager::MOVE_MODE);
    else if (paintToolBar->getPaintMode() == GridAreaManager::ADD_POINT)
      paintToolBar->setPaintMode(GridAreaManager::MOVE_POINT);

  } else {
    editObjectDialog->setAreaStatus(ProfetObjectDialog::AREA_NOT_SELECTED);
  }
  paintToolBar->enableUndo(areaManager->isUndoPossible());
}

void DianaProfetGUI::cancelEditObjectDialog()
{
  // Current area added with createNewObject
  if (areaManager->getCurrentId() == "newArea") {
    areaManager->removeCurrentArea();
  }
  if (!editObjectDialog->showingNewObject()) {
    while (areaManager->undo())
      ;
    gridAreaChanged();
    currentObjectMutex.lock();
    fetObject safeCopy = currentObject;
    currentObjectMutex.unlock();
    try {
      controller.closeObject(safeCopy);
    } catch (Profet::ServerException & se) {
      handleServerException(se);
    }
  }
  setEditObjectDialogVisible(false);
  currentObjectMutex.lock();
  currentObject = fetObject();
  currentObjectMutex.unlock();
  enableObjectButtons(true, false, true);
}

void DianaProfetGUI::setPaintToolBarVisible(bool visible)
{
  showPaintToolBar = visible;
  emit setPaintMode(showPaintToolBar);
  if (showPaintToolBar)
    paintToolBar->show();
  else {
    paintToolBar->hide();
  }
}

void DianaProfetGUI::setViewObjectDialogVisible(bool visible)
{
  showViewObjectDialog = visible;
  if (showViewObjectDialog) {
    logfile.restoreSizeAndPos(viewObjectDialog, "ProfetViewObjectDialog");
    viewObjectDialog->show();
  } else {
    // Logging is in the quit function in the object directly
    viewObjectDialog->hide();
  }
}

void DianaProfetGUI::setEditObjectDialogVisible(bool visible)
{
  showEditObjectDialog = visible;
  //sessionDialog->setEditable(!visible); // doesn't do anything...
  if (showEditObjectDialog) {
    logfile.restoreSizeAndPos(editObjectDialog, "ProfetEditObjectDialog");
    editObjectDialog->show();
  } else {
    // Logging is in the quit function in the object directly
    paintToolBar->enableButtons(PaintToolBar::SELECT_ONLY);
    editObjectDialog->hide();
  }
}

bool DianaProfetGUI::isVisible()
{
  return sessionDialog->isVisible();
}

void DianaProfetGUI::setVisible(bool visible)
{
  LOG4CXX_DEBUG(logger, "setVisible " << visible);
  if (visible) {
    ignoreSynchProblems = true;
    setPaintToolBarVisible(showPaintToolBar);
    emit setPaintMode(showPaintToolBar);
    setEditObjectDialogVisible(showEditObjectDialog);
    // sessionDialog->selectDefault(); too early first time?
    sessionDialog->show();
    emit repaintMap(false);
  } else {
    // First, remove previous PROFET fieldPlot (if any)
    if (getCurrentParameter().length()) {
      emit showProfetField("");
      emit prepareAndPlot();
    }
    setCurrentParam("");
    areaManager->clear();
    emit repaintMap(false);
    sessionDialog->hide();
    editObjectDialog->hide();
    paintToolBar->hide();

    emit setPaintMode(false);
  }
}

int DianaProfetGUI::getBaseObjectIndex(miutil::miString name)
{
  for (size_t i = 0; i < baseObjects.size(); i++)
    if (baseObjects[i].name() == name)
      return int(i);
  return -1;
}

void DianaProfetGUI::setStatistics(map<miutil::miString, float> m)
{
  editObjectDialog->setStatistics(m);
}

void DianaProfetGUI::setActivePoints(list<Point> points)
{
  areaManager->setActivePoints(points);
}

void DianaProfetGUI::rightMouseClicked(float x, float y, int globalX,
    int globalY)
{

  miutil::miString thisAreaId = areaManager->getCurrentId();

  if ( !thisAreaId.exists() ){
    return;
  }

  vector<miutil::miString> areaId = areaManager->getId(Point(x, y));
  unsigned int i=0;
  while (i<areaId.size() && areaId[i]!=thisAreaId) {
    i++;
  }
  if( i==areaId.size()){
    return;
  }

  editObjectAction->setVisible(true);
  deleteObjectAction->setVisible(true);
  startTimesmoothAction->setVisible(true);

  popupMenu->popup(QPoint(globalX, globalY), 0);
}
