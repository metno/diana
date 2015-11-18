
#include "diOpenGLWidget.h"

#include "diOpenGLPainter.h"
#include "diPaintable.h"

#include "diPlotModule.h"

#include <qgl.h>

#include <cmath>

#define MILOGGER_CATEGORY "diana.DiOpenGLWidget"
#include <miLogger/miLogging.h>

namespace /* anonymous*/ {

std::string getGlString(GLenum name)
{
  const GLubyte* t = glGetString(name);
  std::ostringstream ost;
  if (t) {
    ost << "'" << ((const char*) t) << "'";
  } else {
    ost << "ERR:" << glGetError();
  }
  return ost.str();
}

QGLFormat oglfmt()
{
  QGLFormat fmt;
  fmt.setOverlay(false);
  fmt.setDoubleBuffer(true);
  return fmt;
}

} // namespace anonymous

DiOpenGLWidget::DiOpenGLWidget(DiPaintable* p, QWidget* parent)
  : QGLWidget(oglfmt(), parent)
  , glcanvas(new DiOpenGLCanvas(this))
  , glpainter(new DiOpenGLPainter(glcanvas.get()))
  , paintable(p)
  , buffer_data(0)
{
  setFocusPolicy(Qt::StrongFocus);
  p->setCanvas(glcanvas.get());
}

DiOpenGLWidget::~DiOpenGLWidget()
{
  delete[] buffer_data;
}

void DiOpenGLWidget::initializeGL()
{
  static bool version_info = false;
  if (!version_info) {
    METLIBS_LOG_INFO("using OpenGL GL_VERSION=" << getGlString(GL_VERSION)
        << " GL_VENDOR=" << getGlString(GL_VENDOR) << " RENDERER=" << getGlString(GL_RENDERER));
    version_info = true;
  }
  glpainter->ShadeModel(GL_FLAT);
  setAutoBufferSwap(false);
}

void DiOpenGLWidget::paintGL()
{
  if (paintable) {
    paintUnderlay();
    paintOverlay();
    swapBuffers();
  }
}

void DiOpenGLWidget::paintUnderlay()
{
  if (paintable->enable_background_buffer && buffer_data && !paintable->update_background_buffer) {
    // FIXME this is a bad hack
    const Rectangle& ps = PlotModule::instance()->getStaticPlot()->getPlotSize();
    const float delta = (fabs(ps.width()) * 0.1 / width());

    glpainter->PixelZoom(1, 1);
    glpainter->PixelStorei(DiGLPainter::gl_UNPACK_SKIP_ROWS, 0);
    glpainter->PixelStorei(DiGLPainter::gl_UNPACK_SKIP_PIXELS, 0);
    glpainter->PixelStorei(DiGLPainter::gl_UNPACK_ROW_LENGTH, width());
    glpainter->PixelStorei(DiGLPainter::gl_UNPACK_ALIGNMENT, 4);
    glpainter->RasterPos2f(ps.x1 + delta, ps.y1 + delta);

    glpainter->DrawPixels(width(), height(), DiGLPainter::gl_RGBA, DiGLPainter::gl_UNSIGNED_BYTE, buffer_data);
    glpainter->PixelStorei(DiGLPainter::gl_UNPACK_ROW_LENGTH, 0);
    return;
  } else if (!paintable->enable_background_buffer)
    dropBackgroundBuffer();

  paintable->paintUnderlay(glpainter.get());

  if (glpainter->supportsReadPixels()
      && paintable->enable_background_buffer
      && (!buffer_data || paintable->update_background_buffer))
  {
    if (!buffer_data)
      buffer_data = new DiGLPainter::GLuint[4 * width() * height()];

    glpainter->PixelZoom(1, 1);
    glpainter->PixelStorei(DiGLPainter::gl_PACK_SKIP_ROWS, 0);
    glpainter->PixelStorei(DiGLPainter::gl_PACK_SKIP_PIXELS, 0);
    glpainter->PixelStorei(DiGLPainter::gl_PACK_ROW_LENGTH, width());
    glpainter->PixelStorei(DiGLPainter::gl_PACK_ALIGNMENT, 4);

    glpainter->ReadPixels(0, 0, width(), height(), DiGLPainter::gl_RGBA, DiGLPainter::gl_UNSIGNED_BYTE, buffer_data);
    glpainter->PixelStorei(DiGLPainter::gl_PACK_ROW_LENGTH, 0);
    paintable->update_background_buffer = false;
  }
}

void DiOpenGLWidget::paintOverlay()
{
  paintable->paintOverlay(glpainter.get());
}

void DiOpenGLWidget::dropBackgroundBuffer()
{
  delete buffer_data;
  buffer_data = 0;
}

void DiOpenGLWidget::resizeGL(int w, int h)
{
  if (paintable)
    paintable->resize(w, h);
  dropBackgroundBuffer();
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
