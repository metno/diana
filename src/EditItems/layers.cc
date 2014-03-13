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

#include <EditItems/layers.h>

namespace EditItems {

Layer::Layer()
  : id_(nextId())
  , visible_(true)
  , unsavedChanges_(false)
  , name_()
{
}

Layer::Layer(const Layer &other)
  : id_(nextId())
  , visible_(other.isVisible())
  , unsavedChanges_(false)
  , name_(QString("copy of %1").arg(other.name()))
{
  foreach (QSharedPointer<DrawingItemBase> item, other.items_)
    items_.insert(QSharedPointer<DrawingItemBase>(item->clone()));
}

Layer::~Layer()
{
}

int Layer::id() const
{
  return id_;
}

int Layer::nextId_ = 0;

int Layer::nextId()
{
  return nextId_++; // ### not thread safe; use a mutex for that
}

QSet<QSharedPointer<DrawingItemBase> > &Layer::itemsRef()
{
  return items_;
}

QSet<QSharedPointer<DrawingItemBase> > &Layer::selectedItemsRef()
{
  return selItems_;
}

bool Layer::isEmpty() const
{
  return items_.isEmpty();
}

bool Layer::isVisible() const
{
  return visible_;
}

void Layer::setVisible(bool visible)
{
  visible_ = visible;
}

bool Layer::hasUnsavedChanges() const
{
  return unsavedChanges_;
}

void Layer::setUnsavedChanges(bool unsavedChanges)
{
  unsavedChanges_ = unsavedChanges;
}

QString Layer::name() const
{
  return name_;
}

void Layer::setName(const QString &n)
{
  name_ = n.trimmed();
}

Layers::Layers()
  : currLayer_(0)
{
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

int Layers::currentPos() const
{
  return posFromLayer(currLayer_);
}

void Layers::setCurrent(const QSharedPointer<Layer> &layer)
{
  for (int i = 0; i < layers_.size(); ++i) {
    if (layers_.at(i) == layer) {
      currLayer_ = layer;
      return;
    }
  }
}

QSharedPointer<Layer> Layers::layerFromName(const QString &name) const
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
  for (int i = 0; !layerFromName(name).isNull(); ++i)
    name = QString("%1 (%2)").arg(baseName).arg(i);
  return name;
}

// Adds an empty layer and returns a pointer to it.
QSharedPointer<Layer> Layers::addEmpty(bool notify)
{
  QSharedPointer<Layer> layer(new Layer);
  layer->setName(createUniqueName(QString("new layer (%1)").arg(layer->id())));
  layers_.append(layer);
  currLayer_ = layer;
  if (notify)
    emit addedLayer();
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
  layers_.append(layer);
  currLayer_ = layer;
  return layer;
}

void Layers::remove(const QSharedPointer<Layer> &layer)
{
  for (int i = 0; i < layers_.size(); ++i) {
    if (layers_.at(i) == layer) {
      layers_.removeAt(i);
      if (currLayer_ == layer)
        currLayer_.clear();
      break;
    }
  }
}

void Layers::mergeIntoFirst(const QList<QSharedPointer<Layer> > &layers)
{
  // merge
  for (int i = 1; i < layers.size(); ++i) {
    Q_ASSERT((layers.first()->itemsRef() & layers.at(i)->itemsRef()).isEmpty()); // ensure empty intersection
    layers.first()->itemsRef().unite(layers.at(i)->itemsRef());
    Q_ASSERT((layers.first()->selectedItemsRef() & layers.at(i)->selectedItemsRef()).isEmpty()); // ensure empty intersection
    layers.first()->selectedItemsRef().unite(layers.at(i)->selectedItemsRef());
    // NOTE: layers.at(i) is assumed to be removed by the client
  }
}

void Layers::set(const QList<QSharedPointer<Layer> > &layers, bool notify)
{
  layers_ = layers;
  if (notify)
    emit replacedLayers();
}

void Layers::update()
{
  emit updated();
}

void Layers::setCurrentPos(int pos)
{
  if ((pos < 0) || (pos >= layers_.size())) {
    Q_ASSERT(false);
    return;
  }
  currLayer_ = layers_.at(pos);
}

// Returns non-negative pos from layer if found, otherwise -1.
int Layers::posFromLayer(const QSharedPointer<Layer> &layer) const
{
  for (int i = 0; i < layers_.size(); ++i)
    if (layers_.at(i) == layer)
      return i;
  return -1;
}

} // namespace
