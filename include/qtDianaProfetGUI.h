#ifndef QTDIANAPROFETGUI_H_
#define QTDIANAPROFETGUI_H_
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
#include <qobject.h>
#include <QEvent>
#include <QMutex>
#include <QMutexLocker>

#include <diField/diProjectablePolygon.h>
#include <profet/ProfetController.h>
#include <profet/ProfetGUI.h>
#include "qtProfetEvents.h"
#include "qtProfetSessionDialog.h"
#include "qtProfetObjectDialog.h"
#include "diMapMode.h"
#include "diGridAreaManager.h"
#include "diProfetObjectFactory.h"
#include "qtProfetDataModels.h"
#include "qtProfetTimeSmoothDialog.h"

#ifndef NOLOG4CXX
#include <log4cxx/logger.h>
#else
#include <miLogger/logger.h>
#endif

class PaintToolBar;
class QString;
class QMenu;

using namespace Profet;

class DianaProfetGUI : public QObject, public Profet::ProfetGUI  {
  Q_OBJECT
private:
#ifndef NOLOG4CXX
  log4cxx::LoggerPtr logger;
#endif
  ProfetSessionDialog sessionDialog;
  ProfetObjectDialog viewObjectDialog;
  ProfetObjectDialog editObjectDialog;
  GridAreaManager * areaManager;
  PaintToolBar * paintToolBar;
  ProfetObjectFactory objectFactory;
  FetObjectListModel objectModel;
  FetObjectTableModel tableModel;
  vector<fetBaseObject> baseObjects;
  UserListModel userModel;
  SessionListModel sessionModel;
  fetObject  currentObject;
  fetSession currentSession;
  QMenu * popupMenu;

  //to synchronize currentObject accessed by multiple threads
  mutable QMutex currentObjectMutex;
  //to synchronize access to currentParam/curentTime
  mutable QMutex currentParamTimeMutex;
  // Cached because of many requests from multiple threads
  miString currentParam;
  miTime currentTime;
  // needed to keep 'show-status' after hide
  bool showPaintToolBar;
  bool showViewObjectDialog;
  bool showEditObjectDialog;
  bool activeTimeSmooth;
  bool overviewactive;

  bool enableNewbutton_;
  bool enableModifyButtons_;
  bool enableTable_;

  bool ignoreSynchProblems;

  void connectSignals();
  /// Synchronized set'ers
  void setCurrentParam(const miString & p);
  void setCurrentTime(const miTime & t);
  /// gets baseObjects index by name. returns -1 if not found
  int getBaseObjectIndex(miString name);
  /// enable/disable gui elements
  void enableObjectButtons(bool enableNewbutton,
			   bool enableModifyButtons,
			   bool enableTable);

  void handleServerException(Profet::ServerException & se);

  QWidget* parent;
public:
  DianaProfetGUI(Profet::ProfetController & pc,
		 PaintToolBar * ptb, GridAreaManager * gam, QWidget * p);
  virtual ~DianaProfetGUI();
  /**
   * Resets status information (users, objects ... )
   * Used when disconnecting from data manager
   */
  void resetStatus();
  void setCurrentSession(const fetSession & session);
  void setBaseObjects(vector<fetBaseObject> objects);
  void setUser(const Profet::PodsUser & user);
  void removeUser(const Profet::PodsUser & user);
  void setUsers(const vector<Profet::PodsUser> & users);
  void setSession(const fetSession & session, bool remove = false);
  void showMessage(const Profet::InstantMessage & msg);
  void updateObject(const fetObject & object, bool remove = false);
  void updateObjects(const vector<fetObject> & objects);
  void updateObjectSignature(
      const fetObject::Signature & s, bool remove = false);
  void updateObjectSignatures(
      const vector<fetObject::Signature> & s);

  void customEvent(QEvent * e);
  /**
   * Plots specified field in Diana.
   * Field must be prepared in field-manager first
   */
  void showField(const miTime & reftime, const miString & param, const miTime & time);
  void setBaseProjection(Area a, int size_x, int size_y);
  /**
   * Repaints the Diana Map
   */
  void updateMap();
  /**
   * Gets the selected parameter in GUI
   */
  miString getCurrentParameter();
  /**
   * Gets the selected time in GUI
   */
  miTime getCurrentTime();
  /**
   * Gets the selected session in GUI
   */
  fetSession getCurrentSession();
  /**
   * True if any Profet GUI is visible
   */
  bool isVisible();
  /**
   * Show or hide Profet GUI
   */
  void setVisible(bool visible);
  /**
   * Show or hide Paint toolbar
   */
  void setPaintToolBarVisible(bool visible);
  /**
   * Show or hide view object dialog
   */
  void setViewObjectDialogVisible(bool visible);
  /**
   * Show or hide edit object dialog
   */
  void setEditObjectDialogVisible(bool visible);
  /**
   * Gets the current object from Diana GUI
   */
  fetObject getActiveObject(){
    QMutexLocker locker(&currentObjectMutex);
    return currentObject;
  }

  /**
   * set some statistic information for the current object
   */
  void setStatistics(map<miString,float>);

  /**
   * set the list of Points which are actually affected by the active
   * object/mask
   */
  void setActivePoints(vector<Point>);

  /**
   * Right mouse clicked, make popup menu
   */
  void rightMouseClicked(float x, float y, int globalX, int globalY);

  bool selectTime(miTime time);


private slots:
  // EditObjectDialog
  void baseObjectSelected(miString name);
  void objectSelected(const QModelIndex &);
  void saveObject();
  void copyPolygon(miString,miString,bool);
  void selectPolygon(miString);
  void requestPolygonList();
  void collectRelatedTimeValues(vector<fetObject::TimeValues>& tv, miString id_="", bool withPolygon=false);
  void startTimesmooth();
  void processTimesmooth(vector<fetObject::TimeValues> tv);
  void processTimeValues(vector<fetObject::TimeValues> tv,vector<miString> del_ids);
  void endTimesmooth(vector<fetObject::TimeValues> tv);

  void cancelEditObjectDialog();
  void dynamicGuiChanged(); //properties??


  // SessionDialog
  void sessionSelected(int index);
  void sendMessage(const QString &);
  void paramAndTimeSelected(const QModelIndex &);
  void toggleViewObject(bool);
  void createNewObject();
  void editObject();
  void deleteObject();
  void hideProfetPerformed();
  void doReconnect();
//  void doUpdate();
  void showObjectOverview(const QList<QModelIndex> &);
  void toggleObjectOverview(bool turnon, miString par, miTime time);

  // PaintToolBar
  void paintModeChanged(GridAreaManager::PaintMode mode);
  void undoCurrentArea();
  void redoCurrentArea();

  // MainWindow
  void gridAreaChanged();

  // sessionModel
  void sessionModified(const QModelIndex & topLeft, const QModelIndex & bottomRight);

  //popup
  void popupMenuActivated(int);

signals:
  void setPaintMode(bool);
  void showProfetField(miString field);
  void emitTimes( const miString& ,const vector<miTime>& );
  void setTime(const miTime & t);
  void repaintMap(bool onlyObjects);
  void toggleProfetGui();

  void timesmoothProcessed(miTime, miString);

  void updateModelDefinitions();
  void prepareAndPlot();
  void forceDisconnect(bool disableGuiOnly);

};

#endif /*QTDIANAPROFETGUI_H_*/
