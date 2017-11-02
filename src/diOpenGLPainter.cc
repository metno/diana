
#define GL_GLEXT_PROTOTYPES // needed for glWindowPos2i, which is OpenGL 1.4
#include "diOpenGLPainter.h"

#include "diColour.h"
#include "diFontManager.h"
#include "diLinetype.h"
#include "diTesselation.h"

#include <QGLWidget> // for QGlWidget::convertToGLFormat
#include <QImage>
#include <QPolygonF>
#include <QVector2D>

#include <boost/shared_array.hpp>

#include <cassert>

#define MILOGGER_CATEGORY "diana.DiOpenGLPainter"
#include <miLogger/miLogging.h>

DiOpenGLCanvas::DiOpenGLCanvas(QGLWidget* widget)
  : mWidget(widget)
{
}

DiOpenGLCanvas::~DiOpenGLCanvas()
{
}

GLuint DiOpenGLCanvas::bindTexture(const QImage& image)
{
  return mWidget->bindTexture(image);
}

void DiOpenGLCanvas::deleteTexture(GLuint texid)
{
  mWidget->deleteTexture(texid);
}

void DiOpenGLCanvas::parseFontSetup(const std::vector<std::string>& sect_fonts)
{
  METLIBS_LOG_SCOPE();
  fp()->clearFamilies();
  DiGLCanvas::parseFontSetup(sect_fonts);
}

void DiOpenGLCanvas::defineFont(const std::string& fontfam, const std::string& fontfilename,
    const std::string& face, bool use_bitmap)
{
  METLIBS_LOG_SCOPE(LOGVAL(fontfam) << LOGVAL(fontfilename));
  fp()->defineFont(fontfam, fontfilename, face, use_bitmap);
}

void DiOpenGLCanvas::initializeFP()
{
  METLIBS_LOG_SCOPE();
  mFP.reset(new FontManager);
}

void DiOpenGLCanvas::setVpGlSize(int vpw, int vph, float glw, float glh)
{
  METLIBS_LOG_SCOPE(LOGVAL(vpw) << LOGVAL(vph) << LOGVAL(glw) << LOGVAL(glh));
  fp()->setVpSize(vpw, vph);
  fp()->setGlSize(glw, glh);
}

bool DiOpenGLCanvas::setFont(const std::string& font)
{
  return fp()->setFont(lookupFontAlias(font));
}

bool DiOpenGLCanvas::setFont(const std::string& font, float size, FontFace face)
{
  // assume that DiPainter::FontFace == glText::FontFace
  return fp()->set(lookupFontAlias(font), (FontFamily::FontFace)face, size);
}

bool DiOpenGLCanvas::setFontSize(float size)
{
  return fp()->setFontSize(size);
}

bool DiOpenGLCanvas::getTextRect(const QString& text, float& x, float& y, float& w, float& h)
{
  return fp()->getStringRect(text.toStdWString(), x, y, w, h);
}

void DiOpenGLCanvas::DeleteLists(GLuint list, GLsizei range)
{
  glDeleteLists(list, range);
}

GLuint DiOpenGLCanvas::GenLists(GLsizei range)
{
  return glGenLists(range);
}

GLboolean DiOpenGLCanvas::IsList(GLuint list)
{
  return glIsList(list);
}

bool DiOpenGLCanvas::supportsDrawLists() const
{
  return true;
}

QImage DiOpenGLCanvas::convertToGLFormat(const QImage& i)
{
  return QGLWidget::convertToGLFormat(i);
}

// ========================================================================

DiOpenGLPainter::DiOpenGLPainter(DiOpenGLCanvas* canvas)
  : DiGLPainter(canvas)
{
}

void DiOpenGLPainter::Begin(GLenum mode)
{ glBegin(mode); }

void DiOpenGLPainter::Color3d(GLdouble red, GLdouble green, GLdouble blue)
{ glColor3d(red, green, blue); }

void DiOpenGLPainter::Color3f(GLfloat red, GLfloat green, GLfloat blue)
{ glColor3f(red, green, blue); }

void DiOpenGLPainter::Color3fv(const GLfloat *v)
{ glColor3fv(v); }

void DiOpenGLPainter::Color3ub(GLubyte red, GLubyte green, GLubyte blue)
{ glColor3ub(red, green, blue); }

void DiOpenGLPainter::Color3ubv(const GLubyte *v)
{ glColor3ubv(v); }

void DiOpenGLPainter::Color4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{ glColor4d(red, green, blue, alpha); }

void DiOpenGLPainter::Color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{ glColor4f(red, green, blue, alpha); }

void DiOpenGLPainter::Color4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{ glColor4ub(red, green, blue, alpha); }

