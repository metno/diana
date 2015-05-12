
#include "diPaintable.h"

#include "diOpenGLWidget.h"
#include "diPaintGLWidget.h"

// static
QWidget* DiPaintable::createWidget(DiPaintable* p, QWidget* parent)
{
  if (getenv("USE_PAINTGL") == 0)
    return new DiOpenGLWidget(p, parent);
  return new DiPaintGLWidget(p, parent);
}
