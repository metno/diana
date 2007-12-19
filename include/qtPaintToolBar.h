#ifndef QTPAINTTOOLBAR_H_
#define QTPAINTTOOLBAR_H_

#include <qtoolbar.h>
#include <qmainwindow.h>
#include <qaction.h>
#include <diGridAreaManager.h>



class PaintToolBar : public QToolBar {
  Q_OBJECT

public:
  enum PaintToolBarButtons{ALL,SELECT_ONLY,PAINT_ONLY,PAINT_AND_MODIFY};  
  
public:
  PaintToolBar(QMainWindow *parent);
	GridAreaManager::PaintMode getPaintMode();
	void enableButtons(PaintToolBarButtons);
	
private:
  QActionGroup *modeActions;
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
