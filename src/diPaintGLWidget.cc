
#include "diPaintGLWidget.h"

#include "diPaintable.h"

#include <QtGui>

#define MILOGGER_CATEGORY "diana.DiPaintGLWidget"
#include <miLogger/miLogging.h>

DiPaintGLWidget::DiPaintGLWidget(DiPaintable* p, QWidget *parent, bool aa)
  : QWidget(parent)
  , glcanvas(new DiPaintGLCanvas(this))
  , glpainter(new DiPaintGLPainter(glcanvas.get()))
  , paintable(p)
  , background_buffer(0)
  , antialiasing(aa)
{
  setFocusPolicy(Qt::StrongFocus);
  glpainter->ShadeModel(DiGLPainter::gl_FLAT);
  glpainter->HIGH_QUALITY_BUT_SLOW = false;
  p->setCanvas(glcanvas.get());
}

DiPaintGLWidget::~DiPaintGLWidget()
{
  delete background_buffer;
}

void DiPaintGLWidget::dropBackgroundBuffer()
{
  delete background_buffer;
  background_buffer = 0;
}

void DiPaintGLWidget::paintEvent(QPaintEvent* event)
{
  QPainter painter;
  painter.begin(this);
  if (paintable)
    paint(painter);
  painter.end();
}

void DiPaintGLWidget::resizeEvent(QResizeEvent* event)
{
  dropBackgroundBuffer();
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

void DiPaintGLWidget::paint(QPainter& wpainter)
{
  METLIBS_LOG_SCOPE(LOGVAL(paintable->enable_background_buffer)
      << LOGVAL((!background_buffer))
      << LOGVAL(paintable->update_background_buffer));

  glpainter->clear = true;
  if (paintable->enable_background_buffer) {
    if (!background_buffer || paintable->update_background_buffer) {
      if (!background_buffer)
        background_buffer = new QImage(size(), QImage::Format_ARGB32);
      // clear+paint into bg image
      QPainter ipainter(background_buffer);
      ipainter.setRenderHint(QPainter::Antialiasing, antialiasing);
      glpainter->begin(&ipainter);
      METLIBS_LOG_DEBUG("underlay to image");
      paintable->paintUnderlay(glpainter.get());
      glpainter->end();
      ipainter.end();
      paintable->update_background_buffer = false;
    }

    // render bg from buffer
    METLIBS_LOG_DEBUG("underlay from image");
    wpainter.setRenderHint(QPainter::Antialiasing, false);
    wpainter.drawImage(QPoint(0,0), *background_buffer);
    wpainter.setRenderHint(QPainter::Antialiasing, antialiasing);
    glpainter->clear = false;
    glpainter->begin(&wpainter);
  } else {
    METLIBS_LOG_DEBUG("underlay direct");
    dropBackgroundBuffer();
    wpainter.setRenderHint(QPainter::Antialiasing, antialiasing);
    glpainter->begin(&wpainter);
    paintable->paintUnderlay(glpainter.get());
  }

  METLIBS_LOG_DEBUG("overlay");
  paintable->paintOverlay(glpainter.get());
  glpainter->end();
}

void DiPaintGLWidget::updateGL()
{
  update();
}
