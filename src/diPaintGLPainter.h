/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2011-2022 met.no

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

#ifndef PAINTGLPAINTER_H
#define PAINTGLPAINTER_H

#include "diGLPainter.h"

#include <QColor>
#include <QHash>
#include <QList>
#include <QPainter>
#include <QPainterPath>
#include <QPair>
#include <QPen>
#include <QPointF>
#include <QStack>
#include <QTransform>
#include <QVector>
#include <QWidget>

#include <map>
#include <set>

class QPaintDevice;

struct PaintAttributes {
  QRgb color;
  float width;
  QHash<DiGLPainter::GLenum,DiGLPainter::GLenum> polygonMode;
  QVector<qreal> dashes;
  QImage mask;
  qreal dashOffset;
  bool lineStipple;
  bool polygonStipple;
  bool antialiasing;
  QColor bias;
  bool biased;
  QColor scale;
  bool scaled;
  QPointF pixelZoom;
};

struct StencilAttributes {
  DiGLPainter::GLint clear;
  DiGLPainter::GLenum func;
  DiGLPainter::GLint ref;
  DiGLPainter::GLuint mask;
  DiGLPainter::GLenum fail;
  DiGLPainter::GLenum zfail;
  DiGLPainter::GLenum zpass;
  QPainterPath path;

  bool clip;
  bool update;
  bool enabled;
};

struct RenderItem {
  DiGLPainter::GLenum mode;
  QPainter *painter;
  QTransform transform;
  DiGLPainter::GLuint list;
};

class DiPaintGLCanvas : public DiGLCanvas
{
public:
  enum FontScaling {
    S_FIXEDSIZE, S_VIEWPORTSCALED
  };
  enum {
    MAXFONTFACES = 4, MAXFONTS = 10, MAXFONTSIZES = 40
  };

  DiPaintGLCanvas(QPaintDevice* device);
  ~DiPaintGLCanvas();

  void setVpGlSize(int vpw, int vph, float glw, float glh) override;

  bool setFontSize(float size) override;

  bool getTextRect(const QString& str, float& x, float& y, float& w, float& h) override;

  QImage convertToGLFormat(const QImage& i) override;

  float fontScaleX() const
    { return mFontScaleX; }

  float fontScaleY() const
    { return mFontScaleY; }

  const QFont& font() const
    { return mFont; }

  bool fontValid() const { return mFontValid; }

  QPaintDevice* device() const
    { return mDevice; }

  using DiGLCanvas::parseFontSetup;
  void parseFontSetup(const std::vector<std::string>& sect_fonts) Q_DECL_OVERRIDE;

  void defineFont(const std::string& fontfam, const std::string& fontfilename, diutil::FontFace face, bool use_bitmap) override;

protected:
  bool selectFont(const std::string& family, diutil::FontFace face, float size) override;
  bool selectFont(const std::string& family) override;
  bool hasFont(const std::string& family) override;

private:
  bool setFontFace(diutil::FontFace face);

private:
  QPaintDevice* mDevice;
  QFont mFont;
  bool mFontValid;

  float mFontScaleX, mFontScaleY;
  std::map<std::string, QString> fontMap;
  QList<int> fontHandles;
};

class DiPaintGLPainter : public DiGLPainter
{
public:
  DiPaintGLPainter(DiPaintGLCanvas* canvas);
  ~DiPaintGLPainter();

  // begin DiGLPainter interface
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
  void Flush(void) override;
  GLboolean IsEnabled(GLenum cap) override;
  void LineStipple(GLint factor, GLushort pattern) override;
  void LineWidth(GLfloat width) override;
  void PolygonMode(GLenum face, GLenum mode) override;
  void PolygonStipple(const GLubyte *mask) override;
  void PopAttrib(void) override;
  void PushAttrib(GLbitfield mask) override;
  void Ortho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
      GLdouble near_val, GLdouble far_val) override;
  void LoadIdentity(void) override;
  void PopMatrix(void) override;
  void PushMatrix(void) override;
  void Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) override;
  void Scalef(GLfloat x, GLfloat y, GLfloat z) override;
  void Translatef(GLfloat x, GLfloat y, GLfloat z) override;
  void Viewport(GLint x, GLint y, GLsizei width, GLsizei height) override;
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
    { return false; }
  // end DiGLPainter interface

  Colour getColour() override;

  // begin DiPainter interface
  void setColour(const Colour& c, bool alpha = true) override;
  bool drawText(const QString& text, const QPointF& xy, float angle = 0) override;
  void drawCircle(bool fill, float centerx, float centery, float radius) override;
  void drawRect(bool fill, float x1, float y1, float x2, float y2) override;
  void drawLine(float x1, float y1, float x2, float y2) override;
  void drawPolyline(const QPolygonF& points) override;
  void drawPolygon(const QPolygonF& points) override;
  void drawPolygons(const QList<QPolygonF>& polygons) override;
  // end DiPainter interface

  void drawScreenImage(const QPointF& point, const QImage& image) override;

  void begin(QPainter *painter);
  bool isPainting() const;
  void end();

  void renderPrimitive();
  void setViewportTransform();

  void setClipPath();
  void unsetClipPath();

  bool HIGH_QUALITY_BUT_SLOW;

  QPainter *painter;

  QStack<GLenum> stack;
  QStack<QTransform> transformStack;
  QStack<PaintAttributes> attributesStack;

  bool blend;
  QPainter::CompositionMode blendMode;
  bool smooth;

  QPointF rasterPos;
  QHash<GLenum,GLint> pixelStore;

  QColor clearColor;
  bool clear;
  bool colorMask;

  GLenum mode;
  PaintAttributes attributes;
  StencilAttributes stencil;
  QPolygonF points;
  QList<bool> validPoints;
  QList<QRgb> colors;
  QTransform transform;

  QRect viewport;
  QRectF window;

private:
  void plotSubdivided(const QPointF quad[], const QRgb color[], int divisions = 0);
  void setPen();
  void setPolygonColor(const QRgb &color);
  void makeCurrent();
  void setFillMode(bool fill);

  void drawReprojectedSubImage(const QImage& image, const QPolygonF& mapPositions,
      const diutil::Rect& imagepart);
};

#endif // PAINTGLPAINTER_H
