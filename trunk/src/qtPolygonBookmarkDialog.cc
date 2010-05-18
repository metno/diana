#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtPolygonBookmarkDialog.h"
#include <iostream>
#include <QStatusBar>
#include <qUtilities/miLogFile.h>
using namespace std;

PolygonBookmarkDialog::PolygonBookmarkDialog(QWidget* w, std::vector<miutil::miString>& values, miutil::miString lastSavedPolygon ) :
  QMainWindow(w)
  {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowModality(Qt::ApplicationModal);
  // actions: --------------------------

  // copy ========================
  copyAction = new QAction(  QPixmap(), tr("&Copy"), this );
  copyAction->setShortcut(tr("Ctrl+C"));
  copyAction->setStatusTip(tr("Copy polygon"));
  connect( copyAction, SIGNAL( triggered() ) , this, SLOT( copy() ) );

  // cut ========================
  cutAction = new QAction(  QPixmap(), tr("Cut"), this );
  cutAction->setShortcut(tr("Ctrl+X"));
  cutAction->setStatusTip(tr("Cut polygon"));
  connect( cutAction, SIGNAL( triggered() ) , this, SLOT( cut() ) );

  // paste ========================
  pasteAction = new QAction(  QPixmap(), tr("&Paste"), this );
  pasteAction->setShortcut(tr("Ctrl+V"));
  pasteAction->setStatusTip(tr("Paste polygon"));
  connect( pasteAction, SIGNAL( triggered() ) , this, SLOT( paste() ) );

  // paste ========================
  deleteAction = new QAction(  QPixmap(), tr("&Delete"), this );
  deleteAction->setShortcut(tr("Delete"));
  deleteAction->setStatusTip(tr("Move to Trash"));
  connect( deleteAction, SIGNAL( triggered() ) , this, SLOT( moveToTrash() ) );

  // collapse ========================
  collapseAction= new QAction(  QPixmap(), tr("Collapse All"), this );
  collapseAction->setShortcut(tr("Ctrl+-"));
  collapseAction->setStatusTip(tr("Collapse the whole tree"));
  connect( collapseAction, SIGNAL( triggered() ) , this, SLOT( collapse() ) );

  // expand ========================
  expandAction= new QAction(  QPixmap(), tr("expand All"), this );
  expandAction->setShortcut(tr("Ctrl++"));
  expandAction->setStatusTip(tr("expand the whole tree"));
  connect( expandAction, SIGNAL( triggered() ) , this, SLOT( expand() ) );

  // newfolder ========================
  newFolderAction = new QAction(  QPixmap(), tr("New Folder"), this );
  newFolderAction->setShortcut(tr("Ctrl+N"));
  newFolderAction->setStatusTip(tr("Create a new folder"));
  connect( newFolderAction, SIGNAL( triggered() ) , this, SLOT( newFolder() ) );

  renameAction = new QAction(  QPixmap(), tr("Rename"), this );
  renameAction->setShortcut(tr("Ctrl+R"));
  renameAction->setStatusTip(tr("Rename a Folder/Bookmark"));
  connect( renameAction, SIGNAL( triggered() ) , this, SLOT( rename() ) );

  // select ========================
  selectAction = new QAction(  QPixmap(), tr("Select"), this );
  selectAction->setShortcut(tr("Space"));
  selectAction->setStatusTip(tr("Select"));
  connect( selectAction, SIGNAL( triggered() ) , this, SLOT( select() ) );

  // selectAndExit ========================
  selectAndExitAction = new QAction(  QPixmap(), tr("Select and exit"), this );
  selectAndExitAction->setShortcut(tr("Return"));
  selectAndExitAction->setStatusTip(tr("Select and exit"));
  connect( selectAndExitAction, SIGNAL( triggered() ) , this, SLOT( selectAndExit() ) );

  // quit ========================
  quitAction = new QAction(  QPixmap(), tr("&Quit"), this );
  quitAction->setShortcut(tr("Ctrl+Q"));
  quitAction->setStatusTip(tr("Quit"));
  connect( quitAction, SIGNAL( triggered() ) , this, SLOT( quit() ) );


  selectmenu = new QMenu(tr("Select"),this);
  selectmenu->addSeparator();
  selectmenu->addAction(selectAction);
  selectmenu->addAction(selectAndExitAction);

  editmenu = new QMenu(tr("Edit"),this);
  editmenu->addAction(copyAction);
  editmenu->addAction(cutAction);
  editmenu->addAction(pasteAction);
  editmenu->addSeparator();
  editmenu->addAction(deleteAction);
  editmenu->addSeparator();
  editmenu->addAction(collapseAction);
  editmenu->addAction(expandAction);
  editmenu->addSeparator();
  editmenu->addAction(renameAction);
  editmenu->addAction(newFolderAction);
  editmenu->addSeparator();

  menuBar()->addMenu(selectmenu);
  menuBar()->addMenu(editmenu);

  model = new PolygonBookmarkModel(this);


  model->addBookmarks(values);
  connect(model,SIGNAL(warn(miutil::miString)),this,SLOT(warn(miutil::miString)));


  bookmarks = new QTreeView(this);
  bookmarks->setSelectionMode(QAbstractItemView::ExtendedSelection);


  setCentralWidget(bookmarks);

  bookmarks->setModel(model);

  connect(bookmarks, SIGNAL(clicked(QModelIndex)),
      this, SLOT(bookmarkClicked(QModelIndex)));

  connect(model,SIGNAL(bookmarkCopied(miutil::miString,miutil::miString,bool)),
      this,SIGNAL(polygonCopied(miutil::miString, miutil::miString, bool)));
  resize(250,400);

  QModelIndex c;
  if(model->setCurrent(lastSavedPolygon,c )) {
    cout << "found a polygon: " << lastSavedPolygon << endl;
    bookmarks->setCurrentIndex(c);

    emit polygonSelected(lastSavedPolygon );
  }
  miLogFile logfile;
  logfile.setSection("PROFET.LOG");
  logfile.restoreSizeAndPos(this,"ProfetPolygonBookmarkDialog");


}


