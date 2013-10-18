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

#include <fstream>
#include <iostream>
#define MILOGGER_CATEGORY "diana.DrawingDialog"
#include <miLogger/miLogging.h>

#include "diController.h"
#include "diDrawingManager.h"
#include "diEditItemManager.h"
#include "qtDrawingDialog.h"
#include "EditItems/edititembase.h"
#include "EditItems/weatherarea.h"
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

  QHash<EditItemManager::Action, QAction *> actions = EditItemManager::instance()->actions();
  QToolButton *cutButton = new QToolButton();
  cutButton->setDefaultAction(actions[EditItemManager::Cut]);
  QToolButton *copyButton = new QToolButton();
  copyButton->setDefaultAction(actions[EditItemManager::Copy]);
  QToolButton *pasteButton = new QToolButton();
  pasteButton->setDefaultAction(actions[EditItemManager::Paste]);
  QToolButton *editButton = new QToolButton();
  editButton->setDefaultAction(actions[EditItemManager::Edit]);
  QToolButton *loadButton = new QToolButton();
  loadButton->setDefaultAction(actions[EditItemManager::Load]);

  QToolButton *undoButton = new QToolButton();
  undoButton->setDefaultAction(actions[EditItemManager::Undo]);
  QToolButton *redoButton = new QToolButton();
  redoButton->setDefaultAction(actions[EditItemManager::Redo]);

  QVBoxLayout *buttonLayout = new QVBoxLayout();
  buttonLayout->addWidget(cutButton, 0, Qt::AlignJustify | Qt::AlignVCenter);
  buttonLayout->addWidget(copyButton, 0, Qt::AlignJustify | Qt::AlignVCenter);
  buttonLayout->addWidget(pasteButton, 0, Qt::AlignJustify | Qt::AlignVCenter);
  buttonLayout->addWidget(editButton, 0, Qt::AlignJustify | Qt::AlignVCenter);
  buttonLayout->addWidget(loadButton, 0, Qt::AlignJustify | Qt::AlignVCenter);
  buttonLayout->addWidget(undoButton, 0, Qt::AlignJustify | Qt::AlignVCenter);
  buttonLayout->addWidget(redoButton, 0, Qt::AlignJustify | Qt::AlignVCenter);
  buttonLayout->addStretch();

  itemList = new QTreeWidget();
  itemList->setColumnCount(2);
  //itemList->setHeaderHidden(true);
  itemList->setHeaderLabels(QStringList() << tr("Object") << tr("Time"));
  itemList->setRootIsDecorated(false);
  itemList->setSelectionMode(QAbstractItemView::ExtendedSelection);
  itemList->setSortingEnabled(true);

  connect(itemList, SIGNAL(itemSelectionChanged()), SLOT(updateSelection()));

  EditItemManager *editor = EditItemManager::instance();
  connect(editor, SIGNAL(itemAdded(DrawingItemBase*)), SLOT(addItem(DrawingItemBase*)));
  connect(editor, SIGNAL(itemChanged(DrawingItemBase*)), SLOT(updateItem(DrawingItemBase*)));
  connect(editor, SIGNAL(itemRemoved(DrawingItemBase*)), SLOT(removeItem(DrawingItemBase*)));
  connect(editor, SIGNAL(selectionChanged()), SLOT(updateItemList()));
  connect(DrawingManager::instance(), SIGNAL(timesUpdated()), SLOT(updateTimes()));

  QHBoxLayout *controlsLayout = new QHBoxLayout();
  controlsLayout->setMargin(0);
  controlsLayout->addWidget(itemList, 1);
  controlsLayout->addLayout(buttonLayout);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setMargin(4);
  layout->addLayout(controlsLayout);
  //layout->addWidget(editor->getUndoView());
  layout->addLayout(createStandardButtons());

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
  emit emitTimes("DRAWING", times);
}

void DrawingDialog::updateDialog()
{
  METLIBS_LOG_DEBUG("DrawingDialog::updateDialog");
}

std::vector<miutil::miString> DrawingDialog::getOKString()
{
  std::vector<miutil::miString> lines;

  DrawingManager *drawm = DrawingManager::instance();
  if (!drawm->isEnabled())
    return lines;

  for (int i = 0; i < itemList->topLevelItemCount(); ++i) {

    QTreeWidgetItem *listItem = itemList->topLevelItem(i);

    QString type = listItem->data(0, Qt::DisplayRole).toString();
    int group = listItem->data(0, Qt::UserRole).toInt();
    QString time = listItem->data(1, Qt::DisplayRole).toString();
    QVariantList vPoints = listItem->data(0, PointsRole).toList();
    QStringList points;
    foreach (QVariant v, vPoints) {
      QPointF p = v.toPointF();
      points.append(QString("%1,%2").arg(p.x()).arg(p.y()));
    }

    QString line = QString("DRAWING type=%1 group=%2 time=%3 polygon=%4").arg(type)
        .arg(group).arg(time).arg(points.join(":"));
    lines.push_back(line.toStdString());
  }

  return lines;
}

void DrawingDialog::putOKString(const vector<miutil::miString>& vstr)
{
  cerr << "*** DrawingDialog::putOKString" << endl;
}

void DrawingDialog::toggleDrawingMode(bool enable)
{
  DrawingManager::instance()->setEnabled(enable);
}

void DrawingDialog::addItem(DrawingItemBase *item)
{
  QTreeWidgetItem *listItem = new QTreeWidgetItem();
  if (static_cast<EditItem_WeatherArea::WeatherArea*>(item))
      listItem->setText(0, tr("Area"));
  else
      listItem->setText(0, tr("Unknown"));
  listItem->setData(0, IdRole, item->id());
  listItem->setData(0, GroupRole, item->groupId());
  listItem->setData(1, Qt::DisplayRole, item->properties().value("time", QVariant("")));
  QVariantList points;
  foreach (QPointF p, item->getLatLonPoints())
    points.append(QVariant(p));
  listItem->setData(0, PointsRole, points);
  itemList->addTopLevelItem(listItem);
}

void DrawingDialog::removeItem(DrawingItemBase *item)
{
  for (int i = 0; i < itemList->topLevelItemCount(); ++i)
    if (itemList->topLevelItem(i)->data(0, IdRole).toInt() == item->id()) {
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
    ids.insert(listItem->data(0, IdRole).toInt());
  }

  EditItemManager *eim = EditItemManager::instance();
  foreach (DrawingItemBase *item, eim->getItems()) {
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
  EditItemManager *eim = EditItemManager::instance();
  QSet<int> ids;
  foreach (DrawingItemBase *item, eim->getSelectedItems())
    ids.insert(item->id());

  for (int i = 0; i < itemList->topLevelItemCount(); ++i) {
    // Select the item in the list to match the corresponding item on the map.
    QTreeWidgetItem *listItem = itemList->topLevelItem(i);
    listItem->setSelected(ids.contains(listItem->data(0, IdRole).toInt()));
  }
}

/**
 * Updates an item in the item list with properties from an item in the editor.
 */
void DrawingDialog::updateItem(DrawingItemBase *item)
{
  // Refresh the columns for the corresponding list item.
  for (int i = 0; i < itemList->topLevelItemCount(); ++i) {
    QTreeWidgetItem *listItem = itemList->topLevelItem(i);
    if (listItem->data(0, IdRole).toInt() == item->id()) {
      listItem->setData(1, Qt::DisplayRole, item->properties().value("time", QVariant("-")));
    }
  }
}