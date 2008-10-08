#ifndef QPOLYGONBOOKMARKDIALOG_H_
#define QPOLYGONBOOKMARKDIALOG_H_

#include <QTreeView>
#include <QMainWindow>
#include <qPolygonBookmarkModel.h>
#include <puTools/miString.h>
#include <vector>
#include <QAction>
#include <QMenu>
#include <QMenuBar>

class PolygonBookmarkDialog : public QMainWindow{
  Q_OBJECT
private:
  QTreeView            *bookmarks;
  PolygonBookmarkModel *model;
  
  QMenu   *actionmenu;

  QAction *cancelAction;    
  QAction *copyAction;   
  QAction *pasteAction;   
  QAction *quitAction; 
  QAction *cutAction;
  QAction *deleteAction;
  
protected:
  void closeEvent( QCloseEvent* );
  
public:
	PolygonBookmarkDialog(QWidget* w,std::vector<miString>& s);

public slots:
  void bookmarkClicked(QModelIndex);
  void warn(miString);
	
private slots:  
  void quit();
  void cancel();
  void paste(); 
  void copy();
  void cut();
  void moveToTrash();
  
signals:
  void polygonCanceled();                       ///< emitted on cancel (exit) 
  void polygonCopied(miString, miString, bool); ///< from - to - move
  void polygonSelected(miString);               ///< get from db and use that one
};

#endif /*QPOLYGONBOOKMARKDIALOG_H_*/
