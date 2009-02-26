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
#include <qtPaintToolBar.h>
#include <qpixmap.h>
#include <qkeysequence.h> 
#include <QActionGroup>
#include <miString.h>
#include <paint_select.xpm>
#include <paint_move.xpm>
#include <paint_spatial.xpm>
//#include <paint_hide.xpm>
#include <paint_cut.xpm>
#include <paint_undo.xpm>
#include <paint_redo.xpm>
#include <paint_include.xpm>
#include <paint_draw.xpm>
//#include <paint_color.xpm>
#include <paint_add_point.xpm>
#include <paint_remove_point.xpm>
#include <paint_move_point.xpm>

PaintToolBar::PaintToolBar(QMainWindow *parent) 
			: QToolBar(tr("Paint Operations"), parent) {
	
  selectAction = new QAction( QPixmap(paint_select_xpm),tr("&Select"),this );
  selectAction->setShortcut(Qt::Key_1);
  selectAction->setToggleAction(true);
  connect( selectAction, SIGNAL( activated() ), SLOT( sendPaintModeChanged() ) );
  drawAction = new QAction( QPixmap(paint_draw_xpm),tr("&Draw"),this );
  drawAction->setShortcut(Qt::Key_2);
  drawAction->setToggleAction(true);
  connect( drawAction, SIGNAL( activated() ), SLOT( sendPaintModeChanged() ) );
  includeAction = new QAction( QPixmap(paint_include_xpm),tr("&Include"),this );
  includeAction->setShortcut(Qt::Key_3);
  includeAction->setToggleAction(true);
  connect( includeAction, SIGNAL( activated() ), SLOT( sendPaintModeChanged() ) );
  cutAction = new QAction( QPixmap(paint_cut_xpm),tr("&Cut"),this );
  cutAction->setShortcut(Qt::Key_4);
  cutAction->setToggleAction(true);
  connect( cutAction, SIGNAL( activated() ), SLOT( sendPaintModeChanged() ) );
  moveAction = new QAction( QPixmap(paint_move_xpm),tr("&Move"),this );
  moveAction->setShortcut(Qt::Key_5);
  moveAction->setToggleAction(true);
  connect( moveAction, SIGNAL( activated() ), SLOT( sendPaintModeChanged() ) );
  addPointAction = new QAction( QPixmap(paint_add_point_xpm),tr("&Add Point"),this );
  addPointAction->setShortcut(Qt::Key_6);
  addPointAction->setToggleAction(true);
  connect( addPointAction, SIGNAL( activated() ), SLOT( sendPaintModeChanged() ) );
  removePointAction = new QAction( QPixmap(paint_remove_point_xpm),tr("&Remove Point"),this );
  removePointAction->setShortcut(Qt::Key_7);
  removePointAction->setToggleAction(true);
  connect( removePointAction, SIGNAL( activated() ), SLOT( sendPaintModeChanged() ) );
  movePointAction = new QAction( QPixmap(paint_move_point_xpm),tr("&Move Point"),this );
  movePointAction->setShortcut(Qt::Key_8);
  movePointAction->setToggleAction(true);
  connect( movePointAction, SIGNAL( activated() ), SLOT( sendPaintModeChanged() ) );
  spatialAction = new QAction( QPixmap(paint_spatial_xpm),tr("&Spatial Interpolation"),this );
  spatialAction->setShortcut(Qt::Key_9);
  spatialAction->setToggleAction(true);
  connect( spatialAction, SIGNAL( activated() ), SLOT( sendPaintModeChanged() ) );
  undoAction = new QAction( QPixmap(paint_undo_xpm),tr("&Undo"),this );
  undoAction->setShortcut(QKeySequence::Undo);
  connect( undoAction, SIGNAL( activated() ), SIGNAL( undoPressed() ) );
  redoAction = new QAction( QPixmap(paint_redo_xpm),tr("&Redo"),this );
  redoAction->setShortcut(QKeySequence::Redo);
  connect( redoAction, SIGNAL( activated() ), SIGNAL( redoPressed() ) );
	
  modeActions = new QActionGroup(this);
  modeActions->add(selectAction);
  modeActions->add(drawAction);
  modeActions->add(includeAction);
  modeActions->add(cutAction);
  modeActions->add(moveAction);
  modeActions->add(addPointAction);
  modeActions->add(removePointAction);
  modeActions->add(movePointAction);
  modeActions->add(spatialAction);
	
  selectAction->addTo(this);
  addSeparator();
  drawAction->addTo(this);
  includeAction->addTo(this);
  cutAction->addTo(this);
  addSeparator();
  moveAction->addTo(this);
  addSeparator();
  addPointAction->addTo(this);
  removePointAction->addTo(this);
  movePointAction->addTo(this);
  addSeparator();
  spatialAction->addTo(this);
  addSeparator();
  undoAction->addTo(this);
  redoAction->addTo(this);
  
  enableUndo(false);
  enableRedo(false);
}

