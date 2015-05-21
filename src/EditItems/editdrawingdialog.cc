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
#include <EditItems/editdrawingdialog.h>
#include <EditItems/layergroup.h>
#include <EditItems/properties.h>
#include <EditItems/toolbar.h>
#include <editdrawing.xpm> // ### for now

#include <QAction>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>

#include <qtUtility.h>

namespace EditItems {

EditDrawingDialog::EditDrawingDialog(QWidget *parent, Controller *ctrl)
  : DataDialog(parent, ctrl)
{
  drawm_ = DrawingManager::instance();
  connect(drawm_, SIGNAL(updated()), SLOT(updateChoices()));

  editm_ = EditItemManager::instance();
  connect(editm_, SIGNAL(itemAdded(DrawingItemBase *)), SLOT(updateChoices()));
  connect(editm_, SIGNAL(itemChanged(const QVariantMap &)), SLOT(updateChoices()));
  connect(editm_, SIGNAL(itemRemoved(int)), SLOT(updateChoices()));

  // Create an action that can be used to open the dialog from within a menu or toolbar.
  m_action = new QAction(QIcon(QPixmap(editdrawing_xpm)), tr("Edit Drawing Dialog"), this);
  m_action->setShortcut(Qt::SHIFT + Qt::CTRL + Qt::Key_B);
  m_action->setCheckable(true);
  m_action->setIconVisibleInMenu(true);

  propertyList_ = new QTreeView();
  propertyModel_ = new EditDialogModel(tr("Properties"), this);
  propertyList_->setModel(propertyModel_);
  propertyList_->setSelectionMode(QAbstractItemView::MultiSelection);
  propertyList_->setRootIsDecorated(false);
  propertyList_->setSortingEnabled(true);
  connect(propertyList_->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
    SLOT(updateValues()));

  valueList_ = new QTreeView();
  valueModel_ = new EditDialogModel(tr("Values"), this);
  valueList_->setModel(valueModel_);
  valueList_->setSelectionMode(QAbstractItemView::MultiSelection);
  valueList_->setRootIsDecorated(false);
  connect(valueList_->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
    SLOT(filterItems()));

  QPushButton *resetButton = NormalPushButton(tr("Reset"), this);
  connect(resetButton, SIGNAL(clicked()), SIGNAL(resetChoices()));
  QPushButton *hideButton = NormalPushButton(tr("Hide"), this);
  connect(hideButton, SIGNAL(clicked()), SIGNAL(hideData()));

  QHBoxLayout *viewLayout = new QHBoxLayout;
  viewLayout->addWidget(propertyList_);
  viewLayout->addWidget(valueList_);

  QHBoxLayout *bottomLayout = new QHBoxLayout;
  bottomLayout->addWidget(resetButton);
  bottomLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding));
  bottomLayout->addWidget(hideButton);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->addLayout(viewLayout);
  mainLayout->addLayout(bottomLayout);

  setWindowTitle(tr("Edit Drawing Dialog"));
  setFocusPolicy(Qt::StrongFocus);
}

std::string EditDrawingDialog::name() const
{
  return "EDITDRAWING";
}

/**
 * Updates the property list to show the available properties for all items,
 * hiding those properties excluded in the setup file.
 */
void EditDrawingDialog::updateChoices()
{
  QSet<QString> hide = QSet<QString>::fromList(Properties::PropertiesEditor::instance()->propertyRules("hide"));

  // Add some properties that we never want to show.
  hide.insert("visible");

  QSet<QString> properties;

  foreach (DrawingItemBase *item, editm_->allItems() + drawm_->allItems()) {
    const QVariantMap &props = item->propertiesRef();
    foreach (const QString &key, props.keys()) {
      if (!key.contains(":") && !hide.contains(key))
        properties.insert(key);
      else {
        QString section = key.split(":").first();
        if (!hide.contains(section))
          properties.insert(key);
      }
    }
  }

  propertyModel_->setStringList(properties.toList());
  updateValues();
}

void EditDrawingDialog::updateValues()
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
QStringList EditDrawingDialog::currentProperties() const
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
QSet<QString> EditDrawingDialog::currentValues() const
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

void EditDrawingDialog::filterItems()
{
  QStringList properties = currentProperties();
  QSet<QString> values = currentValues();

  // Modify the visibility of the items depending on the properties and values selected.
  foreach (DrawingItemBase *item, editm_->allItems() + drawm_->allItems()) {

    // Set each item to be invisible by default.
    bool visible = false;

    foreach (const QString &property, properties) {
      QVariant value = item->property(property);
      if (value.isValid() && values.contains(value.toString())) {
        visible = true;
        break;
      }
    }

    item->setProperty("visible", visible);
  }

  emit updated();
}

void EditDrawingDialog::updateDialog()
{
  // The items may have been updated, so filter them again.
  //filterItems();
}


EditDialogModel::EditDialogModel(const QString &header, QObject *parent)
  : QStringListModel(parent)
{
  header_ = header;
}

EditDialogModel::~EditDialogModel()
{
}

QVariant EditDialogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role != Qt::DisplayRole)
    return QVariant();

  if (section == 0)
    return QVariant(header_);
  else
    return QVariant();
}

Qt::ItemFlags EditDialogModel::flags(const QModelIndex &index) const
{
  if (index.isValid())
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  else
    return Qt::ItemIsEnabled;
}

} // namespace
