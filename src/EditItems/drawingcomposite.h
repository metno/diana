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

#include <QImage>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include "drawingitembase.h"

namespace DrawingItem_Composite {
class Composite : public DrawingItemBase
{
public:
  enum Orientation {
    Horizontal, Vertical, Diagonal
  };

  Composite(int = -1);
  virtual ~Composite();
  virtual void fromKML(const QHash<QString, QString> & = QHash<QString, QString>());
  virtual QDomNode toKML(const QHash<QString, QString> & = QHash<QString, QString>()) const;
  virtual void draw();

  // Returns the category of the item as required by the style manager.
  virtual Category category() const;

  // Reimplementation of the base class's method.
  virtual QRectF boundingRect() const;

  // Updates the bounding rectangle and lays out elements.
  virtual void updateRect();

  virtual bool hit(const QPointF &pos, bool selected) const;
  virtual bool hit(const QRectF &bbox) const;

  virtual void arrangeElements();
  virtual void createElements();
  DrawingItemBase *elementAt(int index) const;
  void readExtraProperties();
  void writeExtraProperties();

  virtual void setPoints(const QList<QPointF> &points);

protected:
  virtual DrawingItemBase *newCompositeItem() const;
  virtual DrawingItemBase *newPolylineItem() const;
  virtual DrawingItemBase *newSymbolItem() const;
  virtual DrawingItemBase *newTextItem() const;

  QList<DrawingItemBase *> elements_;
  Orientation layout_;

private:
  /// Serialises the properties of the child elements as a string.
  void toKMLExtraData(QXmlStreamWriter &stream, const QVariantList &children) const;
  /// Unpacks serialised property data from a string.
  QVariantList fromKMLExtraData(QXmlStreamReader &stream);

  bool created_;
};

} // namespace

#endif // DRAWINGCOMPOSITE_H
