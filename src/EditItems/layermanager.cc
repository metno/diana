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
  currLayer_.clear();
}

QSharedPointer<Layer> LayerManager::currentLayer() const
{
  return currLayer_;
}

void LayerManager::setCurrentLayer(const QSharedPointer<Layer> &layer)
{
  if (!orderedLayers_.contains(layer))
    qFatal("LayerManager::setCurrentLayer(): layer %p not found in orderedLayers_", (void *)(layer.data()));
  currLayer_ = layer;
}

void LayerManager::addToLayerGroup(QSharedPointer<LayerGroup> &layerGroup, const QList<QSharedPointer<Layer> > &layers)
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

void LayerManager::addToLayerGroup(QSharedPointer<LayerGroup> &layerGroup, const QSharedPointer<Layer> &layer)
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

QSharedPointer<Layer> LayerManager::createNewLayer(const QString &name) const
{
  QSharedPointer<Layer> layer(new Layer(name.isEmpty() ? "new layer" : name));
  ensureUniqueLayerName(layer);
  return layer;
}

QSharedPointer<Layer> LayerManager::createDuplicateLayer(const QSharedPointer<Layer> &srcLayer) const
{
  if (!srcLayer) {
    Q_ASSERT(false);
    return QSharedPointer<Layer>();
  }

  QSharedPointer<Layer> layer(new Layer(*(srcLayer.data())));
  ensureUniqueLayerName(layer);
  return layer;
}

// Copies all items and selected item of \a srcLayers into \a dstLayer.
void LayerManager::mergeLayers(const QList<QSharedPointer<Layer> > &srcLayers, const QSharedPointer<Layer> &dstLayer) const
{
  for (int i = 0; i < srcLayers.size(); ++i) {
    for (int j = 0; j < srcLayers.at(i)->itemCount(); ++j)
      dstLayer->insertItem(srcLayers.at(i)->itemRef(j));
    for (int j = 0; j < srcLayers.at(i)->selectedItemCount(); ++j)
      dstLayer->insertSelectedItem(srcLayers.at(i)->selectedItemRef(j));
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
    if (layer->name() == name)
      return layer;
  }
  return QSharedPointer<Layer>();
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
  if (currLayer_ == layer)
     currLayer_.clear();
}

// Moves \a srcLayer to the other side of \a dstLayer.
void LayerManager::moveLayer(const QSharedPointer<Layer> &srcLayer, const QSharedPointer<Layer> &dstLayer)
{
  const int dstIndex = orderedLayers_.indexOf(dstLayer);
  orderedLayers_.removeOne(srcLayer);
  orderedLayers_.insert(dstIndex, srcLayer);
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
