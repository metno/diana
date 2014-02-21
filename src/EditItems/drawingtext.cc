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
#include "drawingtext.h"
#include "diDrawingManager.h"
#include "diFontManager.h"
#include <QDebug>

namespace DrawingItem_Text {

Text::Text()
{
}

Text::~Text()
{
}

void Text::draw()
{
  if (points_.isEmpty() || text_.isEmpty())
    return;

  DrawingStyleManager *styleManager = DrawingStyleManager::instance();
  styleManager->beginText(this);

  GLfloat s = qMax(pwidth/maprect.width(), pheight/maprect.height());
  fp->set(poptions.fontname, poptions.fontface, poptions.fontsize * s);
  fp->drawStr(text_.toStdString().c_str(), points_.at(0).x(), points_.at(0).y(), 0);

  styleManager->endText(this);
}

QDomNode Text::toKML() const
{
  return DrawingItemBase::toKML(); // call base implementation for now
}

} // namespace
