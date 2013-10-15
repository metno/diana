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

#include <QList>
#include <QPointF>
#include <QVariant>

class DrawingItemBase
{
public:
  DrawingItemBase();
  virtual ~DrawingItemBase();

  // Returns the item's globally unique ID.
  int id() const;

  // Returns the item's group ID if set, or -1 otherwise.
  int groupId() const;

  virtual QString infoString() const { return QString("addr=%1 id=%2").arg((ulong)this, 0, 16).arg(id()); }

  // Returns the item's properties.
  QVariantMap properties() const;
  QVariantMap &propertiesRef();
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
  // \a modes indicates whether the item is selected, hovered, both, or neither.
  // \a incomplete is true iff the item is in the process of being completed (i.e. during manual placement of a new item).
  virtual void draw() = 0;

protected:
  QVariantMap properties_;
  QList<QPointF> points_;
  QList<QPointF> latLonPoints;

private:
  int id_;
  static int nextId_;
  int nextId();
};

#endif