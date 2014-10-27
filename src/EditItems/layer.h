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
//#define QT_SHAREDPOINTER_TRACK_POINTERS
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
  Layer(const QString &, bool = true, bool = true);
  Layer(const Layer &);
  Layer(const QList<QSharedPointer<Layer> > &, const DrawingManager *, bool = true, bool = true);
  ~Layer();
  int id() const;
  int itemCount() const;
  const QSharedPointer<DrawingItemBase> item(int) const;
  QSharedPointer<DrawingItemBase> &itemRef(int);
  QList<QSharedPointer<DrawingItemBase> > items() const;
  QSet<QSharedPointer<DrawingItemBase> > itemSet() const;
  void insertItem(const QSharedPointer<DrawingItemBase> &, bool = true);
  void insertItems(const QList<QSharedPointer<DrawingItemBase> > &, bool = true);
  bool removeItem(const QSharedPointer<DrawingItemBase> &, bool = true);
  void clearItems(bool = true);
  bool containsItem(const QSharedPointer<DrawingItemBase> &) const;
  int selectedItemCount() const;
  const QSharedPointer<DrawingItemBase> selectedItem(int) const;
  QList<QSharedPointer<DrawingItemBase> > selectedItems() const;
  void selectItem(const QSharedPointer<DrawingItemBase> &, bool = true);
  bool deselectItem(const QSharedPointer<DrawingItemBase> &, bool = true);
  bool selectAllItems(bool = true);
  bool deselectAllItems(bool = true);
  bool containsSelectedItem(const QSharedPointer<DrawingItemBase> &) const;
  bool findItem(int, QSharedPointer<DrawingItemBase> &) const;
  bool isEmpty() const;
  bool isRemovable() const;
  bool isAttrsEditable() const;
  bool isEditable() const;
  bool isActive() const;
  bool isSelected() const;
  void setSelected(bool = true);
  bool isVisible() const;
  void setVisible(bool, bool = false);
  bool hasUnsavedChanges() const;
  void setUnsavedChanges(bool);
  QString name() const;
  void setName(const QString &);
  void setLayerGroup(const QSharedPointer<LayerGroup> &);
  const QSharedPointer<LayerGroup> &layerGroupRef() const;
  QSet<QString> srcFiles() const;
  void clearSrcFiles();
  void insertSrcFile(const QString &);
  void uniteSrcFiles(const QSet<QString> &srcFiles);
private:
  int id_;
  static int nextId_;
  static int nextId();
  QList<QSharedPointer<DrawingItemBase> > items_;
  QSharedPointer<LayerGroup> layerGroup_;
  bool selected_;
  bool visible_;
  bool unsavedChanges_;
  QString name_;
  bool removable_;
  bool attrsEditable_;
  QSet<QString> srcFiles_;
signals:
  void updated();
  void visibilityChanged(bool);
};

} // namespace

#endif // EDITITEMSLAYER_H
