/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2008 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "diana_config.h"

#include "qtPolygonBookmarkModel.h"
#include "directory.xpm"
#include "private_directory.xpm"
#include "locked_directory.xpm"
#include "locked_bookmark.xpm"
#include "bookmark.xpm"
#include "trashcan.xpm"
#include <QStringList>

#include <puTools/miStringFunctions.h>


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

void PolygonBookmarkModel::addBookmark(std::string s,bool isFolder)
{
  std::vector<std::string> words = miutil::split(s, ".");

  QStandardItem *parentItem = invisibleRootItem();
  std::string folder;
  int last=words.size();
  int size=last;
  last--;
  bool lockedBookmark=false;

  for (int col = 0; col < size; ++col) {
    // this is the item
    if(col==last && !isFolder) {

      int r=parentItem->rowCount();
      QStandardItem *childItem  = new QStandardItem(words[col].c_str());
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
      folder+=( col ? "." : "") + words[col];

      if (folders.count(folder)) {
        parentItem = itemFromIndex ( folders[folder] );
        if(deletionProtected.count(folders[folder]))
          lockedBookmark=true;

      } else {
        QStandardItem *childItem = new QStandardItem(words[col].c_str());

        bool isTrash=false;
        if(!col) {
          // some special folders in the root folder
          if(folder=="TRASH") {
            childItem->setIcon(trashcanIcon);
            childItem->setEditable(false);
            isTrash=true;
          } else if(folder=="SESSION"){
            childItem->setIcon(lockedDirectoryIcon);
            childItem->setEditable(false);
            lockedBookmark=true;
          } else if (folder=="PRIVATE") {
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
        folders[folder] = childItem->index();
        folderIndex[childItem->index()] = folder;
        if(isTrash)
          trashIndex=childItem->index();
        if(lockedBookmark)
          deletionProtected.insert(childItem->index());
      }
    }
  }
}

void PolygonBookmarkModel::addBookmarks(std::vector<std::string>& s)
{
  for (size_t i=0; i<s.size(); i++) {
    addBookmark(s[i]);
  }
}

bool PolygonBookmarkModel::setCurrent(std::string name, QModelIndex& ind)
{
  if((not !name.empty())) return false;

  std::map<QModelIndex, std::string>::iterator idx;
  for(idx=bookmarks.begin();idx!=bookmarks.end();idx++) {
    if(idx->second==name) {
      setCurrent(idx->first);
      ind=idx->first;
      return true;
    }
  }
  return false;
}

std::string PolygonBookmarkModel::getCurrentName(bool folder)
{
  if(folder) {
    QModelIndex   idx;
    if (bookmarks.count(currentIndex)) {
      QStandardItem *item = itemFromIndex(currentIndex);
      idx=item->parent()->index();
    } else  {
      idx=currentIndex;
    }
    return ( folderIndex.count(idx) ? folderIndex[idx] : "" );
  }

  if ( bookmarks.count(currentIndex) )
    return  bookmarks[currentIndex];

  return ( folderIndex.count(currentIndex) ? folderIndex[currentIndex] : "" );
}

bool PolygonBookmarkModel::currentIsBookmark()
{
  return bool( bookmarks.count(currentIndex));
}

void PolygonBookmarkModel::bookmarkChanged( QStandardItem * item )
{
  if(!item->parent()) return;

  QModelIndex idx = item->parent()->index();

  if(folderIndex.count(idx))
    changeEntry(folderIndex[idx],item);
}

// recursive....
void PolygonBookmarkModel::changeEntry(std::string path, QStandardItem* item)
{

  QModelIndex idx = item->index();
  std::string newname = path+"."+item->text().toStdString();
  if(bookmarks.count(idx)) {
    std::string oldname=bookmarks[idx];
   bookmarks[idx]=newname;
   emit bookmarkCopied(oldname,newname,true);
   return;
  }

  if (folderIndex.count(idx)) {

    std::string oldname = folderIndex[idx];
    folderIndex[idx] = newname;
    folders.erase(oldname);
    folders[newname]=idx;

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

  if ( !folderIndex.count(idx) ) {
    emit warn ("Unable to paste here...");
    return;
  }

  if (deletionProtected.count(idx) && !toTrash) {
    emit warn ("Not allowed to paste here....");
    return;
  }
  pasteList(pasteBuffer,folderIndex[idx]);
  moveItems=false;
}

void PolygonBookmarkModel::moveToTrash(QModelIndexList& pb)
{
  addSelectionToPasteBuffer(pb,true);
  internalPaste(true);
}




void PolygonBookmarkModel::pasteList(QModelIndexList ilist,std::string toFolderName)
{

  for (int i = 0; i < ilist.size(); ++i) {

    QModelIndex  cp_idx = ilist.at(i);
    QStandardItem *item = itemFromIndex(cp_idx);
    std::string fromFullName;
    bool isFolder=false;

    if (folderIndex.count(cp_idx)) {
      fromFullName = folderIndex[cp_idx];
      isFolder=true;
    } else if (bookmarks.count(cp_idx)){
      fromFullName = bookmarks[cp_idx];
    } else
      continue;



    if(moveItems) {
      if(deletionProtected.count(cp_idx) || !item->parent()) {
        std::string w=fromFullName+" is protected - operation not permitted";
        emit warn(w);
        return;
      }
    }

    std::string toFullName=toFolderName+"."+item->text().toStdString();

    if(fromFullName==toFullName) {
      emit warn("trying to copy "+toFullName+" to itself");
      continue;
    }

    if(!isFolder)
      emit bookmarkCopied(fromFullName,toFullName,moveItems);

    addBookmark(toFullName,isFolder);

    // recursive for folders

    if(isFolder)
      if(item->hasChildren()) {
        QModelIndexList qmil;
        for(int j=0; j<item->rowCount();j++) {
          if ( item->child(j) )
            qmil << item->child(j)->index();
        }
        pasteList(qmil,toFullName);
      }

    if(moveItems) {
      folders.erase(fromFullName);
      bookmarks.erase(cp_idx);
      folderIndex.erase(cp_idx);
      if(item->parent())
        item->parent()->takeChild(item->row());
    }
  }

}

