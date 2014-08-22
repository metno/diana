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

#ifndef QPOLYGONBOOKMARKMODEL_H_
#define QPOLYGONBOOKMARKMODEL_H_

#include <QStandardItemModel>
#include <QStandardItem>
#include <QIcon>
#include <vector>
#include <set>

class PolygonBookmarkModel : public QStandardItemModel
{
  Q_OBJECT
private:
  std::map<QModelIndex,std::string> bookmarks;
  std::map<std::string,QModelIndex> folders;
  std::map<QModelIndex,std::string> folderIndex;
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

  void pasteList(QModelIndexList ilist,std::string toDirName);
	void internalPaste(bool toTrash=false);
	void changeEntry(std::string path, QStandardItem* item);
	bool moveItems;

public:
	PolygonBookmarkModel(QObject *p);

	void addBookmark(std::string s,bool isFolder=false);
	void addBookmarks(std::vector<std::string>& s);

	bool     currentIsBookmark();
	bool     currentIsProtected() const { return deletionProtected.count(currentIndex); }
	void     addSelectionToPasteBuffer(QModelIndexList& pb,bool mv=false) {
	  pasteBuffer = pb;
	  moveItems   = mv;
	}

	void     setCurrent(QModelIndex c) { currentIndex = c; }
	bool     setCurrent(std::string n,QModelIndex& c);

	// return the bookmark/folder(for paste purpose)
	std::string getCurrentName(bool folder=false);

	void paste(QModelIndex& c) { setCurrent(c); internalPaste();}
	void moveToTrash(QModelIndexList& pb);

public slots:
  void bookmarkChanged(QStandardItem * item);

signals:
  void warn(std::string);
  void bookmarkCopied(std::string,std::string,bool); // from name, to name, move

};

#endif /*QPOLYGONBOOKMARKMODEL_H_*/
