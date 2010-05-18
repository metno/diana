#ifndef QPOLYGONBOOKMARKDIALOG_H_
#define QPOLYGONBOOKMARKDIALOG_H_

#include <QTreeView>
#include <QMainWindow>
#include <qtPolygonBookmarkModel.h>
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
	PolygonBookmarkDialog(QWidget* w,std::vector<miutil::miString>& s, miutil::miString lastSavedPolygon="");

public slots:
  void bookmarkClicked(QModelIndex);
  void warn(miutil::miString);

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
  void polygonCopied(miutil::miString, miutil::miString, bool); ///< from - to - move
  void polygonSelected(miutil::miString);               ///< get from db and use that one
};

#endif /*QPOLYGONBOOKMARKDIALOG_H_*/
