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

#include "diPlot.h"
#include "drawingitembase.h"

namespace DrawingItem_Text {
class Text : public Plot, public DrawingItemBase
{
public:
  Text();
  virtual ~Text();
  virtual QDomNode toKML(const QHash<QString, QString> & = QHash<QString, QString>()) const;
  virtual void fromKML(const QHash<QString, QString> & = QHash<QString, QString>());
  virtual void draw();

  virtual const QStringList &text() const;
  virtual void setText(const QStringList &lines);

  GLfloat fontScale() const;
  void updateRect();

  // Returns the category of the item as required by the style manager.
  virtual Category category() const;

protected:
  QSizeF getStringSize(const QString &text, int index = -1) const;

  int margin_;
  float spacing_;
};

} // namespace

#endif // DRAWINGTEXT_H
