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
//#include <paint_hide.xpm>
#include <paint_cut.xpm>
//#include <paint_undo.xpm>
#include <paint_include.xpm>
#include <paint_draw.xpm>
//#include <paint_color.xpm>

PaintToolBar::PaintToolBar(QMainWindow *parent) 
			: QToolBar(tr("Paint Operations"), parent) {
	
  selectAction = new QAction( QPixmap(paint_select_xpm),tr("&Select"),this );
  selectAction->setToggleAction(true);
  connect( selectAction, SIGNAL( activated() ), SLOT( sendPaintModeChanged() ) );
  drawAction = new QAction( QPixmap(paint_draw_xpm),tr("&Draw"),this );
  drawAction->setToggleAction(true);
  connect( drawAction, SIGNAL( activated() ), SLOT( sendPaintModeChanged() ) );
  includeAction = new QAction( QPixmap(paint_include_xpm),tr("&Include"),this );
  includeAction->setToggleAction(true);
  connect( includeAction, SIGNAL( activated() ), SLOT( sendPaintModeChanged() ) );
  cutAction = new QAction( QPixmap(paint_cut_xpm),tr("&Cut"),this );
  cutAction->setToggleAction(true);
  connect( cutAction, SIGNAL( activated() ), SLOT( sendPaintModeChanged() ) );
  moveAction = new QAction( QPixmap(paint_move_xpm),tr("&Move"),this );
  moveAction->setToggleAction(true);
  connect( moveAction, SIGNAL( activated() ), SLOT( sendPaintModeChanged() ) );
	
  modeActions = new QActionGroup(this);
  modeActions->add(selectAction);
  modeActions->add(drawAction);
  modeActions->add(includeAction);
  modeActions->add(cutAction);
  modeActions->add(moveAction);
	
  selectAction->addTo(this);
  drawAction->addTo(this);
  includeAction->addTo(this);
  cutAction->addTo(this);
  moveAction->addTo(this);
}

void PaintToolBar::enableButtons(PaintToolBarButtons buttons){
  drawAction->setEnabled(false);
  includeAction->setEnabled(false);
  cutAction->setEnabled(false);
  moveAction->setEnabled(false);
  selectAction->setEnabled(false);
  
  if(buttons == PaintToolBar::SELECT_ONLY){
    setPaintMode(GridAreaManager::SELECT_MODE);
    selectAction->setEnabled(true);
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
  }
  else if(buttons == PaintToolBar::ALL){
    drawAction->setEnabled(true);
    includeAction->setEnabled(true);
    cutAction->setEnabled(true);
    moveAction->setEnabled(true);
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
	else if(newMode == GridAreaManager::SELECT_MODE)
		selectAction->setOn(true);
	sendPaintModeChanged();
}

GridAreaManager::PaintMode PaintToolBar::getPaintMode(){
	if(includeAction->isOn()) return GridAreaManager::INCLUDE_MODE;
	if(cutAction->isOn()) return GridAreaManager::CUT_MODE;
	if(moveAction->isOn()) return GridAreaManager::MOVE_MODE;
	if(selectAction->isOn()) return GridAreaManager::SELECT_MODE;
	else return GridAreaManager::DRAW_MODE;
}

void PaintToolBar::sendPaintModeChanged(){
  emit paintModeChanged(getPaintMode());
}
















