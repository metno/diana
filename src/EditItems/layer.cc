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
  , visible_(true)
  , unsavedChanges_(false)
  , name_(name)
{
}

Layer::Layer(const Layer &other)
  : id_(nextId())
  , visible_(other.visible_)
  , unsavedChanges_(false)
  , name_(QString("copy of %1").arg(other.name()))
{
  foreach (const QSharedPointer<DrawingItemBase> item, other.items_)
    insertItem(QSharedPointer<DrawingItemBase>(item->clone()), false);
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

void Layer::removeItem(const QSharedPointer<DrawingItemBase> &item, bool notify)
{
  items_.removeOne(item);
  if (notify)
    emit updated();
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

void Layer::removeSelectedItem(const QSharedPointer<DrawingItemBase> &item, bool notify)
{
  selItems_.removeOne(item);
  if (notify)
    emit updated();
}

void Layer::clearSelectedItems(bool notify)
{
  selItems_.clear();
  if (notify)
    emit updated();
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
