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

#include "selectall.xpm"
#include "deselectall.xpm"
#include "addempty.xpm"
#include "fileopen.xpm"
#include "merge.xpm"
#include "duplicate.xpm"
#include "remove.xpm"
#include "filesave.xpm"

namespace EditItems {

EditDrawingLayersPane::EditDrawingLayersPane(EditItems::LayerManager *layerManager, const QString &title)
  : LayersPaneBase(layerManager, title, true)
  , addEmptyButton_(0)
  , addFromFileButton_(0)
  , selectAllItemsButton_(0)
  , deselectAllItemsButton_(0)
  , mergeSelectedButton_(0)
  , duplicateSelectedButton_(0)
  , removeSelectedButton_(0)
  , saveSelectedButton_(0)
{
  bottomLayout_->addWidget(showAllButton_ = createToolButton(QPixmap(showall_xpm), "Show all layers", this, SLOT(showAll())));
  bottomLayout_->addWidget(hideAllButton_ = createToolButton(QPixmap(hideall_xpm), "Hide all layers", this, SLOT(hideAll())));
  bottomLayout_->addWidget(addEmptyButton_ = createToolButton(QPixmap(addempty_xpm), "Add an empty layer", this, SLOT(addEmpty())));
  bottomLayout_->addWidget(addFromFileButton_ = createToolButton(QPixmap(fileopen_xpm), "Add layers from file", this, SLOT(addFromFile())));
  bottomLayout_->addWidget(moveUpButton_ = createToolButton(QPixmap(moveup_xpm), "Move selected layer up", this, SLOT(moveSingleSelectedUp())));
  bottomLayout_->addWidget(moveDownButton_ = createToolButton(QPixmap(movedown_xpm), "Move selected layer down", this, SLOT(moveSingleSelectedDown())));
  bottomLayout_->addWidget(editButton_ = createToolButton(QPixmap(edit_xpm), "Edit attributes of selected layer", this, SLOT(editAttrsOfSingleSelected())));
  bottomLayout_->addWidget(selectAllItemsButton_ = createToolButton(QPixmap(selectall_xpm), "Select all items in selected layers", this, SLOT(selectAll())));
  bottomLayout_->addWidget(deselectAllItemsButton_ = createToolButton(QPixmap(deselectall_xpm), "Deselect all items in selected layers", this, SLOT(deselectAll())));
  bottomLayout_->addWidget(mergeSelectedButton_ = createToolButton(QPixmap(merge_xpm), "Merge selected layers", this, SLOT(mergeSelected())));
  bottomLayout_->addWidget(duplicateSelectedButton_ = createToolButton(QPixmap(duplicate_xpm), "Duplicate selected layers", this, SLOT(duplicateSelected())));
  bottomLayout_->addWidget(removeSelectedButton_ = createToolButton(QPixmap(remove_xpm), "Remove selected layers", this, SLOT(removeSelected())));
  bottomLayout_->addWidget(saveSelectedButton_ = createToolButton(QPixmap(filesave), "Save selected layers to file", this, SLOT(saveSelected())));
  bottomLayout_->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding));

  // create context menu actions
  selectAll_act_ = new QAction(QPixmap(selectall_xpm), tr("Select All"), 0);
  selectAll_act_->setIconVisibleInMenu(true);
  deselectAll_act_ = new QAction(QPixmap(deselectall_xpm), tr("Deselect All"), 0);
  deselectAll_act_->setIconVisibleInMenu(true);
  merge_act_ = new QAction(QPixmap(merge_xpm), tr("Merge"), 0);
  merge_act_->setIconVisibleInMenu(true);
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

  int allLayersCount;
  int selectedLayersCount;
  int visibleLayersCount;
  int removableLayersCount;
  getLayerCounts(allLayersCount, selectedLayersCount, visibleLayersCount, removableLayersCount);
  Q_UNUSED(allLayersCount);
  int allItemsCount;
  int selectedItemsCount;
  getSelectedLayersItemCounts(allItemsCount, selectedItemsCount);

  selectAllItemsButton_->setEnabled(selectedItemsCount < allItemsCount);
  deselectAllItemsButton_->setEnabled(selectedItemsCount > 0);
  mergeSelectedButton_->setEnabled(selectedLayersCount > 1);
  duplicateSelectedButton_->setEnabled(selectedLayersCount > 0);
  removeSelectedButton_->setEnabled(removableLayersCount > 0);
  saveSelectedButton_->setEnabled(visibleLayersCount > 0);
}

void EditDrawingLayersPane::addContextMenuActions(QMenu &menu) const
{
  selectAll_act_->setEnabled(selectAllItemsButton_->isEnabled());
  menu.addAction(selectAll_act_);
  deselectAll_act_->setEnabled(deselectAllItemsButton_->isEnabled());
  menu.addAction(deselectAll_act_);
  merge_act_->setEnabled(mergeSelectedButton_->isEnabled());
  menu.addAction(merge_act_);
  duplicate_act_->setEnabled(duplicateSelectedButton_->isEnabled());
  menu.addAction(duplicate_act_);
  remove_act_->setEnabled(removeSelectedButton_->isEnabled());
  menu.addAction(remove_act_);
}

bool EditDrawingLayersPane::handleContextMenuAction(const QAction *action, const QList<LayerWidget *> &layerWidgets)
{
  if (action == selectAll_act_) {
    selectAll();
    return true;
  }

  if (action == deselectAll_act_) {
    deselectAll();
    return true;
  }

  if (action == merge_act_) {
    merge(layerWidgets);
    return true;
  }

  if (action == duplicate_act_) {
    duplicate(layerWidgets);
    return true;
  }

  if (action == remove_act_) {
    LayersPaneBase::remove(layerWidgets);
    return true;
  }

  return false;
}

