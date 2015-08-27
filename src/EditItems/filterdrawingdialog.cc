/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013-2015 met.no

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

#include <diDrawingManager.h>
#include <diEditItemManager.h>
#include <EditItems/filterdrawingdialog.h>
#include <EditItems/itemgroup.h>
#include <EditItems/properties.h>
#include <EditItems/toolbar.h>
#include <editdrawing.xpm> // ### for now

#include <QAction>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>

#include <qtUtility.h>

namespace EditItems {

FilterDrawingWidget::FilterDrawingWidget(QWidget *parent)
  : QWidget(parent)
{
  drawm_ = DrawingManager::instance();
  connect(drawm_, SIGNAL(updated()), SLOT(updateChoices()));

  editm_ = EditItemManager::instance();
  connect(editm_, SIGNAL(itemAdded(DrawingItemBase *)), SLOT(updateChoices()));
  connect(editm_, SIGNAL(itemChanged(const QVariantMap &)), SLOT(updateChoices()));
  connect(editm_, SIGNAL(itemRemoved(int)), SLOT(updateChoices()));
  connect(editm_, SIGNAL(itemStatesReplaced()), SLOT(updateChoices()));
  connect(editm_, SIGNAL(editing(bool)), SLOT(updateChoices()));

  propertyList_ = new QTreeView();
  propertyModel_ = new FilterDrawingModel(tr("Properties"), this);
  propertyList_->setModel(propertyModel_);
  propertyList_->setSelectionMode(QAbstractItemView::MultiSelection);
  propertyList_->setSortingEnabled(true);
  propertyList_->setItemsExpandable(false);
  connect(propertyList_->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
    SLOT(filterItems()));
  connect(propertyModel_, SIGNAL(modelReset()), propertyList_, SLOT(expandAll()));

  QFrame *separator = new QFrame();
  separator->setFrameShape(QFrame::VLine);

  QHBoxLayout *mainLayout = new QHBoxLayout(this);
  mainLayout->addWidget(separator);
  mainLayout->addWidget(propertyList_);
  //mainLayout->addWidget(editm_->getUndoView());
}

FilterDrawingWidget::~FilterDrawingWidget()
{
}

/**
 * Updates the property list to show the available properties for all items,
 * only showing those properties defined in the setup file.
 */
void FilterDrawingWidget::updateChoices()
{
  Properties::PropertiesEditor *ed = Properties::PropertiesEditor::instance();
  QSet<QString> show;
  QSet<QString> hide;

  if (editm_->isEditing()) {
    show = QSet<QString>::fromList(ed->propertyRules("show-editing"));
    hide = QSet<QString>::fromList(ed->propertyRules("hide-editing"));
  } else {
    show = QSet<QString>::fromList(ed->propertyRules("show-drawing"));
    hide = QSet<QString>::fromList(ed->propertyRules("hide-drawing"));
  }

  QHash<QString, QSet<QString> > choices;

  foreach (DrawingItemBase *item, editm_->allItems() + drawm_->allItems()) {

    const QVariantMap &props = item->propertiesRef();
    foreach (const QString &key, props.keys()) {

      bool keep = false;
      bool discard = false;

      foreach (const QString &section, show) {
        if (key.startsWith(section)) {
          keep = true;
          break;
        }
      }
      foreach (const QString &section, hide) {
        if (key.startsWith(section)) {
          discard = true;
          break;
        }
      }

      if (keep && !discard)
        choices[key].insert(props.value(key).toString());
    }
  }

  QHash<QString, QStringList> prepared;
  for (QHash<QString, QSet<QString> >::const_iterator it = choices.begin(); it != choices.end(); ++it) {
    QStringList valueList = it.value().toList();
    valueList.sort();
    prepared[it.key()] = valueList;
  }

  // Record the currently selected filter values.
  QHash<QString, QStringList> selected;

  foreach (const QModelIndex &index, propertyList_->selectionModel()->selectedIndexes()) {
    if (index.parent().isValid()) {
      QString name = index.parent().data().toString();
      QStringList values = selected.value(name);
      values.append(index.data().toString());
      selected[name] = values;
    }
  }

  // Replace the properties in the model with the ones for the current objects.
  propertyModel_->setProperties(prepared);

  // Repopulate the selection model for the filter list.
  propertyList_->selectionModel()->reset();
  QItemSelection selection;

  foreach (const QString &name, selected.keys()) {
    // Find the properties whose values were previously selected.
    foreach (const QString &value, selected.value(name)) {
      QModelIndex index = propertyModel_->find(name, value);
      if (index.isValid())
        selection.select(index, index);
    }
  }

  propertyList_->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);

  // The available properties may have changed, causing the filters to be
  // invalid, so ensure that they are updated.
  filterItems();
}

