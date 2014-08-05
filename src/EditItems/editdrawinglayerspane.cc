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

#include <EditItems/editdrawinglayerspane.h>
#include <EditItems/layer.h>
#include <EditItems/layergroup.h>
#include <EditItems/layermanager.h>
#include <EditItems/dialogcommon.h>
#include <diDrawingManager.h>
#include <diEditItemManager.h>
#include <QMessageBox>
#include <QFileDialog>
#include <QPixmap>
#include <QToolButton>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QMenu>
#include <QAction>

#include "addempty.xpm"
#include "mergevisible.xpm"
#include "duplicate.xpm"
#include "remove.xpm"
#include "filesave.xpm"

namespace EditItems {

EditDrawingLayersPane::EditDrawingLayersPane(EditItems::LayerManager *layerManager, const QString &title)
  : LayersPaneBase(layerManager, title)
  , addEmptyButton_(0)
  , mergeVisibleButton_(0)
  , duplicateCurrentButton_(0)
  , removeCurrentButton_(0)
  , saveVisibleButton_(0)
{
  bottomLayout_->addWidget(addEmptyButton_ = createToolButton(QPixmap(addempty_xpm), "Add an empty layer", this, SLOT(addEmpty())));
  bottomLayout_->addWidget(mergeVisibleButton_ = createToolButton(QPixmap(mergevisible_xpm), "Merge visible layers into a new layer", this, SLOT(mergeVisible())));
  bottomLayout_->addWidget(showAllButton_ = createToolButton(QPixmap(showall_xpm), "Show all layers", this, SLOT(showAll())));
  bottomLayout_->addWidget(hideAllButton_ = createToolButton(QPixmap(hideall_xpm), "Hide all layers", this, SLOT(hideAll())));
  bottomLayout_->addWidget(duplicateCurrentButton_ = createToolButton(QPixmap(duplicate_xpm), "Duplicate the current layer", this, SLOT(duplicateCurrent())));
  bottomLayout_->addWidget(removeCurrentButton_ = createToolButton(QPixmap(remove_xpm), "Remove the current layer", this, SLOT(removeCurrent())));
  bottomLayout_->addWidget(moveCurrentUpButton_ = createToolButton(QPixmap(moveup_xpm), "Move the current layer up", this, SLOT(moveCurrentUp())));
  bottomLayout_->addWidget(moveCurrentDownButton_ = createToolButton(QPixmap(movedown_xpm), "Move the current layer down", this, SLOT(moveCurrentDown())));
  bottomLayout_->addWidget(editCurrentButton_ = createToolButton(QPixmap(edit_xpm), "Edit attributes of the current layer", this, SLOT(editAttrsOfCurrent())));
  bottomLayout_->addWidget(saveVisibleButton_ = createToolButton(QPixmap(filesave), "Save visible layers to file", this, SLOT(saveVisible())));
  bottomLayout_->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding));

  // create context menu actions
  duplicate_act_ = new QAction(QPixmap(duplicate_xpm), tr("Duplicate"), 0);
  duplicate_act_->setIconVisibleInMenu(true);
  remove_act_ = new QAction(QPixmap(remove_xpm), tr("Remove"), 0);
  remove_act_->setIconVisibleInMenu(true);

  // add scratch layer
  const QSharedPointer<Layer> scratchLayer = layerManager->createNewLayer("SCRATCH");
  layerGroup_ = layerManager->addToNewLayerGroup(scratchLayer, "");
  layerGroup_->setActive(true);
  add(scratchLayer, false, false, false);
}

void EditDrawingLayersPane::updateButtons()
{
  LayersPaneBase::updateButtons();

  const int visSize = visibleWidgets().size();
  mergeVisibleButton_->setEnabled(visSize > 1);
  duplicateCurrentButton_->setEnabled(current());

  // the layer may be removed iff it is current, editable, and belongs to a removable widget
  removeCurrentButton_->setEnabled(current() && current()->layer()->isEditable() && current()->isRemovable());

  saveVisibleButton_->setEnabled(visSize > 0);
}

void EditDrawingLayersPane::addContextMenuActions(QMenu &menu) const
{
  duplicate_act_->setEnabled(duplicateCurrentButton_->isEnabled());
  menu.addAction(duplicate_act_);
  remove_act_->setEnabled(removeCurrentButton_->isEnabled());
  menu.addAction(remove_act_);
}

bool EditDrawingLayersPane::handleContextMenuAction(const QAction *action, LayerWidget *layerWidget)
{
  if (action == duplicate_act_) {
    duplicate(layerWidget);
    return true;
  }

  if (action == remove_act_) {
    LayersPaneBase::remove(layerWidget);
    return true;
  }

  return false;
}

