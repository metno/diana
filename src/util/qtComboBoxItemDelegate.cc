/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2021 met.no

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

#include "qtComboBoxItemDelegate.h"

#include <QComboBox>

// adapted on https://wiki.qt.io/Combo_Boxes_in_Item_Views

ComboBoxItemDelegate::ComboBoxItemDelegate(QAbstractItemModel* model, QObject* parent)
    : QStyledItemDelegate(parent)
    , model_(model)
{
}

ComboBoxItemDelegate::~ComboBoxItemDelegate() {}

QWidget* ComboBoxItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& index) const
{
  QComboBox* cb = new QComboBox(parent);
  configureEditor(cb, model_, index);
  return cb;
}

void ComboBoxItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
  QComboBox* cb = qobject_cast<QComboBox*>(editor);
  Q_ASSERT(cb);
  const QString currentText = index.data(Qt::EditRole).toString();
  const int cbIndex = cb->findText(currentText);
  if (cbIndex >= 0)
    cb->setCurrentIndex(cbIndex);
}

void ComboBoxItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
  Q_ASSERT(model == model_);
  QComboBox* cb = qobject_cast<QComboBox*>(editor);
  Q_ASSERT(cb);
  model->setData(index, cb->currentText(), Qt::EditRole);
}