void DiOpenGLPainter::Color4fv(const GLfloat *v)
{ glColor4fv(v); }

void DiOpenGLPainter::Color4ubv(const GLubyte *v)
{ glColor4ubv(v); }

void DiOpenGLPainter::End()
{ glEnd(); }

void DiOpenGLPainter::Indexi(GLint c)
{ glIndexi(c); }

void DiOpenGLPainter::RasterPos2f(GLfloat x, GLfloat y)
{ glRasterPos2f(x, y); }

void DiOpenGLPainter::Rectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{ glRectf(x1, y1, x2, y2); }

void DiOpenGLPainter::TexCoord2f(GLfloat s, GLfloat t)
{ glTexCoord2f(s, t); }

void DiOpenGLPainter::Vertex2dv(const GLdouble *v)
{ glVertex2dv(v); }

void DiOpenGLPainter::Vertex2f(GLfloat x, GLfloat y)
{ glVertex2f(x, y); }

void DiOpenGLPainter::Vertex2i(GLint x, GLint y)
{ glVertex2i(x, y); }

void DiOpenGLPainter::Vertex3f(GLfloat x, GLfloat y, GLfloat z)
{ glVertex3f(x, y, z); }

void DiOpenGLPainter::Vertex3i(GLint x, GLint y, GLint z)
{ glVertex3i(x, y, z); }

void DiOpenGLPainter::DrawArrays(GLenum mode, GLint first, GLsizei count)
{ glDrawArrays(mode, first, count); }

void DiOpenGLPainter::VertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{ glVertexPointer(size, type, stride, ptr); }

void DiOpenGLPainter::BlendFunc(GLenum sfactor, GLenum dfactor)
{ glBlendFunc(sfactor, dfactor); }

void DiOpenGLPainter::ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{ glClearColor(red, green, blue, alpha); }

void DiOpenGLPainter::Clear(GLbitfield mask)
{ glClear(mask); }

void DiOpenGLPainter::ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{ glColorMask(red, green, blue, alpha); }

void DiOpenGLPainter::Disable(GLenum cap)
{ glDisable(cap); }

void DiOpenGLPainter::DisableClientState(GLenum cap)
{ /* 1.1 */ glDisableClientState(cap); /* 1.1 */ }

void DiOpenGLPainter::DrawBuffer(GLenum mode)
{ glDrawBuffer(mode); }

void DiOpenGLPainter::EdgeFlag(GLboolean flag)
{ glEdgeFlag(flag); }

void DiOpenGLPainter::Enable(GLenum cap)
{ glEnable(cap); }

void DiOpenGLPainter::EnableClientState(GLenum cap)
{ /* 1.1 */ glEnableClientState(cap); /* 1.1 */ }

void DiOpenGLPainter::Flush()
{ glFlush(); }

void DiOpenGLPainter::GetFloatv(GLenum pname, GLfloat *params)
{ glGetFloatv(pname, params); }

void DiOpenGLPainter::GetIntegerv(GLenum pname, GLint *params)
{ glGetIntegerv(pname, params); }

GLboolean DiOpenGLPainter::IsEnabled(GLenum cap)
{ return glIsEnabled(cap); }

void DiOpenGLPainter::LineStipple(GLint factor, GLushort pattern)
{ glLineStipple(factor, pattern); }

void DiOpenGLPainter::LineWidth(GLfloat width)
{ glLineWidth(width); }

void DiOpenGLPainter::PointSize(GLfloat size)
{ glPointSize(size); }

void DiOpenGLPainter::PolygonMode(GLenum face, GLenum mode)
{ glPolygonMode(face, mode); }

void DiOpenGLPainter::PolygonStipple(const GLubyte *mask)
{ glPolygonStipple(mask); }

void DiOpenGLPainter::PopAttrib()
{ glPopAttrib(); }

void DiOpenGLPainter::PushAttrib(GLbitfield mask)
{ glPushAttrib(mask); }

void DiOpenGLPainter::Ortho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
    GLdouble near_val, GLdouble far_val)
{ glOrtho(left, right, bottom, top, near_val, far_val); }

void DiOpenGLPainter::LoadIdentity()
{ glLoadIdentity(); }

void DiOpenGLPainter::PopMatrix()
{ glPopMatrix(); }

void DiOpenGLPainter::PushMatrix()
{ glPushMatrix(); }

void DiOpenGLPainter::Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{ glRotatef(angle, x, y, z); }

void DiOpenGLPainter::Scalef(GLfloat x, GLfloat y, GLfloat z)
{ glScalef(x, y, z); }

