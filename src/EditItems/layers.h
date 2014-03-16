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

#ifndef EDITITEMSLAYERS_H
#define EDITITEMSLAYERS_H

#include <QObject>
#include <QSet>
#include <EditItems/drawingitembase.h>
#include <QSharedPointer>

#define CurrentLayer EditItems::Layers::instance()->current()

namespace EditItems {

class Layer {
public:
  Layer();
  Layer(const Layer &);
  ~Layer();
  int id() const;
  QSet<QSharedPointer<DrawingItemBase> > &itemsRef();
  QSet<QSharedPointer<DrawingItemBase> > &selectedItemsRef();
  bool isEmpty() const;
  bool isVisible() const;
  void setVisible(bool);
  bool hasUnsavedChanges() const;
  void setUnsavedChanges(bool);
  QString name() const;
  void setName(const QString &);
private:
  int id_;
  static int nextId_;
  static int nextId();
  QSet<QSharedPointer<DrawingItemBase> > items_;
  QSet<QSharedPointer<DrawingItemBase> > selItems_;
  bool visible_;
  bool unsavedChanges_;
  QString name_;
};

class Layers : public QObject
{
  Q_OBJECT
public:
  static Layers *instance();
  int size() const;
  QSharedPointer<Layer> at(int);
  QSharedPointer<Layer> current() const;
  int currentPos() const;
  void setCurrent(const QSharedPointer<Layer> &);
  QSharedPointer<Layer> layerFromName(const QString &) const;
  QString createUniqueName(const QString &) const;
  QSharedPointer<Layer> addEmpty(bool = true);
  QSharedPointer<Layer> addDuplicate(const QSharedPointer<Layer> &);
  // QSharedPointer<Layer> addFromFile(const QString &); // directly from KML file
  // QSharedPointer<Layer> addFromName(const QString &); // from setup (indirectly from KML file)
  void remove(const QSharedPointer<Layer> &);
  void mergeIntoFirst(const QList<QSharedPointer<Layer> > &);
  QList<QSharedPointer<Layer> > layers() const;
  void set(const QList<QSharedPointer<Layer> > &, bool = true);
  void update();
private:
  Layers();
  static Layers *self; // singleton instance pointer

  QList<QSharedPointer<Layer> > layers_;
  QSharedPointer<Layer> currLayer_;

  void setCurrentPos(int);

  int posFromLayer(const QSharedPointer<Layer> &) const;

signals:
  void updated();
  void replacedLayers();
  void addedLayer();
};

} // namespace

#endif // EDITITEMSLAYERS_H
