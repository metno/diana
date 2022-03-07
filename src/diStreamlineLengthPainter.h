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

#ifndef DIANA_STREAMLINELENGTHPAINTER_H
#define DIANA_STREAMLINELENGTHPAINTER_H 1

#include "diStreamlinePainter.h"

#include <QPolygonF>

class DiGLPainter;
class PlotOptions;

/** Paint lines as segments where higher speed has longer lines.
 */
class StreamlineLengthPainter : public StreamlinePainter
{
public:
  StreamlineLengthPainter(DiGLPainter* gl, const PlotOptions& po);

  void add(float x, float y, float speed) override;
  void close() override;

private:
  void draw();
  void reset()
  {
    dist_ = 0;
    linepoints_.clear();
  }

private:
  DiGLPainter* gl_;
  const PlotOptions& po_;
  const int line_min_length; //! minimum length for drawing line segments
  float cut_;

  QPolygonF linepoints_;
  float dist_;
};

#endif // DIANA_STREAMLINELENGTHPAINTER_H