void DiOpenGLPainter::Translatef(GLfloat x, GLfloat y, GLfloat z)
{ glTranslatef(x, y, z); }

void DiOpenGLPainter::Viewport(GLint x, GLint y, GLsizei width, GLsizei height)
{ glViewport(x, y, width, height); }

void DiOpenGLPainter::CallList(GLuint list)
{ glCallList(list); }

void DiOpenGLPainter::EndList()
{ glEndList(); }

void DiOpenGLPainter::NewList(GLuint list, GLenum mode)
{ glNewList(list, mode); }

void DiOpenGLPainter::ShadeModel(GLenum mode)
{ glShadeModel(mode); }

void DiOpenGLPainter::Bitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig,
    GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{ glBitmap(width, height, xorig, yorig, xmove, ymove, bitmap); }

void DiOpenGLPainter::DrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{ glDrawPixels(width, height, format, type, pixels); }

void DiOpenGLPainter::PixelStorei(GLenum pname, GLint param)
{ glPixelStorei(pname, param); }

void DiOpenGLPainter::PixelTransferf(GLenum pname, GLfloat param)
{ glPixelTransferf(pname, param); }

void DiOpenGLPainter::PixelZoom(GLfloat xfactor, GLfloat yfactor)
{ glPixelZoom(xfactor, yfactor); }

void DiOpenGLPainter::ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
    GLenum format, GLenum type, GLvoid *pixels)
{ glReadPixels(x, y, width, height, format, type, pixels); }

void DiOpenGLPainter::DepthMask(GLboolean flag)
{ glDepthMask(flag); }

void DiOpenGLPainter::ClearStencil(GLint s)
{ glClearStencil(s); }

void DiOpenGLPainter::StencilFunc(GLenum func, GLint ref, GLuint mask)
{ glStencilFunc(func, ref, mask); }

void DiOpenGLPainter::StencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{ glStencilOp(fail, zfail, zpass); }

void DiOpenGLPainter::BindTexture(GLenum target, GLuint texture)
{ glBindTexture(target, texture); }

void DiOpenGLPainter::DeleteTextures(GLsizei n, const GLuint *textures)
{ glDeleteTextures(n, textures); }

void DiOpenGLPainter::GenTextures(GLsizei n, GLuint *textures)
{ glGenTextures(n, textures); }

void DiOpenGLPainter::TexEnvf(GLenum target, GLenum pname, GLfloat param)
{ glTexEnvf(target, pname, param); }

void DiOpenGLPainter::TexParameteri(GLenum target, GLenum pname, GLint param)
{ glTexParameteri(target, pname, param); }

void DiOpenGLPainter::TexImage2D(GLenum target, GLint level, GLint internalFormat,
    GLsizei width, GLsizei height, GLint border,
    GLenum format, GLenum type, const GLvoid *pixels)
{ glTexImage2D(target, level, internalFormat, width, height, border, format, type, pixels); }

// ========================================================================

bool DiOpenGLPainter::drawText(const QString& text, const QPointF& xy, float angle)
{
  DiOpenGLCanvas* c = (DiOpenGLCanvas*) canvas();
  return c->fp()->drawStr(text.toStdWString(), xy.x(), xy.y(), angle);
}

void DiOpenGLPainter::drawPolygon(const QPolygonF& points)
{
  QList<QPolygonF> p;
  p << points;
  drawPolygons(p);
}

void DiOpenGLPainter::drawPolygons(const QList<QPolygonF>& polygons)
{
  const int npoly = polygons.size();
  if (npoly == 0)
    return;

  int *countpos= new int[npoly];
  int npoints = 0;
  for (int i=0; i<npoly; ++i) {
    countpos[i] = polygons.at(i).size();
    npoints += countpos[i];
  }

  GLdouble* gldata = new GLdouble[3*npoints];
  int k=0;
  for (int i=0; i<npoly; ++i) {
    const QPolygonF& poly = polygons.at(i);
    for (int j=0; j<poly.size(); ++j) {
      gldata[k++] = poly.at(j).x();
      gldata[k++] = poly.at(j).y();
      gldata[k++] = 0;
    }
  }

  tesselation(gldata, npoly, countpos);

  delete[] gldata;
  delete[] countpos;
}

void DiOpenGLPainter::drawScreenImage(const QPointF& point, const QImage& image)
{
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glShadeModel(GL_FLAT);

  QImage glimage = QGLWidget::convertToGLFormat(image);
  glPixelZoom(1, 1);
  glWindowPos2f(point.x(), point.y()); // this is opengl 1.4
  glDrawPixels(glimage.width(), glimage.height(), GL_RGBA, gl_UNSIGNED_BYTE, glimage.bits());
}
