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
#ifndef _qtQuickEditOptions_h
#define _qtQuickEditOptions_h

#include <QDialog>

#include <vector>
#include <diCommonTypes.h>
#include <diQuickMenues.h>

class QLabel;
class QPushButton;
class QLineEdit;
class QListWidget;
class QListWidgetItem;

/**
   \brief Administrate option lists for quick menues
   
   Dialogue for creating option lists for use in quick menues
*/
class QuickEditOptions : public QDialog {
  Q_OBJECT
private:
  std::vector<quickMenuOption> options;
  QListWidget* list;
  QLineEdit* choices;
  QPushButton* upButton;
  QPushButton* downButton;
  QPushButton* newButton;
  QPushButton* renameButton;
  QPushButton* eraseButton;

  int keynum;
  void updateList();

public:
  QuickEditOptions(QWidget*, std::vector<quickMenuOption>& opt);

  std::vector<quickMenuOption> getOptions();

  signals:
  void help(const char* );  ///< activate help

private slots:
  void helpClicked();  // help-button callback
  void listClicked ( QListWidgetItem * ); // new select in list
  void chChanged(const QString&); // choices-text changed callback
  void upClicked();    // move item up
  void downClicked();  // move item down
  void newClicked();   // new key
  void renameClicked();// rename key
  void eraseClicked(); // erase key

};

#endif
