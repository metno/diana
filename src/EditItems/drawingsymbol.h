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

#ifndef DRAWINGITEM_SYMBOL_H
#define DRAWINGITEM_SYMBOL_H

#include "drawingitembase.h"

namespace DrawingItem_Symbol {

#define DEFAULT_SYMBOL_SIZE 48
#define DEFAULT_SYMBOL_SIZE_STRING "48"

class Symbol : public DrawingItemBase
{
public:
  Symbol(int = -1);
  virtual ~Symbol();
  virtual QDomNode toKML(const QHash<QString, QString> &extraExtData) const;
  virtual void fromKML(const QHash<QString, QString> & = QHash<QString, QString>());
  virtual QRectF boundingRect() const;

  virtual bool hit(const QPointF &, bool) const;
  virtual bool hit(const QRectF &) const;

  static int defaultSize();

protected:
  QList<QPointF> boundingSquare() const;
  virtual void draw();

  // Returns the category of the item as required by the style manager.
  virtual Category category() const;

private:
  QString name;
};

} // namespace

#endif // DRAWINGITEM_SYMBOL_H
