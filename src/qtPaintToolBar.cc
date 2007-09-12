#include <qtPaintToolBar.h>
#include <qpixmap.h>
#include <qkeysequence.h> 
#include <miString.h>
#include <paint_select.xpm>
#include <paint_new.xpm>
#include <paint_delete.xpm>
#include <paint_move.xpm>
#include <paint_hide.xpm>
#include <paint_cut.xpm>
//#include <paint_undo.xpm>
#include <paint_include.xpm>
#include <paint_draw.xpm>
//#include <paint_color.xpm>

PaintToolBar::PaintToolBar(QMainWindow *parent, GridAreaManager *gam) 
			: QToolBar(tr("Paint Operations"), parent,QMainWindow::Bottom, FALSE,"painttoolbar") {
	areaManager = gam;
	mainwindow = parent; 
	
	
    selectAction = new QAction( QPixmap(paint_select_xpm),tr("&Select"),QKeySequence(), this );
	selectAction->setToggleAction(true);
	connect( selectAction, SIGNAL( activated() ) , this, SLOT( modeChanged() ) );
//  newAction = new QAction( QPixmap(paint_new_xpm),tr("&New"),QKeySequence(), this );
//	newAction->setToggleAction(false);
//	connect( newAction, SIGNAL( activated() ) , this, SLOT( newActionPerformed() ) );
//  deleteAction = new QAction( QPixmap(paint_delete_xpm),tr("&Delete"),QKeySequence(), this );
//	deleteAction->setToggleAction(false);
//	connect( deleteAction, SIGNAL( activated() ) , this, SLOT( deleteActionPerformed() ) );
	drawAction = new QAction( QPixmap(paint_draw_xpm),tr("&Draw"),QKeySequence(), this );
	drawAction->setToggleAction(true);
	connect( drawAction, SIGNAL( activated() ) , this, SLOT( modeChanged() ) );
	includeAction = new QAction( QPixmap(paint_include_xpm),tr("&Include"),QKeySequence(), this );
	includeAction->setToggleAction(true);
	connect( includeAction, SIGNAL( activated() ) , this, SLOT( modeChanged() ) );
	cutAction = new QAction( QPixmap(paint_cut_xpm),tr("&Cut"),QKeySequence(), this );
	cutAction->setToggleAction(true);
	connect( cutAction, SIGNAL( activated() ) , this, SLOT( modeChanged() ) );
	moveAction = new QAction( QPixmap(paint_move_xpm),tr("&Move"),QKeySequence(), this );
	moveAction->setToggleAction(true);
	connect( moveAction, SIGNAL( activated() ) , this, SLOT( modeChanged() ) );
//	colorAction = new QAction( QPixmap(paint_color_xpm),tr("&Color"),QKeySequence(), this );
//	colorAction->setToggleAction(false);
//	connect( colorAction, SIGNAL( activated() ) , this, SLOT( colorActionPerformed() ) );
//	undoAction = new QAction( QPixmap(paint_undo_xpm),tr("&Undo"),QKeySequence(), this );
//	undoAction->setToggleAction(false);
//	connect( undoAction, SIGNAL( activated() ) , this, SLOT( undoActionPerformed() ) );
//	closeAction = new QAction( QPixmap(paint_hide_xpm),tr("&Hide"),QKeySequence(), this );
//	closeAction->setToggleAction(false);
//	connect( closeAction, SIGNAL( activated() ) , this, SLOT( closeActionPerformed() ) );
	
	modeActions = new QActionGroup(this,0);
	modeActions->add(selectAction);
	modeActions->add(drawAction);
	modeActions->add(includeAction);
	modeActions->add(cutAction);
	modeActions->add(moveAction);
	
	selectAction->addTo(this);
//	newAction->addTo(this);
//	deleteAction->addTo(this);
//	addSeparator();
	drawAction->addTo(this);
	includeAction->addTo(this);
	cutAction->addTo(this);
	moveAction->addTo(this);
//	colorAction->addTo(this);
//	addSeparator();
//	undoAction->addTo(this);
//	closeAction->addTo(this);
	setPaintMode(areaManager->getPaintMode());
	catchGridAreaChanged();
//	colorAction->setEnabled(true);
//	undoAction->setEnabled(areaManager->isUndoPossible());
}

void PaintToolBar::modeChanged(){
	areaManager->setPaintMode(getPaintMode());
}
/*
void PaintToolBar::newActionPerformed(){
	miString b = areaManager->addNextArea();
	setPaintMode(GridAreaManager::DRAW_MODE);
	catchGridAreaChanged();
	emit updateGridAreaPlot();
}
*/
/*
void PaintToolBar::deleteActionPerformed(){
	areaManager->removeCurrentArea();
	emit updateGridAreaPlot();
}
*/
/*	
void PaintToolBar::undoActionPerformed(){
	areaManager->undo();
	catchGridAreaChanged();
	emit updateGridAreaPlot();
}
*/
/*
void PaintToolBar::closeActionPerformed(){
	emit hidePaintToolBar();
}
*/
void PaintToolBar::setPaintMode(int newMode){
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
	modeChanged();
}

GridAreaManager::PaintMode PaintToolBar::getPaintMode(){
	if(includeAction->isOn()) return GridAreaManager::INCLUDE_MODE;
	if(cutAction->isOn()) return GridAreaManager::CUT_MODE;
	if(moveAction->isOn()) return GridAreaManager::MOVE_MODE;
	if(selectAction->isOn()) return GridAreaManager::SELECT_MODE;
	else return GridAreaManager::DRAW_MODE;
}

void PaintToolBar::catchGridAreaChanged(){
	bool selected = areaManager->isAreaSelected();
	includeAction->setEnabled(selected);
	cutAction->setEnabled(selected);
	moveAction->setEnabled(selected);
	drawAction->setEnabled(areaManager->hasCurrentArea());
	// Un-select draw if area is selected
	if(drawAction->isOn() && selected)
		setPaintMode(GridAreaManager::SELECT_MODE);
	if(areaManager->isEmptyAreaSelected())
		setPaintMode(GridAreaManager::DRAW_MODE);
//	undoAction->setEnabled(areaManager->isUndoPossible());
//	cerr << "PaintToolBar::catchGridAreaChanged selected area : " << selected 
//		<< ", paintMode after : " << getPaintMode() << endl; 
}
