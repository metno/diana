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
  connect(polyLineCombo, SIGNAL(currentIndexChanged(int)), polyLineAction, SLOT(trigger()));
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
  symbolAction = actions[EditItemManager::CreateSymbol];
  addAction(symbolAction);
  actionGroup->addAction(actions[EditItemManager::CreateSymbol]);

  // Create a combo box containing specific symbols.
  symbolCombo = new QComboBox();
  connect(symbolCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(setSymbolType(int)));
  connect(symbolCombo, SIGNAL(currentIndexChanged(int)), symbolAction, SLOT(trigger()));
  addWidget(symbolCombo);

  DrawingManager *dm = DrawingManager::instance();

  // Create an entry for each style. Use the name as an internal identifier
  // since we may decide to use tr() on the visible name at some point.

  foreach (QString name, dm->symbolNames()) {
    QIcon icon(QPixmap::fromImage(dm->getSymbolImage(name, 32, 32)));
    symbolCombo->addItem(icon, name, name);
  }

  symbolCombo->setCurrentIndex(0);

  // *** create text ***
  addAction(actions[EditItemManager::CreateText]);
  actionGroup->addAction(actions[EditItemManager::CreateText]);

  // Select the first action in the group by default.
  actionGroup->actions().at(0)->trigger();
}

void ToolBar::setPolyLineType(int index)
{
  // Obtain the style identifier from the style action and store it in the
  // main polyline action for later retrieval by the EditItemManager.
  polyLineAction->setData(polyLineCombo->itemData(index));
}

void ToolBar::setSymbolType(int index)
{
  // Obtain the style identifier from the style action and store it in the
  // main symbol action for later retrieval by the EditItemManager.
  symbolAction->setData(symbolCombo->itemData(index));
}

} // namespace
