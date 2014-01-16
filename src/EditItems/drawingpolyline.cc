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
#include <diTesselation.h>
#include "drawingpolyline.h"

namespace DrawingItem_PolyLine {

PolyLine::PolyLine()
{ 
  // ### FOR TESTING:
  propertiesRef().insert("style:type", "custom");
  propertiesRef().insert("style:lineType", QVariant());
  static int nn = 0;
  if (nn++ % 2)
    propertiesRef().insert("style:lineWidth", 2);
  else
    propertiesRef().insert("style:lineWidth", 1);
  propertiesRef().insert("style:lineColor", QColor(0, 0, 0));
}

PolyLine::~PolyLine()
{
}

void PolyLine::draw()
{
  if (points_.isEmpty())
    return;

  // Find the polygon style to use, if one exists.
  PolygonStyle *style = static_cast<PolygonStyle *>(getStyle());

  // draw the interior
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glEnable( GL_BLEND );
  GLdouble *gldata = new GLdouble[points_.size() * 3];
  for (int i = 0; i < points_.size(); ++i) {
    const QPointF p = points_.at(i);
    gldata[3 * i] = p.x();
    gldata[3 * i + 1] = p.y();
    gldata[3 * i + 2] = 0.0;
  }

  // Use the fill colour defined in the style.
  style->beginFill();

  beginTesselation();
  int npoints = points_.size();
  tesselation(gldata, 1, &npoints);
  endTesselation();
  delete[] gldata;

  style->endFill();

  // draw the outline
  QColor color = properties().value("style:lineColor").value<QColor>();
  if (!color.isValid())
    color = QColor(0, 0, 0);
  glColor3ub(color.red(), color.green(), color.blue());
  //
  glPushAttrib(GL_LINE_BIT);
  bool ok = false;
  const int lineWidth = properties().value("style:lineWidth").toInt(&ok);
  glLineWidth(ok ? lineWidth : 1);

  //
  glBegin(GL_LINE_LOOP);
  foreach (QPointF p, points_)
    glVertex2i(p.x(), p.y());
  glEnd(); // GL_LINE_LOOP

  //
  glPopAttrib();
}

QDomNode PolyLine::toKML() const
{
  return DrawingItemBase::toKML(); // call base implementation for now
}

} // namespace
