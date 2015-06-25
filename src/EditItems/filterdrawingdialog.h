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

#ifndef FILTERDRAWINGDIALOG_H
#define FILTERDRAWINGDIALOG_H

#include <QStringListModel>
#include <QWidget>

class DrawingManager;
class EditItemManager;
class QTreeView;

namespace EditItems {

class FilterDrawingModel : public QStringListModel
{
  Q_OBJECT

public:
  FilterDrawingModel(const QString &header, QObject *parent = 0);
  virtual ~FilterDrawingModel();

  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

  Qt::ItemFlags flags(const QModelIndex &index) const;

private:
  QString header_;
};

class FilterDrawingWidget : public QWidget
{
  Q_OBJECT

public:
  FilterDrawingWidget(QWidget *parent = 0);
  virtual ~FilterDrawingWidget();

signals:
  void updated();

public slots:
  void updateChoices();

private slots:
  void updateValues();
  void filterItems();

private:
  QStringList currentProperties() const;
  QSet<QString> currentValues() const;

  DrawingManager *drawm_;
  EditItemManager *editm_;
  FilterDrawingModel *propertyModel_;
  FilterDrawingModel *valueModel_;
  QTreeView *propertyList_;
  QTreeView *valueList_;
};

} // namespace

#endif // FILTERDRAWINGDIALOG_H
