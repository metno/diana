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
#include <puTools/miString.h>
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
#include <paint_help.xpm>

miutil::miString PaintToolBar::helpPageName = "ug_profetdrawingtools.html";

PaintToolBar::PaintToolBar(QMainWindow *parent)
			: QToolBar(tr("Paint Operations"), parent) {

  selectAction = new QAction( QPixmap(paint_select_xpm),tr("&Select"),this );
  selectAction->setShortcut(Qt::Key_1);
  selectAction->setCheckable(true);
  connect( selectAction, SIGNAL( triggered() ), SLOT( sendPaintModeChanged() ) );
  drawAction = new QAction( QPixmap(paint_draw_xpm),tr("&Draw"),this );
  drawAction->setShortcut(Qt::Key_2);
  drawAction->setCheckable(true);
  connect( drawAction, SIGNAL( triggered() ), SLOT( sendPaintModeChanged() ) );
  includeAction = new QAction( QPixmap(paint_include_xpm),tr("&Include"),this );
  includeAction->setShortcut(Qt::Key_3);
  includeAction->setCheckable(true);
  connect( includeAction, SIGNAL( triggered() ), SLOT( sendPaintModeChanged() ) );
  cutAction = new QAction( QPixmap(paint_cut_xpm),tr("&Cut"),this );
  cutAction->setShortcut(Qt::Key_4);
  cutAction->setCheckable(true);
  connect( cutAction, SIGNAL( triggered() ), SLOT( sendPaintModeChanged() ) );
  moveAction = new QAction( QPixmap(paint_move_xpm),tr("&Move"),this );
  moveAction->setShortcut(Qt::Key_5);
  moveAction->setCheckable(true);
  connect( moveAction, SIGNAL( triggered() ), SLOT( sendPaintModeChanged() ) );
  addPointAction = new QAction( QPixmap(paint_add_point_xpm),tr("&Add Point"),this );
  addPointAction->setShortcut(Qt::Key_6);
  addPointAction->setCheckable(true);
  connect( addPointAction, SIGNAL( triggered() ), SLOT( sendPaintModeChanged() ) );
  removePointAction = new QAction( QPixmap(paint_remove_point_xpm),tr("&Remove Point"),this );
  removePointAction->setShortcut(Qt::Key_7);
  removePointAction->setCheckable(true);
  connect( removePointAction, SIGNAL( triggered() ), SLOT( sendPaintModeChanged() ) );
  movePointAction = new QAction( QPixmap(paint_move_point_xpm),tr("&Move Point"),this );
  movePointAction->setShortcut(Qt::Key_8);
  movePointAction->setCheckable(true);
  connect( movePointAction, SIGNAL( triggered() ), SLOT( sendPaintModeChanged() ) );
  spatialAction = new QAction( QPixmap(paint_spatial_xpm),tr("&Spatial Interpolation"),this );
  spatialAction->setShortcut(Qt::Key_9);
  spatialAction->setCheckable(true);
  connect( spatialAction, SIGNAL( triggered() ), SLOT( sendPaintModeChanged() ) );
  undoAction = new QAction( QPixmap(paint_undo_xpm),tr("&Undo"),this );
  undoAction->setShortcut(QKeySequence::Undo);
  connect( undoAction, SIGNAL( triggered() ), SIGNAL( undoPressed() ) );
  redoAction = new QAction( QPixmap(paint_redo_xpm),tr("&Redo"),this );
  redoAction->setShortcut(QKeySequence::Redo);
  connect( redoAction, SIGNAL( triggered() ), SIGNAL( redoPressed() ) );
  displayHelpAction = new QAction( QPixmap(paint_help_xpm),tr("&Help"),this );
  connect( displayHelpAction, SIGNAL( triggered() ), SLOT( helpPressed() ) );

  modeActions = new QActionGroup(this);
  modeActions->addAction(selectAction);
  modeActions->addAction(drawAction);
  modeActions->addAction(includeAction);
  modeActions->addAction(cutAction);
  modeActions->addAction(moveAction);
  modeActions->addAction(addPointAction);
  modeActions->addAction(removePointAction);
  modeActions->addAction(movePointAction);
  modeActions->addAction(spatialAction);

  addAction(selectAction);
  addSeparator();
  addAction(drawAction);
  addAction(includeAction);
  addAction(cutAction);
  addSeparator();
  addAction(moveAction);
  addSeparator();
  addAction(addPointAction);
  addAction(removePointAction);
  addAction(movePointAction);
  addSeparator();
  addAction(spatialAction);
  addSeparator();
  addAction(undoAction);
  addAction(redoAction);
  addSeparator();
  addAction(displayHelpAction);

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
		drawAction->setChecked(true);
	else if(newMode == GridAreaManager::INCLUDE_MODE)
		includeAction->setChecked(true);
	else if(newMode == GridAreaManager::CUT_MODE)
		cutAction->setChecked(true);
	else if(newMode == GridAreaManager::MOVE_MODE)
		moveAction->setChecked(true);
  else if(newMode == GridAreaManager::SPATIAL_INTERPOLATION)
    spatialAction->setChecked(true);
	else if(newMode == GridAreaManager::SELECT_MODE)
		selectAction->setChecked(true);
  else if(newMode == GridAreaManager::ADD_POINT)
    addPointAction->setChecked(true);
  else if(newMode == GridAreaManager::REMOVE_POINT)
    removePointAction->setChecked(true);
  else if(newMode == GridAreaManager::MOVE_POINT)
    movePointAction->setChecked(true);
	sendPaintModeChanged();
}

GridAreaManager::PaintMode PaintToolBar::getPaintMode(){
	if(includeAction->isChecked()) return GridAreaManager::INCLUDE_MODE;
	else if(cutAction->isChecked()) return GridAreaManager::CUT_MODE;
	else if(moveAction->isChecked()) return GridAreaManager::MOVE_MODE;
	else if(spatialAction->isChecked()) return GridAreaManager::SPATIAL_INTERPOLATION;
	else if(selectAction->isChecked()) return GridAreaManager::SELECT_MODE;
	else if(addPointAction->isChecked()) return GridAreaManager::ADD_POINT;
  else if(removePointAction->isChecked()) return GridAreaManager::REMOVE_POINT;
  else if(movePointAction->isChecked()) return GridAreaManager::MOVE_POINT;
	else return GridAreaManager::DRAW_MODE;
}

bool PaintToolBar::isPaintEnabled() {
  // Painting is possible whenever draw action is enabled
  return drawAction->isChecked();
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

void PaintToolBar::helpPressed() {
  emit showsource(helpPageName,"");
}
