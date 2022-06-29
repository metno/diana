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

#include "diStreamlinePalettePainter.h"

#include "diGLPainter.h"
#include "diPolyContouring.h"
#include "diUtilities.h" // find_index

#include "mi_fieldcalc/FieldDefined.h" // fieldUndef

namespace {
const Colour* colorForLevel(const PlotOptions& po_, int li)
{
  // FIXME this is complicated and a partial copy of DianaLines::paint_polygons()

  const int ncolours = po_.palettecolours.size();
  const int ncolours_cold = po_.palettecolours_cold.size();
  if (li != DianaLevels::UNDEF_LEVEL) {
    if (li <= 0 and ncolours_cold) {
      const int idx = diutil::find_index(po_.repeat, ncolours_cold, -li);
      return &po_.palettecolours_cold[idx];
    } else if (ncolours) {
      if (li <= 0 and !po_.loglinevalues().empty())
        return nullptr;
      const int idx = diutil::find_index(po_.repeat, ncolours, li - 1);
      return &po_.palettecolours[idx];
    }
  }
  return &po_.linecolour;
}
} // namespace

StreamlinePalettePainter::StreamlinePalettePainter(DiGLPainter* gl, float scale, const PlotOptions& po)
    : gl_(gl)
    , po_(po)
    , arrow_marker_(false)
    , marker_size_(po_.linewidth * (arrow_marker_ ? 6 : 0.75) * scale)
    , line_min_length(5)
{
  gl_->setLineStyle(po_.linecolour, po_.linewidth, true);
  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_LINE);

  levels_ = dianaLevelsForPlotOptions(po_, fieldUndef);
}

void StreamlinePalettePainter::add(float x, float y, float speed)
{
  linepoints_ << QPointF(x, y);
  linelevels_.push_back(levels_->level_for_value(speed));
}

void StreamlinePalettePainter::close()
{
  if (linepoints_.size() >= line_min_length) {
#if 1
    for (int i = 1; i < linepoints_.size(); ++i) {
      const auto li = linelevels_[i - 1];
      if (const Colour* c = colorForLevel(po_, li)) {
        gl_->setColour(*c);
        const auto& p0 = linepoints_[i - 1];
        const auto& p1 = linepoints_[i];
        gl_->drawLine(p0.x(), p0.y(), p1.x(), p1.y());
      }
    }
#else
    gl_->drawPolyline(linepoints_);
#endif

    gl_->setColour(po_.linecolour);
    if (!arrow_marker_) {
      gl_->drawCircle(true, linepoints_.back().x(), linepoints_.back().y(), marker_size_);
    } else {
      const QPointF& p1 = linepoints_[linepoints_.size() - 2];
      const QPointF& p2 = linepoints_.back();
      gl_->drawArrowHead(p1.x(), p1.y(), p2.x(), p2.y(), marker_size_);
    }
  }
  linepoints_.clear();
  linelevels_.clear();
}
