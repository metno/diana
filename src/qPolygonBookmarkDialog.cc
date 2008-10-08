#include "qPolygonBookmarkDialog.h"
#include <iostream>
#include <QStatusBar>

using namespace std;

PolygonBookmarkDialog::PolygonBookmarkDialog(QWidget* w, std::vector<miString>& values ) : 
  QMainWindow(w)
  {
  setAttribute(Qt::WA_DeleteOnClose); 
  
  // actions: --------------------------

  // copy ========================
  copyAction = new QAction(  QPixmap(), tr("&Copy"), this );
  copyAction->setShortcut(tr("Ctrl+C"));
  copyAction->setStatusTip(tr("Copy polygon"));
  connect( copyAction, SIGNAL( triggered() ) , this, SLOT( copy() ) );

  // copy ========================
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

   
  // cancel ========================
  cancelAction = new QAction(  QPixmap(), tr("Cancel"), this );
  cancelAction->setStatusTip(tr("Cancel"));
  connect( cancelAction, SIGNAL( triggered() ) , this, SLOT( cancel() ) );

  // quit ========================
  quitAction = new QAction(  QPixmap(), tr("&Quit"), this );
  quitAction->setShortcut(tr("Ctrl+Q"));
  quitAction->setStatusTip(tr("Quit"));
  connect( quitAction, SIGNAL( triggered() ) , this, SLOT( quit() ) );

  actionmenu = new QMenu(tr("Action"),this);
  actionmenu->addAction(copyAction);
  actionmenu->addAction(cutAction);
  actionmenu->addAction(pasteAction);
  actionmenu->addSeparator();
  actionmenu->addAction(deleteAction);
  actionmenu->addSeparator();
  actionmenu->addAction(cancelAction);
  actionmenu->addSeparator();
  actionmenu->addAction(quitAction);

  menuBar()->addMenu(actionmenu);

  model = new PolygonBookmarkModel(this);
  
  
  model->addBookmarks(values);
  connect(model,SIGNAL(warn(miString)),this,SLOT(warn(miString)));
  
  
  bookmarks = new QTreeView(this);
  bookmarks->setSelectionMode(QAbstractItemView::ExtendedSelection);
   
  
  setCentralWidget(bookmarks);

  bookmarks->setModel(model);
  
  connect(bookmarks, SIGNAL(clicked(QModelIndex)),
      this, SLOT(bookmarkClicked(QModelIndex)));
  
  connect(model,SIGNAL(bookmarkCopied(miString,miString,bool)),
      this,SIGNAL(polygonCopied(miString, miString, bool)));
  resize(250,400);
  
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
  close();
}

void PolygonBookmarkDialog::cancel()
{
  emit polygonCanceled();
  close();
}

void PolygonBookmarkDialog::paste() 
{
  model->paste();
}

void PolygonBookmarkDialog::warn(miString w)
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






