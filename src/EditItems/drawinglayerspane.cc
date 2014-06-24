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

namespace EditItems {

DrawingLayersPane::DrawingLayersPane(EditItems::LayerManager *layerManager, const QString &title)
  : LayersPaneBase(layerManager, title)
{
  bottomLayout_->addWidget(showAllButton_ = createToolButton(QPixmap(showall_xpm), "Show all layers", this, SLOT(showAll())));
  bottomLayout_->addWidget(hideAllButton_ = createToolButton(QPixmap(hideall_xpm), "Hide all layers", this, SLOT(hideAll())));
  bottomLayout_->addWidget(moveCurrentUpButton_ = createToolButton(QPixmap(moveup_xpm), "Move the current layer up", this, SLOT(moveCurrentUp())));
  bottomLayout_->addWidget(moveCurrentDownButton_ = createToolButton(QPixmap(movedown_xpm), "Move the current layer down", this, SLOT(moveCurrentDown())));
  bottomLayout_->addWidget(editCurrentButton_ = createToolButton(QPixmap(edit_xpm), "Edit the current layer", this, SLOT(editCurrent())));
  bottomLayout_->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding));
}

void DrawingLayersPane::updateButtons()
{
  LayersPaneBase::updateButtons();
}

} // namespace
