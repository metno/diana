/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013-2018 met.no

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

#include "filterdrawingdialog.h"

#include "EditItems/itemgroup.h"
#include "EditItems/properties.h"
#include "EditItems/toolbar.h"
#include "diDrawingManager.h"
#include "diEditItemManager.h"

#include "qtUtility.h"

#include <QAction>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>

#include "editdrawing.xpm" // ### for now

namespace EditItems {

FilterDrawingWidget::FilterDrawingWidget(QWidget *parent)
  : QWidget(parent)
{
  drawm_ = DrawingManager::instance();
  connect(drawm_, &DrawingManager::updated, this, &FilterDrawingWidget::updateChoices);

  editm_ = EditItemManager::instance();
  connect(editm_, SIGNAL(itemAdded(DrawingItemBase *)), SLOT(updateChoices()));
  connect(editm_, SIGNAL(itemChanged(const QVariantMap &)), SLOT(updateChoices()));
  connect(editm_, SIGNAL(itemRemoved(int)), SLOT(updateChoices()));
  connect(editm_, SIGNAL(itemStatesReplaced()), SLOT(updateChoices()));
  connect(editm_, SIGNAL(editing(bool)), SLOT(updateChoices()));

  propertyModel_ = new FilterDrawingModel(tr("Properties"), this);
  propertyList_ = new QTreeView();
  propertyList_->setModel(propertyModel_);
  propertyList_->setSelectionMode(QAbstractItemView::NoSelection);
  propertyList_->setSortingEnabled(true);
  propertyList_->setItemsExpandable(false);
  connect(propertyModel_, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
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

  for (DrawingItemBase* item : editm_->allItems() + drawm_->allItems()) {

    const QVariantMap &props = item->propertiesRef();
    for (const QString& key : props.keys()) {

      bool keep = false;
      bool discard = false;

      for (const QString& section : show) {
        if (key.startsWith(section)) {
          keep = true;
          break;
        }
      }
      for (const QString& section : hide) {
        if (key.startsWith(section)) {
          discard = true;
          break;
        }
      }

      if (keep && !discard)
        choices[key].insert(props.value(key).toString());
    }
  }

  FilterProperty_ql fpl;
  QHash<QString, QStringList> filter = drawm_->getFilter();
  for (QHash<QString, QSet<QString> >::const_iterator it = choices.begin(); it != choices.end(); ++it) {
    FilterProperty fp;
    fp.property = it.key();

    QHash<QString, QStringList>::const_iterator itF = filter.find(fp.property);
    if (itF == filter.end())
      fp.checked = Qt::Unchecked;
    else
      fp.checked = Qt::Checked;

    QStringList valueList = it.value().toList();
    valueList.sort();
    for (int i = 0; i < valueList.size(); ++i) {
      const QString& v = valueList.at(i);
      fp.values << FilterValue(v, (itF != filter.end() && itF.value().contains(v)) ? Qt::Checked : Qt::Unchecked);
    }

    fpl << fp;
  }

  // Replace the properties in the model with the ones for the current objects.
  propertyModel_->setProperties(fpl);

  // The available properties may have changed, causing the filters to be
  // invalid, so ensure that they are updated.
  filterItems();
}

/**
 * Sets the filters for the drawing and editing managers using the current
 * set of selected properties and values.
 */
void FilterDrawingWidget::filterItems()
{
  QHash<QString, QStringList> filter = propertyModel_->checkedItems();

  drawm_->setFilter(filter);
  editm_->setFilter(filter);

  Q_EMIT updated();
}

void FilterDrawingWidget::disableFilter(bool disable)
{
  setDisabled(disable);
  Q_EMIT updated();
}

// ======================================================
// Model for displaying item properties and their values.
// ======================================================

static const unsigned long long PROPERTY_ID = 0xFFFFFFFF;

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
    if (row < filter_.size())
      return createIndex(row, column, PROPERTY_ID);
  } else {
    // Child item - store the parent's row in the created index.
    if (parent.row() >= 0 && parent.row() < filter_.size())
      return createIndex(row, column, parent.row());
  }

  return QModelIndex();
}

