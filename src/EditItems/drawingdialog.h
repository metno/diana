/*
  Diana - A Free Meteorological Visualisation Tool

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

#ifndef DRAWINGDIALOG_H
#define DRAWINGDIALOG_H

#include <QHash>
#include <QStringListModel>

#include "qtDataDialog.h"

class DrawingItemBase;
class DrawingManager;
class EditItemManager;
class QListView;

namespace EditItems {

class FilterDrawingWidget;

class DrawingModel : public QAbstractListModel
{
  Q_OBJECT

public:
  DrawingModel(QObject *parent = 0);
  virtual ~DrawingModel();

  QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

  Qt::ItemFlags flags(const QModelIndex &index) const;

  QMap<QString, QString> items() const;
  void setItems(const QMap<QString, QString> &items);

  QModelIndex find(const QString &name) const;

public slots:
  void appendDrawing(const QString &name, const QString &fileName);
  void appendDrawing(const QString &fileName);

private:
  QMap<QString, QString> items_;
};

class DrawingDialog : public DataDialog
{
  Q_OBJECT

public:
  DrawingDialog(QWidget *, Controller *);

  virtual std::string name() const;
  virtual void updateDialog();
  virtual std::vector<std::string> getOKString();
  virtual void putOKString(const std::vector<std::string> &);

signals:
  void filterToggled(bool);
  void editingMode(bool);

public slots:
  void loadFile();
  void quickSave();
  void saveAllItems();
  void saveFilteredItems();
  void saveSelectedItems();
  void saveVisibleItems();

private slots:
  void activateDrawing(const QItemSelection &selected, const QItemSelection &deselected);
  void editDrawings();
  void extend(bool enable);
  void makeProduct();
  void showActiveContextMenu(const QPoint &pos);
  void updateButtons();
  void updateQuickSaveButton();
  virtual void updateTimes();

private:
  void updateFileInfo(const QList<DrawingItemBase *> &items, const QString &fileName);
  void saveFile(const QList<DrawingItemBase *> &items, const QString &fileName);

  DrawingModel drawingsModel_;
  DrawingModel activeDrawingsModel_;
  DrawingModel editingModel_;
  DrawingManager *drawm_;
  EditItemManager *editm_;
  FilterDrawingWidget *filterWidget_;
  QListView *activeList_;
  QListView *drawingsList_;
  QPushButton *editButton_;
  QPushButton *filterButton_;
  QPushButton *quickSaveButton_;
  QString quickSaveName_;
};

} // namespace

#endif // DRAWINGDIALOG_H
