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
  : size_(32)
{
}

Symbol::~Symbol()
{
}

QList<QPointF> Symbol::boundingSquare() const
{
  if (points_.isEmpty())
    return QList<QPointF>();

  Q_ASSERT(points_.size() == 1);

  const int w_2 = size_ / 2;
  QList<QPointF> points;
  const int x = points_.first().x();
  const int y = points_.first().y();
  points.append(QPointF(x - w_2, y - w_2));
  points.append(QPointF(x - w_2, y + w_2));
  points.append(QPointF(x + w_2, y + w_2));
  points.append(QPointF(x + w_2, y - w_2));
  return points;
}

void Symbol::draw()
{
  if (points_.isEmpty())
    return;

  const QList<QPointF> bbox = boundingSquare();

  DrawingManager::instance()->drawSymbol("Default",
    bbox.at(0).x(), bbox.at(0).y(),
    bbox.at(2).x() - bbox.at(0).x(), bbox.at(2).y() - bbox.at(0).y());
}

QDomNode Symbol::toKML() const
{
  return DrawingItemBase::toKML(); // call base implementation for now
}

} // namespace