bool EditDrawingLayersPane::handleKeyPressEvent(QKeyEvent *event)
{
  if ((event->key() == Qt::Key_Delete) || (event->key() == Qt::Key_Backspace)) {
    removeSelected();
    return true;
  }
  return false;
}

void EditDrawingLayersPane::addDuplicate(const QSharedPointer<Layer> &layer)
{
  // initialize screen coordinates from lat/lon
  for (int i = 0; i < layer->itemCount(); ++i)
    DrawingManager::instance()->setFromLatLonPoints(*(layer->itemRef(i)), layer->item(i)->getLatLonPoints());

  const QSharedPointer<Layer> newLayer = layerMgr_->createDuplicateLayer(QList<QSharedPointer<Layer> >() << layer, EditItemManager::instance());
  layerMgr_->addToLayerGroup(layerGroup_, newLayer);
  add(newLayer);
}

void EditDrawingLayersPane::add(const QSharedPointer<Layer> &layer, bool skipUpdate, bool removable, bool nameEditable)
{
  LayerWidget *layerWidget = new LayerWidget(layerMgr_, layer, showInfo_, removable, nameEditable);
  layout_->addWidget(layerWidget);
  initLayerWidget(layerWidget);
  if (!skipUpdate)
    selectExclusive(layerWidget);
}

void EditDrawingLayersPane::addEmpty()
{
  const QSharedPointer<Layer> newLayer = layerMgr_->createNewLayer();
  layerMgr_->addToLayerGroup(layerGroup_, newLayer);
  add(newLayer);
}

void EditDrawingLayersPane::addFromFile()
{
  QString error;
  const QList<QSharedPointer<Layer> > layers = createLayersFromFile(layerMgr_, &error);
  if (!error.isEmpty()) {
    QMessageBox::warning(0, "Error", QString("failed to add to layer group from file: %1").arg(error));
    return;
  }

  if (layers.isEmpty()) {
    QMessageBox::warning(0, "Warning", QString("no layers found"));
    return;
  }

  foreach (const QSharedPointer<Layer> &layer, layers) {
    layerMgr_->addToLayerGroup(layerGroup_, layer);
    add(layer, true);
  }

  selectExclusive(widgets(layers));

  emit updated();
}

void EditDrawingLayersPane::selectAll()
{
  foreach (LayerWidget *lw, selectedWidgets())
    lw->layer()->selectAllItems();
}

void EditDrawingLayersPane::deselectAll()
{
  foreach (LayerWidget *lw, selectedWidgets())
    lw->layer()->deselectAllItems();
}

// Merges two or more layers into the topmost layer, removing the other layers.
void EditDrawingLayersPane::merge(const QList<LayerWidget *> &layerWidgets)
{
  if (layerWidgets.size() < 2)
    return;

  if (QMessageBox::warning(
         this, "Merge layers", QString("About to merge %1 layer%2; continue?")
        .arg(layerWidgets.size()).arg(layerWidgets.size() != 1 ? "s" : ""),
         QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
    return;

  const QList<LayerWidget *> srcLayerWidgets = layerWidgets.mid(1);
  LayerWidget *tgtLayerWidget = layerWidgets.first();
  layerMgr_->mergeLayers(layers(srcLayerWidgets), tgtLayerWidget->layer());
  remove(srcLayerWidgets, false, false);
}

void EditDrawingLayersPane::mergeSelected()
{
  merge(selectedWidgets());
}

// Merges one or more layers into a single new layer, keeping the original layers.
void EditDrawingLayersPane::duplicate(const QList<LayerWidget *> &layerWidgets)
{
  if (layerWidgets.isEmpty())
    return;

  const QSharedPointer<Layer> newLayer = layerMgr_->createDuplicateLayer(layers(layerWidgets), EditItemManager::instance());
  layerMgr_->addToLayerGroup(layerGroup_, newLayer);
  LayerWidget *newLayerWidget = new LayerWidget(layerMgr_, newLayer, showInfo_);
  layout_->insertWidget(layout_->indexOf(layerWidgets.last()) + 1, newLayerWidget);
  initLayerWidget(newLayerWidget);
  select(newLayerWidget);
  select(layerWidgets, false);
}

void EditDrawingLayersPane::duplicateSelected()
{
  duplicate(selectedWidgets());
}

void EditDrawingLayersPane::removeSelected()
{
  remove(selectedWidgets());
}

void EditDrawingLayersPane::saveSelected() const
{
  const QString fileName = QFileDialog::getSaveFileName(0, QObject::tr("Save File"),
    DrawingManager::instance()->getWorkDir(), QObject::tr("KML files (*.kml);; All files (*)"));
  if (fileName.isEmpty())
    return;

  QString error = LayersPaneBase::saveSelected(fileName);

  if (!error.isEmpty())
    QMessageBox::warning(0, "Error", QString("failed to save selected layers to file: %1").arg(error));
}

void EditDrawingLayersPane::handleLayerUpdate()
{
  updateButtons();
}

void EditDrawingLayersPane::initLayerWidget(LayerWidget *layerWidget)
{
  LayersPaneBase::initLayerWidget(layerWidget);
  if (layerWidget->layer())
    connect(layerWidget->layer().data(), SIGNAL(updated()), SLOT(handleLayerUpdate()), Qt::UniqueConnection);
}

} // namespace
