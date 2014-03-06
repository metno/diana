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

#include "diEditItemManager.h"
#include "EditItems/drawingstylemanager.h"
#include "EditItems/toolbar.h"
#include <QActionGroup>
#include <QMenu>

namespace EditItems {

ToolBar *ToolBar::instance()
{
  if (!ToolBar::self)
    ToolBar::self = new ToolBar();
  return ToolBar::self;
}

ToolBar *ToolBar::self = 0;

ToolBar::ToolBar(QWidget *parent)
    : QToolBar(QString("%1").arg("Paint Operations (NEW)"), parent) // ### TBD: use tr() properly
{
  QActionGroup *actionGroup = new QActionGroup(this);
  QHash<EditItemManager::Action, QAction *> actions = EditItemManager::instance()->actions();

  // *** select ***
  addAction(actions[EditItemManager::Select]);
  actionGroup->addAction(actions[EditItemManager::Select]);

  // *** create polyline ***
  polyLineAction = actions[EditItemManager::CreatePolyLine];
  addAction(polyLineAction);

  // Create a menu containing specific polyline types.
  QMenu *polyLineMenu = new QMenu(this);
  actions[EditItemManager::CreatePolyLine]->setMenu(polyLineMenu);
  actionGroup->addAction(actions[EditItemManager::CreatePolyLine]);

  QActionGroup *polyLineGroup = new QActionGroup(this);
  connect(polyLineGroup, SIGNAL(triggered(QAction *)), this, SLOT(setPolyLineType(QAction *)));

  DrawingStyleManager *dsm = DrawingStyleManager::instance();
  bool first = true;

  foreach (QString name, dsm->styleNames()) {

    // Create an action for each style. Use the name as an internal identifier
    // since we may decide to use tr() on the visible name at some point.
    QAction *polyLineMenuAction = new QAction(name, polyLineGroup);
    polyLineMenuAction->setData(name);

    polyLineMenuAction->setCheckable(true);
    if (first) {
      polyLineMenuAction->setChecked(true);
      setPolyLineType(polyLineMenuAction);
      first = false;
    }
    polyLineMenu->addAction(polyLineMenuAction);
  }

  // *** create symbol ***
  addAction(actions[EditItemManager::CreateSymbol]);
  actionGroup->addAction(actions[EditItemManager::CreateSymbol]);
}

void ToolBar::setPolyLineType(QAction *action)
{
  // Obtain the style identifier from the style action and store it in the
  // main polyline action.
  polyLineAction->setData(action->data());
}

} // namespace