/**
 * Returns the selected properties in the property list.
 */
QHash<QString, QStringList> FilterDrawingWidget::currentProperties() const
{
  QModelIndexList indexes = propertyList_->selectionModel()->selectedIndexes();
  QHash<QString, QStringList> choices;

  // Find all selected values and insert them into the hash under their
  // associated properties.
  foreach (const QModelIndex &index, indexes) {
    if (index.parent().isValid())
      choices[index.parent().data().toString()].append(index.data().toString());
  }

  return choices;
}

/**
 * Sets the filters for the drawing and editing managers using the current
 * set of selected properties and values.
 */
void FilterDrawingWidget::filterItems()
{
  QHash<QString, QStringList> filter = currentProperties();

  drawm_->setFilter(filter);
  editm_->setFilter(filter);

  emit updated();
}

void FilterDrawingWidget::disableFilter(bool disable)
{
  setDisabled(disable);
  emit updated();
}

QStringList FilterDrawingWidget::properties() const
{
  return propertyModel_->properties();
}

// ======================================================
// Model for displaying item properties and their values.
// ======================================================

FilterDrawingModel::FilterDrawingModel(const QString &header, QObject *parent)
  : QAbstractItemModel(parent)
{
  header_ = header;
}

FilterDrawingModel::~FilterDrawingModel()
{
}

QModelIndex FilterDrawingModel::index(int row, int column, const QModelIndex &parent) const
{
  if (!parent.isValid()) {
    // Top level item - use an invalid row value as an identifier.
    if (row < order_.size())
      return createIndex(row, column, 0xffffffff);
  } else {
    // Child item - store the parent's row in the created index.
    if (parent.row() >= 0 && parent.row() < order_.size())
      return createIndex(row, column, parent.row());
  }

  return QModelIndex();
}

QModelIndex FilterDrawingModel::parent(const QModelIndex &index) const
{
  // The root item and top level items return an invalid index.
  if (!index.isValid() || index.internalId() == 0xffffffff)
    return QModelIndex();

  // Child items return an index that refers to the relevant top level item.
  return createIndex(index.internalId(), 0, 0xffffffff);
}

bool FilterDrawingModel::hasChildren(const QModelIndex &index) const
{
  // The root item has children.
  if (!index.isValid())
    return true;

  // Child items have no children.
  if (index.internalId() != 0xffffffff)
    return false;

  // All top level items have children.
  return true;
}

int FilterDrawingModel::columnCount(const QModelIndex &parent) const
{
  return 1;
}

int FilterDrawingModel::rowCount(const QModelIndex &parent) const
{
  // The root item has rows corresponding to the available properties.
  if (!parent.isValid())
    return order_.size();

  // Top level items have rows corresponding to the available values for
  // each property.
  else if (parent.row() >= 0 && parent.row() < order_.size() && parent.column() == 0) {
    QString key = order_.at(parent.row());
    return choices_.value(key).size();

  } else
    return 0;
}

QVariant FilterDrawingModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid() || index.row() < 0 || index.column() != 0 || role != Qt::DisplayRole)
    return QVariant();

  // The values of top level items are obtained from the order list.
  if (index.internalId() == 0xffffffff) {
    QString name = order_.at(index.row());
    if (index.row() < order_.size())
        return QVariant(name);

  // The values of child items are obtained from the values of the hash.
  } else if (index.internalId() < order_.size()) {
    // Child item
    QString name = order_.at(index.internalId());
    QStringList values = choices_.value(name);
    if (index.row() < values.size())
      return QVariant(values.at(index.row()));
  }

  return QVariant();
}

QVariant FilterDrawingModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role != Qt::DisplayRole)
    return QVariant();

  if (section == 0)
    return QVariant(header_);
  else
    return QVariant();
}

Qt::ItemFlags FilterDrawingModel::flags(const QModelIndex &index) const
{
  if (index.isValid() && hasChildren(index))
    return Qt::ItemIsEnabled;
  else
    return QAbstractItemModel::flags(index);
}

/**
 * Returns the property names in a well-defined order.
 */
QStringList FilterDrawingModel::properties() const
{
  return order_;
}

void FilterDrawingModel::setProperties(const QHash<QString, QStringList> &choices)
{
  beginResetModel();
  choices_ = choices;
  order_ = choices.keys();
  endResetModel();
}

QModelIndex FilterDrawingModel::find(const QString &name, const QString &value) const
{
  int i = order_.indexOf(name);
  if (i == -1)
    return QModelIndex();

  int j = choices_.value(name).indexOf(value);
  if (j == -1)
    return QModelIndex();

  return createIndex(j, 0, i);
}

} // namespace
