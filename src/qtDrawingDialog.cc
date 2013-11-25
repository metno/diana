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
#include "EditItems/editpolyline.h"
#include <paint_mode.xpm>       // reused for area drawing functionality

#include <QAction>
#include <QPushButton>
#include <QToolBar>
#include <QVBoxLayout>

DrawingDialog::DrawingDialog(QWidget *parent, Controller *ctrl)
  : DataDialog(parent, ctrl)
{
  // Create an action that can be used to open the dialog from within a menu or toolbar.
  m_action = new QAction(QIcon(QPixmap(paint_mode_xpm)), tr("Painting tools"), this);
  m_action->setShortcutContext(Qt::ApplicationShortcut);
  m_action->setShortcut(Qt::ALT + Qt::Key_B);
  m_action->setCheckable(true);
  m_action->setIconVisibleInMenu(true);
  connect(m_action, SIGNAL(toggled(bool)), SLOT(toggleDrawingMode(bool)));

  // Add buttons for actions exposed by the editor.
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

  // When the apply, hide or apply & hide buttons are pressed, we reset the undo stack.
  connect(this, SIGNAL(applyData()), EditItemManager::instance(), SLOT(reset()));
  connect(this, SIGNAL(hideData()), EditItemManager::instance(), SLOT(reset()));

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
  layout->addWidget(new ToolPanel);
  layout->addLayout(createStandardButtons());

  setWindowTitle(tr("Drawing"));
}

DrawingDialog::~DrawingDialog()
{
}

std::string DrawingDialog::name() const
{
  return "DRAWING";
}

void DrawingDialog::updateTimes()
{
  std::vector<miutil::miTime> times = DrawingManager::instance()->getTimes();
  emit emitTimes("DRAWING", times);
}

void DrawingDialog::updateDialog()
{
}

std::vector<std::string> DrawingDialog::getOKString()
{
  std::vector<std::string> lines;

  DrawingManager *drawm = DrawingManager::instance();

  for (int i = 0; i < itemList->topLevelItemCount(); ++i) {

    QTreeWidgetItem *listItem = itemList->topLevelItem(i);
    int id = listItem->data(0, IdRole).toInt();
    DrawingItemBase *item = itemHash[id];

    QString type = listItem->data(0, Qt::DisplayRole).toString();
    int group = item->groupId();
    QString time = item->property("time", "").toString();
    QStringList points;
    foreach (QPointF p, item->getLatLonPoints())
      points.append(QString("%1,%2").arg(p.x()).arg(p.y()));

    QString line = QString("DRAWING type=%1 group=%2 time=%3 points=%4").arg(type)
        .arg(group).arg(time).arg(points.join(":"));
    lines.push_back(line.toStdString());
  }

  return lines;
}

void DrawingDialog::putOKString(const std::vector<std::string>& vstr)
{
  // Submit the lines as new input.
  std::vector<std::string> inp;
  inp.insert(inp.begin(), vstr.begin(), vstr.end());
  DrawingManager::instance()->processInput(inp);
}

void DrawingDialog::toggleDrawingMode(bool enable)
{
  EditItemManager::instance()->setEditing(enable);
}

static QString shortClassName(const QString &className)
{
  const int separatorPos = className.lastIndexOf("::");
  return separatorPos == -1
      ? className
      : className.mid(separatorPos + 2);
}

void DrawingDialog::addItem(DrawingItemBase *item)
{
  QTreeWidgetItem *listItem = new QTreeWidgetItem();
  EditItemBase *editItem = dynamic_cast<EditItemBase *>(item);
  if (editItem)
      listItem->setText(0, shortClassName(editItem->metaObject()->className()));
  else
      listItem->setText(0, tr("Unknown"));
  listItem->setData(0, IdRole, item->id());
  listItem->setData(1, Qt::DisplayRole, item->property("time", ""));
  itemList->addTopLevelItem(listItem);
  listItemHash[item->id()] = listItem;
  itemHash[item->id()] = item;
}

void DrawingDialog::removeItem(DrawingItemBase *item)
{
  int i = itemList->indexOfTopLevelItem(listItemHash[item->id()]);
  delete itemList->takeTopLevelItem(i);
  listItemHash.remove(item->id());
  itemHash.remove(item->id());
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

void DrawingDialog::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Escape) {
    EditItemManager::instance()->setSelectMode();
    event->accept();
  } else {
    DataDialog::keyPressEvent(event);
  }
}

/**
 * Updates an item in the item list with properties from an item in the editor.
 */
void DrawingDialog::updateItem(DrawingItemBase *item)
{
  // Refresh the columns for the corresponding list item.
  QTreeWidgetItem *listItem = listItemHash[item->id()];
  listItem->setData(1, Qt::DisplayRole, item->property("time", ""));
}

ToolPanel::ToolPanel(QWidget *parent)
  : QWidget(parent)
{
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setSizeConstraint(QLayout::SetFixedSize);
  layout->setMargin(0);

  QHash<EditItemManager::Action, QAction *> actions = EditItemManager::instance()->actions();

  QToolButton *selectButton = new QToolButton();
  selectButton->setDefaultAction(actions[EditItemManager::Select]);
  layout->addWidget(selectButton);

  QToolButton *polyLineButton = new QToolButton();
  polyLineButton->setDefaultAction(actions[EditItemManager::CreatePolyLine]);
  layout->addWidget(polyLineButton);

  QToolButton *symbolButton = new QToolButton();
  symbolButton->setDefaultAction(actions[EditItemManager::CreateSymbol]);
  layout->addWidget(symbolButton);
}

ToolPanel::~ToolPanel()
{
}
