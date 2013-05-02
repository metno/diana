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
#ifndef QTPAINTTOOLBAR_H_
#define QTPAINTTOOLBAR_H_

#include <QToolBar>
#include <QMainWindow>
#include <QAction>
#include <QActionGroup>
#include <diGridAreaManager.h>
#include <puTools/miString.h>


class PaintToolBar : public QToolBar {
  Q_OBJECT

public:
  enum PaintToolBarButtons{ALL,SELECT_ONLY,PAINT_ONLY,PAINT_AND_MODIFY};  
  
public:
  PaintToolBar(QMainWindow *parent);
	GridAreaManager::PaintMode getPaintMode();
	void enableButtons(PaintToolBarButtons);
	bool isPaintEnabled();
	void enableUndo(bool enable);
  void enableRedo(bool enable);
	
private:
  static miutil::miString helpPageName;
  QActionGroup *modeActions;
  QAction *selectAction;
  QAction *drawAction;
  QAction *includeAction;
  QAction *cutAction;
  QAction *moveAction;
  QAction *addPointAction;
  QAction *removePointAction;
  QAction *movePointAction;
  QAction *spatialAction;
  QAction *undoAction;
  QAction *redoAction;
  QAction *displayHelpAction;
  
private slots:
  void sendPaintModeChanged();
  void helpPressed();
  
public slots:
	void setPaintMode(GridAreaManager::PaintMode);
	  	
signals:
  void paintModeChanged(GridAreaManager::PaintMode mode);
  void undoPressed();
  void redoPressed();
  // display help (using existing name convention)
  void showsource(const std::string source,const std::string tag);
};

#endif /*QTPAINTTOOLBAR_H_*/
