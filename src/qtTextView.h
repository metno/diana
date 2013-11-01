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
#ifndef _TextView_h
#define _TextView_h

#include <QDialog>

#include <qstring.h>
#include <map>

class QTabWidget;
class QTextEdit;

/**
   \brief text viewer

   Text viewer for data from other applications via socket interface
*/
class TextWidget : public QWidget
{
  Q_OBJECT
private:

  QTextEdit* textEdit;

public:
  int id;
  TextWidget(QWidget* parent, const std::string& text, int id_);
  TextWidget(QWidget* parent);
  void setText( std::string );
};

/**
   \brief Multiple text viewers

   Multiple text viewers for data from other applications via socket interface

*/

class TextView : public QDialog
{
  Q_OBJECT
private:

  QTabWidget* tabwidget;
  std::map<int,TextWidget*> idmap;
  TextWidget* current;

public:
  TextView(QWidget* parent);
  void setText( int id, const std::string& name, const std::string& text );
  void deleteTab( int id );
  void deleteTab();

private slots:
  void printSlot();

signals:
void printClicked(int);

};

#endif
