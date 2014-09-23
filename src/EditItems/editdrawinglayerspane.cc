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
#include <EditItems/modifylayerscommand.h>
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

#include "undo.xpm"
#include "redo.xpm"
#include "selectall.xpm"
#include "deselectall.xpm"
#include "addempty.xpm"
#include "fileopen.xpm"
#include "merge.xpm"
#include "duplicate.xpm"
#include "remove.xpm"
#include "filesave.xpm"

#define MILOGGER_CATEGORY "diana.DrawingManager"
#include <miLogger/miLogging.h>

namespace EditItems {

EditDrawingLayersPane::EditDrawingLayersPane(EditItems::LayerManager *layerManager, const QString &title)
  : LayersPaneBase(layerManager, title, true, true)
  , scratchLayerName_("SCRATCH")
  , addEmptyButton_(0)
  , addFromFileButton_(0)
  , selectAllItemsButton_(0)
  , deselectAllItemsButton_(0)
  , mergeSelectedButton_(0)
  , duplicateSelectedButton_(0)
  , removeSelectedButton_(0)
  , saveSelectedButton_(0)
  , undoButton_(0)
  , redoButton_(0)
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
  //
  bottomLayout_->addWidget(undoButton_ = createToolButton(QPixmap(undo_xpm), "Undo", this, SLOT(undo())));
  undoButton_->setEnabled(EditItemManager::instance()->undoStack()->canUndo());
  connect(EditItemManager::instance()->undoStack(), SIGNAL(canUndoChanged(bool)), undoButton_, SLOT(setEnabled(bool)));
  //
  bottomLayout_->addWidget(redoButton_ = createToolButton(QPixmap(redo_xpm), "Redo", this, SLOT(redo())));
  redoButton_->setEnabled(EditItemManager::instance()->undoStack()->canRedo());
  connect(EditItemManager::instance()->undoStack(), SIGNAL(canRedoChanged(bool)), redoButton_, SLOT(setEnabled(bool)));
  //
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
  save_act_ = new QAction(QPixmap(filesave), tr("Save"), 0);
  save_act_->setIconVisibleInMenu(true);

  // add scratch layer
  const QSharedPointer<Layer> scratchLayer = layerManager->createNewLayer(scratchLayerName_, false, false);
  layerManager->addToNewLayerGroup(scratchLayer, "");
  scratchLayer->layerGroupRef()->setActive(true);
  add(scratchLayer, false);

  connect(EditItemManager::instance(), SIGNAL(loadFile(const QString &)), SLOT(loadFile(const QString &)));
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
  save_act_->setEnabled(saveSelectedButton_->isEnabled());
  menu.addAction(save_act_);
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

