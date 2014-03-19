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

#ifndef DRAWINGITEMBASE_H
#define DRAWINGITEMBASE_H

#include <QColor>
#include <QList>
#include <QPointF>
#include <QVariant>
#include <QDomNode>
#include <GL/gl.h>

#define Drawing(i) dynamic_cast<DrawingItemBase *>(i)
#define ConstDrawing(i) dynamic_cast<const DrawingItemBase *>(i)

class QDomDocumentFragment;

class DrawingItemBase
{
  friend class EditItemBase;
public:
  DrawingItemBase();
  virtual ~DrawingItemBase();

  // Returns the item's globally unique ID.
  int id() const;

  // Returns a deep copy of this item.
  DrawingItemBase *clone() const;

  // Returns the item's group ID if set, or -1 otherwise.
  int groupId() const;

  virtual QString infoString() const { return QString("addr=%1 id=%2").arg((ulong)this, 0, 16).arg(id()); }

  QVariant property(const QString &name, const QVariant &default_ = QVariant()) const;
  void setProperty(const QString &name, const QVariant &value);
  // Returns the item's properties.
  QVariantMap properties() const;
  QVariantMap &propertiesRef();
  const QVariantMap &propertiesRef() const;
  // Sets the item's properties.
  void setProperties(const QVariantMap &);

  // Returns the item's points.
  virtual QList<QPointF> getPoints() const;
  // Sets the item's points.
  virtual void setPoints(const QList<QPointF> &points);

  // Returns the item's geographic points.
  virtual QList<QPointF> getLatLonPoints() const;
  // Sets the item's geographic points.
  virtual void setLatLonPoints(const QList<QPointF> &points);

  // Draws the item.
  virtual void draw() = 0;

  // Returns the item's KML representation.
  virtual QDomNode toKML(const QHash<QString, QString> & = QHash<QString, QString>()) const;

protected:
  virtual DrawingItemBase *cloneSpecial() const = 0;
  mutable QVariantMap properties_;
  QList<QPointF> points_;
  QList<QPointF> latLonPoints_;

private:
  int id_;
  static int nextId_;
  int nextId();

  QDomElement createExtDataElement(QDomDocument &, const QHash<QString, QString> &) const;
  QDomElement createPointOrPolygonElement(QDomDocument &) const;
  QDomElement createTimeSpanElement(QDomDocument &) const;
  QDomElement createPlacemarkElement(QDomDocument &) const;
};
 
#if (QT_VERSION < QT_VERSION_CHECK(4, 8, 0))
#include <QSharedPointer>
uint qHash(const QSharedPointer<DrawingItemBase> &key)
{
  return qHash(key.data());
}
#endif

#endif
