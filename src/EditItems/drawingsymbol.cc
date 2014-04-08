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


#include "GL/gl.h"
#include "drawingsymbol.h"
#include "diDrawingManager.h"

namespace DrawingItem_Symbol {

Symbol::Symbol()
{
  size_ = 32;
}

Symbol::~Symbol()
{
}

QList<QPointF> Symbol::boundingSquare() const
{
  if (points_.isEmpty())
    return QList<QPointF>();

  QList<QPointF> points;
  points.append(points_.at(0) + QPointF(-size_/2, -size_/2));
  points.append(points_.at(0) + QPointF(size_/2, -size_/2));
  points.append(points_.at(0) + QPointF(size_/2, size_/2));
  points.append(points_.at(0) + QPointF(-size_/2, size_/2));
  return points;
}

void Symbol::draw()
{
  if (points_.isEmpty())
    return;

  DrawingManager::instance()->drawSymbol(property("style:type", "Default").toString(),
    points_.at(0).x() - size_/2, points_.at(0).y() - size_/2, size_, size_);
}

QDomNode Symbol::toKML(const QHash<QString, QString> &extraExtData) const
{
  QHash<QString, QString> extra;
  extra["size"] = QString::number(size_);
  return DrawingItemBase::toKML(extra.unite(extraExtData));
}

void Symbol::fromKML(const QHash<QString, QString> &extraExtData)
{
  size_ = extraExtData.value("met:size", "32").toInt();
}

DrawingItemBase::Category Symbol::category() const
{
  return DrawingItemBase::Symbol;
}

} // namespace
