
#include "diOpenGLWidget.h"

#include "diOpenGLPainter.h"
#include "diPaintable.h"

#include <qgl.h>

static QGLFormat oglfmt()
{
  QGLFormat fmt;
  fmt.setOverlay(false);
  fmt.setDoubleBuffer(true);
  return fmt;
}

DiOpenGLWidget::DiOpenGLWidget(DiPaintable* p, QWidget* parent)
  : QGLWidget(oglfmt(), parent)
  , glcanvas(new DiOpenGLCanvas(this))
  , glpainter(new DiOpenGLPainter(glcanvas.get()))
  , paintable(p)
{
  setFocusPolicy(Qt::StrongFocus);
  p->setCanvas(glcanvas.get());
}

void DiOpenGLWidget::initializeGL()
{
  glpainter->ShadeModel(GL_FLAT);
  setAutoBufferSwap(false);
}

void DiOpenGLWidget::paintGL()
{
  if (paintable) {
    paintable->paint(glpainter.get());
    swapBuffers();
  }
}

void DiOpenGLWidget::resizeGL(int w, int h)
{
  if (paintable)
    paintable->resize(w, h);
  glpainter->Viewport(0, 0, (GLint)w, (GLint)h);
  updateGL();
  setFocus();
}

void DiOpenGLWidget::keyPressEvent(QKeyEvent *ke)
{
  if (paintable && paintable->handleKeyEvents(ke))
    updateGL();
}

void DiOpenGLWidget::keyReleaseEvent(QKeyEvent *ke)
{
  if (paintable && paintable->handleKeyEvents(ke))
    updateGL();
}

void DiOpenGLWidget::mousePressEvent(QMouseEvent* me)
{
  if (paintable && paintable->handleMouseEvents(me))
    updateGL();
}

void DiOpenGLWidget::mouseMoveEvent(QMouseEvent* me)
{
  if (paintable && paintable->handleMouseEvents(me))
    updateGL();
}

void DiOpenGLWidget::mouseReleaseEvent(QMouseEvent* me)
{
  if (paintable && paintable->handleMouseEvents(me))
    updateGL();
}

void DiOpenGLWidget::mouseDoubleClickEvent(QMouseEvent* me)
{
  if (paintable && paintable->handleMouseEvents(me))
    updateGL();
}

void DiOpenGLWidget::wheelEvent(QWheelEvent *we)
{
  if (paintable && paintable->handleWheelEvents(we))
    updateGL();
}
