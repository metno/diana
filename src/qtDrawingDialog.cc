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

#include "diController.h"
#include "diDrawingManager.h"
#include "qtDrawingDialog.h"
#include "EditItems/edititembase.h"
#include "EditItems/weatherarea.h"
#include "EditItems/weatherfront.h"
#include <paint_mode.xpm>       // reused for area drawing functionality

#include <QAction>
#include <QPushButton>
#include <QToolBar>
#include <QVBoxLayout>

DrawingDialog::DrawingDialog(QWidget *parent, Controller *ctrl)
  : DataDialog(parent, ctrl)
{
  m_action = new QAction(QIcon(QPixmap(paint_mode_xpm)), tr("Painting tools"), this);
  m_action->setShortcutContext(Qt::ApplicationShortcut);
  m_action->setShortcut(Qt::ALT + Qt::Key_B);
  m_action->setCheckable(true);
  m_action->setIconVisibleInMenu(true);
  connect(m_action, SIGNAL(toggled(bool)), SLOT(toggleDrawingMode(bool)));

  QHash<DrawingManager::Action, QAction *> actions = DrawingManager::instance()->actions();
  QToolButton *cutButton = new QToolButton();
  cutButton->setDefaultAction(actions[DrawingManager::Cut]);
  QToolButton *copyButton = new QToolButton();
  copyButton->setDefaultAction(actions[DrawingManager::Copy]);
  QToolButton *pasteButton = new QToolButton();
  pasteButton->setDefaultAction(actions[DrawingManager::Paste]);
  QToolButton *editButton = new QToolButton();
  editButton->setDefaultAction(actions[DrawingManager::Edit]);
  QToolButton *loadButton = new QToolButton();
  loadButton->setDefaultAction(actions[DrawingManager::Load]);

  QToolButton *undoButton = new QToolButton();
  undoButton->setDefaultAction(actions[DrawingManager::Undo]);
  QToolButton *redoButton = new QToolButton();
  redoButton->setDefaultAction(actions[DrawingManager::Redo]);

  QVBoxLayout *buttonLayout = new QVBoxLayout();
  buttonLayout->addWidget(cutButton);
  buttonLayout->addWidget(copyButton);
  buttonLayout->addWidget(pasteButton);
  buttonLayout->addWidget(editButton);
  buttonLayout->addWidget(loadButton);
  buttonLayout->addWidget(undoButton);
  buttonLayout->addWidget(redoButton);
  buttonLayout->addStretch();

  itemList = new QTreeWidget();
  itemList->setColumnCount(2);
  //itemList->setHeaderHidden(true);
  itemList->setHeaderLabels(QStringList() << tr("Object") << tr("Time"));
  itemList->setRootIsDecorated(false);
  itemList->setSelectionMode(QAbstractItemView::ExtendedSelection);
  itemList->setSortingEnabled(true);

  connect(itemList, SIGNAL(itemSelectionChanged()), SLOT(updateSelection()));

  EditItemManager *editor = DrawingManager::instance()->getEditItemManager();
  connect(editor, SIGNAL(itemAdded(EditItemBase*)), SLOT(addItem(EditItemBase*)));
  connect(editor, SIGNAL(itemChanged(EditItemBase*)), SLOT(updateItem(EditItemBase*)));
  connect(editor, SIGNAL(itemRemoved(EditItemBase*)), SLOT(removeItem(EditItemBase*)));
  connect(editor, SIGNAL(selectionChanged()), SLOT(updateItemList()));
  connect(DrawingManager::instance(), SIGNAL(timesUpdated()), SLOT(updateTimes()));

  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setMargin(4);
  layout->addWidget(itemList, 1);
  layout->addLayout(buttonLayout);
  //layout->addWidget(editor->getUndoView());

  setWindowTitle(tr("Drawing"));
}

DrawingDialog::~DrawingDialog()
{
}

std::string DrawingDialog::name() const
{
  return "DrawingDialog";
}

void DrawingDialog::updateTimes()
{
  std::vector<miutil::miTime> times = DrawingManager::instance()->getTimes();
  emit emitTimes("drawing", times);
}

void DrawingDialog::toggleDrawingMode(bool enable)
{
  m_ctrl->setDrawingModeEnabled(enable);
}

void DrawingDialog::addItem(EditItemBase *item)
{
  QTreeWidgetItem *listItem = new QTreeWidgetItem();
  if (static_cast<EditItem_WeatherArea::WeatherArea*>(item))
      listItem->setText(0, tr("Area"));
  else if (static_cast<EditItem_WeatherFront::WeatherFront*>(item))
      listItem->setText(0, tr("Front"));
  else
      listItem->setText(0, tr("Unknown"));
  listItem->setData(0, Qt::UserRole, item->id());
  listItem->setData(1, Qt::DisplayRole, item->properties().value("time", QVariant("-")));
  itemList->addTopLevelItem(listItem);
}

void DrawingDialog::removeItem(EditItemBase *item)
{
  for (int i = 0; i < itemList->topLevelItemCount(); ++i)
    if (itemList->topLevelItem(i)->data(0, Qt::UserRole).toInt() == item->id()) {
      delete itemList->takeTopLevelItem(i);
      break;
    }
}

/**
 * Updates the selection in the editor from the selection in the item list.
 */
void DrawingDialog::updateSelection()
{
  QSet<int> ids;
  foreach (QTreeWidgetItem *listItem, itemList->selectedItems()) {
    ids.insert(listItem->data(0, Qt::UserRole).toInt());
  }

  EditItemManager *eim = DrawingManager::instance()->getEditItemManager();
  foreach (EditItemBase *item, eim->getItems()) {
    if (ids.contains(item->id()))
      eim->selectItem(item);
    else
      eim->deselectItem(item);
  }

  eim->repaint();
}

/**
 * Updates the selection in the item list from the selection in the editor.
 */
void DrawingDialog::updateItemList()
{
  EditItemManager *eim = DrawingManager::instance()->getEditItemManager();
  QSet<int> ids;
  foreach (EditItemBase *item, eim->getSelectedItems())
    ids.insert(item->id());

  for (int i = 0; i < itemList->topLevelItemCount(); ++i) {
    // Select the item in the list to match the corresponding item on the map.
    QTreeWidgetItem *listItem = itemList->topLevelItem(i);
    listItem->setSelected(ids.contains(listItem->data(0, Qt::UserRole).toInt()));
  }
}

/**
 * Updates an item in the item list with properties from an item in the editor.
 */
void DrawingDialog::updateItem(EditItemBase *item)
{
  // Refresh the columns for the corresponding list item.
  for (int i = 0; i < itemList->topLevelItemCount(); ++i) {
    QTreeWidgetItem *listItem = itemList->topLevelItem(i);
    if (listItem->data(0, Qt::UserRole).toInt() == item->id()) {
      listItem->setData(1, Qt::DisplayRole, item->properties().value("time", QVariant("-")));
    }
  }
}