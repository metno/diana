/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018 met.no

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

#include "qtTreeFilterProxyModel.h"

TreeFilterProxyModel::TreeFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

bool TreeFilterProxyModel::filterAcceptsRow(int row, const QModelIndex& parent) const
{
  if (!parent.isValid()) {
    // accept highest level (model groups) if group name matches
    const QModelIndex group = sourceModel()->index(row, 0, parent);
    if (acceptIndex(group))
      return true;

    // also accept if any child (model name) matches
    const int modelnames = sourceModel()->rowCount(group);
    for (int r = 0; r < modelnames; ++r) {
      if (acceptIndex(group.child(r, 0)))
        return true;
    }
  } else {
    // accept if model group or model name match
    if (acceptIndex(parent) || acceptIndex(parent.child(row, 0)))
      return true;
  }
  return false;
}

bool TreeFilterProxyModel::acceptIndex(const QModelIndex& idx) const
{
  const QString text = sourceModel()->data(idx).toString();
  return (filterRegExp().indexIn(text) >= 0); // match anywhere
}
