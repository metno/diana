/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013-2016 met.no

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
#include <QAbstractItemModel>
#include <QItemSelection>

#include "qtDataDialog.h"

class DrawingItemBase;
class DrawingManager;
class EditItemManager;
class QTreeView;

namespace EditItems {

class FilterDrawingWidget;

class DrawingModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  enum Roles {
    NameRole = Qt::DisplayRole,
    FileNameRole = Qt::UserRole
  };

  DrawingModel(QObject *parent = 0);
  virtual ~DrawingModel();

  QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex &index) const;
  bool hasChildren(const QModelIndex &index = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

  Qt::ItemFlags flags(const QModelIndex &index) const;
  bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

  QMap<QString, QString> items() const;
  void setItems(const QMap<QString, QString> &items);
  void setTitle(const QString &title);

  QModelIndex find(const QString &name) const;
  QModelIndex findFile(const QString &fileName) const;

public slots:
  void appendDrawing(const QString &name, const QString &fileName);
  void appendDrawing(const QString &fileName);

private:
  QStringList listFiles(const QString &fileName) const;

  QString title_;
  QMap<QString, QString> items_;
  QStringList order_;
  QHash<QString, QStringList> fileCache_;
};

class DrawingDialog : public DataDialog
{
  Q_OBJECT

public:
  DrawingDialog(QWidget *, Controller *);

  static DrawingDialog *instance();

  std::string name() const override;
  void updateDialog() override;
  PlotCommand_cpv getOKString() override;
  void putOKString(const PlotCommand_cpv&) override;

  bool showsMore() override;

protected:
  void doShowMore(bool enable) override;

Q_SIGNALS:
  void filterToggled(bool);
  void editingMode(bool);

public Q_SLOTS:
  void loadFile();
  void loadFile(const QString &fileName);
  void quickSave();
  void reload();
  void saveAllItems();
  void saveFilteredItems();
  void saveSelectedItems();
  void saveVisibleItems();

private Q_SLOTS:
  void activateDrawing(const QItemSelection &selected, const QItemSelection &deselected);
  void clearItems();
  void editDrawings(const QModelIndex &index = QModelIndex());
  void makeProduct();
  void removeActiveDrawings();
  void showActiveContextMenu(const QPoint &pos);
  void showDrawingContextMenu(const QPoint &pos);
  void updateQuickSaveButton();
  void updateTimes() override;

private:
  QSet<QPair<QString, QString> > itemProducts(const QList<DrawingItemBase *> &items);
  void updateFileInfo(const QList<DrawingItemBase *> &items, const QString &fileName);
  void saveFile(const QList<DrawingItemBase *> &items, const QString &fileName);

  DrawingModel activeDrawingsModel_;
  DrawingModel drawingsModel_;
  DrawingModel editingModel_;
  DrawingManager *drawm_;
  EditItemManager *editm_;
  FilterDrawingWidget *filterWidget_;
  QTreeView *activeList_;
  QTreeView *drawingsList_;
  QTreeView *editingList_;
  QPushButton *filterButton_;
  QPushButton *quickSaveButton_;
  QString quickSaveName_;

  static DrawingDialog *self_;
};

} // namespace

#endif // DRAWINGDIALOG_H