  if (action == save_act_) {
    save(layerWidgets);
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

  Q_ASSERT(undoEnabled_);
  const QSharedPointer<Layer> newLayer = layerMgr_->createDuplicateLayer(layer, EditItemManager::instance());
  EditItemManager::instance()->undoStack()->push(
        new AddLayersCommand(
          "add duplicated layer", layerMgr_, newLayer, layerMgr_->layerGroups().indexOf(layerMgr_->findLayer(scratchLayerName_)->layerGroupRef())));
}

void EditDrawingLayersPane::add(const QSharedPointer<Layer> &layer, bool skipUpdate)
{
  LayerWidget *layerWidget = new LayerWidget(layerMgr_, layer, showInfo_);
  layout_->addWidget(layerWidget);
  initLayerWidget(layerWidget);
  if (!skipUpdate)
    selectExclusive(layerWidget);
}

void EditDrawingLayersPane::addEmpty()
{
  Q_ASSERT(undoEnabled_);
  EditItemManager::instance()->undoStack()->push(
        new AddEmptyLayerCommand(layerMgr_, layerMgr_->layerGroups().indexOf(layerMgr_->findLayer(scratchLayerName_)->layerGroupRef())));
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

  Q_ASSERT(undoEnabled_);
  EditItemManager::instance()->undoStack()->push(
        new AddLayersCommand(
          "add layer(s) from file", layerMgr_, layers, layerMgr_->layerGroups().indexOf(layerMgr_->findLayer(scratchLayerName_)->layerGroupRef())));
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

  Q_ASSERT(undoEnabled_);
  QBitArray mergeable(layerMgr_->orderedLayers().size());
  foreach (LayerWidget *lw, layerWidgets)
    mergeable.setBit(layerMgr_->orderedLayers().indexOf(lw->layer()));
  EditItemManager::instance()->undoStack()->push(new MergeLayersCommand(layerMgr_, mergeable));
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

  Q_ASSERT(undoEnabled_);
  QBitArray srcLayerIndexes(layerMgr_->orderedLayers().size());
  foreach (LayerWidget *lw, layerWidgets)
    srcLayerIndexes.setBit(layerMgr_->orderedLayers().indexOf(lw->layer()));
  EditItemManager::instance()->undoStack()->push(
        new DuplicateLayersCommand(layerMgr_, srcLayerIndexes, layerMgr_->layerGroups().indexOf(layerMgr_->findLayer(scratchLayerName_)->layerGroupRef())));
}

void EditDrawingLayersPane::duplicateSelected()
{
  duplicate(selectedWidgets());
}

void EditDrawingLayersPane::removeSelected()
{
  remove(selectedWidgets());
}

void EditDrawingLayersPane::save(const QList<LayerWidget *> &layerWidgets)
{
  QSet<QString> srcFiles;
  foreach (LayerWidget *lw, layerWidgets) {
    srcFiles.unite(lw->layer()->srcFiles());
    foreach (const QSharedPointer<DrawingItemBase> &item, lw->layer()->items()) {
      if (item->properties().contains("srcFile"))
          srcFiles.insert(item->properties().value("srcFile").toString());
    }
  }

  QString fileName;
  bool cancel;
  if (!srcFiles.isEmpty()) {
    QStringList sortedSrcFiles = srcFiles.toList();
    qSort(sortedSrcFiles);
    fileName = selectString("Select a file", "Select another file", lastSelSaveFName_, sortedSrcFiles, cancel);
  }
  if (cancel)
    return;
  if (fileName.isEmpty()) {
    fileName = QFileDialog::getSaveFileName(0, QObject::tr("Save File"),
                                            DrawingManager::instance()->getWorkDir(), QObject::tr("KML files (*.kml);; All files (*)"));
    if (fileName.isEmpty())
      return;
  } else {
    const QFileInfo fileInfo(fileName);
    if (fileInfo.exists() &&
        (QMessageBox::warning(
           this, "Overwrite file?", QString("A file named %1 already exists.\nDo you want to replace it?").arg(fileInfo.absoluteFilePath()),
           QMessageBox::Yes | QMessageBox::No) == QMessageBox::No))
        return;
  }

  lastSelSaveFName_ = fileName;
  QString error = LayersPaneBase::saveSelected(fileName);

  if (!error.isEmpty())
    QMessageBox::warning(0, "Error", QString("failed to save selected layers to file: %1").arg(error));
}

void EditDrawingLayersPane::saveSelected()
{
  save(selectedWidgets());
}

void EditDrawingLayersPane::undo()
{
  EditItemManager::instance()->undoStack()->undo();
}

void EditDrawingLayersPane::redo()
{
  EditItemManager::instance()->undoStack()->redo();
}

void EditDrawingLayersPane::handleLayersUpdate()
{
  LayersPaneBase::handleLayersUpdate();
  updateButtons();
}

void EditDrawingLayersPane::loadFile(const QString &fileName)
{
  QString error;
  const QList<QSharedPointer<Layer> > layers = createLayersFromFile(fileName, layerMgr_, &error);\
  if (error.isEmpty()) {
    Q_ASSERT(undoEnabled_);
    EditItemManager::instance()->undoStack()->push(
          new AddLayersCommand(
            "add layer(s) from file", layerMgr_, layers, layerMgr_->layerGroups().indexOf(layerMgr_->findLayer(scratchLayerName_)->layerGroupRef())));
  } else {
    METLIBS_LOG_WARN("Failed to load layers from file " << fileName.toStdString() << ": " << error.toStdString());
  }
}

} // namespace
