#ifndef QTPAINTTOOLBAR_H_
#define QTPAINTTOOLBAR_H_

#include <q3toolbar.h>
#include <q3mainwindow.h>
#include <qaction.h>
//Added by qt3to4:
#include <Q3ActionGroup>
#include <diGridAreaManager.h>



class PaintToolBar : public Q3ToolBar {
  Q_OBJECT

public:
  enum PaintToolBarButtons{ALL,SELECT_ONLY,PAINT_ONLY,PAINT_AND_MODIFY};  
  
public:
  PaintToolBar(Q3MainWindow *parent);
	GridAreaManager::PaintMode getPaintMode();
	void enableButtons(PaintToolBarButtons);
	
private:
  Q3ActionGroup *modeActions;
  QAction *selectAction;
  QAction *drawAction;
	QAction *includeAction;
	QAction *cutAction;
	QAction *moveAction;
  	
private slots:
  void sendPaintModeChanged();
  
public slots:
	void setPaintMode(GridAreaManager::PaintMode);
	  	
signals:
  void paintModeChanged(GridAreaManager::PaintMode mode);
};

#endif /*QTPAINTTOOLBAR_H_*/
