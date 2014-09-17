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

Layer::Layer(const QString &name, bool removable)
  : id_(nextId())
  , selected_(false)
  , visible_(true)
  , unsavedChanges_(false)
  , name_(name)
  , removable_(removable)
{
}

Layer::Layer(const Layer &other)
  : id_(other.id_)
  , items_(other.items_)
  , selected_(other.selected_)
  , visible_(other.visible_)
  , unsavedChanges_(other.unsavedChanges_)
  , name_(other.name_)
  , removable_(other.removable_)
  , srcFiles_(other.srcFiles_)
{
}

Layer::Layer(const QList<QSharedPointer<Layer> > &srcLayers, const DrawingManager *dm, bool removable)
  : id_(nextId())
  , selected_(false)
  , visible_(false)
  , unsavedChanges_(false)
  , name_(QString("copy of %1 layers").arg(srcLayers.size()))
  , removable_(removable)
{
  foreach (const QSharedPointer<Layer> &srcLayer, srcLayers) {
    if (srcLayer->visible_)
      visible_ = true;
    foreach (const QSharedPointer<DrawingItemBase> item, srcLayer->items_)
      insertItem(QSharedPointer<DrawingItemBase>(item->clone(dm)), false);
    srcFiles_.unite(srcLayer->srcFiles());
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
  return selectedItems().size();
}

const QSharedPointer<DrawingItemBase> Layer::selectedItem(int pos) const
{
  const QList<QSharedPointer<DrawingItemBase> > selItems = selectedItems();
  if ((pos < 0) || (pos >= selItems.size()))
    qFatal("Layer::selectedItem(): index %d outside valid range [0, %d]", pos, selItems.size() - 1);
  return selItems.at(pos);
}

QList<QSharedPointer<DrawingItemBase> > Layer::selectedItems() const
{
  QList<QSharedPointer<DrawingItemBase> > selItems;
  foreach (const QSharedPointer<DrawingItemBase> &item, items_)
    if (item->selected())
      selItems.append(item);
  return selItems;
}

void Layer::selectItem(const QSharedPointer<DrawingItemBase> &item, bool notify)
{
  if (!item->selected()) {
    item->setSelected();
    if (notify)
      emit updated();
  }
}

bool Layer::deselectItem(const QSharedPointer<DrawingItemBase> &item, bool notify)
{
  bool deselected = false;
  if (item->selected()) {
    item->setSelected(false);
    deselected = true;
    if (notify)
      emit updated();
  }
  return deselected;
}

bool Layer::selectAllItems(bool notify)
{
  int unselCount = 0;
  foreach (const QSharedPointer<DrawingItemBase> &item, items_)
    if (!item->selected()) {
      item->setSelected(true);
      unselCount++;
    }

  if (unselCount > 0) {
    if (notify)
      emit updated();
    return true;
  }

  return false;
}

bool Layer::deselectAllItems(bool notify)
{
  int selCount = 0;
  foreach (const QSharedPointer<DrawingItemBase> &item, items_)
    if (item->selected()) {
      item->setSelected(false);
      selCount++;
    }

  if (selCount > 0) {
    if (notify)
      emit updated();
    return true;
  }

  return false;
}

bool Layer::containsSelectedItem(const QSharedPointer<DrawingItemBase> &item) const
{
  return items_.contains(item) && item->selected();
}

bool Layer::findItem(int id, QSharedPointer<DrawingItemBase> &item) const
{
  foreach (const QSharedPointer<DrawingItemBase> &item_, items_)
    if (item_->id() == id) {
      item = item_;
      return true;
    }
  return false;
}

bool Layer::isEmpty() const
{
  return items_.isEmpty();
}

bool Layer::isEditable() const
{
  return layerGroup_->isEditable();
}

bool Layer::isRemovable() const
{
  return removable_;
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

void Layer::setLayerGroup(const QSharedPointer<LayerGroup> &layerGroup)
{
  layerGroup_ = layerGroup;
}

const QSharedPointer<LayerGroup> &Layer::layerGroupRef() const
{
  return layerGroup_;
}

QSet<QString> Layer::srcFiles() const
{
  return srcFiles_;
}

void Layer::insertSrcFile(const QString &srcFile)
{
  srcFiles_.insert(srcFile);
}

void Layer::uniteSrcFiles(const QSet<QString> &srcFiles)
{
  srcFiles_.unite(srcFiles);
}

} // namespace
