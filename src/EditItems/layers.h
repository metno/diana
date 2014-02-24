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
#include <QSharedPointer>

#define CurrentLayer EditItems::Layers::instance()->current()

class DrawingItemBase;

namespace EditItems {

class Layer {
public:
  Layer();
  Layer(const Layer &);
  ~Layer();
  int id() const;
  QSet<DrawingItemBase *> &items();
  QSet<DrawingItemBase *> &selectedItems();
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
  QSet<DrawingItemBase *> items_;
  QSet<DrawingItemBase *> selItems_;
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
  Layer *at(int);
  Layer *current();
  int currentPos() const;
  void setCurrent(Layer *);
  Layer *layerFromName(const QString &);
  Layer *addEmpty();
  Layer *addDuplicate(Layer *);
  // Layer *addFromFile(const QString &); // directly from KML file
  // Layer *addFromName(const QString &); // from setup (indirectly from KML file)
  void remove(Layer *);
  void reorder(QList<Layer *>);
  void mergeIntoFirst(QList<Layer *>);
  void update();
private:
  Layers();
  static Layers *self; // singleton instance pointer

  QList<Layer *> layers_;
  Layer *currLayer_;

  void setCurrentPos(int);

  int posFromLayer(Layer *) const;
  Layer *layerFromId(int);
signals:
  void updated();
};

} // namespace

#endif // EDITITEMSLAYERS_H
