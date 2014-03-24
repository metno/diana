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
#include "diPlot.h"

namespace DrawingItem_Text {
class Text : public Plot, public DrawingItemBase
{
public:
  Text();
  virtual ~Text();
  virtual QDomNode toKML() const;
  virtual void draw();

protected:
  QSizeF getStringSize(int index = -1) const;

  QString text_;
  int margin_;
};

} // namespace

#endif // DRAWINGTEXT_H
