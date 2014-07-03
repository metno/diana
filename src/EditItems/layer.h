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

#ifndef EDITITEMSLAYER_H
#define EDITITEMSLAYER_H

#include <QObject>
#include <QSharedPointer>
#include <QList>
#include <QSet>
#include <QString>
#include <EditItems/drawingitembase.h>

class DrawingManager;

namespace EditItems {

class LayerGroup;

class Layer : public QObject {
  Q_OBJECT
  friend class LayerManager;
public:
  Layer(const QString &);
  Layer(const Layer &, const DrawingManager *);
  ~Layer();
  int id() const;
  int itemCount() const;
  const QSharedPointer<DrawingItemBase> item(int) const;
  QSharedPointer<DrawingItemBase> &itemRef(int);
  QList<QSharedPointer<DrawingItemBase> > items() const;
  QSet<QSharedPointer<DrawingItemBase> > itemSet() const;
  void insertItem(const QSharedPointer<DrawingItemBase> &, bool = true);
  void removeItem(const QSharedPointer<DrawingItemBase> &, bool = true);
  void clearItems(bool = true);
  bool containsItem(const QSharedPointer<DrawingItemBase> &) const;
  int selectedItemCount() const;
  const QSharedPointer<DrawingItemBase> selectedItem(int) const;
  QSharedPointer<DrawingItemBase> &selectedItemRef(int);
  QList<QSharedPointer<DrawingItemBase> > selectedItems() const;
  QSet<QSharedPointer<DrawingItemBase> > selectedItemSet() const;
  void insertSelectedItem(const QSharedPointer<DrawingItemBase> &, bool = true);
  void removeSelectedItem(const QSharedPointer<DrawingItemBase> &, bool = true);
  void clearSelectedItems(bool = true);
  bool containsSelectedItem(const QSharedPointer<DrawingItemBase> &) const;
  bool isEmpty() const;
  bool isEditable() const;
  bool isActive() const;
  bool isVisible() const;
  void setVisible(bool);
  bool hasUnsavedChanges() const;
  void setUnsavedChanges(bool);
  QString name() const;
  void setName(const QString &);
  const QSharedPointer<LayerGroup> &layerGroupRef() const;
private:
  int id_;
  static int nextId_;
  static int nextId();
  QList<QSharedPointer<DrawingItemBase> > items_;
  QList<QSharedPointer<DrawingItemBase> > selItems_;
  QSharedPointer<LayerGroup> layerGroup_;
  bool visible_;
  bool unsavedChanges_;
  QString name_;
signals:
  void updated();
};

} // namespace

#endif // EDITITEMSLAYER_H
