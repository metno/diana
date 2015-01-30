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

#include <EditItems/modifylayerscommand.h>
#include <EditItems/layermanager.h>
#include <EditItems/layergroup.h>
#include <EditItems/layer.h>
#include <diEditItemManager.h>

#include <QBitArray>

namespace EditItems {

ModifyLayersCommand::ModifyLayersCommand(
    const QString &text,
    LayerManager *layerMgr,
    QUndoCommand *parent)
    : QUndoCommand(text, parent)
    , layerMgr_(layerMgr)
{
  layerMgr_->copyState(&oldLayerGroups_, &oldOrderedLayers_);
  layerMgr_->copyState(&newLayerGroups_, &newOrderedLayers_);
}

void ModifyLayersCommand::undo()
{
  layerMgr_->replaceState(oldLayerGroups_, oldOrderedLayers_);
  EditItemManager::instance()->updateJoins();
}

void ModifyLayersCommand::redo()
{
  layerMgr_->replaceState(newLayerGroups_, newOrderedLayers_);
  EditItemManager::instance()->updateJoins();
}

void ModifyLayersCommand::removeLayers(const QList<QSharedPointer<Layer> > &removableLayers)
{
  foreach (const QSharedPointer<Layer> &layer, removableLayers) {
    newOrderedLayers_.removeOne(layer);
    foreach (const QSharedPointer<LayerGroup> &layerGroup, newLayerGroups_)
      layerGroup->layersRef().removeOne(layer);
  }

  // avoid empty selection
  bool foundSelected = false;
  foreach (const QSharedPointer<Layer> &layer, newOrderedLayers_)
    if (layer->isSelected()) {
      foundSelected = true;
      break;
    }
  if ((!foundSelected) && (!newOrderedLayers_.isEmpty()))
    newOrderedLayers_.first()->setSelected();
}

AddEmptyLayerCommand::AddEmptyLayerCommand(LayerManager *layerMgr, int layerGroupIndex)
  : ModifyLayersCommand("add empty layer", layerMgr)
{
  const QSharedPointer<Layer> newLayer = layerMgr_->createNewLayer(newLayerGroups_.at(layerGroupIndex));
  newOrderedLayers_.append(newLayer);

  // single-select the new layer
  foreach (const QSharedPointer<Layer> &layer, newOrderedLayers_)
    layer->setSelected(layer == newLayer);
}

AddLayersCommand::AddLayersCommand(const QString &text, LayerManager *layerMgr, const QList<QSharedPointer<Layer> > &layers, int layerGroupIndex)
  : ModifyLayersCommand(text, layerMgr)
{
  init(layers, layerGroupIndex);
}

AddLayersCommand::AddLayersCommand(const QString &text, LayerManager *layerMgr, const QSharedPointer<Layer> &layer, int layerGroupIndex)
  : ModifyLayersCommand(text, layerMgr)
{
  init(QList<QSharedPointer<Layer> >() << layer, layerGroupIndex);
}

void AddLayersCommand::init(const QList<QSharedPointer<Layer> > &layers, int layerGroupIndex)
{
  // unselect the existing layers
  foreach (const QSharedPointer<Layer> &layer, newOrderedLayers_)
    layer->setSelected(false);

  // insert and select the new layers
  const QSharedPointer<LayerGroup> layerGroup = newLayerGroups_.at(layerGroupIndex);
  foreach (const QSharedPointer<Layer> &layer, layers) {
    newOrderedLayers_.append(layer);
    layerGroup->layersRef().append(layer);
    layer->setLayerGroup(layerGroup);
    layer->setSelected(true);
  }
}

DuplicateLayersCommand::DuplicateLayersCommand(LayerManager *layerMgr, const QBitArray &srcLayerIndexes, int layerGroupIndex)
  : ModifyLayersCommand("duplicate layer(s)", layerMgr)
{
  QList<QSharedPointer<Layer> > srcLayers;
  for (int i = 0; i < srcLayerIndexes.size(); ++i)
    if (srcLayerIndexes.testBit(i))
      srcLayers.append(newOrderedLayers_.at(i));

  const QSharedPointer<Layer> newLayer = layerMgr_->createDuplicateLayer(newLayerGroups_.at(layerGroupIndex), srcLayers, EditItemManager::instance());
  newOrderedLayers_.append(newLayer);

  // single-select the new layer
  foreach (const QSharedPointer<Layer> &layer, newOrderedLayers_)
    layer->setSelected(layer == newLayer);
}

RemoveLayersCommand::RemoveLayersCommand(LayerManager *layerMgr, const QBitArray &removable)
  : ModifyLayersCommand("remove layer(s)", layerMgr)
{
  QList<QSharedPointer<Layer> > removableLayers;
  for (int i = 0; i < removable.size(); ++i)
    if (removable.testBit(i))
      removableLayers.append(newOrderedLayers_.at(i));

  removeLayers(removableLayers);
}

MergeLayersCommand::MergeLayersCommand(LayerManager *layerMgr, const QBitArray &mergeable)
  : ModifyLayersCommand("merge layers", layerMgr)
{
  if (mergeable.count(true) < 2)
    return;

  QSharedPointer<Layer> dstLayer;
  QList<QSharedPointer<Layer> > srcLayers;
  for (int i = 0; i < mergeable.size(); ++i) {
    if (mergeable.testBit(i)) {
      const QSharedPointer<Layer> layer = newOrderedLayers_.at(i);
      if (dstLayer.isNull())
        dstLayer = layer;
      else
        srcLayers.append(layer);
    }
  }

  LayerManager::mergeLayers(srcLayers, dstLayer);
  removeLayers(srcLayers);
}

ModifyLayerSelectionCommand::ModifyLayerSelectionCommand(LayerManager *layerMgr, const QBitArray &selected)
  : ModifyLayersCommand("modify layer selection", layerMgr)
{
  for (int i = 0; i < newOrderedLayers_.size(); ++i)
    newOrderedLayers_.at(i)->setSelected(selected.testBit(i));
}

ModifyLayerVisibilityCommand::ModifyLayerVisibilityCommand(LayerManager *layerMgr, const QBitArray &visible)
  : ModifyLayersCommand("modify layer visibility", layerMgr)
{
  for (int i = 0; i < newOrderedLayers_.size(); ++i)
    newOrderedLayers_.at(i)->setVisible(visible.testBit(i));
}

MoveLayerCommand::MoveLayerCommand(LayerManager *layerMgr, int from, int to)
  : ModifyLayersCommand("move layer", layerMgr)
{
  newOrderedLayers_.insert(to, newOrderedLayers_.takeAt(from));
}

RefreshLayersCommand::RefreshLayersCommand(LayerManager *layerMgr, const QMap<QString, QSharedPointer<QList<QSharedPointer<DrawingItemBase> > > > &newLayerItems)
  : ModifyLayersCommand("refresh layer(s)", layerMgr)
{
  foreach (const QSharedPointer<Layer> &layer, newOrderedLayers_) {
    if (newLayerItems.contains(layer->name())) {
      layer->clearItems(false);
      layer->insertItems(*(newLayerItems.value(layer->name()).data()), false);
    }
  }
}

} // namespace
