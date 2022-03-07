/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2022 met.no

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

#include "diStreamlineLengthPainter.h"

#include "diColour.h"
#include "diGLPainter.h"
#include "diPlotOptions.h"

StreamlineLengthPainter::StreamlineLengthPainter(DiGLPainter* gl, const PlotOptions& po)
    : gl_(gl)
    , po_(po)
    , line_min_length(3)
    , cut_(2)
    , dist_(0)
{
  gl_->setLineStyle(po_.linecolour, po_.linewidth, true);
  gl_->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_LINE);

  gl_->Enable(DiGLPainter::gl_BLEND);
  gl_->BlendFunc(DiGLPainter::gl_SRC_ALPHA, DiGLPainter::gl_ONE_MINUS_SRC_ALPHA);
}

void StreamlineLengthPainter::add(float x, float y, float speed)
{
  linepoints_ << QPointF(x, y);
  dist_ += 1 / std::max(speed, 0.01f);
  draw();
}

void StreamlineLengthPainter::draw()
{
  if (dist_ < cut_)
    return;

  const int npoints = linepoints_.size();
  if (npoints >= line_min_length) {
    for (int i = 1; i < npoints; ++i) {
      Colour c(po_.linecolour);
      c.set(Colour::alpha, c.A() * i / float(npoints));
      gl_->setColour(c);
      const auto& p0 = linepoints_[i - 1];
      const auto& p1 = linepoints_[i];
      gl_->drawLine(p0.x(), p0.y(), p1.x(), p1.y());
    }
  }

  reset();
}

void StreamlineLengthPainter::close()
{
  draw();
  reset(); // always reset, not only if drawing
}
