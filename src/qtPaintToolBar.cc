#include <qtPaintToolBar.h>
#include <qpixmap.h>
#include <qkeysequence.h> 
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
			: QToolBar(tr("Paint Operations"), parent,QMainWindow::Bottom, FALSE,"painttoolbar") {
	
  selectAction = new QAction( QPixmap(paint_select_xpm),tr("&Select"),QKeySequence(), this );
	selectAction->setToggleAction(true);
	connect( selectAction, SIGNAL( activated() ) , 
	    this, SLOT( sendPaintModeChanged() ) );
	drawAction = new QAction( QPixmap(paint_draw_xpm),tr("&Draw"),QKeySequence(), this );
	drawAction->setToggleAction(true);
	connect( drawAction, SIGNAL( activated() ) , 
      this, SLOT( sendPaintModeChanged() ) );
	includeAction = new QAction( QPixmap(paint_include_xpm),tr("&Include"),QKeySequence(), this );
	includeAction->setToggleAction(true);
	connect( includeAction, SIGNAL( activated() ) ,
      this, SLOT( sendPaintModeChanged() ) );
	cutAction = new QAction( QPixmap(paint_cut_xpm),tr("&Cut"),QKeySequence(), this );
	cutAction->setToggleAction(true);
	connect( cutAction, SIGNAL( activated() ) , 
      this, SLOT( sendPaintModeChanged() ) );
	moveAction = new QAction( QPixmap(paint_move_xpm),tr("&Move"),QKeySequence(), this );
	moveAction->setToggleAction(true);
	connect( moveAction, SIGNAL( activated() ) , 
      this, SLOT( sendPaintModeChanged() ) );
	
	modeActions = new QActionGroup(this,0);
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

