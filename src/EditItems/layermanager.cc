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
  // Create a default layer group at index 0.
  // The default layer group is editable and must always contain at least one layer.
  QSharedPointer<LayerGroup> layerGroup(new LayerGroup("default"));
  QSharedPointer<Layer> layer(new Layer("new layer"));
  layer->layerGroup_ = layerGroup;
  layerGroup->layers_.append(layer);
  currLayer_ = layer;
  layerGroups_.append(layerGroup);
  orderedLayers_.append(layer);
}

LayerManager *LayerManager::instance()
{
  if (!LayerManager::self)
    LayerManager::self = new LayerManager();
  return LayerManager::self;
}

LayerManager *LayerManager::self = 0;

// Clear everything, and keep just the default (empty) layer in the default group.
void LayerManager::reset()
{
  while (layerGroups_.size() > 1)
    layerGroups_.removeAt(layerGroups_.size() - 1);
  while (orderedLayers_.size() > 1)
    orderedLayers_.removeAt(orderedLayers_.size() - 1);
  orderedLayers_.first()->clearItems(false);
  currLayer_ = orderedLayers_.first();
}

QSharedPointer<Layer> LayerManager::currentLayer(bool editableOnly) const
{
  return (editableOnly && (!currLayer_->isEditable())) ? QSharedPointer<Layer>() : currLayer_;
}

void LayerManager::setCurrentLayer(const QSharedPointer<Layer> &layer)
{
  if (!orderedLayers_.contains(layer))
    qFatal("LayerManager::setCurrentLayer(): layer %p not found in orderedLayers_", (void *)(layer.data()));
  currLayer_ = layer;
}

