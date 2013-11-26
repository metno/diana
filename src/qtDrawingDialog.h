/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2013 met.no

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
#ifndef _drawingdialog_h
#define _drawingdialog_h

#include <qnamespace.h>
#include <QStandardItemModel>
#include "EditItems/edititembase.h"
#include "qtDataDialog.h"

class DrawingDialog : public DataDialog
{
  Q_OBJECT

public:
  enum ItemRoles {
    IdRole = Qt::UserRole,
  };

  DrawingDialog(QWidget *parent, Controller *ctrl);
  ~DrawingDialog();

  std::string name() const;
  std::vector<std::string> getOKString();
  void putOKString(const std::vector<std::string>& vstr);

public slots:
  void updateTimes();
  void toggleDrawingMode(bool);
  void updateDialog();

protected:
  void keyPressEvent(QKeyEvent *);

private slots:
  void addItem(DrawingItemBase *item);
  void removeItem(DrawingItemBase *item);
  void chooseDrawing();
  void selectDrawing(const QItemSelection& current);

private:
  Controller *ctrl;
  QSet<int> itemIds;
  QHash<int, DrawingItemBase *> itemHash;
  QListView *drawingList;
  QListView *chosenDrawingList;
  QStandardItemModel chosenDrawingModel;
};

class ToolPanel : public QWidget
{
  Q_OBJECT
public:
  ToolPanel(QWidget *parent = 0);
  ~ToolPanel();
};

#endif
