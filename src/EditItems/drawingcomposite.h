/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2014 met.no

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

#ifndef DRAWINGCOMPOSITE_H
#define DRAWINGCOMPOSITE_H

#include "drawingitembase.h"

namespace DrawingItem_Composite {
class Composite : public DrawingItemBase
{
public:
  Composite();
  virtual ~Composite();
  virtual QDomNode toKML(const QHash<QString, QString> & = QHash<QString, QString>()) const;
  virtual void draw();

  // Returns the category of the item as required by the style manager.
  virtual Category category() const;

  // Sets the item's geographic points.
  virtual void setLatLonPoints(const QList<QPointF> &points);

  // Reimplementation of the base class's method.
  virtual QRectF boundingRect() const;

  // Updates the bounding rectangle and lays out elements.
  virtual void updateRect();

protected:
  virtual void createElements();

  QList<DrawingItemBase *> elements_;
  Qt::Orientation layout_;
};

} // namespace

#endif // DRAWINGCOMPOSITE_H