bool EditDrawingLayersPane::handleKeyPressEvent(QKeyEvent *event)
{
  if ((event->key() == Qt::Key_Delete) || (event->key() == Qt::Key_Backspace)) {
    removeCurrent();
    return true;
  }
  return false;
}

void EditDrawingLayersPane::add(const QSharedPointer<Layer> &layer, bool skipUpdate, bool removable, bool nameEditable)
{
  LayerWidget *layerWidget = new LayerWidget(layerMgr_, layer, showInfo_, removable, nameEditable);
  layout_->addWidget(layerWidget);
  initLayerWidget(layerWidget);
  if (!skipUpdate) {
    setCurrent(layerWidget);
    ensureCurrentVisible();
    handleWidgetsUpdate();
  }
}

void EditDrawingLayersPane::addEmpty()
{
  const QSharedPointer<Layer> newLayer = layerMgr_->createNewLayer();
  layerMgr_->addToLayerGroup(layerGroup_, newLayer);
  add(newLayer);
}

void EditDrawingLayersPane::mergeVisible()
{
#if 0
  const QList<LayerWidget *> visLayerWidgets = visibleWidgets();

  if (visLayerWidgets.size() > 1) {
    if (QMessageBox::warning(
          this, "Merge visible layers", "Really merge visible layers?",
          QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
      return;

    const QSharedPointer<Layer> newLayer = layerMgr_->createNewLayer("merged layer");
    layerMgr_->addToDefaultLayerGroup(newLayer); // ### NOTE: the default layer group is obsolete!

    layerMgr_->mergeLayers(layers(visLayerWidgets), newLayer);

    LayerWidget *newLayerWidget = new LayerWidget(layerMgr_, newLayer, showInfo_);
    layout_->insertWidget(layout_->count(), newLayerWidget);
    initialize(newLayerWidget);
    setCurrent(newLayerWidget);
    ensureCurrentVisible();
    handleWidgetsUpdate();

    // ### remove source layers (i.e. those associated with visLayerWidgets) ... TBD
  }

  handleWidgetsUpdate();
#else
  QMessageBox::warning(this, "tmp disabled", "merging visible is temporarily disabled!", QMessageBox::Ok);
#endif
}

void EditDrawingLayersPane::duplicate(LayerWidget *srcLayerWidget)
{
  const QSharedPointer<Layer> newLayer = layerMgr_->createDuplicateLayer(srcLayerWidget->layer(), EditItemManager::instance());
  layerMgr_->addToLayerGroup(layerGroup_, newLayer);
  const int srcIndex = layout_->indexOf(srcLayerWidget);
  LayerWidget *newLayerWidget = new LayerWidget(layerMgr_, newLayer, showInfo_);
  layout_->insertWidget(srcIndex + 1, newLayerWidget);
  initialize(newLayerWidget);
  setCurrent(newLayerWidget);
  ensureCurrentVisible();
  handleWidgetsUpdate();
}

void EditDrawingLayersPane::duplicateCurrent()
{
  duplicate(current());
}

void EditDrawingLayersPane::removeCurrent()
{
  remove(current());
}

void EditDrawingLayersPane::saveVisible() const
{
  const QString fileName = QFileDialog::getSaveFileName(0, QObject::tr("Save File"),
    DrawingManager::instance()->getWorkDir(), QObject::tr("KML files (*.kml);; All files (*)"));
  if (fileName.isEmpty())
    return;

  QString error = LayersPaneBase::saveVisible(fileName);

  if (!error.isEmpty())
    QMessageBox::warning(0, "Error", QString("failed to save visible layers to file: %1").arg(error));
}

void EditDrawingLayersPane::initialize(LayerWidget *layerWidget)
{
  connect(layerWidget, SIGNAL(mouseClicked(QMouseEvent *)), SLOT(mouseClicked(QMouseEvent *)), Qt::UniqueConnection);
  connect(layerWidget, SIGNAL(mouseDoubleClicked(QMouseEvent *)), SLOT(mouseDoubleClicked(QMouseEvent *)), Qt::UniqueConnection);
  connect(layerWidget, SIGNAL(visibilityChanged(bool)), SLOT(handleWidgetsUpdate()), Qt::UniqueConnection);
  if (layerWidget->layer()) {
    layerWidget->updateLabels();
    connect(layerWidget->layer().data(), SIGNAL(updated()), SIGNAL(updated()), Qt::UniqueConnection);
    connect(layerWidget->layer().data(), SIGNAL(updated()), layerWidget, SLOT(updateLabels()), Qt::UniqueConnection);
  }
}

} // namespace
