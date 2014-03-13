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
#include <QApplication>
#include <QComboBox>
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
    : QToolBar(QApplication::translate("EditItems::ToolBar", "Paint Operations") + " (NEW)", parent)
{
  QActionGroup *actionGroup = new QActionGroup(this);
  QHash<EditItemManager::Action, QAction *> actions = EditItemManager::instance()->actions();

  // *** select ***
  addAction(actions[EditItemManager::Select]);
  actionGroup->addAction(actions[EditItemManager::Select]);

  // *** create polyline ***
  polyLineAction = actions[EditItemManager::CreatePolyLine];
  addAction(polyLineAction);
  actionGroup->addAction(polyLineAction);

  // Create a combo box containing specific polyline types.
  polyLineCombo = new QComboBox();
  connect(polyLineCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(setPolyLineType(int)));
  addWidget(polyLineCombo);

  DrawingStyleManager *dsm = DrawingStyleManager::instance();

  // Create an entry for each style. Use the name as an internal identifier
  // since we may decide to use tr() on the visible name at some point.
  QStringList styles = dsm->styles();
  styles.sort();
  foreach (QString name, styles)
    polyLineCombo->addItem(name, name);

  polyLineCombo->setCurrentIndex(0);

  // *** create symbol ***
  addAction(actions[EditItemManager::CreateSymbol]);
  actionGroup->addAction(actions[EditItemManager::CreateSymbol]);

  // *** create text ***
  addAction(actions[EditItemManager::CreateText]);
  actionGroup->addAction(actions[EditItemManager::CreateText]);
}

void ToolBar::setPolyLineType(int index)
{
  // Obtain the style identifier from the style action and store it in the
  // main polyline action.
  polyLineAction->setData(polyLineCombo->itemData(index));
}

} // namespace