void PaintToolBar::enableButtons(PaintToolBarButtons buttons){
  drawAction->setEnabled(false);
  includeAction->setEnabled(false);
  cutAction->setEnabled(false);
  moveAction->setEnabled(false);
  spatialAction->setEnabled(false);
  selectAction->setEnabled(false);
  addPointAction->setEnabled(false);
  removePointAction->setEnabled(false);
  movePointAction->setEnabled(false);
  
  if(buttons == PaintToolBar::SELECT_ONLY){
    setPaintMode(GridAreaManager::SELECT_MODE);
    selectAction->setEnabled(true);  
    enableUndo(false);
    enableRedo(false);
  }
  else if(buttons == PaintToolBar::PAINT_ONLY){
    setPaintMode(GridAreaManager::DRAW_MODE);
    drawAction->setEnabled(true);
  }
  else if(buttons == PaintToolBar::PAINT_AND_MODIFY){
    if(getPaintMode() == GridAreaManager::SELECT_MODE){
      setPaintMode(GridAreaManager::DRAW_MODE);
    }
    drawAction->setEnabled(true);
    includeAction->setEnabled(true);
    cutAction->setEnabled(true);
    moveAction->setEnabled(true);
    spatialAction->setEnabled(true);
    addPointAction->setEnabled(true);
    removePointAction->setEnabled(true);
    movePointAction->setEnabled(true);
  }
  else if(buttons == PaintToolBar::ALL){
    drawAction->setEnabled(true);
    includeAction->setEnabled(true);
    cutAction->setEnabled(true);
    moveAction->setEnabled(true);
    spatialAction->setEnabled(true);
    addPointAction->setEnabled(true);
    removePointAction->setEnabled(true);
    movePointAction->setEnabled(true);
    selectAction->setEnabled(true);
  }
}

void PaintToolBar::setPaintMode(GridAreaManager::PaintMode newMode){
	if(newMode == GridAreaManager::DRAW_MODE)
		drawAction->setOn(true);
	else if(newMode == GridAreaManager::INCLUDE_MODE)
		includeAction->setOn(true);
	else if(newMode == GridAreaManager::CUT_MODE)
		cutAction->setOn(true);
	else if(newMode == GridAreaManager::MOVE_MODE)
		moveAction->setOn(true);
  else if(newMode == GridAreaManager::SPATIAL_INTERPOLATION)
    spatialAction->setOn(true);
	else if(newMode == GridAreaManager::SELECT_MODE)
		selectAction->setOn(true);
  else if(newMode == GridAreaManager::ADD_POINT)
    addPointAction->setOn(true);
  else if(newMode == GridAreaManager::REMOVE_POINT)
    removePointAction->setOn(true);
  else if(newMode == GridAreaManager::MOVE_POINT)
    movePointAction->setOn(true);
	sendPaintModeChanged();
}

GridAreaManager::PaintMode PaintToolBar::getPaintMode(){
	if(includeAction->isOn()) return GridAreaManager::INCLUDE_MODE;
	else if(cutAction->isOn()) return GridAreaManager::CUT_MODE;
	else if(moveAction->isOn()) return GridAreaManager::MOVE_MODE;
	else if(spatialAction->isOn()) return GridAreaManager::SPATIAL_INTERPOLATION;
	else if(selectAction->isOn()) return GridAreaManager::SELECT_MODE;
	else if(addPointAction->isOn()) return GridAreaManager::ADD_POINT;
  else if(removePointAction->isOn()) return GridAreaManager::REMOVE_POINT;
  else if(movePointAction->isOn()) return GridAreaManager::MOVE_POINT;
	else return GridAreaManager::DRAW_MODE;
}

bool PaintToolBar::isPaintEnabled() {
  // Painting is possible whenever draw action is enabled
  return drawAction->isOn();
}

void PaintToolBar::sendPaintModeChanged(){
  emit paintModeChanged(getPaintMode());
}

void PaintToolBar::enableUndo(bool enable){
  if(drawAction->isEnabled())
    undoAction->setEnabled(enable);
  else 
    undoAction->setEnabled(false);
}

void PaintToolBar::enableRedo(bool enable){
  if(drawAction->isEnabled())
    redoAction->setEnabled(enable);
  else 
    redoAction->setEnabled(false);
}
