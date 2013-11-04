/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2006 met.no

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
#ifndef _qtQuickAdmin_h
#define _qtQuickAdmin_h

#include <QDialog>

#include <vector>
#include <diCommonTypes.h>
#include <diQuickMenues.h>

class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class QTextEdit;

/**
   \brief Administration of quick menus

   Dialogue for administration of quick menus:
   - copy, delete menu-trees
   - read quick menus from file
*/
class QuickAdmin : public QDialog {
  Q_OBJECT
private:
  std::vector<quickMenu> menus; // datastructure for quickmenus

  QPushButton* upButton;
  QPushButton* downButton;
  QPushButton* newButton;
  QPushButton* newfileButton;
  QPushButton* renameButton;
  QPushButton* eraseButton;
  QPushButton* copyButton;
  QPushButton* pasteButton;
  QPushButton* optionButton;
  QTreeWidget* menutree;               // tree of menus
  QTextEdit* comedit;                // editor for command-text
  bool autochange;

  int activeMenu;
  int activeElement;

  quickMenu MenuCopy;
  quickMenuItem MenuItemCopy;
  int copyMenu;
  int copyElement;

  int firstcustom;
  int lastcustom;

  void updateWidgets();
  void updateCommand();

public:
  QuickAdmin(QWidget*, std::vector<quickMenu>& qm,
	     int fc, int lc);

  std::vector<quickMenu> getMenus() const;
  int FirstCustom() const {return firstcustom;}
  int LastCustom() const {return lastcustom;}

  signals:
  void help(const char* );  ///< activate help

private slots:
  void selectionChanged(QTreeWidgetItem * ,int); // new selection in menutree
  void comChanged();   // command-text changed callback
  void helpClicked();  // help-button callback
  void upClicked();    // move item up
  void downClicked();  // move item down
  void newClicked();   // new menu-item
  void newfileClicked();// new menu from file
  void renameClicked();// rename menu or item
  void eraseClicked(); // erase menu-item
  void copyClicked();  // copy menu-item
  void pasteClicked(); // paste menu-item
  void optionClicked();// edit option-menus
};

#endif