QSharedPointer<LayerGroup> &LayerManager::defaultLayerGroup()
{
  return layerGroups_.first(); // by definition (see this ctor)
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

void LayerManager::addToNewLayerGroup(const QList<QSharedPointer<Layer> > &layers)
{
  QSharedPointer<LayerGroup> layerGroup = createNewLayerGroup();
  layerGroups_.append(layerGroup);
  addToLayerGroup(layerGroup, layers);
}

void LayerManager::addToDefaultLayerGroup(const QList<QSharedPointer<Layer> > &layers)
{
  addToLayerGroup(defaultLayerGroup(), layers);
}

void LayerManager::addToDefaultLayerGroup(const QSharedPointer<Layer> &layer)
{
  addToDefaultLayerGroup(QList<QSharedPointer<Layer> >() << layer);
}

void LayerManager::replaceInDefaultLayerGroup(const QList<QSharedPointer<Layer> > &layers)
{
  // remove existing layers
  foreach (const QSharedPointer<Layer> layer, defaultLayerGroup()->layers_)
    removeLayer(layer);

  addToDefaultLayerGroup(layers);
}

QSharedPointer<LayerGroup> LayerManager::createNewLayerGroup() const
{
  QSharedPointer<LayerGroup> layerGroup(new LayerGroup("new layer group"));
  ensureUniqueLayerGroupName(layerGroup);
  return layerGroup;
}

QSharedPointer<Layer> LayerManager::createNewLayer() const
{
  QSharedPointer<Layer> layer(new Layer("new layer"));
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

#if 0 // obsolete

Layers::Layers()
  : currLayer_(0)
{
  connect(LayerCollections::instance(), SIGNAL(updated()), SLOT(handleLayerCollectionsUpdate()));
  handleLayerCollectionsUpdate();
}

Layers *Layers::instance()
{
  if (!Layers::self)
    Layers::self = new Layers();
  return Layers::self;
}

Layers *Layers::self = 0;

int Layers::size() const
{
  return layers_.size();
}

QSharedPointer<Layer> Layers::at(int pos)
{
  if ((pos >= 0) && (pos < layers_.size()))
    return layers_.at(pos);
  return QSharedPointer<Layer>();
}

QSharedPointer<Layer> Layers::current() const
{
  return currLayer_;
}

void Layers::setCurrent(const QSharedPointer<Layer> &layer)
{
  if (currLayer_ == layer)
    return; // already current
  for (int i = 0; i < layers_.size(); ++i) {
    if (layers_.at(i) == layer) {
      currLayer_ = layer;
      return;
    }
  }
}

QSharedPointer<Layer> Layers::find(const QString &name) const
{
  foreach (QSharedPointer<Layer> layer, layers_) {
    if (layer->name() == name)
      return layer;
  }
  return QSharedPointer<Layer>();
}

QString Layers::createUniqueName(const QString &baseName) const
{
  QString name = baseName;
  for (int i = 0; !find(name).isNull(); ++i)
    name = QString("%1 (%2)").arg(baseName).arg(i);
  return name;
}

void Layers::ensureUniqueName(const QSharedPointer<Layer> &layer) const
{
  layer->setName(createUniqueName(layer->name()));
}

// Adds an empty layer and returns a pointer to it.
QSharedPointer<Layer> Layers::addEmpty(bool notify)
{
  QSharedPointer<Layer> layer(Layer::createEmptyLayer());
  layer->setName(createUniqueName(QString("new layer (%1)").arg(layer->id())));
  //layers_.append(layer);
  currLayer_ = layer;
  if (notify)
    emit updated();
  return layer;
}

// Adds a duplicate of an existing layer and returns a pointer to the duplicate,
// or 0 if not found.
QSharedPointer<Layer> Layers::addDuplicate(const QSharedPointer<Layer> &srcLayer)
{
  if (srcLayer.isNull()) {
    Q_ASSERT(false);
    return QSharedPointer<Layer>();
  }

  QSharedPointer<Layer> layer(new Layer(*(srcLayer.data())));
  ensureUniqueName(layer);
  layers_.append(layer);
  currLayer_ = layer;
  return layer;
}

void Layers::add(const QSharedPointer<Layer> &layer)
{
  ensureUniqueName(layer);
  layers_.append(layer);
  currLayer_ = layer;
}

void Layers::add(const QList<QSharedPointer<Layer> > &layers)
{
  foreach (const QSharedPointer<Layer> layer, layers)
    add(layer);
}

void Layers::remove(const QSharedPointer<Layer> &layer)
{
  layers_.removeOne(layer);
  layer->layerCollectionRef()->remove(layer, false);
  if (currLayer_ == layer)
     currLayer_.clear();
}

#if 0 // disabled for now
void Layers::mergeIntoFirst(const QList<QSharedPointer<Layer> > &layers)
{
  // merge
  for (int i = 1; i < layers.size(); ++i) {
    for (int j = 0; j < layers.at(i)->itemCount(); ++j)
      layers.first()->insertItem(layers.at(i)->itemRef(j));
    for (int j = 0; j < layers.at(i)->selectedItemCount(); ++j)
      layers.first()->insertSelectedItem(layers.at(i)->selectedItemRef(j));
    // NOTE: layers.at(i) is assumed to be removed by the client
  }
}
#endif

QList<QSharedPointer<Layer> > Layers::layers() const
{
  return layers_;
}

void Layers::set(const QList<QSharedPointer<Layer> > &layers, bool notify)
{
  layers_ = layers;
  if (notify)
    emit updated();
}

// Moves \a srcLayer to the other side of \a dstLayer.
void Layers::move(const QSharedPointer<Layer> &srcLayer, const QSharedPointer<Layer> &dstLayer)
{
  const int dstIndex = layers_.indexOf(dstLayer);
  layers_.removeOne(srcLayer);
  layers_.insert(dstIndex, srcLayer);
}

void Layers::setCurrentPos(int pos)
{
  if ((pos < 0) || (pos >= layers_.size())) {
    Q_ASSERT(false);
    return;
  }
  currLayer_ = layers_.at(pos);
}

LayerCollection::LayerCollection(const QString &name, bool active)
  : id_(nextId())
  , name_(name.isEmpty() ? LayerCollections::instance()->createUniqueName(QString("new layer collection (%1)").arg(id_)) : name)
  , active_(active)
{
}

LayerCollection::~LayerCollection()
{
}

int LayerCollection::id() const
{
  return id_;
}

int LayerCollection::nextId_ = 0;

int LayerCollection::nextId()
{
  return nextId_++; // ### not thread safe; use a mutex for that
}

QString LayerCollection::name() const
{
  return name_;
}

QList<QSharedPointer<Layer> > &LayerCollection::layersRef()
{
  return layers_;
}

void LayerCollection::initialize(const QSharedPointer<Layer> &layer) const
{
  connect(layer.data(), SIGNAL(updated()), SIGNAL(layerUpdated()));
}

void LayerCollection::initialize(const QList<QSharedPointer<Layer> > &layers) const
{
  foreach (const QSharedPointer<Layer> layer, layers)
    initialize(layer);
}

void LayerCollection::set(const QList<QSharedPointer<Layer> > &layers, bool notify)
{
  initialize(layers);
  layers_ = layers;
  // Layers::instance()->add(layers); // ### obsolete?
  if (notify)
    emit updated();
}

void LayerCollection::add(const QSharedPointer<Layer> &layer, bool notify)
{
  initialize(layer);
  layers_.append(layer);
  Layers::instance()->add(layer);
  if (notify)
    emit updated();
}

void LayerCollection::add(const QList<QSharedPointer<Layer> > &layers, bool notify)
{
  foreach (const QSharedPointer<Layer> layer, layers)
    add(layer, notify);
}

void LayerCollection::remove(const QSharedPointer<Layer> &layer, bool notify)
{
  layers_.removeOne(layer);
  if (notify)
    emit updated();
}

bool LayerCollection::isActive() const
{
  return active_;
}

void LayerCollection::setActive(bool active)
{
  active_ = active;
  emit updated();
}

QSharedPointer<Layer> LayerCollection::find(const Layer *layer) const
{
  QList<QSharedPointer<Layer> >::const_iterator it = layers_.begin();
  while (it != layers_.end()) {
    if ((*it).data() == layer)
      return *it;
    ++it;
  }
  return QSharedPointer<Layer>();
}

LayerCollections::LayerCollections()
{
  // insert the default layer collection at index 0 and make it initially active
  add(QSharedPointer<LayerCollection>(new LayerCollection("default layer collection", true)));
}

LayerCollections *LayerCollections::instance()
{
  if (!LayerCollections::self)
    LayerCollections::self = new LayerCollections();
  return LayerCollections::self;
}

LayerCollections *LayerCollections::self = 0;

int LayerCollections::size() const
{
  return layerCollections_.size();
}

QSharedPointer<LayerCollection> LayerCollections::at(int pos) const
{
  if ((pos >= 0) && (pos < layerCollections_.size()))
    return layerCollections_.at(pos);
  return QSharedPointer<LayerCollection>();
}

// Returns a reference to the default layer collection.
QSharedPointer<LayerCollection> &LayerCollections::default_()
{
  return layerCollections_.first();
}

void LayerCollections::add(const QSharedPointer<LayerCollection> &layerCollection)
{
  layerCollections_.append(layerCollection);
  connect(layerCollection.data(), SIGNAL(layerUpdated()), SIGNAL(layerUpdated()));
  connect(layerCollection.data(), SIGNAL(updated()), SIGNAL(updated()));

  // move layers from their old layer collections to this one
  foreach (const QSharedPointer<Layer> layer, layerCollection->layersRef()) {
    layer->layerCollectionRef()->remove(layer, false);
    layer->setLayerCollection(layerCollection);
  }

  emit updated();
}

QSharedPointer<Layer> LayerCollections::addEmptyLayer(const QSharedPointer<LayerCollection> &layerCollection)
{
  Layer *layer = new Layer(layerCollection);
  return layerCollection->find(layer);
}

QSharedPointer<LayerCollection> LayerCollections::find(const QString &name) const
{
  foreach (QSharedPointer<LayerCollection> layerCollection, layerCollections_) {
    if (layerCollection->name() == name)
      return layerCollection;
  }
  return QSharedPointer<LayerCollection>();
}

QString LayerCollections::createUniqueName(const QString &baseName) const
{
  QString name = baseName;
  for (int i = 0; !find(name).isNull(); ++i)
    name = QString("%1 (%2)").arg(baseName).arg(i);
  return name;
}

void LayerCollections::notify()
{
  emit layersUpdated();
}

#endif // obsolete

} // namespace
