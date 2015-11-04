
#include "diPaintable.h"

#include "diOpenGLWidget.h"
#include "diPaintGLWidget.h"

#define MILOGGER_CATEGORY "diana.DiPaintable"
#include <miLogger/miLogging.h>

// static
QWidget* DiPaintable::createWidget(DiPaintable* p, QWidget* parent)
{
  static int widget_type = -1;
  if (widget_type < 0) {
    if (getenv("USE_PAINTGL") == 0) {
      widget_type = 0;
      METLIBS_LOG_INFO("using OpenGL");
    } else {
      widget_type = 1;
      METLIBS_LOG_INFO("using PaintGL");
    }
  }
  if (widget_type == 0)
    return new DiOpenGLWidget(p, parent);
  else
    return new DiPaintGLWidget(p, parent);
}
