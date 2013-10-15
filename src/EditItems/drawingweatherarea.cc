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
#include "drawingweatherarea.h"

namespace DrawingItem_WeatherArea {

WeatherArea::WeatherArea()
{
}

WeatherArea::~WeatherArea()
{
}

void WeatherArea::draw()
{
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
    glColor4ub(128, 128, 128, 50);
    beginTesselation();
    int npoints = points_.size();
    tesselation(gldata, 1, &npoints);
    endTesselation();
    delete[] gldata;

    // draw the outline
    glBegin(GL_LINE_LOOP);
    glColor3ub(color_.red(), color_.green(), color_.blue());
    foreach (QPointF p, points_)
        glVertex2i(p.x(), p.y());
    glEnd();
}

} // namespace