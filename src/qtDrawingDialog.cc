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

  QToolBar *drawingToolBar = new QToolBar();
  foreach (QAction *a, DrawingManager::instance()->actions())
    drawingToolBar->addAction(a);

  itemList = new QListWidget();

  EditItemManager *editor = DrawingManager::instance()->getEditItemManager();
  connect(editor, SIGNAL(itemAdded(EditItemBase*)), SLOT(addItem(EditItemBase*)));
  connect(editor, SIGNAL(itemRemoved(EditItemBase*)), SLOT(removeItem(EditItemBase*)));
  connect(DrawingManager::instance(), SIGNAL(timesUpdated()), SLOT(updateTimes()));

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addWidget(drawingToolBar);
  layout->addWidget(itemList);
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
  QListWidgetItem *listItem = new QListWidgetItem();
  if (static_cast<EditItem_WeatherArea::WeatherArea*>(item))
      listItem->setText(tr("Area"));
  else if (static_cast<EditItem_WeatherFront::WeatherFront*>(item))
      listItem->setText(tr("Front"));
  else
      listItem->setText(tr("Unknown"));
  listItem->setData(Qt::UserRole, item->id());
  itemList->addItem(listItem);
}

void DrawingDialog::removeItem(EditItemBase *item)
{
  for (int i = 0; i < itemList->count(); ++i)
    if (itemList->item(i)->data(Qt::UserRole).toInt() == item->id()) {
      delete itemList->takeItem(i);
      break;
    }
}
