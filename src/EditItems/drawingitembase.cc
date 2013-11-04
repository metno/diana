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

#include "drawingitembase.h"

DrawingItemBase::DrawingItemBase()
    : id_(nextId())
{
}

DrawingItemBase::~DrawingItemBase()
{
}

int DrawingItemBase::nextId_ = 0;

int DrawingItemBase::id() const { return id_; }

int DrawingItemBase::nextId()
{
    return nextId_++; // ### not thread safe; use a mutex for that
}

int DrawingItemBase::groupId() const
{
    const QVariant vgid = properties_.value("groupId");
    int gid = -1;
    if (vgid.isValid()) {
        bool ok;
        gid = vgid.toInt(&ok);
        if (!ok)
            gid = -1;
    }
    return gid;
}

QVariant DrawingItemBase::property(const QString &name, const QVariant &default_) const
{
  if (properties_.contains(name))
    return properties_.value(name);
  else
    return default_;
}

void DrawingItemBase::setProperty(const QString &name, const QVariant &value)
{
  properties_[name] = value;
}

QVariantMap DrawingItemBase::properties() const
{
  return properties_;
}

QVariantMap &DrawingItemBase::propertiesRef()
{
  return properties_;
}

void DrawingItemBase::setProperties(const QVariantMap &properties)
{
  properties_ = properties;
  if (properties.contains("points")) {
    QVariantList points = properties.value("points").toList();
    points_.clear();
    latLonPoints.clear();
    foreach (QVariant v, points) {
      latLonPoints.append(v.toPointF());
    }
  }
}

QList<QPointF> DrawingItemBase::getPoints() const
{
    return points_;
}

void DrawingItemBase::setPoints(const QList<QPointF> &points)
{
    points_ = points;
}

QList<QPointF> DrawingItemBase::getLatLonPoints() const
{
    return latLonPoints;
}

void DrawingItemBase::setLatLonPoints(const QList<QPointF> &points)
{
    latLonPoints = points;
}
