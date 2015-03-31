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
#include "EditItems/drawingstylemanager.h"

namespace DrawingItem_Symbol {

Symbol::Symbol(int id)
  : DrawingItemBase(id)
{
  properties_["size"] = DEFAULT_SYMBOL_SIZE;
}

Symbol::~Symbol()
{
}

QList<QPointF> Symbol::boundingSquare() const
{
  if (points_.isEmpty())
    return QList<QPointF>();

  int size = properties_.value("size", DEFAULT_SYMBOL_SIZE).toInt();

  QList<QPointF> points;
  points.append(points_.at(0) + QPointF(-size/2, -size/2));
  points.append(points_.at(0) + QPointF(size/2, -size/2));
  points.append(points_.at(0) + QPointF(size/2, size/2));
  points.append(points_.at(0) + QPointF(-size/2, size/2));
  return points;
}

QRectF Symbol::boundingRect() const
{
  if (points_.isEmpty())
    return QRectF();

  QString name = property("style:type", "Default").toString();
  int size = properties_.value("size", DEFAULT_SYMBOL_SIZE).toInt();
  QSize defaultSize = DrawingManager::instance()->getSymbolSize(name);
  float aspect = defaultSize.height()/float(defaultSize.width());

  return QRectF(points_.at(0).x() - size/2, points_.at(0).y() - aspect*size/2, size, aspect*size);
}

bool Symbol::hit(const QPointF &pos, bool selected) const
{
  const QPolygonF polygon(boundingSquare().toVector());
  return polygon.containsPoint(pos, Qt::OddEvenFill);
}

bool Symbol::hit(const QRectF &rect) const
{
  Q_UNUSED(rect);
  return false; // for now
}

void Symbol::draw()
{
  if (points_.isEmpty())
    return;
  DrawingStyleManager::instance()->drawSymbol(this);
}

QDomNode Symbol::toKML(const QHash<QString, QString> &extraExtData) const
{
  QHash<QString, QString> extra;
  extra["size"] = QString::number(properties_.value("size", DEFAULT_SYMBOL_SIZE).toInt());
  return DrawingItemBase::toKML(extra.unite(extraExtData));
}

void Symbol::fromKML(const QHash<QString, QString> &extraExtData)
{
  properties_["size"] = extraExtData.value("met:size", DEFAULT_SYMBOL_SIZE_STRING).toInt();
}

DrawingItemBase::Category Symbol::category() const
{
  return DrawingItemBase::Symbol;
}

int Symbol::defaultSize()
{
  return DEFAULT_SYMBOL_SIZE;
}

} // namespace