void PolygonBookmarkDialog::bookmarkClicked(QModelIndex idx)
{
  model->setCurrent(idx);
  if (model->currentIsBookmark()) { // excludes directories...
    emit polygonSelected( model->getCurrentName() );
  }
}

void PolygonBookmarkDialog::closeEvent(QCloseEvent * e)
{
     quit();
}

void PolygonBookmarkDialog::quit()
{
  miLogFile logfile;
  logfile.setSection("PROFET.LOG");
  logfile.logSizeAndPos(this,"ProfetPolygonBookmarkDialog");
  emit polygonQuit();
  close();
}

void PolygonBookmarkDialog::paste()
{
  QModelIndex idx=bookmarks->selectionModel()->currentIndex ();
  model->paste(idx);
}

void PolygonBookmarkDialog::warn(miutil::miString w)
{
  statusBar()->showMessage(w.cStr(),6000);
}


void PolygonBookmarkDialog::copy()
{
  QModelIndexList ilist=bookmarks->selectionModel()->selectedIndexes();
  model->addSelectionToPasteBuffer(ilist,false);
  warn("copy ");
}


void PolygonBookmarkDialog::cut()
{
  QModelIndexList ilist=bookmarks->selectionModel()->selectedIndexes();
  model->addSelectionToPasteBuffer(ilist,true);
  warn("cut ");
}


void PolygonBookmarkDialog::moveToTrash()
{
  QModelIndexList ilist=bookmarks->selectionModel()->selectedIndexes();
  model->moveToTrash(ilist);
  warn("delete ");
}

void PolygonBookmarkDialog::expand()
{
  bookmarks->expandAll();
}

void PolygonBookmarkDialog::collapse()
{
  bookmarks->collapseAll();
}

void PolygonBookmarkDialog::select()
{
  QModelIndex idx=bookmarks->selectionModel()->currentIndex ();
  bookmarkClicked(idx);
}


void PolygonBookmarkDialog::selectAndExit()
{
  QModelIndex idx=bookmarks->selectionModel()->currentIndex ();
  bookmarkClicked(idx);
  if (model->currentIsBookmark())
    quit();

}



void PolygonBookmarkDialog::newFolder()
{
  QModelIndex idx=bookmarks->selectionModel()->currentIndex ();
  model->setCurrent(idx);
  if(model->currentIsProtected())
    return;

  miutil::miString newfolder= model->getCurrentName(true);
  newfolder+=".New Folder";
  model->addBookmark(newfolder,true);
}

void PolygonBookmarkDialog::rename()
{
  QModelIndex idx=bookmarks->selectionModel()->currentIndex();
  bookmarks->edit(idx);
}




