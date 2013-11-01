#ifndef QPOLYGONBOOKMARKDIALOG_H_
#define QPOLYGONBOOKMARKDIALOG_H_

#include "qtPolygonBookmarkModel.h"
#include <QTreeView>
#include <QMainWindow>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <vector>

class PolygonBookmarkDialog : public QMainWindow{
  Q_OBJECT
private:
  QTreeView            *bookmarks;
  PolygonBookmarkModel *model;

  QMenu   *selectmenu;
  QMenu   *editmenu;

  QAction *selectAndExitAction;
  QAction *copyAction;
  QAction *pasteAction;
  QAction *quitAction;
  QAction *cutAction;
  QAction *deleteAction;
  QAction *selectAction;
  QAction *collapseAction;
  QAction *expandAction;
  QAction *newFolderAction;
  QAction *renameAction;

protected:
  void closeEvent( QCloseEvent* );

public:
	PolygonBookmarkDialog(QWidget* w,std::vector<std::string>& s, std::string lastSavedPolygon="");

public slots:
  void bookmarkClicked(QModelIndex);
  void warn(std::string);

private slots:
  void quit();
  void select();
  void selectAndExit();
  void paste();
  void copy();
  void cut();
  void moveToTrash();
  void expand();
  void collapse();
  void newFolder();
  void rename();


signals:
  void polygonCanceled();                       ///< emitted on cancel ( cast polygon)
  void polygonQuit();                           ///< close dialog but keep the polygon
  void polygonCopied(std::string, std::string, bool); ///< from - to - move
  void polygonSelected(std::string);               ///< get from db and use that one
};

#endif /*QPOLYGONBOOKMARKDIALOG_H_*/
