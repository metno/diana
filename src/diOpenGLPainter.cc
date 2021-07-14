/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2021 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

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
    : fp_(new FontManager)
    , vpw_(1)
    , vph_(1)
    , glw_(1)
    , glh_(1)
    , mWidget(widget)
{
}

DiOpenGLCanvas::~DiOpenGLCanvas()
{
}

void DiOpenGLCanvas::parseFontSetup(const std::vector<std::string>& sect_fonts)
{
  METLIBS_LOG_SCOPE();
  mWidget->makeCurrent();
  fp()->clearFamilies();
  DiGLCanvas::parseFontSetup(sect_fonts);
  applyVpGlSize();
  mWidget->doneCurrent();
}

void DiOpenGLCanvas::defineFont(const std::string& fontfam, const std::string& fontfilename, diutil::FontFace face, bool use_bitmap)
{
  METLIBS_LOG_SCOPE(LOGVAL(fontfam) << LOGVAL(fontfilename));
  fp()->defineFont(fontfam, fontfilename, face, use_bitmap);
}

void DiOpenGLCanvas::setVpGlSize(int vpw, int vph, float glw, float glh)
{
  vpw_ = vpw;
  vph_ = vph;
  glw_ = glw;
  glh_ = glh;

  applyVpGlSize();
}

void DiOpenGLCanvas::applyVpGlSize()
{
  fp()->setVpGlSize(vpw_, vph_, glw_, glh_);
}

bool DiOpenGLCanvas::selectFont(const std::string& family)
{
  return fp()->setFont(family);
}

bool DiOpenGLCanvas::selectFont(const std::string& family, diutil::FontFace face, float size)
{
  return fp()->set(family, face, size);
}

bool DiOpenGLCanvas::setFontSize(float size)
{
  return fp()->setFontSize(size);
}

bool DiOpenGLCanvas::hasFont(const std::string& family)
{
  return fp()->hasFont(family);
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

void DiOpenGLPainter::End()
{ glEnd(); }

void DiOpenGLPainter::RasterPos2f(GLfloat x, GLfloat y)
{ glRasterPos2f(x, y); }

void DiOpenGLPainter::Vertex2f(GLfloat x, GLfloat y)
{ glVertex2f(x, y); }

void DiOpenGLPainter::Vertex3f(GLfloat x, GLfloat y, GLfloat z)
{ glVertex3f(x, y, z); }

void DiOpenGLPainter::Vertex3i(GLint x, GLint y, GLint z)
{ glVertex3i(x, y, z); }

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

void DiOpenGLPainter::EdgeFlag(GLboolean flag)
{ glEdgeFlag(flag); }

void DiOpenGLPainter::Enable(GLenum cap)
{ glEnable(cap); }

void DiOpenGLPainter::Flush()
{ glFlush(); }

GLboolean DiOpenGLPainter::IsEnabled(GLenum cap)
{ return glIsEnabled(cap); }

void DiOpenGLPainter::LineStipple(GLint factor, GLushort pattern)
{ glLineStipple(factor, pattern); }

void DiOpenGLPainter::LineWidth(GLfloat width)
{ glLineWidth(width); }

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

// ========================================================================

Colour DiOpenGLPainter::getColour()
{
  DiGLPainter::GLfloat c[4];
  glGetFloatv(gl_CURRENT_COLOR, c);
  return Colour::fromF(c[0], c[1], c[2], c[3]);
}

// ========================================================================

void DiOpenGLPainter::setColour(const Colour& c, bool alpha)
{
  glColor4ub(c.R(), c.G(), c.B(), alpha ? c.A() : 255);
}

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
