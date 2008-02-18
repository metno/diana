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
#include <profet/ProfetController.h>
#include <profet/ProfetGUI.h>
#include "qtProfetEvents.h"
#include "qtProfetSessionDialog.h"
#include "qtProfetObjectDialog.h"
#include "diMapMode.h"
#include "diGridAreaManager.h"
#include "diProfetObjectFactory.h"

#ifndef NOLOG4CXX
#include <log4cxx/logger.h>
#else
#include <miLogger/logger.h>
#endif

class PaintToolBar;
class QString;

class DianaProfetGUI : public QObject, public Profet::ProfetGUI  {
  Q_OBJECT
private:
#ifndef NOLOG4CXX
  log4cxx::LoggerPtr logger;
#endif
  ProfetSessionDialog sessionDialog;
  ProfetObjectDialog objectDialog;
  GridAreaManager * areaManager;
  PaintToolBar * paintToolBar;
  ProfetObjectFactory objectFactory;
  vector<fetObject> objects;
  vector<fetBaseObject> baseObjects;
  fetObject currentObject;
  miString currentParam;
  // needed to keep 'show-status' after hide
  bool showPaintToolBar;
  bool showObjectDialog;
  
  void connectSignals();
  /// gets baseObjects index by name. returns -1 if not found
  int getBaseObjectIndex(miString name);
  int getObjectIndex(miString id);
  
  miString user;
  
public:
  DianaProfetGUI(Profet::ProfetController & pc, 
		 PaintToolBar * ptb, GridAreaManager * gam, QWidget * parent);
  virtual ~DianaProfetGUI();
  
  void setSessionInfo(fetModel model, vector<fetParameter> parameters,
		      fetSession session);
  void setBaseObjects(vector<fetBaseObject> objects);
  void setUsers(vector<Profet::PodsUser> users);
  void showMessage(const Profet::InstantMessage & msg);
  void setObjects(vector<fetObject> objects);
  void setObjectSignatures( vector<fetObject::Signature> s);

  void customEvent(QEvent * e);
  /**
   * Plots specified field in Diana.
   * Field must be prepared in field-manager first
   */
  void showField(miString param, miTime time);
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
   * Show or hide object dialog
   */
  void setObjectDialogVisible(bool visible);
  /**
   * Gets the current object from Diana GUI
   */
  fetObject getActiveObject(){
    return currentObject;
  }
	
	
  /**
   * set some statistic information for the current object
   */
  void setStatistics(map<miString,float>);
  
  /**
   * set the list of Points which are actually affected bye the active
   * object/mask
   */
  void setActivePoints(vector<Point>);


		  
private slots:
// ObjectDialog
  void baseObjectSelected(miString name);
  void objectSelected(miString id);
  void saveObject();
//   void deleteObject(miString id);
  void cancelObjectDialog();
  void dynamicGuiChanged(); //properties??
  
  // SessionDialog
  void sendMessage(const QString &);
  void paramAndTimeSelected(miString p, miTime t);
  void createNewObject();
  void editObject();
  void deleteObject();
  void hideProfetPerformed();
  
// PaintToolBar
  void paintModeChanged(GridAreaManager::PaintMode mode);
  
  // MainWindow	
  void gridAreaChanged();
	
signals:
  void setPaintMode(bool);
  void showProfetField(miString field);
  void setTime(const miTime & t);
  void repaintMap(bool onlyObjects);
  void toggleProfetGui();
};

#endif /*QTDIANAPROFETGUI_H_*/
