
#include "diPaintable.h"

#include "diOpenGLWidget.h"
#include "diPaintGLWidget.h"

#define MILOGGER_CATEGORY "diana.DiPaintable"
#include <miLogger/miLogging.h>

namespace /* anonymous*/ {

bool eq(const char* a, const char* b)
{
  return (a == b) || (a != 0 && b != 0 && strcasecmp(a, b) == 0);
}

} // namespace anonymous

// static
QWidget* DiPaintable::createWidget(DiPaintable* p, QWidget* parent)
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
    return new DiOpenGLWidget(p, parent);
  else
    return new DiPaintGLWidget(p, parent);
}

DiPaintable::DiPaintable()
  : enable_background_buffer(false)
  , update_background_buffer(false)
  , mCanvas(0)
{
}

void DiPaintable::setCanvas(DiCanvas* canvas)
{
  mCanvas = canvas;
}
