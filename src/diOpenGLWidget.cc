
#define GL_GLEXT_PROTOTYPES // needed for glWindowPos2i, which is OpenGL 1.4

#include "diOpenGLWidget.h"

#include "diOpenGLPainter.h"
#include "diPaintable.h"

#include <qgl.h>

#include <cmath>
#include <cstdlib>
#include <sstream>

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

int gl_major = 0, gl_minor = 0;

void check_gl_version()
{
  glGetIntegerv(GL_MAJOR_VERSION, &gl_major);
  if (glGetError() == 0) {
    glGetIntegerv(GL_MINOR_VERSION, &gl_minor);
  } else {
    const GLubyte* gl_version = glGetString(GL_VERSION);
    if (gl_version != 0) {
      const std::string glversion((const char*)gl_version);
      const int end_major = glversion.find('.');
      const int end_minor = glversion.find_first_not_of("0123456789", end_major+1);
      gl_major = std::atoi(glversion.substr(0, end_major).c_str());
      gl_minor = std::atoi(glversion.substr(end_major+1, end_minor).c_str());
    }
  }
  METLIBS_LOG_DEBUG("OpenGL version major=" << gl_major << " minor=" << gl_minor);
}

bool have_opengl(int major, int minor)
{
  return gl_major > major || (gl_major == major && gl_minor >= minor);
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

    check_gl_version();
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
    // glWindowPos requires OpenGL 1.4; we only allocate "buffer_data" if we have this
    glWindowPos2i(0, 0);

    glpainter->PixelZoom(1, 1);
    glpainter->PixelStorei(DiGLPainter::gl_UNPACK_SKIP_ROWS, 0);
    glpainter->PixelStorei(DiGLPainter::gl_UNPACK_SKIP_PIXELS, 0);
    glpainter->PixelStorei(DiGLPainter::gl_UNPACK_ROW_LENGTH, width());
    glpainter->PixelStorei(DiGLPainter::gl_UNPACK_ALIGNMENT, 4);
    glpainter->DrawPixels(width(), height(), DiGLPainter::gl_RGBA, DiGLPainter::gl_UNSIGNED_BYTE, buffer_data);
    glpainter->PixelStorei(DiGLPainter::gl_UNPACK_ROW_LENGTH, 0);
    return;
  } else if (!paintable->enable_background_buffer)
    dropBackgroundBuffer();

  paintable->paintUnderlay(glpainter.get());

  if (have_opengl(1, 4) && glpainter->supportsReadPixels()
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
  delete[] buffer_data;
  buffer_data = 0;
}

void DiOpenGLWidget::resizeGL(int w, int h)
{
  if (paintable)
    paintable->resize(w, h);
  dropBackgroundBuffer();
  glpainter->Viewport(0, 0, (GLint)w, (GLint)h);
  update();
  setFocus();
}

void DiOpenGLWidget::keyPressEvent(QKeyEvent *ke)
{
  if (paintable && paintable->handleKeyEvents(ke))
    update();
}

void DiOpenGLWidget::keyReleaseEvent(QKeyEvent *ke)
{
  if (paintable && paintable->handleKeyEvents(ke))
    update();
}

void DiOpenGLWidget::mousePressEvent(QMouseEvent* me)
{
  if (paintable && paintable->handleMouseEvents(me))
    update();
}

void DiOpenGLWidget::mouseMoveEvent(QMouseEvent* me)
{
  if (paintable && paintable->handleMouseEvents(me))
    update();
}

void DiOpenGLWidget::mouseReleaseEvent(QMouseEvent* me)
{
  if (paintable && paintable->handleMouseEvents(me))
    update();
}

void DiOpenGLWidget::mouseDoubleClickEvent(QMouseEvent* me)
{
  if (paintable && paintable->handleMouseEvents(me))
    update();
}

void DiOpenGLWidget::wheelEvent(QWheelEvent *we)
{
  if (paintable && paintable->handleWheelEvents(we))
    update();
}