QModelIndex FilterDrawingModel::parent(const QModelIndex &index) const
{
  // The root item and top level items return an invalid index.
  if (!index.isValid() || index.internalId() == PROPERTY_ID)
    return QModelIndex();

  // Child items return an index that refers to the relevant top level item.
  return createIndex(index.internalId(), 0, PROPERTY_ID);
}

bool FilterDrawingModel::hasChildren(const QModelIndex &index) const
{
  // The root item has children.
  if (!index.isValid())
    return true;

  if (index.column() != 0)
    return false;

  // Child items have no children.
  if (index.internalId() != PROPERTY_ID)
    return false;

  // All top level items have children.
  return true;
}

int FilterDrawingModel::columnCount(const QModelIndex&) const
{
  return 1;
}

int FilterDrawingModel::rowCount(const QModelIndex &parent) const
{
  // The root item has rows corresponding to the available properties.
  if (!parent.isValid())
    return filter_.size();

  // Top level items have rows corresponding to the available values for
  // each property.
  if (parent.row() >= 0 && parent.row() < filter_.size() && parent.column() == 0) {
    return filter_.at(parent.row()).values.size();
  } else {
    return 0;
  }
}

QVariant FilterDrawingModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid() || index.row() < 0 || index.column() > 1)
    return QVariant();

  // The values of top level items are obtained from the order list.
  if (index.internalId() == PROPERTY_ID) {
    if (index.row() < filter_.size()) {
      const FilterProperty& fp = filter_.at(index.row());
      if (role == Qt::DisplayRole)
        return fp.property;
      else if (role == Qt::CheckStateRole)
        return fp.checked;
    }

  // The values of child items are obtained from the values of the hash.
  } else if (index.internalId() < (size_t)filter_.size()) {
    const FilterProperty& fp = filter_.at(index.internalId());
    if (index.row() < fp.values.size()) {
      const FilterValue& fv = fp.values.at(index.row());
      if (role == Qt::DisplayRole)
        return fv.value;
      else if (role == Qt::CheckStateRole)
        return fv.checked;
    }
  }

  return QVariant();
}

bool FilterDrawingModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  // Only allow the check states of valid items to be changed.
  if (!index.isValid() || index.row() < 0 || index.column() != 0 || role != Qt::CheckStateRole)
    return false;

  if (index.internalId() == PROPERTY_ID) {
    if (index.row() < filter_.size()) {
      FilterProperty& fp = filter_[index.row()];
      fp.checked = Qt::CheckState(value.toInt());
      Q_EMIT dataChanged(index, index);
      // change enabled state of children
      const QModelIndex child0 = createIndex(0, 0, index.row());
      const QModelIndex childN = createIndex(fp.values.size() - 1, 0, index.row());
      Q_EMIT dataChanged(child0, childN);
      return true;
    }
  }

  if (index.internalId() < (size_t)filter_.size()) {
    FilterProperty& fp = filter_[index.internalId()];
    if (index.row() < fp.values.size()) {
      FilterValue& fv = fp.values[index.row()];
      fv.checked = Qt::CheckState(value.toInt());
      Q_EMIT dataChanged(index, index);
      return true;
    }
  }

  return false;
}

QVariant FilterDrawingModel::headerData(int section, Qt::Orientation, int role) const
{
  if (role == Qt::DisplayRole && section == 0)
    return QVariant(header_);
  return QVariant();
}

Qt::ItemFlags FilterDrawingModel::flags(const QModelIndex &index) const
{
  if (!index.isValid())
    return 0;

  Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
  const unsigned long long id = index.internalId();
  if (id == PROPERTY_ID || (id < (size_t)filter_.size() && filter_.at(id).checked == Qt::Checked))
    flags |= Qt::ItemIsEnabled;
  return flags;
}

void FilterDrawingModel::setProperties(const FilterProperty_ql& filter)
{
  beginResetModel();
  filter_ = filter;
  endResetModel();
}

QHash<QString, QStringList> FilterDrawingModel::checkedItems() const
{
  QHash<QString, QStringList> items;

  for (const FilterProperty& fp : filter_) {
    if (fp.checked == Qt::Checked) {
      const QString& name = fp.property;
      QStringList checkeditems;
      for (const FilterValue& fv : fp.values) {
        if (fv.checked == Qt::Checked)
          checkeditems.append(fv.value);
      }
      items[name] = checkeditems;
    }
  }
  return items;
}

} // namespace EditItems
