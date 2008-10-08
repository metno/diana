#include "qPolygonBookmarkModel.h"
#include "directory.xpm"
#include "private_directory.xpm"
#include "locked_directory.xpm"
#include "locked_bookmark.xpm"
#include "bookmark.xpm"
#include "trashcan.xpm"
#include <QStringList>

PolygonBookmarkModel::PolygonBookmarkModel(QObject* p) : QStandardItemModel(p) 
{
  directoryIcon.addPixmap(QPixmap(directory_xpm));
  lockedDirectoryIcon.addPixmap(QPixmap(locked_directory_xpm));
  privateDirectoryIcon.addPixmap(QPixmap(private_directory_xpm));
  lockedBookmarkIcon.addPixmap(QPixmap(locked_bookmark_xpm));
  bookmarkIcon.addPixmap(QPixmap(bookmark_xpm));
  trashcanIcon.addPixmap(QPixmap(trashcan_xpm));
  
  connect(this,SIGNAL( itemChanged( QStandardItem *) ),
        this, ( SLOT( bookmarkChanged( QStandardItem *) ) ));
   
  QStringList head;
  head << tr("Polygon Bookmarks");
  setHorizontalHeaderLabels ( head );
  
  addBookmark("TRASH",true);
  addBookmark("COMMON",true);
  addBookmark("SESSION",true);
  addBookmark("PRIVATE",true);

}




void PolygonBookmarkModel::addBookmark(miString s,bool isDirectory)
{
  vector<miString> words = s.split(".");
 
  QStandardItem *parentItem = invisibleRootItem();
  miString dir;
  int last=words.size();
  int size=last;
  last--;
  bool lockedBookmark=false;
  
  for (int col = 0; col < size; ++col) {
    // this is the item
    if(col==last && isDirectory==false) {
      
      int r=parentItem->rowCount();
      QStandardItem *childItem  = new QStandardItem(words[col].cStr());
      if(lockedBookmark) {
        childItem->setIcon(lockedBookmarkIcon);
        childItem->setEditable(false);
      } else {
        childItem->setIcon(bookmarkIcon);
      }
      
      parentItem->insertRow(r,childItem);
      bookmarks[childItem->index()] = s;
      if(lockedBookmark)
        deletionProtected.insert(childItem->index());


    } else {
      dir+=( col ? "." : "") + words[col];

      if (directories.count(dir)) {
        parentItem = itemFromIndex ( directories[dir] );
        if(deletionProtected.count(directories[dir]))
          lockedBookmark=true;
        
      } else {
        QStandardItem *childItem = new QStandardItem(words[col].cStr());
     
        bool isTrash=false;
        if(!col) {
          // some special folders in the root directory 
          if(dir=="TRASH") {
            childItem->setIcon(trashcanIcon);
            childItem->setEditable(false);
            isTrash=true;
          } else if(dir=="SESSION"){
            childItem->setIcon(lockedDirectoryIcon);
            childItem->setEditable(false);
            lockedBookmark=true;
          } else if (dir=="PRIVATE") {
            childItem->setIcon(privateDirectoryIcon);
            childItem->setEditable(false);
          } else 
            childItem->setIcon(directoryIcon);
        } else {
          if(lockedBookmark) {
            childItem->setIcon(lockedDirectoryIcon);
            childItem->setEditable(false);
          } else
          childItem->setIcon(directoryIcon);
        }
        parentItem->appendRow(childItem);
        parentItem       = childItem;
        directories[dir] = childItem->index();
        dirIndex[childItem->index()] = dir;
        if(isTrash)
          trashIndex=childItem->index();
        if(lockedBookmark)
          deletionProtected.insert(childItem->index());
      }
    }
  }
}

void PolygonBookmarkModel::addBookmarks(std::vector<miString>& s)
{
  for (int i=0; i<s.size(); i++) { 
    addBookmark(s[i]);
  }
}

