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

#ifndef EDITITEMSLAYERMANAGER_H
#define EDITITEMSLAYERMANAGER_H

#include <QObject>
#include <QSharedPointer>
#include <QList>
#include <EditItems/drawingitembase.h>

#define CurrLayer EditItems::LayerManager::instance()->currentLayer()
#define CurrEditLayer EditItems::LayerManager::instance()->currentLayer(true)

namespace EditItems {

class LayerGroup;
class Layer;

class LayerManager : public QObject
{
  Q_OBJECT
public:
  static LayerManager *instance();
  void resetFirstDefaultLayer();
  QSharedPointer<Layer> currentLayer(bool = false) const;
  void setCurrentLayer(const QSharedPointer<Layer> &);
  QSharedPointer<LayerGroup> &defaultLayerGroup();
  void addToNewLayerGroup(const QList<QSharedPointer<Layer> > &);
  void addToDefaultLayerGroup(const QList<QSharedPointer<Layer> > &);
  void addToDefaultLayerGroup(const QSharedPointer<Layer> &);
  void replaceInDefaultLayerGroup(const QList<QSharedPointer<Layer> > &);
  QSharedPointer<LayerGroup> createNewLayerGroup() const;
  QSharedPointer<Layer> createNewLayer() const;
  QSharedPointer<Layer> createDuplicateLayer(const QSharedPointer<Layer> &) const;
  const QList<QSharedPointer<LayerGroup> > &layerGroups() const;
  const QList<QSharedPointer<Layer> > &orderedLayers() const;
  QSharedPointer<LayerGroup> findLayerGroup(const QString &) const;
  QSharedPointer<LayerGroup> findLayerGroup(const QSharedPointer<Layer> &) const;
  QSharedPointer<Layer> findLayer(const QString &) const;
  void removeLayer(const QSharedPointer<Layer> &);
  void moveLayer(const QSharedPointer<Layer> &, const QSharedPointer<Layer> &);

private:
  LayerManager();
  static LayerManager *self; // singleton instance pointer

  QList<QSharedPointer<LayerGroup> > layerGroups_;
  // the layers of all layer groups organized in a single, ordered list:
  QList<QSharedPointer<Layer> > orderedLayers_;
  QSharedPointer<Layer> currLayer_;

  void addToLayerGroup(QSharedPointer<LayerGroup> &, const QList<QSharedPointer<Layer> > &);

  QString createUniqueLayerGroupName(const QString &) const;
  void ensureUniqueLayerGroupName(const QSharedPointer<LayerGroup> &) const;

  QString createUniqueLayerName(const QString &) const;
  void ensureUniqueLayerName(const QSharedPointer<Layer> &) const;
};

#if 0 // obsolete
class Layers : public QObject
{
  Q_OBJECT
public:
  static Layers *instance();
  int size() const;
  QSharedPointer<Layer> at(int);
  QSharedPointer<Layer> current() const;
  void setCurrent(const QSharedPointer<Layer> &);
  QSharedPointer<Layer> find(const QString &) const;
  QString createUniqueName(const QString &) const;
  QSharedPointer<Layer> addEmpty(bool = true);
  QSharedPointer<Layer> addDuplicate(const QSharedPointer<Layer> &);
  void add(const QSharedPointer<Layer> &);
  void add(const QList<QSharedPointer<Layer> > &);
  void remove(const QSharedPointer<Layer> &);
#if 0 // disabled for now
  void mergeIntoFirst(const QList<QSharedPointer<Layer> > &);
#endif
  QList<QSharedPointer<Layer> > layers() const;
  void set(const QList<QSharedPointer<Layer> > &, bool = true); // ### is this used by anyone?
  void move(const QSharedPointer<Layer> &, const QSharedPointer<Layer> &);
private:
  Layers();
  static Layers *self; // singleton instance pointer
  QList<QSharedPointer<Layer> > layers_;
  QSharedPointer<Layer> currLayer_;
  void setCurrentPos(int);
  void ensureUniqueName(const QSharedPointer<Layer> &) const;
signals:
  void layersUpdated(); // structure of layer list
  void layerUpdated(); // contents of single layer
};

class LayerCollection : public QObject
{
  Q_OBJECT
public:
  LayerCollection(const QString & = QString(), bool = true);
  ~LayerCollection();
  int id() const;
  QString name() const;
  QList<QSharedPointer<Layer> > &layersRef();
  void set(const QList<QSharedPointer<Layer> > &, bool = true);
  void add(const QSharedPointer<Layer> &, bool = true);
  void add(const QList<QSharedPointer<Layer> > &, bool = true);
  void remove(const QSharedPointer<Layer> &, bool = true);
  bool isActive() const;
  void setActive(bool);
  QSharedPointer<Layer> find(const Layer *) const;
private:
  int id_;
  static int nextId_;
  static int nextId();
  QString name_;
  QList<QSharedPointer<Layer> > layers_;
  bool active_;
  void initialize(const QSharedPointer<Layer> &) const;
  void initialize(const QList<QSharedPointer<Layer> > &) const;
signals:
  void updated();
};

class LayerCollections : public QObject
{
  Q_OBJECT
public:
  static LayerCollections *instance();
  int size() const;
  QSharedPointer<LayerCollection> at(int) const;
  QSharedPointer<LayerCollection> &default_();
  void add(const QSharedPointer<LayerCollection> &);
  QSharedPointer<Layer> addEmptyLayer(const QSharedPointer<LayerCollection> &);
  QString createUniqueName(const QString &) const;
  void notify();
private:
  LayerCollections();
  static LayerCollections *self; // singleton instance pointer
  QList<QSharedPointer<LayerCollection> > layerCollections_;
  QSharedPointer<LayerCollection> find(const QString &) const;
signals:
  void layersUpdated(); // structure of layer list
  void layerUpdated(); // contents of single layer (i.e. should not require clearing and rebuilding a layout etc.)
};
#endif // obsolete

} // namespace

#endif // EDITITEMSLAYERMANAGER_H
