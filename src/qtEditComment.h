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
#ifndef _editComment_h
#define _editComment_h

#include <QDialog>
#include <qsplitter.h>

class QTextEdit;
class ToggleButton;
class Controller;
class ObjectManager;

/**
   \brief Dialogue for writing comments to edit products
*/
class EditComment :public QDialog
{
  Q_OBJECT
public:

  EditComment( QWidget* parent, Controller* llctrl, bool edit);
protected:
  void closeEvent( QCloseEvent* );

private:
  Controller*    m_ctrl;
  ObjectManager* m_objm;

  QTextEdit * mEdit;
  QTextEdit * mEdit2;
  QSplitter *split;
  ToggleButton *showOld;

  bool inComment; //true if editing comment
  bool inEditSession;    //true if comments for edit session

signals:
  void CommentHide();

private slots:
 void showOldToggled(bool);
 void textChanged();


public slots:
  /// start writing comments
  void startComment();
  ///  read old comments for this edit product
  void readComment();
  /// save comments for editing
  void saveComment();
  /// stop writing comments
  void stopComment();
};

#endif




