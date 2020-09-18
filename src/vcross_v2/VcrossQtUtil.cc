/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2020 met.no

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

#include "VcrossQtUtil.h"

#include "diLinetype.h"

#include <mi_fieldcalc/math_util.h>

#include <QtGui/QPainter>

namespace vcross {
namespace util {

void updateMaxStringWidth(QPainter& painter, float& w, const std::string& txt)
{
  const float txt_w = painter.fontMetrics().width(QString::fromStdString(txt));
  miutil::maximize(w, txt_w);
}

void setDash(QPen& pen, bool stipple, int factor, unsigned short pattern)
{
  if (factor <= 0 or pattern == 0xFFFF or pattern == 0)
    stipple = false;

  if (not stipple) {
    pen.setStyle(Qt::SolidLine);
    return;
  }

  // adapted from glLineStipple in PaintGL/paintgl.cc 

  QVector<qreal> dashes;
  unsigned short state = pattern & 1;
  bool gapStart = (pattern & 1) == 0;
  int number = 0;
  int total = 0;
  
  for (int i = 0; i < 16; ++i) {
    unsigned short dash = pattern & 1;
    if (dash == state)
      number++;
    else {
      dashes << number * factor;
      total += number * factor;
      state = dash;
      number = 1;
    }
    pattern = pattern >> 1;
  }
  if (number > 0)
    dashes << number * factor;
  
  // Ensure that the pattern has an even number of elements by inserting
  // a zero-size element if necessary.
  if (dashes.size() % 2 == 1)
    dashes << 0;

  /* If the pattern starts with a gap then move it to the end of the
     vector and adjust the starting offset to its location to ensure
     that it appears at the start of the pattern. (This is because
     QPainter's dash pattern rendering assumes that the first element
     is a line. */
  int dashOffset = 0;
  if (gapStart) {
    dashes << dashes.first();
    dashes.pop_front();
    dashOffset = total - dashes.last();
  }
  
  pen.setDashPattern(dashes);
  pen.setDashOffset(dashOffset);
}

void setDash(QPen& pen, const Linetype& linetype)
{
  vcross::util::setDash(pen, linetype.stipple, linetype.factor, linetype.bmap);
}

} // namespace util
} // namespace vcross
