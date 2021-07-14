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

#ifndef DIOPENGLPAINTER_H
#define DIOPENGLPAINTER_H 1

#include "diGLPainter.h"

#include <memory>

class FontManager;

class QGLWidget;

class DiOpenGLCanvas : public DiGLCanvas {
public:
  DiOpenGLCanvas(QGLWidget* widget);
  ~DiOpenGLCanvas();

  void parseFontSetup(const std::vector<std::string>& sect_fonts) Q_DECL_OVERRIDE;
  void defineFont(const std::string& fontfam, const std::string& fontfilename, diutil::FontFace face, bool use_bitmap) Q_DECL_OVERRIDE;

  void setVpGlSize(int vpw, int vph, float glw, float glh) override;
  bool setFontSize(float size) override;
  bool getTextRect(const QString& text, float& x, float& y, float& w, float& h) override;

  inline FontManager* fp() { return fp_.get(); }

  void DeleteLists(GLuint list, GLsizei range) Q_DECL_OVERRIDE;
  GLuint GenLists(GLsizei range) Q_DECL_OVERRIDE;
  GLboolean IsList(GLuint list) Q_DECL_OVERRIDE;
  bool supportsDrawLists() const Q_DECL_OVERRIDE;

  QImage convertToGLFormat(const QImage& i) Q_DECL_OVERRIDE;

protected:
  bool selectFont(const std::string& family) override;
  bool selectFont(const std::string& family, diutil::FontFace face, float size) override;
  bool hasFont(const std::string& family) override;

private:
  void applyVpGlSize();

private:
  std::unique_ptr<FontManager> fp_;
  int vpw_, vph_;
  float glw_, glh_;
  QGLWidget* mWidget;
};

class DiOpenGLPainter : public DiGLPainter {
public:
  DiOpenGLPainter(DiOpenGLCanvas* canvas);

  void Begin(GLenum mode) override;
  void End() override;
  void RasterPos2f(GLfloat x, GLfloat y) override;
  void Vertex2f(GLfloat x, GLfloat y) override;
  void Vertex3f(GLfloat x, GLfloat y, GLfloat z) override;
  void Vertex3i(GLint x, GLint y, GLint z) override;
  void BlendFunc(GLenum sfactor, GLenum dfactor) override;
  void ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) override;
  void Clear(GLbitfield mask) override;
  void ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) override;
  void Disable(GLenum cap) override;
  void EdgeFlag(GLboolean flag) override;
  void Enable(GLenum cap) override;
  void Flush() override;
  GLboolean IsEnabled(GLenum cap) override;
  void LineStipple(GLint factor, GLushort pattern) override;
  void LineWidth(GLfloat width) override;
  void PolygonMode(GLenum face, GLenum mode) override;
  void PolygonStipple(const GLubyte *mask) override;
  void PopAttrib() override;
  void PushAttrib(GLbitfield mask) override;
  void Ortho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
             GLdouble near_val, GLdouble far_val) override;
  void LoadIdentity() override;
  void PopMatrix() override;
  void PushMatrix() override;
  void Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) override;
  void Scalef(GLfloat x, GLfloat y, GLfloat z) override;
  void Translatef(GLfloat x, GLfloat y, GLfloat z) override;
  void Viewport(GLint x, GLint y, GLsizei width, GLsizei height) override;
  void CallList(GLuint list) override;
  void EndList() override;
  void NewList(GLuint list, GLenum mode) override;
  void ShadeModel(GLenum mode) override;
  void DrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type,
      const GLvoid *pixels) override;
  void PixelStorei(GLenum pname, GLint param) override;
  void PixelTransferf(GLenum pname, GLfloat param) override;
  void PixelZoom(GLfloat xfactor, GLfloat yfactor) override;
  void ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
      GLenum format, GLenum type, GLvoid *pixels) override;
  void DepthMask(GLboolean flag) override;
  void ClearStencil(GLint s) override;
  void StencilFunc(GLenum func, GLint ref, GLuint mask) override;
  void StencilOp(GLenum fail, GLenum zfail, GLenum zpass) override;
  bool supportsReadPixels() const override
    { return true; }

  // ========================================

  Colour getColour() override;

  // ========================================

  void setColour(const Colour& c, bool alpha = true) override;
  bool drawText(const QString& text, const QPointF& xy, float angle) override;
  void drawPolygon(const QPolygonF& points) override;
  void drawPolygons(const QList<QPolygonF>& polygons) override;

  void drawScreenImage(const QPointF&, const QImage&) override;
};

#endif // DIOPENGLPAINTER_H
