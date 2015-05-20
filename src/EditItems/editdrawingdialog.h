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

#ifndef EDITDRAWINGDIALOG_H
#define EDITDRAWINGDIALOG_H

#include "qtDataDialog.h"
#include <QStringListModel>

class DrawingManager;
class EditItemManager;

namespace EditItems {

class EditDialogModel : public QStringListModel
{
  Q_OBJECT

public:
  EditDialogModel(const QString &header, QObject *parent = 0);
  virtual ~EditDialogModel();

  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

  Qt::ItemFlags flags(const QModelIndex &index) const;

private:
  QString header_;
};

class EditDrawingDialog : public DataDialog
{
  Q_OBJECT

public:
  EditDrawingDialog(QWidget *, Controller *);

  virtual std::string name() const;
  virtual void updateDialog() {};
  virtual std::vector<std::string> getOKString() { return std::vector<std::string>(); }
  virtual void putOKString(const std::vector<std::string> &) {}

public slots:
  void updateChoices();

signals:
  void resetChoices();

private slots:
  void filterItems();
  virtual void updateTimes() {} // n/a

private:
  DrawingManager *drawm_;
  EditItemManager *editm_;
  EditDialogModel *propertyModel_;
  EditDialogModel *valueModel_;
};

} // namespace

#endif // EDITDRAWINGDIALOG_H