miString PolygonBookmarkModel::getCurrentName(bool dir)
{
  if(dir) {
    QModelIndex   idx;
    if (bookmarks.count(currentIndex)) {
      QStandardItem *item = itemFromIndex(currentIndex);
      idx=item->parent()->index();
    } else  {
      idx=currentIndex;
    }
    return ( dirIndex.count(idx) ? dirIndex[idx] : "" );
  }

  if ( bookmarks.count(currentIndex) )  
    return  bookmarks[currentIndex]; 
 
  return ( dirIndex.count(currentIndex) ? dirIndex[currentIndex] : "" );
}  

bool PolygonBookmarkModel::currentIsBookmark()
{
  return bool( bookmarks.count(currentIndex));
}  

void PolygonBookmarkModel::bookmarkChanged( QStandardItem * item )
{
  if(!item->parent()) return;
  
  QModelIndex idx = item->parent()->index();

  if(dirIndex.count(idx))
    changeEntry(dirIndex[idx],item);
}

// recursive....
void PolygonBookmarkModel::changeEntry(miString path, QStandardItem* item)
{
   
  QModelIndex idx = item->index();
  miString newname = path+"."+item->text().toStdString();
  if(bookmarks.count(idx)) {
    miString oldname=bookmarks[idx];
   bookmarks[idx]=newname;
   emit bookmarkCopied(oldname,newname,true);
   return;
  }
    
  if (dirIndex.count(idx)) {
    
    miString oldname = dirIndex[idx];
    dirIndex[idx]    = newname;
    directories.erase(oldname);
    directories[newname]=idx;
    
    if(item->hasChildren()) {
      QModelIndexList qmil;
      for(int j=0; j<item->rowCount();j++) {
        if ( item->child(j) )
          changeEntry(newname,item->child(j));
      }
     }
  }
}

void PolygonBookmarkModel::internalPaste(bool toTrash)
{
  
  if(pasteBuffer.empty()) {
    emit warn("Nothing to paste...");
    return;
  } 

  QStandardItem *item = itemFromIndex(currentIndex);
  QModelIndex idx;
  if(toTrash) {
    idx=trashIndex;
    moveItems=true;
  } else {
    if ( bookmarks.count(currentIndex) ) {
      idx=item->parent()->index();
    } else
      idx=currentIndex;
  }
  
  if ( !dirIndex.count(idx) ) {
    emit warn ("Unable to paste here...");
    return;
  }
  
  if (deletionProtected.count(idx) && !toTrash) {
    emit warn ("Not allowed to paste here....");
    return;
  }
  pasteList(pasteBuffer,dirIndex[idx]);    
  moveItems=false;
}

void PolygonBookmarkModel::moveToTrash(QModelIndexList& pb)
{
  addSelectionToPasteBuffer(pb,true);
  internalPaste(true);
}




void PolygonBookmarkModel::pasteList(QModelIndexList ilist,miString toDirName)
{

  for (int i = 0; i < ilist.size(); ++i) {

    QModelIndex  cp_idx = ilist.at(i);
    QStandardItem *item = itemFromIndex(cp_idx);
    miString fromFullName;
    bool isDirectory=false;

    if (dirIndex.count(cp_idx)) {
      fromFullName = dirIndex[cp_idx];
      isDirectory=true;
    } else if (bookmarks.count(cp_idx)){
      fromFullName = bookmarks[cp_idx];
    } else
      continue;



    if(moveItems) {
      if(deletionProtected.count(cp_idx) || !item->parent()) {
        miString w=fromFullName+" is protected - operation not permitted";
        emit warn(w);
        return;
      }
    }

    miString toFullName=toDirName+"."+item->text().toStdString();

    if(fromFullName==toFullName) {
      emit warn("trying to copy "+toFullName+" to itself");
      continue;
    }

    if(!isDirectory) 
      emit bookmarkCopied(fromFullName,toFullName,moveItems);

    addBookmark(toFullName,isDirectory);

    // recursive for directories

    if(isDirectory)
      if(item->hasChildren()) {
        QModelIndexList qmil;
        for(int j=0; j<item->rowCount();j++) {
          if ( item->child(j) )
            qmil << item->child(j)->index();
        }
        pasteList(qmil,toFullName);
      }      

    if(moveItems) {
      directories.erase(fromFullName);
      bookmarks.erase(cp_idx);
      dirIndex.erase(cp_idx);
      if(item->parent()) 
        item->parent()->takeChild(item->row());
    }
  }

}

