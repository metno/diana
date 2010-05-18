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
  std::map<QModelIndex,miutil::miString> bookmarks;
  std::map<miutil::miString,QModelIndex> folders;
  std::map<QModelIndex,miutil::miString> folderIndex;
  std::set<QModelIndex>          deletionProtected;
  QModelIndexList           pasteBuffer;
  QModelIndex               currentIndex;
  QModelIndex               trashIndex;

  QIcon directoryIcon;
  QIcon lockedDirectoryIcon;
  QIcon privateDirectoryIcon;
  QIcon lockedBookmarkIcon;
  QIcon bookmarkIcon;
  QIcon trashcanIcon;

  void pasteList(QModelIndexList ilist,miutil::miString toDirName);
	void internalPaste(bool toTrash=false);
	void changeEntry(miutil::miString path, QStandardItem* item);
	bool moveItems;

public:
	PolygonBookmarkModel(QObject *p);

	void addBookmark(miutil::miString s,bool isFolder=false);
	void addBookmarks(std::vector<miutil::miString>& s);

	bool     currentIsBookmark();
	bool     currentIsProtected() const { return deletionProtected.count(currentIndex); }
	void     addSelectionToPasteBuffer(QModelIndexList& pb,bool mv=false) {
	  pasteBuffer = pb;
	  moveItems   = mv;
	}

	void     setCurrent(QModelIndex c) { currentIndex = c; }
	bool     setCurrent(miutil::miString n,QModelIndex& c);

	// return the bookmark/folder(for paste purpose)
	miutil::miString getCurrentName(bool folder=false);

	void paste(QModelIndex& c) { setCurrent(c); internalPaste();}
	void moveToTrash(QModelIndexList& pb);

public slots:
  void bookmarkChanged(QStandardItem * item);

signals:
  void warn(miutil::miString);
  void bookmarkCopied(miutil::miString,miutil::miString,bool); // from name, to name, move

};

#endif /*QPOLYGONBOOKMARKMODEL_H_*/
