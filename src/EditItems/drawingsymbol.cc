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
}

Symbol::~Symbol()
{
}

QList<QPointF> Symbol::boundingSquare() const
{
  if (points_.size() < 2)
    return QList<QPointF>();

  QList<QPointF> points;
  points.append(points_.at(0));
  points.append(QPointF(points_.at(1).x(), points_.at(0).y()));
  points.append(points_.at(1));
  points.append(QPointF(points_.at(0).x(), points_.at(1).y()));
  return points;
}

void Symbol::draw()
{
  if (points_.size() < 2)
    return;

  DrawingManager::instance()->drawSymbol(property("style:type", "Default").toString(),
    points_.at(0).x(), points_.at(0).y(),
    points_.at(1).x() - points_.at(0).x(), points_.at(1).y() - points_.at(0).y());
}

QDomNode Symbol::toKML() const
{
  return DrawingItemBase::toKML(); // call base implementation for now
}

DrawingItemBase::Category Symbol::category() const
{
  return DrawingItemBase::Symbol;
}

} // namespace
