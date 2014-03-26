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

#include "GL/gl.h"
#include "drawingcomposite.h"
#include "diDrawingManager.h"

namespace DrawingItem_Composite {

Composite::Composite()
{
}

Composite::~Composite()
{
}

void Composite::draw()
{
  if (children.isEmpty())
    return;
}

QDomNode Composite::toKML() const
{
  return DrawingItemBase::toKML(); // call base implementation for now
}

DrawingItemBase::Category Composite::category() const
{
  return DrawingItemBase::Composite;
}

} // namespace
