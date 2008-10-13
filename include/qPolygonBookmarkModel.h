#ifndef QPOLYGONBOOKMARKMODEL_H_
#define QPOLYGONBOOKMARKMODEL_H_

#include <QStandardItemModel>
#include <QStandardItem>
#include <QIcon>
#include <vector>
#include <set>
#include <puTools/miString.h>





class PolygonBookmarkModel : public QStandardItemModel {
  Q_OBJECT
private:
  map<QModelIndex,miString> bookmarks;
  map<miString,QModelIndex> folders;
  map<QModelIndex,miString> folderIndex;
  set<QModelIndex>          deletionProtected;
  QModelIndexList           pasteBuffer;
  QModelIndex               currentIndex;
  QModelIndex               trashIndex;
  
  QIcon directoryIcon;
  QIcon lockedDirectoryIcon;
  QIcon privateDirectoryIcon;
  QIcon lockedBookmarkIcon;
  QIcon bookmarkIcon;
  QIcon trashcanIcon;
  
  void pasteList(QModelIndexList ilist,miString toDirName);
	void internalPaste(bool toTrash=false);
	void changeEntry(miString path, QStandardItem* item);
	bool moveItems;
	
public:                              
	PolygonBookmarkModel(QObject *p);

	void addBookmark(miString s,bool isFolder=false);
	void addBookmarks(std::vector<miString>& s);

	bool     currentIsBookmark();
	bool     currentIsProtected() const { return deletionProtected.count(currentIndex); }
	void     addSelectionToPasteBuffer(QModelIndexList& pb,bool mv=false) {
	  pasteBuffer = pb; 
	  moveItems   = mv;
	}

	void     setCurrent(QModelIndex& c) { currentIndex = c; }
	
	// return the bookmark/folder(for paste purpose)
	miString getCurrentName(bool folder=false);

	void paste(QModelIndex& c) { setCurrent(c); internalPaste();}
	void moveToTrash(QModelIndexList& pb);
	
public slots: 
  void bookmarkChanged(QStandardItem * item);

signals:
  void warn(miString);
  void bookmarkCopied(miString,miString,bool); // from name, to name, move
  
};

#endif /*QPOLYGONBOOKMARKMODEL_H_*/
