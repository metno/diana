/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2018 met.no

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

#include "WebMapPlotListModel.h"

#include "WebMapDialog.h"

#define MILOGGER_CATEGORY "diana.WebMapPlotListModel"
#include <miLogger/miLogging.h>

namespace {
const size_t IDX_INVALID = size_t(-1);
} // namespace

WebMapPlotListModel::WebMapPlotListModel(WebMapDialog *parent )
  : QAbstractListModel(parent)
{
}

int WebMapPlotListModel::rowCount(const QModelIndex&) const
{
  return dialog()->plotCommandCount();
}

QVariant WebMapPlotListModel::data(const QModelIndex& index, int role) const
{
  if (role == Qt::DisplayRole) {
    if (KVListPlotCommand_cp pc = std::dynamic_pointer_cast<const KVListPlotCommand>(dialog()->plotCommand(index.row()))) {
      const size_t idx_ws = pc->find(WEBMAP_SERVICE);
      const size_t idx_wl = pc->find(WEBMAP_LAYER);
      QString ws = (idx_ws != IDX_INVALID) ? QString::fromStdString(pc->get(idx_ws).value()) : "?";
      QString wl = (idx_wl != IDX_INVALID) ? QString::fromStdString(pc->get(idx_wl).value()) : "?";
      return QString("%1 -- %2").arg(ws).arg(wl);
    }
  }
  return QVariant();
}

void WebMapPlotListModel::onPlotsRemoveBegin()
{
  beginResetModel();
}

void WebMapPlotListModel::onPlotsRemoveEnd()
{
  endResetModel();
}

void WebMapPlotListModel::onPlotAddBegin(int idx)
{
  Q_EMIT beginInsertRows(QModelIndex(), idx, idx);
}

void WebMapPlotListModel::onPlotAddEnd()
{
  Q_EMIT endInsertRows();
}

WebMapDialog* WebMapPlotListModel::dialog() const
{
  return static_cast<WebMapDialog*>(parent());
}
