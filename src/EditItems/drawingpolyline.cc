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

#include "GL/gl.h"
#include "drawingpolyline.h"
#include "diDrawingManager.h"

namespace DrawingItem_PolyLine {

PolyLine::PolyLine()
{ 
#if 0
  propertiesRef().insert("style:type", "custom");
  propertiesRef().insert("style:lineType", QVariant());
  static int nn = 0;
  if (nn++ % 2)
    propertiesRef().insert("style:lineWidth", 2);
  else
    propertiesRef().insert("style:lineWidth", 1);
  propertiesRef().insert("style:lineColor", QColor(0, 0, 0));
#endif
}

PolyLine::~PolyLine()
{
}

void PolyLine::draw()
{
  if (points_.isEmpty())
    return;

  // Find the polygon style to use, if one exists.
  QString style = property("style:type").toString();
  DrawingStyleManager *styleManager = DrawingStyleManager::instance();

  // Use the fill colour defined in the style.
  styleManager->beginFill(style);

  styleManager->fillLoop(style, points_);

  styleManager->endFill(style);

  // Draw the outline using the border colour and line pattern defined in
  // the style.
  styleManager->beginLine(style);
  glBegin(GL_LINE_LOOP);

  styleManager->drawLoop(style, points_);

  glEnd(); // GL_LINE_LOOP
  styleManager->endLine(style);
}

QDomNode PolyLine::toKML() const
{
  return DrawingItemBase::toKML(); // call base implementation for now
}

} // namespace
