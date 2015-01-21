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

#include <EditItems/drawinglayerspane.h>
#include <EditItems/layermanager.h>
#include <EditItems/dialogcommon.h>
#include <QPixmap>
#include <QToolButton>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QMenu>
#include <QAction>

#include "duplicate_to_editable.xpm"
#include "edit.xpm"
#include "hideall.xpm"
#include "movedown.xpm"
#include "moveup.xpm"
#include "showall.xpm"

namespace EditItems {

DrawingLayersPane::DrawingLayersPane(EditItems::LayerManager *layerManager, const QString &title)
  : LayersPaneBase(layerManager, title, false, false)
{
  bottomLayout_->addWidget(showAllButton_ = createToolButton(QPixmap(showall_xpm), "Show all layers", this, SLOT(showAll())));
  bottomLayout_->addWidget(hideAllButton_ = createToolButton(QPixmap(hideall_xpm), "Hide all layers", this, SLOT(hideAll())));
  bottomLayout_->addWidget(moveUpButton_ = createToolButton(QPixmap(moveup_xpm), "Move selected layer up", this, SLOT(moveSingleSelectedUp())));
  bottomLayout_->addWidget(moveDownButton_ = createToolButton(QPixmap(movedown_xpm), "Move selected layer down", this, SLOT(moveSingleSelectedDown())));
  bottomLayout_->addWidget(editButton_ = createToolButton(QPixmap(edit_xpm), "Edit attributes of selected layer", this, SLOT(editAttrsOfSingleSelected())));
  bottomLayout_->addWidget(duplicateToEditableButton_ =
      createToolButton(QPixmap(duplicate_to_editable_xpm), "Duplicate selected layer to an editable layer", this, SLOT(duplicateSelectedToEditable())));
  bottomLayout_->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding));

  // create context menu actions
  duplicateToEditable_act_ = new QAction(QPixmap(duplicate_to_editable_xpm), tr("Duplicate to editable"), 0);
  duplicateToEditable_act_->setIconVisibleInMenu(true);
}

void DrawingLayersPane::updateButtons()
{
  LayersPaneBase::updateButtons();

  int allLayersCount;
  int selectedLayersCount;
  int visibleLayersCount;
  int removableLayersCount;
  getLayerCounts(allLayersCount, selectedLayersCount, visibleLayersCount, removableLayersCount);
  Q_UNUSED(allLayersCount);
  Q_UNUSED(visibleLayersCount);
  Q_UNUSED(removableLayersCount);

  duplicateToEditableButton_->setEnabled(selectedLayersCount == 1);
}

void DrawingLayersPane::addContextMenuActions(QMenu &menu) const
{
  duplicateToEditable_act_->setEnabled(duplicateToEditableButton_->isEnabled());
  menu.addAction(duplicateToEditable_act_);
}

bool DrawingLayersPane::handleContextMenuAction(const QAction *action, const QList<LayerWidget *> &layerWidgets)
{
  if (action == duplicateToEditable_act_) {
    if (layerWidgets.size() == 1)
      duplicateToEditable(layerWidgets.first()->layer());
    return true;
  }

  return false;
}

// Duplicates the given layer to a new layer in the EditDrawingDialog.
void DrawingLayersPane::duplicateToEditable(const QSharedPointer<Layer> &layer)
{
  emit newEditLayerRequested(layer);
}

// Duplicates the selected layer to a new layer in the EditDrawingDialog.
void DrawingLayersPane::duplicateSelectedToEditable()
{
  const QList<LayerWidget *> selWidgets = selectedWidgets();
  if (selWidgets.size() != 1)
    return;
  duplicateToEditable(selWidgets.first()->layer());
}

} // namespace
