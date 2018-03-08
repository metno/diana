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

#include "qtStationDialogModel.h"

#include <QModelIndex>
#include <QVariant>

#define MILOGGER_CATEGORY "diana.StationDialogModel"
#include <miLogger/miLogging.h>

/**
 * \class StationDialogModel
 * \brief Provides a model to reference StationPlot and Station information held in a
 * data structure.
 *
 * The StationDialogModel class provides a model that encapsulates the data structures
 * describing a collection of StationPlot objects and the Station objects they contain.
 *
 * The model exposes a single level structure to views. The top level contains two columns
 * and a series of rows, each corresponding to a StationPlot object. The model index for the
 * first column of each row can be dereferenced to obtain the name of the StationPlot; the
 * index for the second column can be used to obtain the URL of the data.
 */

/**
 * Constructs a new model for accessing data held by \a info with the given \a parent.
 */
StationDialogModel::StationDialogModel(stationDialogInfo& info, QObject* parent)
    : QAbstractItemModel(parent)
    , m_info(info)
{
}

/**
 * \reimp
 */
QModelIndex StationDialogModel::index(int row, int column, const QModelIndex& parent) const
{
  if (!parent.isValid()) {
    if (row >= 0 && (unsigned int)row < m_info.sets.size() && column >= 0 && column < 2)
      return createIndex(row, column, -1);
  }

  return QModelIndex();
}

/**
 * \reimp
 */
QModelIndex StationDialogModel::parent(const QModelIndex& index) const
{
  return QModelIndex();
}

/**
 * \reimp
 * Returns the number of rows contained by the item corresponding to the given
 * \a parent index.
 *
 * For top-level items, this function returns the total number of StationPlot
 * objects. For child items, it returns the total number of Station objects held
 * by the StationPlot corresponding to \a parent.
 */
int StationDialogModel::rowCount(const QModelIndex& parent) const
{
  if (!parent.isValid())
    return m_info.sets.size();

  return 0;
}

/**
 * \reimp
 */
int StationDialogModel::columnCount(const QModelIndex& parent) const
{
  return 2;
}

/**
 * \reimp
 * Returns the data for the item corresponding to the given \a index, for
 * the specified \a role.
 *
 * For top-level items, this function returns the name of the corresponding
 * StationPlot object. For child items, it returns the relevant property for
 * the corresponding Station object.
 */
QVariant StationDialogModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if (role != Qt::DisplayRole && role != Qt::UserRole)
    return QVariant();

  if (index.internalId() == -1) {
    // Top level item
    if (index.row() >= 0 && (unsigned int)index.row() < m_info.sets.size()) {
      if (index.column() == 0) {
        if (role == Qt::DisplayRole)
          return QVariant(QString::fromStdString(m_info.sets[index.row()].name));
        else {
          return QVariant(m_info.chosen[m_info.sets[index.row()].url]);
        }
      } else if (index.column() == 1) {
        if (role == Qt::DisplayRole)
          return QVariant(QString::fromStdString(m_info.sets[index.row()].url));
        else
          return QVariant(m_info.chosen[m_info.sets[index.row()].url]);
      }
    }
  }

  return QVariant();
}

QVariant StationDialogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role != Qt::DisplayRole)
    return QVariant();

  if (section == 0)
    return tr("Name");
  else if (section == 1)
    return tr("Source");

  return QVariant();
}

/**
 * Updates the model with the new data specified by \a info and resets the model
 * to ensure that attached views display the new data.
 */
void StationDialogModel::updateData(stationDialogInfo& info)
{
  beginResetModel();
  m_info = info;
  endResetModel();
}

/**
 * \reimp
 */
Qt::ItemFlags StationDialogModel::flags(const QModelIndex& index) const
{
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

/**
 * Sets new data for the item corresponding to the given \a index.
 * The data given by \a value is stored according to the specified \a role.
 *
 * For top-level items, the data stored in the first column is the name of
 * the corresponding StationPlot object.
 *
 * For child items, the data stored in each column is the value of the relevant
 * property of the corresponding Station object.
 */
bool StationDialogModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if (!index.isValid() || !value.isValid())
    return false;
  else if (role != Qt::EditRole)
    return false;
  else if (index.row() < 0 || (unsigned int)index.row() >= m_info.sets.size())
    return false;

  if (index.internalId() == -1) {
    // Top level item

    if (index.column() == 0)
      m_info.sets[index.row()].name = value.toString().toStdString();
    else if (index.column() == 1)
      m_info.sets[index.row()].url = value.toString().toStdString();
    else
      return false;

    emit dataChanged(index, index);
    return true;
  }

  return false;
}

stationSetInfo& StationDialogModel::set(int row) const
{
  return m_info.sets[row];
}
