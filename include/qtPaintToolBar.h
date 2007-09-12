#ifndef QTPAINTTOOLBAR_H_
#define QTPAINTTOOLBAR_H_

#include <qtoolbar.h>
#include <qmainwindow.h>
#include <qaction.h>
#include <diGridAreaManager.h>

class PaintToolBar : public QToolBar {
    Q_OBJECT
public:
    PaintToolBar(QMainWindow *parent, GridAreaManager *gam);
	GridAreaManager::PaintMode getPaintMode();
	void catchGridAreaChanged();
	
private:
	QMainWindow *mainwindow;
	GridAreaManager *areaManager;

    QActionGroup *modeActions;
    QAction *selectAction;
//  	QAction *newAction;
//  	QAction *deleteAction;
    QAction *drawAction;
  	QAction *includeAction;
  	QAction *cutAction;
  	QAction *moveAction;
//  	QAction *colorAction;
//  	QAction *undoAction;
//    QAction *closeAction;
  	
public slots:
	void setPaintMode(int);
	  	
private slots:
	void modeChanged();
//	void newActionPerformed();
//	void deleteActionPerformed();
//	void colorActionPerformed();
//	void undoActionPerformed();
//	void closeActionPerformed();
	
signals:
  	void hidePaintToolBar();
  	void updateGridAreaPlot();
  	
};
#endif /*QTPAINTTOOLBAR_H_*/
