/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017 met.no

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

#include "diPaintableWidget.h"

#include "diOpenGLWidget.h"
#include "diPaintGLWidget.h"

#define MILOGGER_CATEGORY "diana.DiPaintableWidget"
#include <miLogger/miLogging.h>

namespace /* anonymous*/ {

bool eq(const char* a, const char* b)
{
  return (a == b) || (a != 0 && b != 0 && strcasecmp(a, b) == 0);
}

} // namespace anonymous

namespace diana {
QWidget* createPaintableWidget(Paintable* p, UiEventHandler* i, QWidget* parent)
{
  static int widget_type = -1;
  if (widget_type < 0) {
    const char* USE_PAINTGL = getenv("USE_PAINTGL");
    if (!USE_PAINTGL || eq(USE_PAINTGL, "0") || eq(USE_PAINTGL, "no") || eq(USE_PAINTGL, "false")) {
      widget_type = 0;
      METLIBS_LOG_INFO("using OpenGL");
    } else {
      widget_type = 1;
      METLIBS_LOG_INFO("using PaintGL");
    }
  }
  if (widget_type == 0)
    return new DiOpenGLWidget(p, i, parent);
  else
    return new DiPaintGLWidget(p, i, parent);
}
} // namespace diana
