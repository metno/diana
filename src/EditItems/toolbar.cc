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
#include "EditItems/editcomposite.h"
#include "EditItems/drawingstylemanager.h"
#include "EditItems/toolbar.h"
#include <QActionGroup>
#include <QApplication>
#include <QComboBox>

namespace EditItems {

ToolBar *ToolBar::instance()
{
  if (!ToolBar::self_)
    ToolBar::self_ = new ToolBar();
  return ToolBar::self_;
}

ToolBar *ToolBar::self_ = 0;

ToolBar::ToolBar(QWidget *parent)
    : QToolBar(QApplication::translate("EditItems::ToolBar", "Paint Operations") + " (NEW)", parent)
    , nonSelectActionLocked_(false)
{
  DrawingStyleManager *dsm = DrawingStyleManager::instance();

  QActionGroup *actionGroup = new QActionGroup(this);
  QHash<EditItemManager::Action, QAction *> actions = EditItemManager::instance()->actions();

  // *** select ***
  selectAction_ = actions[EditItemManager::Select];
  addAction(selectAction_);
  actionGroup->addAction(selectAction_);
  connect(selectAction_, SIGNAL(triggered(bool)), SLOT(handleSelectActionTriggered(bool)));

  // *** create polyline ***
  polyLineAction_ = actions[EditItemManager::CreatePolyLine];
  addAction(polyLineAction_);
  actionGroup->addAction(polyLineAction_);
  connect(polyLineAction_, SIGNAL(triggered(bool)), SLOT(handleNonSelectActionTriggered(bool)));

  // Create a combo box containing specific polyline types.
  polyLineCombo_ = new QComboBox();
  connect(polyLineCombo_, SIGNAL(currentIndexChanged(int)), this, SLOT(setPolyLineType(int)));
  connect(polyLineCombo_, SIGNAL(currentIndexChanged(int)), polyLineAction_, SLOT(trigger()));
  addWidget(polyLineCombo_);

  // Create an entry for each style. Use the name as an internal identifier
  // since we may decide to use tr() on the visible name at some point.
  QStringList styles = dsm->styles(DrawingItemBase::PolyLine);
  styles.sort();
  foreach (QString name, styles)
    polyLineCombo_->addItem(name, name);

  polyLineCombo_->setCurrentIndex(0);
  setPolyLineType(0);

  // *** create symbol ***
  symbolAction_ = actions[EditItemManager::CreateSymbol];
  addAction(symbolAction_);
  actionGroup->addAction(actions[EditItemManager::CreateSymbol]);
  connect(symbolAction_, SIGNAL(triggered(bool)), SLOT(handleNonSelectActionTriggered(bool)));

  // Create a combo box containing specific symbols.
  symbolCombo_ = new QComboBox();
  connect(symbolCombo_, SIGNAL(currentIndexChanged(int)), this, SLOT(setSymbolType(int)));
  connect(symbolCombo_, SIGNAL(currentIndexChanged(int)), symbolAction_, SLOT(trigger()));
  addWidget(symbolCombo_);

  DrawingManager *dm = DrawingManager::instance();

  // Create an entry for each style. Use the name as an internal identifier
  // since we may decide to use tr() on the visible name at some point.

  foreach (QString name, dm->symbolNames()) {
    QIcon icon(QPixmap::fromImage(dm->getSymbolImage(name, 32, 32)));
    symbolCombo_->addItem(icon, name, name);
  }

  symbolCombo_->setCurrentIndex(0);
  setSymbolType(0);

  // *** create text ***
  textAction_ = actions[EditItemManager::CreateText];
  addAction(textAction_);
  actionGroup->addAction(textAction_);
  connect(textAction_, SIGNAL(triggered(bool)), SLOT(handleNonSelectActionTriggered(bool)));

  // Create a combo box containing specific text types.
  textCombo_ = new QComboBox();
  connect(textCombo_, SIGNAL(currentIndexChanged(int)), this, SLOT(setTextType(int)));
  connect(textCombo_, SIGNAL(currentIndexChanged(int)), textAction_, SLOT(trigger()));
  addWidget(textCombo_);

  // Create an entry for each style. Use the name as an internal identifier
  // since we may decide to use tr() on the visible name at some point.
  styles = dsm->styles(DrawingItemBase::Text);
  styles.sort();
  foreach (QString name, styles)
    textCombo_->addItem(name, name);

  textCombo_->setCurrentIndex(0);
  setTextType(0);

  // *** create composite ***
  compositeAction_ = actions[EditItemManager::CreateComposite];
  addAction(compositeAction_);
  actionGroup->addAction(compositeAction_);
  connect(compositeAction_, SIGNAL(triggered(bool)), SLOT(handleNonSelectActionTriggered(bool)));

  // Create a combo box containing specific composite types.
  compositeCombo_ = new QComboBox();
  connect(compositeCombo_, SIGNAL(currentIndexChanged(int)), this, SLOT(setCompositeType(int)));
  connect(compositeCombo_, SIGNAL(currentIndexChanged(int)), compositeAction_, SLOT(trigger()));
  addWidget(compositeCombo_);

  // Create an entry for each style. Use the name as an internal identifier
  // since we may decide to use tr() on the visible name at some point.
  styles = dsm->styles(DrawingItemBase::Composite);
  styles.sort();

  foreach (QString name, styles) {
    if (dsm->getStyle(DrawingItemBase::Composite, name).value("hide").toBool() == false) {
      QImage image = dsm->toImage(DrawingItemBase::Composite, name);
      QIcon icon(QPixmap::fromImage(image));
      compositeCombo_->addItem(icon, name, name);
    }
  }

  compositeCombo_->setCurrentIndex(0);
  setCompositeType(0);

  // Select the first action in the group by default.
  actionGroup->actions().at(0)->trigger();
}

bool ToolBar::nonSelectActionLocked() const
{
  return nonSelectActionLocked_;
}

void ToolBar::setSelectAction()
{
  selectAction_->trigger();
}

void ToolBar::setCreatePolyLineAction(const QString &type)
{
  const int index = polyLineCombo_->findText(type);
  if (index >= 0) {
    polyLineCombo_->setCurrentIndex(index);
    polyLineAction_->trigger();
  }
}

void ToolBar::handleSelectActionTriggered(bool checked)
{
  if (checked)
    nonSelectActionLocked_ = false;
}

void ToolBar::handleNonSelectActionTriggered(bool checked)
{
  if (checked)
    nonSelectActionLocked_ = QApplication::keyboardModifiers().testFlag(Qt::ControlModifier);
}

void ToolBar::setPolyLineType(int index)
{
  // Obtain the style identifier from the style action and store it in the
  // main polyline action for later retrieval by the EditItemManager.
  polyLineAction_->setData(polyLineCombo_->itemData(index));
}

void ToolBar::setSymbolType(int index)
{
  // Obtain the style identifier from the style action and store it in the
  // main symbol action for later retrieval by the EditItemManager.
  symbolAction_->setData(symbolCombo_->itemData(index));
}

void ToolBar::setTextType(int index)
{
  // Obtain the style identifier from the style action and store it in the
  // main text action for later retrieval by the EditItemManager.
  textAction_->setData(textCombo_->itemData(index));
}

void ToolBar::setCompositeType(int index)
{
  // Obtain the style identifier from the style action and store it in the
  // main text action for later retrieval by the EditItemManager.
  compositeAction_->setData(compositeCombo_->itemData(index));
}

void ToolBar::showEvent(QShowEvent *event)
{
  emit visible(true);
  QWidget::showEvent(event);
}

void ToolBar::hideEvent(QHideEvent *event)
{
  emit visible(false);
  QWidget::hideEvent(event);
}

} // namespace
