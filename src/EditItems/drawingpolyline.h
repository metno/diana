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

#ifndef DRAWINGITEM_POLYLINE_H
#define DRAWINGITEM_POLYLINE_H

#include "drawingitembase.h"
#include <QColor>
#include <QDomDocument>
#include <QSet>

namespace DrawingItem_PolyLine {

class PolyLine : public DrawingItemBase
{
public:
  PolyLine(int = -1);
  virtual ~PolyLine();
  virtual QDomNode toKML() const;

  virtual bool hit(const QPointF &, bool) const;
  virtual bool hit(const QRectF &) const;
  int hitLine(const QPointF &) const;

  static qreal dist2(const QPointF &, const QPointF &);
  static qreal distance2(const QPointF &, const QPointF &, const QPointF &);
  qreal distance(const QPointF &) const;

  // Returns the category of the item as required by the style manager.
  virtual Category category() const;

protected:
  QColor color_;

private:
  virtual void draw();
};

} // namespace

#endif // DRAWINGITEM_POLYLINE_H
