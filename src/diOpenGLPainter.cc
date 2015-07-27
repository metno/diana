
#include "diOpenGLPainter.h"

#include "diColour.h"
#include "diFontManager.h"
#include "diLinetype.h"
#include "diTesselation.h"

#include <QGLWidget>
#include <QGLContext>
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

GLuint DiOpenGLCanvas::bindTexture(const QImage& image)
{
  return mWidget->bindTexture(image);
}

void DiOpenGLCanvas::deleteTexture(GLuint texid)
{
  mWidget->deleteTexture(texid);
}

void DiOpenGLCanvas::initializeFP()
{
  METLIBS_LOG_SCOPE();
  mFP.reset(new FontManager);
  mFP->parseSetup();
  mFP->setFont("BITMAPFONT");
  mFP->setFontFace(glText::F_NORMAL);
  mFP->setScalingType(glText::S_FIXEDSIZE);
}

void DiOpenGLCanvas::setVpGlSize(float vpw, float vph, float glw, float glh)
{
  METLIBS_LOG_SCOPE(LOGVAL(vpw) << LOGVAL(vpw) << LOGVAL(glw) << LOGVAL(glw));
  fp()->setVpSize(vpw, vph);
  fp()->setGlSize(glw, glh);
}

bool DiOpenGLCanvas::setFont(const std::string& font)
{
  return fp()->setFont(font.c_str());
}

bool DiOpenGLCanvas::setFont(const std::string& font, float size, FontFace face)
{
  // assume that DiPainter::FontFace == glText::FontFace
  return fp()->set(font.c_str(), (glText::FontFace)face, size);
}

bool DiOpenGLCanvas::setFontSize(float size)
{
  return fp()->setFontSize(size);
}

bool DiOpenGLCanvas::getCharSize(char ch, float& w, float& h)
{
  return fp()->getCharSize(ch, w, h);
}

bool DiOpenGLCanvas::getTextSize(const std::string& text, float& w, float& h)
{
  return fp()->getStringSize(text.c_str(), w, h);
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

bool DiOpenGLPainter::drawText(const std::string& text, float x, float y, float angle)
{
  DiOpenGLCanvas* c = (DiOpenGLCanvas*) canvas();
  return c->fp()->drawStr(text.c_str(), x, y, angle);
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

  beginTesselation();
  tesselation(gldata, npoly, countpos);
  endTesselation();

  delete[] gldata;
  delete[] countpos;
}

void DiOpenGLPainter::drawReprojectedImage(const QImage& image, const float* mapPositionsXY, bool smooth)
{
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glShadeModel(smooth ? GL_SMOOTH : GL_FLAT);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  const GLuint width = image.width(), height = image.height(),
      size = width * height, size1 = (width+1) * (height+1);

  glVertexPointer(2, GL_FLOAT, 0, mapPositionsXY);
  glEnableClientState(GL_VERTEX_ARRAY);

  boost::shared_array<GLubyte> big(new GLubyte[4*size1]);
  size_t ib = 0;
  for (size_t iy=0; iy<height; ++iy) {
    for (size_t ix=0; ix<width; ++ix) {
      const QRgb p = image.pixel(ix, iy);
      big[ib++] = qRed(p);
      big[ib++] = qGreen(p);
      big[ib++] = qBlue(p);
      big[ib++] = qAlpha(p);
    }
    // transparent pixel at end of line
    for (size_t i=0; i<4; ++i)
      big[ib++] = 0;
  }
  for (size_t ix=0; ix<=width; ++ix) {
    for (size_t i=0; i<4; ++i)
      big[ib++] = 0;
  }
  glColorPointer(4, GL_UNSIGNED_BYTE, 0, big.get());
  glEnableClientState(GL_COLOR_ARRAY);

  QVector<GLuint> indices;
  indices.reserve(4*size);
  for (GLuint iy=0; iy < height; ++iy) {
    for (GLuint ix=0; ix < width; ++ix) {
      const QRgb p = image.pixel(ix, iy);
      if (qAlpha(p) == 0)
        continue;
      const GLuint i00 = (width+1) * iy + ix, i01 = i00 + (width+1), i10 = i00 + 1, i11 = i01 + 1;
      indices.append(i10);
      indices.append(i11);
      indices.append(i01);
      indices.append(i00);
    }
  }

  glDrawElements(GL_QUADS, indices.size(), GL_UNSIGNED_INT, indices.constData());

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
}
