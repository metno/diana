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

#ifndef DIANA_STREAMLINEPALETTEPAINTER_H
#define DIANA_STREAMLINEPALETTEPAINTER_H 1

#include "diStreamlinePainter.h"

#include <QPolygonF>

#include <vector>

class DiGLPainter;
class PlotOptions;

// from "diPolyContouring.h"
class DianaLevels;
typedef std::shared_ptr<DianaLevels> DianaLevels_p;

class StreamlinePalettePainter : public StreamlinePainter
{
public:
  StreamlinePalettePainter(DiGLPainter* gl, float scale, const PlotOptions& po);

  void add(float x, float y, float speed) override;
  void close() override;

private:
  DiGLPainter* gl_;
  const PlotOptions& po_;
  DianaLevels_p levels_;

  bool arrow_marker_;        // set from PlotOptions.frame
  float marker_size_;        // if draw_arrow: size of arrow; else radius of marker circle
  const int line_min_length; //! minimum length for drawing line segments

  QPolygonF linepoints_;
  std::vector<int> linelevels_;
};

#endif // DIANA_STREAMLINEPALETTEPAINTER_H
