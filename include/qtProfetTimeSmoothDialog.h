#ifndef QTPROFETTIMESMOOTHDIALOG_H
#define QTPROFETTIMESMOOTHDIALOG_H

/*
  $Id$

  Copyright (C) 2006 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <QMainWindow>
#include <QAction>
#include <QScrollArea>
#include <QMenuBar>
#include <QMenu>
#include <QSignalMapper>

#include <puTools/miTime.h>
#include <puTools/miString.h>

#include <vector>
#include <map>

#include <qtProfetTimeControl.h>


using namespace std;


class ProfetTimeSmoothDialog : public QMainWindow {
Q_OBJECT
private:
  
  QMenu            *actionmenu;
  QMenu            *methodmenu;
  QMenu            *parametermenu;  
  
  QAction          *interpolationCopyAction;
//  QAction          *interpolationGaussAction;
  QAction          *interpolationLinearAction;
  QAction          *interpolationLineResetAction;
  QAction          *interpolationSingleResetAction;
  
  QActionGroup     *methodGroup;
  
  QAction          *runAction;    
  QAction          *undoAction;   
  QAction          *redoAction;   
  QAction          *quitAction; 
  
  QScrollArea       *scrolla;
  ProfetTimeControl *control;
  QSignalMapper     *parameterSignalMapper;
  
public:
  ProfetTimeSmoothDialog(QWidget* p, vector<fetObject::TimeValues>& obj,vector<miTime>& tim);
  
public slots:
  void processed(miTime tim, miString obj_id); 

private slots: 
  void warn(miString w);
  void undo();
  void redo();
  void run();
  void quit();  
  void setMethodLinear();
  void setMethodCopy();
  void setMethodGauss();
  void setMethodSingleReset();
  void setMethodLineReset();
  void toggleParameters(const QString& pname);
signals:
  void runObjects(vector<fetObject::TimeValues> obj);  
  
};

#endif
