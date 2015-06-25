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

  propertyList_ = new QTreeView();
  propertyModel_ = new FilterDrawingModel(tr("Properties"), this);
  propertyList_->setModel(propertyModel_);
  propertyList_->setSelectionMode(QAbstractItemView::MultiSelection);
  propertyList_->setRootIsDecorated(false);
  propertyList_->setSortingEnabled(true);
  connect(propertyList_->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
    SLOT(updateValues()));

  valueList_ = new QTreeView();
  valueModel_ = new FilterDrawingModel(tr("Values"), this);
  valueList_->setModel(valueModel_);
  valueList_->setSelectionMode(QAbstractItemView::MultiSelection);
  valueList_->setRootIsDecorated(false);
  valueList_->setSortingEnabled(true);
  connect(valueList_->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
    SLOT(filterItems()));

  QVBoxLayout *viewLayout = new QVBoxLayout;
  viewLayout->addWidget(propertyList_);
  viewLayout->addWidget(valueList_);

  QFrame *separator = new QFrame();
  separator->setFrameShape(QFrame::VLine);

  QHBoxLayout *mainLayout = new QHBoxLayout(this);
  mainLayout->addWidget(separator);
  mainLayout->addLayout(viewLayout);
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
  QSet<QString> show = QSet<QString>::fromList(ed->propertyRules("show"));
  QSet<QString> hide = QSet<QString>::fromList(ed->propertyRules("hide"));

  QSet<QString> keep;
  QSet<QString> discard;

  foreach (DrawingItemBase *item, editm_->allItems() + drawm_->allItems()) {
    const QVariantMap &props = item->propertiesRef();
    foreach (const QString &key, props.keys()) {
      foreach (const QString &section, show) {
        if (key.startsWith(section)) {
          keep.insert(key);
          break;
        }
      }
      foreach (const QString &section, hide) {
        if (key.startsWith(section)) {
          discard.insert(key);
          break;
        }
      }
    }
  }

  propertyModel_->setStringList((keep - discard).toList());
  updateValues();
}

void FilterDrawingWidget::updateValues()
{
  QStringList properties = currentProperties();
  QSet<QString> values;

  foreach (DrawingItemBase *item, editm_->allItems() + drawm_->allItems()) {
    foreach (const QString &property, properties) {
      QVariant value = item->property(property);
      if (value.isValid())
        values.insert(value.toString());
    }
  }

  valueModel_->setStringList(values.toList());
  filterItems();
}

/**
 * Returns the selected properties in the property list, or all available
 * properties if none are selected.
 */
QStringList FilterDrawingWidget::currentProperties() const
{
  QModelIndexList indexes = propertyList_->selectionModel()->selectedIndexes();
  QStringList properties;

  if (!indexes.isEmpty()) {
    foreach (const QModelIndex &index, indexes)
      properties.append(index.data().toString());
  } else
    properties = propertyModel_->stringList();

  return properties;
}

/**
 * Returns the values for each of the current properties.
 */
QSet<QString> FilterDrawingWidget::currentValues() const
{
  QModelIndexList indexes = valueList_->selectionModel()->selectedIndexes();
  QSet<QString> values;

  if (!indexes.isEmpty()) {
    foreach (const QModelIndex &index, indexes)
      values.insert(index.data().toString());
  } else
    values = QSet<QString>::fromList(valueModel_->stringList());

  return values;
}

void FilterDrawingWidget::filterItems()
{
  QStringList properties = currentProperties();
  QSet<QString> values = currentValues();

  drawm_->setFilter(QPair<QStringList, QSet<QString> >(properties, values));
  editm_->setFilter(QPair<QStringList, QSet<QString> >(properties, values));

  emit updated();
}

// ======================================================
// Model for displaying item properties and their values.
// ======================================================

FilterDrawingModel::FilterDrawingModel(const QString &header, QObject *parent)
  : QStringListModel(parent)
{
  header_ = header;
}

FilterDrawingModel::~FilterDrawingModel()
{
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
  if (index.isValid())
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  else
    return Qt::ItemIsEnabled;
}

} // namespace
