
#include "diPaintGLWidget.h"

#include "diPaintable.h"

#include <QtGui>

DiPaintGLWidget::DiPaintGLWidget(DiPaintable* p, QWidget *parent, bool aa)
  : QWidget(parent)
  , glcanvas(new DiPaintGLCanvas(this))
  , glpainter(new DiPaintGLPainter(glcanvas.get()))
  , paintable(p)
  , antialiasing(aa)
{
  glpainter->ShadeModel(DiGLPainter::gl_FLAT);
  p->setCanvas(glcanvas.get());
}

DiPaintGLWidget::~DiPaintGLWidget()
{
}

void DiPaintGLWidget::paintEvent(QPaintEvent* event)
{
  QPainter painter;
  painter.begin(this);
  if (antialiasing)
    painter.setRenderHint(QPainter::Antialiasing);
  paint(&painter);
  painter.end();
}

void DiPaintGLWidget::resizeEvent(QResizeEvent* event)
{
  int w = event->size().width();
  int h = event->size().height();
  glpainter->Viewport(0, 0, w, h);
  paintable->resize(w, h);
}

void DiPaintGLWidget::keyPressEvent(QKeyEvent *ke)
{
  if (paintable && paintable->handleKeyEvents(ke))
    update();
}

void DiPaintGLWidget::keyReleaseEvent(QKeyEvent *ke)
{
  if (paintable && paintable->handleKeyEvents(ke))
    update();
}

void DiPaintGLWidget::mousePressEvent(QMouseEvent* me)
{
  if (paintable && paintable->handleMouseEvents(me))
    update();
}

void DiPaintGLWidget::mouseMoveEvent(QMouseEvent* me)
{
  if (paintable && paintable->handleMouseEvents(me))
    update();
}

void DiPaintGLWidget::mouseReleaseEvent(QMouseEvent* me)
{
  if (paintable && paintable->handleMouseEvents(me))
    update();
}

void DiPaintGLWidget::mouseDoubleClickEvent(QMouseEvent* me)
{
  if (paintable && paintable->handleMouseEvents(me))
    update();
}

void DiPaintGLWidget::wheelEvent(QWheelEvent *we)
{
  if (paintable && paintable->handleWheelEvents(we))
    update();
}

void DiPaintGLWidget::paint(QPainter *painter)
{
  glpainter->begin(painter);
  paintable->paint(glpainter.get());
  glpainter->end();
}

void DiPaintGLWidget::updateGL()
{
  update();
}
