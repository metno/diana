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

 
  // select ========================
  selectAction = new QAction(  QPixmap(), tr("Select"), this );
  selectAction->setShortcut(tr("Space"));
  selectAction->setStatusTip(tr("Select"));
  connect( selectAction, SIGNAL( triggered() ) , this, SLOT( select() ) );


  // newfolder ========================
  newFolderAction = new QAction(  QPixmap(), tr("New Folder"), this );
  newFolderAction->setShortcut(tr("Ctrl+N"));
  newFolderAction->setStatusTip(tr("Create a new folder"));
  connect( newFolderAction, SIGNAL( triggered() ) , this, SLOT( newFolder() ) );

  renameAction = new QAction(  QPixmap(), tr("Rename"), this );
  renameAction->setShortcut(tr("Ctrl+R"));
  renameAction->setStatusTip(tr("Rename a Folder/Bookmark"));
  connect( renameAction, SIGNAL( triggered() ) , this, SLOT( rename() ) );

 
  // cancel ========================
  cancelAction = new QAction(  QPixmap(), tr("Cancel"), this );
  cancelAction->setStatusTip(tr("Cancel"));
  connect( cancelAction, SIGNAL( triggered() ) , this, SLOT( cancel() ) );

  // quit ========================
  quitAction = new QAction(  QPixmap(), tr("&Quit"), this );
  quitAction->setShortcut(tr("Ctrl+Q"));
  quitAction->setStatusTip(tr("Quit"));
  connect( quitAction, SIGNAL( triggered() ) , this, SLOT( quit() ) );

  
  filemenu = new QMenu(tr("File"),this);
  filemenu->addAction(cancelAction);
  filemenu->addSeparator();
  filemenu->addAction(quitAction);
  
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
  editmenu->addAction(selectAction);
  editmenu->addSeparator();
  editmenu->addAction(renameAction);
  editmenu->addAction(newFolderAction);
  editmenu->addSeparator();
  
  menuBar()->addMenu(filemenu);
  menuBar()->addMenu(editmenu);

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
  emit polygonQuit();
  close();
}

void PolygonBookmarkDialog::cancel()
{
  emit polygonCanceled();
  close();
}

void PolygonBookmarkDialog::paste() 
{
  QModelIndex idx=bookmarks->selectionModel()->currentIndex ();
  model->paste(idx);
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

void PolygonBookmarkDialog::newFolder()
{
  QModelIndex idx=bookmarks->selectionModel()->currentIndex ();
  model->setCurrent(idx);
  if(model->currentIsProtected())
    return;
  
  miString newfolder= model->getCurrentName(true);
  newfolder+=".New Folder";
  model->addBookmark(newfolder,true);
}

void PolygonBookmarkDialog::rename()
{
  QModelIndex idx=bookmarks->selectionModel()->currentIndex();
  bookmarks->edit(idx);
}




