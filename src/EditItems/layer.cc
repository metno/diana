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

#include <EditItems/layer.h>
#include <EditItems/layergroup.h>

namespace EditItems {

Layer::Layer(const QString &name)
  : id_(nextId())
  , selected_(false)
  , visible_(true)
  , unsavedChanges_(false)
  , name_(name)
{
}

Layer::Layer(const QList<QSharedPointer<Layer> > &srcLayers, const DrawingManager *dm)
  : id_(nextId())
  , selected_(false)
  , visible_(false)
  , unsavedChanges_(false)
  , name_(QString("copy of %1 layers").arg(srcLayers.size()))
{
  foreach (const QSharedPointer<Layer> &srcLayer, srcLayers) {
    if (srcLayer->visible_)
      visible_ = true;
    foreach (const QSharedPointer<DrawingItemBase> item, srcLayer->items_)
      insertItem(QSharedPointer<DrawingItemBase>(item->clone(dm)), false);
  }
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

int Layer::itemCount() const
{
  return items_.count();
}

const QSharedPointer<DrawingItemBase> Layer::item(int pos) const
{
  if ((pos < 0) || (pos >= items_.size()))
    qFatal("Layer::item(): index %d outside valid range [0, %d]", pos, items_.size() - 1);
  return items_.at(pos);
}

QSharedPointer<DrawingItemBase> &Layer::itemRef(int pos)
{
  if ((pos < 0) || (pos >= items_.size()))
    qFatal("Layer::itemRef(): index %d outside valid range [0, %d]", pos, items_.size() - 1);
  return items_[pos];
}

QList<QSharedPointer<DrawingItemBase> > Layer::items() const
{
  return items_;
}

QSet<QSharedPointer<DrawingItemBase> > Layer::itemSet() const
{
  return items_.toSet();
}

void Layer::insertItem(const QSharedPointer<DrawingItemBase> &item, bool notify)
{
  items_.append(item);
  if (notify)
    emit updated();
}

bool Layer::removeItem(const QSharedPointer<DrawingItemBase> &item, bool notify)
{
  const bool removed = items_.removeOne(item);
  if (notify)
    emit updated();
  return removed;
}

void Layer::clearItems(bool notify)
{
  items_.clear();
  if (notify)
    emit updated();
}

bool Layer::containsItem(const QSharedPointer<DrawingItemBase> &item) const
{
  return items_.contains(item);
}

int Layer::selectedItemCount() const
{
  return selItems_.count();
}

const QSharedPointer<DrawingItemBase> Layer::selectedItem(int pos) const
{
  if ((pos < 0) || (pos >= selItems_.size()))
    qFatal("Layer::selectedItem(): index %d outside valid range [0, %d]", pos, selItems_.size() - 1);
  return selItems_.at(pos);
}

QSharedPointer<DrawingItemBase> &Layer::selectedItemRef(int pos)
{
  if ((pos < 0) || (pos >= selItems_.size()))
    qFatal("Layer::selectedItemRef(): index %d outside valid range [0, %d]", pos, selItems_.size() - 1);
  return selItems_[pos];
}

QList<QSharedPointer<DrawingItemBase> > Layer::selectedItems() const
{
  return selItems_;
}

QSet<QSharedPointer<DrawingItemBase> > Layer::selectedItemSet() const
{
  return selItems_.toSet();
}

void Layer::insertSelectedItem(const QSharedPointer<DrawingItemBase> &item, bool notify)
{
  if (!selItems_.contains(item)) {
    selItems_.append(item);
    if (notify)
      emit updated();
  }
}

bool Layer::removeSelectedItem(const QSharedPointer<DrawingItemBase> &item, bool notify)
{
  const bool removed = selItems_.removeOne(item);
  if (notify)
    emit updated();
  return removed;
}

bool Layer::clearSelectedItems(bool notify)
{
  const bool origEmpty = selItems_.isEmpty();
  selItems_.clear();
  if (notify)
    emit updated();
  return !origEmpty;
}

bool Layer::containsSelectedItem(const QSharedPointer<DrawingItemBase> &item) const
{
  return selItems_.contains(item);
}

bool Layer::isEmpty() const
{
  return items_.isEmpty();
}

bool Layer::isEditable() const
{
  return layerGroup_->isEditable();
}

bool Layer::isActive() const
{
  return layerGroup_->isActive();
}

bool Layer::isSelected() const
{
  return selected_;
}

void Layer::setSelected(bool selected)
{
  selected_ = selected;
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

const QSharedPointer<LayerGroup> &Layer::layerGroupRef() const
{
  return layerGroup_;
}

} // namespace
