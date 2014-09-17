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

#include <EditItems/layermanager.h>
#include <EditItems/layer.h>
#include <EditItems/layergroup.h>

namespace EditItems {

LayerManager::LayerManager()
{
}

LayerManager::~LayerManager()
{
}

bool LayerManager::isEmpty() const
{
  foreach(const QSharedPointer<Layer> &layer, orderedLayers_) {
    if (layer.data()->itemCount() != 0)
      return false;
  }

  return true;
}

void LayerManager::clear()
{
  layerGroups_.clear();
  orderedLayers_.clear();
}

QList<QSharedPointer<Layer> > LayerManager::selectedLayers() const
{
  QList<QSharedPointer<Layer> > selLayers;
  foreach(const QSharedPointer<Layer> &layer, orderedLayers_)
    if (layer->isSelected())
      selLayers.append(layer);
  return selLayers;
}

int LayerManager::selectedLayersItemCount() const
{
  int count = 0;
  foreach(const QSharedPointer<Layer> &layer, orderedLayers_)
    if (layer->isSelected())
      count += layer->itemCount();
  return count;
}

QSet<QSharedPointer<DrawingItemBase> > LayerManager::itemsInSelectedLayers(bool selectedItemsOnly) const
{
  QSet<QSharedPointer<DrawingItemBase> > items;

  foreach(const QSharedPointer<Layer> &layer, orderedLayers_)
    if (layer->isSelected()) {
      if (selectedItemsOnly)
        items.unite(layer->selectedItems().toSet());
      else
        items.unite(layer->itemSet());
    }

  return items;
}

bool LayerManager::selectedLayersContainItem(const QSharedPointer<DrawingItemBase> &item) const
{
  foreach(const QSharedPointer<Layer> &layer, orderedLayers_)
    if (layer->isSelected() && layer->containsItem(item))
      return true;
  return false;
}

void LayerManager::selectItem(const QSharedPointer<DrawingItemBase> &item, QSharedPointer<Layer> &layer, bool exclusive, bool notify)
{
  if (exclusive)
    deselectAllItems();
  layer->selectItem(item, notify);
}

bool LayerManager::selectItem(const QSharedPointer<DrawingItemBase> &item, bool exclusive, bool notify)
{
  QSharedPointer<Layer> layer;
  if (!findLayer(item, layer))
    return false;

  selectItem(item, layer, exclusive, notify);
  return true;
}

bool LayerManager::selectItem(int id, bool exclusive, bool notify)
{
  QSharedPointer<DrawingItemBase> item;
  QSharedPointer<Layer> layer;
  if (!findItem(id, item, layer))
    return false;
  selectItem(item, layer, exclusive, notify);
  return true;
}

bool LayerManager::deselectItem(const QSharedPointer<DrawingItemBase> &item, bool notify)
{
  foreach(const QSharedPointer<Layer> &layer, orderedLayers_)
    if (layer->deselectItem(item, notify))
      return true;
  return false;
}

bool LayerManager::deselectAllItems(bool notify)
{
  bool cleared = false;
  foreach(const QSharedPointer<Layer> &layer, orderedLayers_)
    if (layer->deselectAllItems(notify))
      cleared = true;
  return cleared;
}


void LayerManager::addToLayerGroup(const QSharedPointer<LayerGroup> &layerGroup, const QList<QSharedPointer<Layer> > &layers)
{
  foreach(const QSharedPointer<Layer> &layer, layers) {
    // ensure that the layer doesn't exist already (in some layer group)
    QSharedPointer<LayerGroup> existingLayerGroup =findLayerGroup(layer);
    if (existingLayerGroup)
      qFatal("layer '%s' already exists in layer group '%s'",
             layer->name().toLatin1().data(),
             existingLayerGroup->name().toLatin1().data());

    ensureUniqueLayerName(layer);
    orderedLayers_.append(layer);
    layer->layerGroup_ = layerGroup;
  }

  layerGroup->layers_.append(layers);
}

void LayerManager::addToLayerGroup(const QSharedPointer<LayerGroup> &layerGroup, const QSharedPointer<Layer> &layer)
{
  addToLayerGroup(layerGroup, QList<QSharedPointer<Layer> >() << layer);
}

QSharedPointer<LayerGroup> LayerManager::addToNewLayerGroup(const QList<QSharedPointer<Layer> > &layers, const QString &name)
{
  QSharedPointer<LayerGroup> layerGroup = createNewLayerGroup(name);
  layerGroups_.append(layerGroup);
  addToLayerGroup(layerGroup, layers);
  return layerGroup;
}

QSharedPointer<LayerGroup> LayerManager::addToNewLayerGroup(const QSharedPointer<Layer> &layer, const QString &name)
{
  return addToNewLayerGroup(QList<QSharedPointer<Layer> >() << layer, name);
}

QSharedPointer<LayerGroup> LayerManager::createNewLayerGroup(const QString &name) const
{
  QSharedPointer<LayerGroup> layerGroup(new LayerGroup(name.isEmpty() ? "new layer group" : name));
  ensureUniqueLayerGroupName(layerGroup);
  return layerGroup;
}

QSharedPointer<Layer> LayerManager::createNewLayer(const QString &name, bool removable) const
{
  QSharedPointer<Layer> layer(new Layer(name.isEmpty() ? "new layer" : name, removable));
  ensureUniqueLayerName(layer);
  return layer;
}

QSharedPointer<Layer> LayerManager::createNewLayer(const QSharedPointer<LayerGroup> &layerGroup, const QString &name, bool removable)
{
  QSharedPointer<Layer> layer = createNewLayer(name, removable);
  addToLayerGroup(layerGroup, layer);
  return layer;
}

QSharedPointer<Layer> LayerManager::createDuplicateLayer(const QList<QSharedPointer<Layer> > &srcLayers, const DrawingManager *dm) const
{
  if (srcLayers.isEmpty()) {
    Q_ASSERT(false);
    return QSharedPointer<Layer>();
  }

  QSharedPointer<Layer> layer(new Layer(srcLayers, dm));
  ensureUniqueLayerName(layer);
  return layer;
}

QSharedPointer<Layer> LayerManager::createDuplicateLayer(const QSharedPointer<Layer> &srcLayer, const DrawingManager *dm) const
{
  return createDuplicateLayer(QList<QSharedPointer<Layer> >() << srcLayer, dm);
}

QSharedPointer<Layer> LayerManager::createDuplicateLayer(
    const QSharedPointer<LayerGroup> &layerGroup, const QList<QSharedPointer<Layer> > &srcLayers, const DrawingManager *dm)
{
  QSharedPointer<Layer> layer = createDuplicateLayer(srcLayers, dm);
  addToLayerGroup(layerGroup, layer);
  return layer;
}

// Copies all items and selected items of \a srcLayers into \a dstLayer.
void LayerManager::mergeLayers(const QList<QSharedPointer<Layer> > &srcLayers, const QSharedPointer<Layer> &dstLayer)
{
  for (int i = 0; i < srcLayers.size(); ++i) {
    for (int j = 0; j < srcLayers.at(i)->itemCount(); ++j)
      dstLayer->insertItem(srcLayers.at(i)->itemRef(j));
    for (int j = 0; j < srcLayers.at(i)->selectedItemCount(); ++j)
      dstLayer->selectItem(srcLayers.at(i)->selectedItem(j));
    dstLayer->uniteSrcFiles(srcLayers.at(i)->srcFiles());
  }
}

const QList<QSharedPointer<LayerGroup> > &LayerManager::layerGroups() const
{
  return layerGroups_;
}

const QList<QSharedPointer<Layer> > &LayerManager::orderedLayers() const
{
  return orderedLayers_;
}

QSharedPointer<LayerGroup> LayerManager::findLayerGroup(const QString &name) const
{
  foreach (const QSharedPointer<LayerGroup> &layerGroup, layerGroups_) {
    if (layerGroup->name() == name)
      return layerGroup;
  }
  return QSharedPointer<LayerGroup>();
}

QSharedPointer<LayerGroup> LayerManager::findLayerGroup(const QSharedPointer<Layer> &layer) const
{
  foreach (const QSharedPointer<LayerGroup> &layerGroup, layerGroups_) {
    if (layerGroup->layers_.contains(layer))
      return layerGroup;
  }
  return QSharedPointer<LayerGroup>();
}

QSharedPointer<Layer> LayerManager::findLayer(const QString &name) const
{
  foreach (const QSharedPointer<Layer> &layer, orderedLayers_) {
    if (layer->name().toLower() == name.toLower())
      return layer;
  }
  return QSharedPointer<Layer>();
}

bool LayerManager::findItem(int id, QSharedPointer<DrawingItemBase> &item, QSharedPointer<Layer> &layer) const
{
  foreach(const QSharedPointer<Layer> &layer_, orderedLayers_) {
    if (layer_->findItem(id, item)) {
      layer = layer_;
      return true;
    }
  }
  return false;
}

bool LayerManager::findLayer(const QSharedPointer<DrawingItemBase> &item, QSharedPointer<Layer> &layer) const
{
  foreach(const QSharedPointer<Layer> &layer_, orderedLayers_)
    if (layer_->containsItem(item)) {
      layer = layer_;
      return true;
    }
  return false;
}

void LayerManager::removeLayer(const QSharedPointer<Layer> &layer)
{
  QSharedPointer<LayerGroup> layerGroup = layer->layerGroup_;

  // ### check these for now; remove (or convert to warnings) after testing:
  if (!layerGroup)
    qFatal("LayerManager::removeLayer(): layer '%s' doesn't belong to a layer group",
           layer->name().toLatin1().data());
  if (!layerGroup->isEditable())
    qFatal("LayerManager::removeLayer(): layer '%s' belongs to a non-editable layer group: '%s'",
           layer->name().toLatin1().data(),
           layerGroup->name().toLatin1().data());

  layerGroup->layers_.removeOne(layer);
  orderedLayers_.removeOne(layer);
}

// Moves \a srcLayer to the other side of \a dstLayer.
void LayerManager::moveLayer(const QSharedPointer<Layer> &srcLayer, const QSharedPointer<Layer> &dstLayer)
{
  const int dstIndex = orderedLayers_.indexOf(dstLayer);
  orderedLayers_.removeOne(srcLayer);
  orderedLayers_.insert(dstIndex, srcLayer);
}

void LayerManager::removeItem(const QSharedPointer<DrawingItemBase> &item, bool notify)
{
  foreach (const QSharedPointer<Layer> &layer, orderedLayers_)
    if (layer->removeItem(item, notify))
      return;
}

void LayerManager::copyState(QList<QSharedPointer<LayerGroup> > *layerGroups, QList<QSharedPointer<Layer> > *orderedLayers) const
{
  // copy layer groups
  foreach (const QSharedPointer<LayerGroup> &layerGroup, layerGroups_) {
    QSharedPointer<LayerGroup> newLayerGroup(new LayerGroup(*(layerGroup.data())));
    foreach (const QSharedPointer<Layer> &layer, newLayerGroup->layersRef())
      layer->setLayerGroup(newLayerGroup);
    layerGroups->append(newLayerGroup);
  }

  // copy ordered layers
  foreach (const QSharedPointer<Layer> &layer, orderedLayers_) {
    int i = -1; // layer group index
    int j = -1; // layer index
    for (i = 0; i < layerGroups_.size(); ++i) {
      const QSharedPointer<LayerGroup> layerGroup = layerGroups_.at(i);
      j = layerGroup->layersRef().indexOf(layer);
      if (j >= 0)
        break;
    }

    if ((i >= 0) && (j >= 0))
      orderedLayers->append(layerGroups->at(i)->layersRef().at(j));
    else
      qFatal("LayerManager::copyState(): layer not found: i=%d, j=%d", i, j);
  }
}

void LayerManager::replaceState(const QList<QSharedPointer<LayerGroup> > &layerGroups, const QList<QSharedPointer<Layer> > &orderedLayers)
{
  layerGroups_ = layerGroups;
  orderedLayers_ = orderedLayers;
  emit stateReplaced();
}

QBitArray LayerManager::selected() const
{
  QBitArray sel(orderedLayers_.size());
  for (int i = 0; i < orderedLayers_.size(); ++i)
    sel.setBit(i, orderedLayers_.at(i)->isSelected());
  return sel;
}

QBitArray LayerManager::visible() const
{
  QBitArray vis(orderedLayers_.size());
  for (int i = 0; i < orderedLayers_.size(); ++i)
    vis.setBit(i, orderedLayers_.at(i)->isVisible());
  return vis;
}

QString LayerManager::createUniqueLayerGroupName(const QString &baseName) const
{
  QString name = baseName;
  for (int i = 0; findLayerGroup(name); ++i)
    name = QString("%1 (%2)").arg(baseName).arg(i);
  return name;
}

void LayerManager::ensureUniqueLayerGroupName(const QSharedPointer<LayerGroup> &layerGroup) const
{
  layerGroup->setName(createUniqueLayerGroupName(layerGroup->name()));
}

QString LayerManager::createUniqueLayerName(const QString &baseName) const
{
  QString name = baseName;
  for (int i = 0; findLayer(name); ++i)
    name = QString("%1 (%2)").arg(baseName).arg(i);
  return name;
}

void LayerManager::ensureUniqueLayerName(const QSharedPointer<Layer> &layer) const
{
  layer->setName(createUniqueLayerName(layer->name()));
}

} // namespace
