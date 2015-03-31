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

#ifndef DRAWINGTEXT_H
#define DRAWINGTEXT_H

#include "drawingitembase.h"

namespace DrawingItem_Text {
class Text : public DrawingItemBase
{
public:
  Text(int = -1);
  virtual ~Text();
  virtual QDomNode toKML(const QHash<QString, QString> & = QHash<QString, QString>()) const;
  virtual void fromKML(const QHash<QString, QString> & = QHash<QString, QString>());
  virtual void draw();

  virtual QStringList text() const;
  virtual void setText(const QStringList &lines);

  QRectF drawingRect() const;

  void updateRect();

  virtual bool hit(const QPointF &pos, bool selected) const;
  virtual bool hit(const QRectF &bbox) const;

  // Returns the category of the item as required by the style manager.
  virtual Category category() const;

  float margin() const;
  float spacing() const;
  float fontSize() const;
  QSizeF getStringSize(const QString &text, int index = -1) const;

private:
  static float defaultMargin() { return 4; }
  static float defaultSpacing() { return 0.5; }
};

} // namespace

#endif // DRAWINGTEXT_H
