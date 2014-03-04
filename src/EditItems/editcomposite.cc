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

#include "editcomposite.h"

namespace EditItem_Composite {

Composite::Composite()
{
}

Composite::~Composite()
{
}

DrawingItemBase *Composite::cloneSpecial() const
{
  Composite *item = new Composite;
  copyBaseData(item);
  // ### copy special data from this into item ... TBD
  return item;
}

bool Composite::hit(const QPointF &pos, bool selected) const
{
  return false;
}

bool Composite::hit(const QRectF &bbox) const
{
  return false;
}

void Composite::resize(const QPointF &)
{

}

void Composite::updateControlPoints()
{

}

void Composite::drawHoverHighlighting(bool) const
{

}

} // namespace EditItem_Composite
