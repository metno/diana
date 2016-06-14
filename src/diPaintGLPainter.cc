/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2011 met.no

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

#include "diPaintGLPainter.h"

#include "diGlUtilities.h"
#include "diLocalSetupParser.h"
#include "miSetupParser.h"

#include <QtGui>

#include <cmath>

#define MILOGGER_CATEGORY "diana.DiPaintGLPainter"
#include <miLogger/miLogging.h>

#if 0
#define IFDEBUG(x) x
#else
#define IFDEBUG(x) do { } while (false)
#endif

namespace {

const int TEXTURE_CACHE_SIZE = 16;

} // namespace

DiPaintGLCanvas::DiPaintGLCanvas(QPaintDevice* device)
  : mDevice(device)
  , mFont(QFont(), mDevice)
  , mFontScaleX(1)
  , mFontScaleY(1)
{
  METLIBS_LOG_SCOPE();
}

DiPaintGLCanvas::~DiPaintGLCanvas()
{
}

bool DiPaintGLCanvas::setFont(const std::string& name, const float size,
    const FontFace face)
{
  setFont(name);
  setFontFace(face);
  setFontSize(size);
  return true;
}

bool DiPaintGLCanvas::setFont(const std::string& name)
{
  METLIBS_LOG_SCOPE(LOGVAL(name));

  const std::string family = lookupFontAlias(name);
  QHash<QString,QString>::const_iterator it = fontMap.constFind(QString::fromStdString(family));
  if (it == fontMap.constEnd())
    return false;

  mFont.setFamily(it.value());
  mFont.setStyleStrategy(QFont::NoFontMerging);
  return true;
}

bool DiPaintGLCanvas::setFontFace(FontFace face)
{
  if (face & 1)
    mFont.setWeight(QFont::Bold);
  else
    mFont.setWeight(QFont::Normal);
  mFont.setItalic((face & 2) != 0);
  return true;
}

bool DiPaintGLCanvas::setFontSize(const float size)
{
  mFont.setPointSizeF(size);
  return true;
}

void DiPaintGLCanvas::setVpGlSize(float vpw, float vph, float glw, float glh)
{
  mFontScaleX = glw / vpw;
  mFontScaleY = glh / vph;
}

void DiPaintGLCanvas::defineFont(const std::string& fontfam, const std::string& fontfilename,
    const std::string& face, bool use_bitmap)
{
  METLIBS_LOG_SCOPE(LOGVAL(fontfam) << LOGVAL(fontfilename) << LOGVAL(face));

  int handle = QFontDatabase::addApplicationFont(QString::fromStdString(fontfilename));
  if (handle == -1)
    return;

  QStringList families = QFontDatabase::applicationFontFamilies(handle);
  if (families.isEmpty())
    return;

  const QString qfont = QString::fromStdString(fontfam);
  Q_FOREACH(const QString& family, families) {
    fontMap[qfont] = family;
  }
}

bool DiPaintGLCanvas::getTextRect(const QString& str, float& x, float& y, float& w, float& h)
{
  if (str.length() == 0) {
    x = y = w = h = 0;
    return false;
  }
  QFontMetricsF fm(mFont, mDevice);
  QRectF rect = fm.tightBoundingRect(str);
  x = rect.x() * mFontScaleX;
  y = -rect.bottom() * mFontScaleY;
  w = rect.width() * mFontScaleX;
  h = rect.height() * mFontScaleY;
  if (w == 0 || str.trimmed().isEmpty())
    h = 0;

  return true;
}

QImage DiPaintGLCanvas::convertToGLFormat(const QImage& i)
{
  return i.transformed(QTransform().scale(1, -1)).rgbSwapped();
}

// ========================================================================

DiPaintGLPainter::DiPaintGLPainter(DiPaintGLCanvas* canvas)
  : DiGLPainter(canvas)
  , HIGH_QUALITY_BUT_SLOW(true)
  , painter(0)
  , clear(true)
{
  makeCurrent();
}

DiPaintGLPainter::~DiPaintGLPainter()
{
}

void DiPaintGLPainter::makeCurrent()
{
  useTexture = false;
  blend = false;
  blendMode = QPainter::CompositionMode_Source;
  pointSize = 1.0;

  clearColor = QColor(0, 0, 0, 0);
  colorMask = true;

  stencil.clear = 0;
  stencil.path = QPainterPath();
  stencil.fail = gl_KEEP;
  stencil.zfail = gl_KEEP;
  stencil.zpass = gl_KEEP;
  stencil.func = gl_ALWAYS;
  stencil.ref = 0;
  stencil.mask = ~0x0;
  stencil.clip = false;
  stencil.update = false;
  stencil.enabled = false;

  attributes.color = qRgba(255, 255, 255, 255);
  attributes.width = 1.0;
  attributes.polygonMode[gl_FRONT] = gl_FILL;
  attributes.polygonMode[gl_BACK] = gl_FILL;
  attributes.lineStipple = false;
  attributes.polygonStipple = false;
  attributes.antialiasing = false;
  attributes.bias = QColor(0, 0, 0, 255);
  attributes.biased = false;
  attributes.scale = QColor(255, 255, 255, 255);
  attributes.scaled = false;
  attributes.pixelZoom = QPointF(1, 1);

  points.clear();
  validPoints.clear();
  colors.clear();

  stack.clear();
  renderStack.clear();
  transformStack.clear();
  transform = QTransform();
  attributesStack.clear();

  clientState = 0;
}

void DiPaintGLPainter::begin(QPainter *painter)
{
  // Use the painter supplied.
  this->painter = painter;

  if (clear && colorMask) {
    painter->fillRect(0, 0, painter->device()->width(), painter->device()->height(), clearColor);
    clear = false;
  }

  // Start painting with the preset anti-aliasing attribute.
  painter->setRenderHint(QPainter::Antialiasing, false);
}

bool DiPaintGLPainter::isPainting() const
{
  return painter != 0;
}

void DiPaintGLPainter::end()
{
  painter = 0;
}

void DiPaintGLPainter::setPen()
{
  qreal width = attributes.width;
  if (isPrinting())
    width /= 3.0;

  QPen pen = QPen(QColor::fromRgba(attributes.color), width);
  pen.setCapStyle(Qt::FlatCap);
  pen.setCosmetic(true);
  if (attributes.lineStipple) {
    if (attributes.dashes.size() > 1) {
      /* Set the dash pattern on the pen if defined, adjusting
         the length of each element to compensate for the line
         width. */
      QVector<qreal> dashes;
      if (width == 0) {
        if (isPrinting())
          width = 0.5;
        else
          width = 1;
      }

      for (int i = 0; i < attributes.dashes.size(); ++i)
        dashes << attributes.dashes[i]/width;

      pen.setDashPattern(dashes);
      pen.setDashOffset(attributes.dashOffset/width);
    } else if (attributes.dashes.size() == 1) {
      if (!attributes.dashes[0])
        pen.setStyle(Qt::NoPen);
    }
  }

  painter->setPen(pen);
}

void DiPaintGLPainter::setPolygonColor(const QRgb &color)
{
  switch (attributes.polygonMode[gl_FRONT]) {
  case gl_FILL:
    painter->setPen(Qt::NoPen);
    if (attributes.polygonStipple && !attributes.mask.isNull()) {
      QVector<QRgb> colours;
      colours << qRgba(0, 0, 0, 0) << attributes.color;
      attributes.mask.setColorTable(colours);
      painter->setBrush(attributes.mask);
      painter->setBrushOrigin(viewport.bottomLeft()); // is this correct?
    } else
      painter->setBrush(QColor::fromRgba(color));
    break;
  case gl_LINE: {
    setPen();
    painter->setBrush(Qt::NoBrush);
    break;
  }
  default:
    break;
  }

  if (blend)
    painter->setCompositionMode(blendMode);
  else
    painter->setCompositionMode(QPainter::CompositionMode_Source);
}

#define COLOR_BLEND2(a, b) qRgba((qRed(a) + qRed(b))/2.0, \
                                 (qGreen(a) + qGreen(b))/2.0, \
                                 (qBlue(a) + qBlue(b))/2.0, \
                                 (qAlpha(a) + qAlpha(b))/2.0)

#define COLOR_BLEND4(a, b, c, d) \
    qRgba((qRed(a) + qRed(b) + qRed(c) + qRed(d))/4.0, \
          (qGreen(a) + qGreen(b) + qGreen(c) + qGreen(d))/4.0, \
          (qBlue(a) + qBlue(b) + qBlue(c) + qBlue(d))/4.0, \
          (qAlpha(a) + qAlpha(b) + qAlpha(c) + qAlpha(d))/4.0)

void DiPaintGLPainter::plotSubdivided(const QPointF quad[], const QRgb color[], int divisions)
{
  if (divisions > 0) {
    QPointF center = (quad[0] + quad[1] + quad[2] + quad[3])/4.0;
    QRgb centerColor = COLOR_BLEND4(color[0], color[1], color[2], color[3]);

    for (int j = 0; j < 4; ++j) {
      QPointF newQuad[4];
      newQuad[0] = quad[j];
      newQuad[1] = (quad[j] + quad[(j + 1) % 4])/2.0;
      newQuad[2] = center;
      newQuad[3] = (quad[j] + quad[(j + 3) % 4])/2.0;

      QRgb newColor[4];
      newColor[0] = color[j];
      newColor[1] = COLOR_BLEND2(color[j], color[(j + 1) % 4]);
      newColor[2] = centerColor;
      newColor[3] = COLOR_BLEND2(color[j], color[(j + 3) % 4]);

      plotSubdivided(newQuad, newColor, divisions - 1);
    }

  } else {
    painter->setBrush(QColor((qRed(color[0]) + qRed(color[1]) + qRed(color[2]) + qRed(color[3]))/4,
            (qGreen(color[0]) + qGreen(color[1]) + qGreen(color[2]) + qGreen(color[3]))/4,
            (qBlue(color[0]) + qBlue(color[1]) + qBlue(color[2]) + qBlue(color[3]))/4,
            (qAlpha(color[0]) + qAlpha(color[1]) + qAlpha(color[2]) + qAlpha(color[3]))/4));
    painter->drawConvexPolygon(quad, 4);
  }
}

void DiPaintGLPainter::renderPrimitive()
{
  if (points.size() == 0)
    return;

  static QPointF poly[4];
  static QRgb color[4];

  switch (mode) {
  case gl_POINTS:
    setPen();
    if (colorMask) {
      for (int i = 0; i < points.size(); ++i) {
        painter->drawPoint(points.at(i));
      }
    }
    break;
  case gl_LINES:
    setPen();
    painter->setRenderHint(QPainter::Antialiasing, attributes.antialiasing);

    if (colorMask) {
      for (int i = 0; i < points.size() - 1; i += 2) {
        painter->drawLine(points.at(i), points.at(i+1));
      }
    }
    break;
  case gl_LINE_LOOP:
    setPen();
    painter->setRenderHint(QPainter::Antialiasing, attributes.antialiasing);
    points.append(points.at(0));
    if (colorMask)
      painter->drawPolyline(points);
    break;
  case gl_LINE_STRIP: {
    setPen();
    painter->setRenderHint(QPainter::Antialiasing, attributes.antialiasing);
    if (colorMask)
      painter->drawPolyline(points);
    break;
  }
  case gl_TRIANGLES: {
    setPolygonColor(attributes.color);
    painter->setRenderHint(QPainter::Antialiasing, attributes.antialiasing);

    for (int i = 0; i < points.size() - 2; i += 3) {
      if (validPoints.at(i) && validPoints.at(i + 1) && validPoints.at(i + 2)) {
        for (int j = 0; j < 3; ++j)
          poly[j] = points.at(i + j);
        if (colorMask)
          painter->drawConvexPolygon(poly, 3);
      }
    }
    break;
  }
  case gl_TRIANGLE_STRIP: {
    setPolygonColor(attributes.color);

    poly[0] = points.at(0);
    poly[1] = points.at(1);
    for (int i = 2; i < points.size(); ++i) {
      poly[i % 3] = points.at(i);
      if (colorMask)
        painter->drawConvexPolygon(poly, 3);
    }
    break;
  }
  case gl_TRIANGLE_FAN: {
    setPolygonColor(attributes.color);

    poly[0] = points.at(0);
    poly[1] = points.at(1);
    for (int i = 2; i < points.size(); ++i) {
      poly[2 - (i % 2)] = points.at(i);
      if (colorMask)
        painter->drawConvexPolygon(poly, 3);
    }
    break;
  }
  case gl_QUADS: {
    // Optimisation: Diana only ever draws filled, blended quads without edges.
    if (blend) {
      painter->setPen(Qt::NoPen);
      painter->setCompositionMode(blendMode);
    } else
      setPolygonColor(attributes.color);

    QPolygonF quad(4);
    int ok = 0;

    for (int i = 0; i < points.size(); ++i) {
      if (!validPoints.at(i)) {
        ok = 0;
        continue;
      }

      quad[i % 4] = points.at(i);
      ok++;

      if (i % 4 != 3)     // Loop again for the first three points.
        continue;
      else if (ok != 4) {
        ok = 0;         // If not enough points then reset the
        continue;       // counter and examine the next quad.
      }

      if (colorMask) {
        if (useTexture) {
          QImage texture = textures[currentTexture];
          QTransform t;
          QPolygonF source(QRectF(texture.rect()));
          if (QTransform::quadToQuad(source, quad, t)) {
            painter->save();
            // No need to record this transformation.
            painter->setTransform(transform * t);
            painter->drawImage(0, 0, texture);
            painter->restore();
          }
        } else {
          if (blend) {
            // Optimisation: Diana only ever draws filled, blended quads without edges.
            int j = (i / 4) * 4;
            painter->setBrush(QColor::fromRgba(colors.at(j)));
          }
          painter->drawConvexPolygon(quad);
        }
      }

      ok = 0;
    }
    break;
  }
  case gl_QUAD_STRIP: {
    if (!blend)
      setPolygonColor(attributes.color);
    else
      painter->setPen(Qt::NoPen);

    poly[0] = points.at(0);
    poly[1] = points.at(1);
    color[0] = colors.at(0);
    color[1] = colors.at(1);

    int j, k;
    for (int i = 2; i < points.size() - 1; i += 2) {

      if (i % 4 == 2) {
        j = i + 1;
        k = i;
      } else {
        j = i;
        k = i + 1;
      }

      poly[i % 4] = points.at(j);
      poly[i % 4 + 1] = points.at(k);

      if (blend && smooth) {
        color[i % 4] = colors.at(j);
        color[i % 4 + 1] = colors.at(k);
      }

      if (colorMask) {
        if (validPoints.at(i - 2) && validPoints.at(i - 1) && validPoints.at(i) && validPoints.at(i + 1)) {
          if (blend) {
            painter->setCompositionMode(blendMode);
            if (smooth) {
              plotSubdivided(poly, color);
            } else {
              setPolygonColor(colors.at(i));
              painter->drawConvexPolygon(poly, 4);
            }
          } else
            painter->drawConvexPolygon(poly, 4);
        }
      }
    }
    break;
  }
  case gl_POLYGON: {
    setPolygonColor(attributes.color);
    painter->setRenderHint(QPainter::Antialiasing, attributes.antialiasing);

    QPolygonF poly;
    for (int i = 0; i < points.size(); ++i) {
      if (validPoints.at(i))
        poly.append(points.at(i));
    }
    if (colorMask)
      painter->drawPolygon(poly);
    else {
      QPainterPath newPath;
      newPath.addPolygon(poly);
      newPath.closeSubpath();

      newPath = newPath.united(newPath.translated(-attributes.width, -attributes.width));
      stencil.path += newPath.translated(0.5*attributes.width, 0.5*attributes.width);
    }

    break;
  }
  default:
    break;
  }

  points.clear();
  validPoints.clear();
  colors.clear();

  // Turn off anti-aliasing if it was enabled.
  if (attributes.antialiasing)
    painter->setRenderHint(QPainter::Antialiasing, false);
}

void DiPaintGLPainter::setViewportTransform()
{
  QTransform t;
  t = transform.translate(viewport.left(), viewport.top());
  t = t.scale(viewport.width()/window.width(), viewport.height()/window.height());
  t = t.translate(-window.left(), -window.top());
  transform = t;
}

void DiPaintGLPainter::setClipPath()
{
  if (stencil.enabled && stencil.clip && !stencil.path.isEmpty()) {
    QPainterPath p;
    p.addRect(viewport);
    QPainterPath clipPath = p - stencil.path;
    painter->setClipPath(clipPath);
  }
}

void DiPaintGLPainter::unsetClipPath()
{
  if (stencil.enabled && stencil.clip)
    painter->setClipRect(viewport);
}

DiGLPainter::GLuint DiPaintGLPainter::bindTexture(const QImage &image)
{
  qint64 key = image.cacheKey();
  if (textureCache.contains(key))
    return textureCache.value(key);

  GLuint t;

  if (textureCache.size() >= TEXTURE_CACHE_SIZE) {
    // The textures in the texture cache were generated by this method
    // so we can hopefully delete them as required.
    QMutableHashIterator<qint64, GLuint> it(textureCache);

    // Remove an arbitrary texture from the cache.
    if (it.hasNext()) {
      it.next();

      // Record the texture ID and remove this entry.
      t = it.value();
      it.remove();

      // Remove the corresponding entry from the texture container
      // and reuse the texture ID.
    }
  }

  QImage image2 = image.mirrored();

  GenTextures(1, &t);
  textures[t] = image2;
  textureCache[key] = t;

  return t;
}

void DiPaintGLPainter::drawTexture(const QPointF &pos, GLuint texture)
{
  float x = transform.dx() + pos.x();
  float y = transform.dy() - pos.y();
  QImage image = textures.value(texture);

  painter->save();
  // It seems that we need to explicitly set the composition mode.
  painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
  painter->drawImage(x, y - image.height(), image);
  painter->restore();
}

#define ENSURE_CTX_AND_PAINTER if (!this->painter) return;
#define ENSURE_CTX_AND_PAINTER_BOOL if (!this->painter) return false;

void DiPaintGLPainter::Begin(GLenum mode)
{
    this->stack.push(this->mode);
    this->mode = mode;
}

void DiPaintGLPainter::BindTexture(GLenum target, GLuint texture)
{
    // Assume target == gl_TEXTURE_2D
    this->currentTexture = texture;
}

void DiPaintGLPainter::Bitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig,
              GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
    // Diana only uses this to displace bitmaps.
    this->bitmapMove = this->transform * QPointF(xmove, ymove);
}

void DiPaintGLPainter::BlendFunc(GLenum sfactor, GLenum dfactor)
{
    ENSURE_CTX_AND_PAINTER
    if (sfactor == gl_SRC_ALPHA && dfactor == gl_ONE_MINUS_SRC_ALPHA)
        this->blendMode = QPainter::CompositionMode_SourceOver;
}

void DiPaintGLPainter::ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    this->clearColor = QColor(red * 255, green * 255, blue * 255, alpha * 255);
}

void DiPaintGLPainter::Clear(GLbitfield mask)
{
    if (mask & gl_COLOR_BUFFER_BIT) {
        if (this->isPainting() && this->colorMask)
            this->painter->fillRect(0, 0, this->painter->device()->width(),
                                         this->painter->device()->height(), this->clearColor);
        else
            this->clear = true; // Is this used?
    }
    if (mask & gl_STENCIL_BUFFER_BIT) {
        if (this->isPainting())
            this->stencil.path = QPainterPath();
        else
            this->clear = true; // Is this used?
    }
}

void DiPaintGLPainter::ClearStencil(GLint s)
{
    this->stencil.clear = s;
}

void DiPaintGLPainter::Color3d(GLdouble red, GLdouble green, GLdouble blue)
{
    this->attributes.color = qRgba(red * 255, green * 255, blue * 255, 255);
    if (this->painter && !this->blend)
        this->renderPrimitive();
}

void DiPaintGLPainter::Color3f(GLfloat red, GLfloat green, GLfloat blue)
{
    this->attributes.color = qRgba(red * 255, green * 255, blue * 255, 255);
    if (this->painter && !this->blend)
        this->renderPrimitive();
}

void DiPaintGLPainter::Color3fv(const GLfloat *v)
{
    this->attributes.color = qRgba(v[0] * 255, v[1] * 255, v[2] * 255, 255);
    if (this->painter && !this->blend)
        this->renderPrimitive();
}

void DiPaintGLPainter::Color3ub(GLubyte red, GLubyte green, GLubyte blue)
{
    this->attributes.color = qRgba(red, green, blue, 255);
    if (this->painter && !this->blend)
        this->renderPrimitive();
}

void DiPaintGLPainter::Color3ubv(const GLubyte *v)
{
    this->attributes.color = qRgba(v[0], v[1], v[2], 255);
    if (this->painter && !this->blend)
        this->renderPrimitive();
}

void DiPaintGLPainter::Color4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
    this->attributes.color = qRgba(red * 255, green * 255, blue * 255, alpha * 255);
    if (this->painter && !this->blend)
        this->renderPrimitive();
}

void DiPaintGLPainter::Color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    this->attributes.color = qRgba(red * 255, green * 255, blue * 255, alpha * 255);
    if (this->painter && !this->blend)
        this->renderPrimitive();
}

void DiPaintGLPainter::Color4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    this->attributes.color = qRgba(red, green, blue, alpha);
    if (this->painter && !this->blend)
        this->renderPrimitive();
}

void DiPaintGLPainter::Color4fv(const GLfloat *v)
{
    this->attributes.color = qRgba(v[0] * 255, v[1] * 255, v[2] * 255, v[3] * 255);
    if (this->painter && !this->blend)
        this->renderPrimitive();
}

void DiPaintGLPainter::Color4ubv(const GLubyte *v)
{
    this->attributes.color = qRgba(v[0], v[1], v[2], v[3]);
    if (this->painter && !this->blend)
        this->renderPrimitive();
}

void DiPaintGLPainter::ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    // These are only enabled or disabled together in Diana.
    this->colorMask = red | green | blue | alpha;
}

void DiPaintGLPainter::DeleteTextures(GLsizei n, const GLuint *textures)
{
    for (int i = 0; i < n; ++i)
        this->textures.remove(textures[i]);
}

void DiPaintGLPainter::DepthMask(GLboolean flag)
{
    // Unimplemented - this may be required for correct frame plotting.
}

void DiPaintGLPainter::Disable(GLenum cap)
{
    switch (cap) {
    case gl_BLEND:
        this->blend = false;
        break;
    case gl_TEXTURE_2D:
        this->useTexture = false;
        break;
    case gl_LINE_STIPPLE:
        this->attributes.lineStipple = false;
        break;
    case gl_MULTISAMPLE:
        this->attributes.antialiasing = false;
        break;
    case gl_POLYGON_STIPPLE:
        this->attributes.polygonStipple = false;
        break;
    case gl_STENCIL_TEST:
        this->stencil.enabled = false;
        break;
    default:
        break;
    }
}

void DiPaintGLPainter::DisableClientState(GLenum cap)
{
  this->clientState &= ~cap;
}

void DiPaintGLPainter::DrawArrays(GLenum mode, GLint first, GLsizei count)
{
  ENSURE_CTX_AND_PAINTER

  if (!(this->clientState & gl_VERTEX_ARRAY))
    return;

  Begin(mode);
  if (this->vertexSize == 2 && this->vertexType == gl_DOUBLE) {
    GLdouble *ptr = (GLdouble *)(this->vertexPointer);
    for (GLint i = first * 2; i < (first + count) * 2; i += 2)
      Vertex2dv(&ptr[i]);
  }
  End();
}

void DiPaintGLPainter::DrawBuffer(GLenum mode)
{
    /* Unimplemented - code using this function is unlikely to be used when
       this OpenGL wrapper is in use. */
}

void DiPaintGLPainter::DrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type,
                  const GLvoid *pixels)
{
    ENSURE_CTX_AND_PAINTER
    // Assuming type == gl_UNSIGNED_BYTE

    if (!this->colorMask) return;

    int sx = this->pixelStore[gl_UNPACK_SKIP_PIXELS];
    int sy = this->pixelStore[gl_UNPACK_SKIP_ROWS];
    int sr = this->pixelStore[gl_UNPACK_ROW_LENGTH];

    QImage image = QImage((const uchar *)pixels + (sr * 4 * sy) + (sx * 4), width, height, sr * 4, QImage::Format_ARGB32).rgbSwapped();
    QImage destImage;

    // Process the image according to the transfer function parameters.
    if (this->attributes.scaled) {
        destImage = QImage(image.size(), QImage::Format_ARGB32);
        destImage.fill(this->attributes.scale);

        QPainter scalePainter;
        scalePainter.begin(&destImage);
        scalePainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        scalePainter.drawImage(0, 0, image);
        scalePainter.end();
    } else
        destImage = image;

    // Apply a bias to the image's colours if defined. This uses the original
    // image as a mask and applies a solid colour through that mask to
    // effectively recolour the image.
    if (this->attributes.biased) {
        QImage biasImage(image.size(), QImage::Format_ARGB32);
        biasImage.fill(this->attributes.bias);

        QPainter biasPainter;
        biasPainter.begin(&destImage);
        biasPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        biasPainter.drawImage(0, 0, biasImage);
        biasPainter.end();
    }

    this->painter->save();
    // Set the clip path, but don't unset it - the state will be restored.
    this->setClipPath();

    // It seems that we need to explicitly set the composition mode.
    this->painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    // No need to record the following transformation because we will only use it once.
    this->painter->resetTransform();
    this->painter->translate(this->rasterPos);
    this->painter->scale(this->attributes.pixelZoom.x(), -this->attributes.pixelZoom.y());
    this->painter->drawImage(0, 0, destImage);
    this->painter->restore();

    // Update the raster position.
    this->rasterPos += this->bitmapMove;
}

void DiPaintGLPainter::EdgeFlag(GLboolean flag)
{
    /* Unimplemented - this seems to be called only with gl_TRUE in Diana and
       should be true by default. */
}

void DiPaintGLPainter::Enable(GLenum cap)
{
    ENSURE_CTX_AND_PAINTER
    switch (cap) {
    case gl_BLEND:
        this->blend = true;
        break;
    case gl_TEXTURE_2D:
        this->useTexture = true;
        break;
    case gl_LINE_STIPPLE:
        this->attributes.lineStipple = true;
        break;
    case gl_MULTISAMPLE:
        this->attributes.antialiasing = true;
        break;
    case gl_POLYGON_STIPPLE:
        this->attributes.polygonStipple = true;
        break;
    case gl_STENCIL_TEST:
        this->stencil.enabled = true;
        break;
    default:
        break;
    }
}

void DiPaintGLPainter::EnableClientState(GLenum cap)
{
  this->clientState |= cap;
}

void DiPaintGLPainter::End()
{
    ENSURE_CTX_AND_PAINTER

    this->setClipPath();
    this->renderPrimitive();
    this->unsetClipPath();

    this->mode = this->stack.pop();
}

void DiPaintGLPainter::Flush()
{
    ENSURE_CTX_AND_PAINTER
    this->renderPrimitive();
}

void DiPaintGLPainter::GenTextures(GLsizei n, GLuint *textures)
{
    GLuint min = 0;
    if (!this->textures.isEmpty()) {

        foreach (GLuint i, this->textures.keys())
            if (min == 0 || i < min) min = i;

        // If the requested number of textures will fit below the minimum texture
        // ID in the hash then simply use the values below the minimum.
        // Start assigning values from 1, not 0.
        if (min > (GLuint)n) {
            for (GLuint i = 1; i <= (GLuint)n; ++i) {
                this->textures[i] = QImage();
                textures[i - 1] = i;
            }
            return;
        }
    }

    // Find the highest ID and use values above that.
    GLuint max = min;
    foreach (GLuint i, this->textures.keys())
        if (i > max) max = i;

    for (GLuint i = 0; i < (GLuint)n; ++i) {
        this->textures[max + 1 + i] = QImage();
        textures[i] = max + i + 1;
    }
}

void DiPaintGLPainter::GetFloatv(GLenum pname, GLfloat *params)
{
    switch (pname) {
    case gl_CURRENT_COLOR:
    {
        QRgb c = this->attributes.color;
        params[0] = qRed(c)/255.0;
        params[1] = qGreen(c)/255.0;
        params[2] = qBlue(c)/255.0;
        params[3] = qAlpha(c)/255.0;
        break;
    }
    case gl_LINE_WIDTH:
        params[0] = this->attributes.width;
        break;
    default:
        break;
    }
}

void DiPaintGLPainter::GetIntegerv(GLenum pname, GLint *params)
{
    ENSURE_CTX_AND_PAINTER
    switch (pname) {
    case gl_VIEWPORT:
        params[0] = this->painter->window().left();
        params[1] = this->painter->window().top();
        params[2] = this->painter->window().width();
        params[3] = this->painter->window().height();
        break;
    case gl_MAX_VIEWPORT_DIMS:
        params[0] = 65535;
        params[1] = 65535;
        break;
    default:
        break;
    }
}

void DiPaintGLPainter::Indexi(GLint c)
{
    /* Unimplemented - either explicitly documented as unused in Diana or
       used in code that is unlikely to be executed. */
}

DiGLPainter::GLboolean DiPaintGLPainter::IsEnabled(GLenum cap)
{
    switch (cap) {
    case gl_BLEND:
        return this->blend;
    case gl_TEXTURE_2D:
        return this->useTexture;
    case gl_LINE_STIPPLE:
        return this->attributes.lineStipple;
    case gl_MULTISAMPLE:
        return this->attributes.antialiasing;
    case gl_POLYGON_STIPPLE:
        return this->attributes.polygonStipple;
    case gl_STENCIL_TEST:
        return this->stencil.enabled;
    default:
        return false;
    }
}

void DiPaintGLPainter::LineStipple(GLint factor, GLushort pattern)
{
  QVector<qreal> dashes;
  if (pattern == 0xAAAA || pattern == 0x5555) {
    // alternating bits -- these two patterns seem to produce bad
    // PostScript, we replace them
    pattern = 0x3333;
  }
  GLushort state = pattern & 1;
  bool gapStart = (pattern & 1) == 0;
  int number = 0;
  int total = 0;

  for (int i = 0; i < 16; ++i) {
    GLushort dash = pattern & 1;
    if (dash == state)
      number++;
    else {
      dashes << number * factor;
      total += number * factor;
      state = dash;
      number = 1;
    }
    pattern = pattern >> 1;
  }
  if (number > 0)
    dashes << number * factor;

  if (dashes.size() > 1) {
    // Ensure that the pattern has an even number of elements by inserting
    // a zero-size element if necessary.
    if (dashes.size() % 2 == 1)
      dashes << 0;

    /* If the pattern starts with a gap then move it to the end of the
       vector and adjust the starting offset to its location to ensure
       that it appears at the start of the pattern. (This is because
       QPainter's dash pattern rendering assumes that the first element
       is a line. */
    if (gapStart) {
      dashes << dashes.first();
      dashes.pop_front();
      this->attributes.dashOffset = total - dashes.last();
    } else
      this->attributes.dashOffset = 0;
  } else if (dashes.size() == 1) {
    dashes[0] = (gapStart) ? 0 : 1;
  }
  this->attributes.dashes = dashes;
}

void DiPaintGLPainter::LineWidth(GLfloat width)
{
    ENSURE_CTX_AND_PAINTER
    this->attributes.width = width;
}

void DiPaintGLPainter::LoadIdentity()
{
  ENSURE_CTX_AND_PAINTER;
  this->transform = QTransform();
}

void DiPaintGLPainter::Ortho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
             GLdouble near_val, GLdouble far_val)
{
  this->window = QRectF(left, top, right - left, bottom - top);
  this->setViewportTransform();
}

void DiPaintGLPainter::PixelStorei(GLenum pname, GLint param)
{
  this->pixelStore[pname] = param;
}

void DiPaintGLPainter::PixelTransferf(GLenum pname, GLfloat param)
{
  switch (pname) {
  case gl_RED_BIAS:
    this->attributes.bias.setRedF(param);
    this->attributes.biased |= (param != 0.0);
    break;
  case gl_GREEN_BIAS:
    this->attributes.bias.setGreenF(param);
    this->attributes.biased |= (param != 0.0);
    break;
  case gl_BLUE_BIAS:
    this->attributes.bias.setBlueF(param);
    this->attributes.biased |= (param != 0.0);
    break;
  case gl_ALPHA_SCALE:
    this->attributes.scale.setAlphaF(param);
    this->attributes.scaled |= (param != 1.0);
    break;
  }
}

void DiPaintGLPainter::PixelZoom(GLfloat xfactor, GLfloat yfactor)
{
  this->attributes.pixelZoom = QPointF(xfactor, yfactor);
}

void DiPaintGLPainter::PointSize(GLfloat size)
{
  this->pointSize = size;
}

void DiPaintGLPainter::PolygonMode(GLenum face, GLenum mode)
{
  if (face == gl_FRONT_AND_BACK) {
    PolygonMode(gl_FRONT, mode);
    PolygonMode(gl_BACK, mode);
    return;
  }

  switch (mode) {
  case gl_FILL:
    this->attributes.polygonMode[face] = gl_FILL;
    break;
  case gl_LINE:
    this->attributes.polygonMode[face] = gl_LINE;
    break;
  default:
    break;
  }
}

void DiPaintGLPainter::PolygonStipple(const GLubyte *mask)
{
  this->attributes.mask = QImage(mask, 32, 32, QImage::Format_Mono).mirrored();
  QVector<QRgb> colours;
  colours << qRgba(0, 0, 0, 0) << this->attributes.color;
  this->attributes.mask.setColorTable(colours);
}

void DiPaintGLPainter::PopMatrix()
{
  this->transform = this->transformStack.pop();
}

void DiPaintGLPainter::PopAttrib(void)
{
  this->attributes = this->attributesStack.pop();
}

void DiPaintGLPainter::PushAttrib(GLbitfield mask)
{
  if (mask & gl_LINE_BIT)
    this->attributesStack.push(this->attributes);
  else if (mask & gl_POLYGON_BIT)
    this->attributesStack.push(this->attributes);
  else if (mask & gl_COLOR_BUFFER_BIT)
    this->attributesStack.push(this->attributes);
  else if (mask & gl_CURRENT_BIT)
    this->attributesStack.push(this->attributes);
  else if (mask & gl_PIXEL_MODE_BIT)
    this->attributesStack.push(this->attributes);
}

void DiPaintGLPainter::PushMatrix()
{
  this->transformStack.push(this->transform);
}

void DiPaintGLPainter::RasterPos2f(GLfloat x, GLfloat y)
{
  this->rasterPos = this->transform * QPointF(x, y);
}

void DiPaintGLPainter::ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                  GLenum format, GLenum type, GLvoid *pixels)
{
  /* Unimplemented - code using this function is unlikely to be used when
     this OpenGL wrapper is in use. */
}

void DiPaintGLPainter::Rectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
  Begin(gl_POLYGON);
  Vertex2f(x1, y1);
  Vertex2f(x2, y1);
  Vertex2f(x2, y2);
  Vertex2f(x1, y2);
  End();
}

void DiPaintGLPainter::Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
  // Diana only rotates about the z axis.
  this->transform = this->transform.rotate(angle);
}

void DiPaintGLPainter::Scalef(GLfloat x, GLfloat y, GLfloat z)
{
  Q_UNUSED(z);
  this->transform = this->transform.scale(x, y);
}

void DiPaintGLPainter::ShadeModel(GLenum mode)
{
  switch (mode) {
  case gl_FLAT:
    this->smooth = false;
    break;
  case gl_SMOOTH:
  default:
    this->smooth = true;
    break;
  }
}

void DiPaintGLPainter::StencilFunc(GLenum func, GLint ref, GLuint mask)
{
  this->stencil.func = func;
  this->stencil.ref = ref;
  this->stencil.mask = mask;
}

void DiPaintGLPainter::StencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
  this->stencil.fail = fail;
  this->stencil.zfail = zfail;
  this->stencil.zpass = zpass;

  // Special cases for Diana. We perform a test here because glStencilOp
  // is typically called after glStencilFunc in Diana code. Both tend to
  // be called with a limited range of input parameters.
  if (this->stencil.func == gl_ALWAYS && this->stencil.ref == 1 &&
      this->stencil.mask == 1 && zpass == gl_REPLACE) {

    this->stencil.update = true;
    this->stencil.clip = false;

  } else if (this->stencil.func == gl_EQUAL && this->stencil.ref == 0 &&
      zpass == gl_KEEP) {

    this->stencil.update = false;
    this->stencil.clip = true;

  } else if (this->stencil.func == gl_NOTEQUAL && this->stencil.ref == 1 &&
      zpass == gl_KEEP) {

    this->stencil.update = false;
    this->stencil.clip = true;
  }
}

void DiPaintGLPainter::TexCoord2f(GLfloat s, GLfloat t)
{
  // Unimplemented - unused in Diana.
}

void DiPaintGLPainter::TexEnvf(GLenum target, GLenum pname, GLfloat param)
{
}

void DiPaintGLPainter::TexImage2D(GLenum target, GLint level, GLint internalFormat,
    GLsizei width, GLsizei height, GLint border,
    GLenum format, GLenum type, const GLvoid *pixels)
{
  QImage texture = QImage(width, height, QImage::Format_ARGB32);
  // Assuming type == gl_UNSIGNED_BYTE
  int length = width * height;
  texture.loadFromData((const uchar *)pixels, length);
  this->textures[this->currentTexture] = texture;
}

void DiPaintGLPainter::TexParameteri(GLenum target, GLenum pname, GLint param)
{
  // Unimplemented - unused in Diana.
}

void DiPaintGLPainter::Translatef(GLfloat x, GLfloat y, GLfloat z)
{
  Q_UNUSED(z);
  this->transform.translate(x, y);
}

void DiPaintGLPainter::Vertex2dv(const GLdouble *v)
{
  QPointF p = this->transform * QPointF(v[0], v[1]);
  this->points.append(p);
  this->validPoints.append(!isnan(p.x()) && !isnan(p.y()));
  this->colors.append(this->attributes.color);
}

void DiPaintGLPainter::Vertex2f(GLfloat x, GLfloat y)
{
  QPointF p = this->transform * QPointF(x, y);
  this->points.append(p);
  this->validPoints.append(!isnan(p.x()) && !isnan(p.y()));
  this->colors.append(this->attributes.color);
}

void DiPaintGLPainter::Vertex2i(GLint x, GLint y)
{
  Vertex2f(x, y);
}

void DiPaintGLPainter::Vertex3f(GLfloat x, GLfloat y, GLfloat z)
{
  Q_UNUSED(z);
  QPointF p = this->transform * QPointF(x, y);
  this->points.append(p);
  this->validPoints.append(!isnan(p.x()) && !isnan(p.y()));
  this->colors.append(this->attributes.color);
}

void DiPaintGLPainter::Vertex3i(GLint x, GLint y, GLint z)
{
  Vertex3f(x, y, z);
}

void DiPaintGLPainter::VertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
  this->vertexSize = size;
  this->vertexType = type;
  this->vertexStride = stride;
  this->vertexPointer = ptr;
}

void DiPaintGLPainter::Viewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
  this->viewport = QRect(x, y, width, height);
  this->setViewportTransform();

  if (this->isPainting())
    this->painter->setClipRect(this->viewport);
}

void DiPaintGLPainter::paintCircle(float centerx, float centery, float radius)
{
  const QPointF center = transform.map(QPointF(centerx, centery));
  const QPointF left   = transform.map(QPointF(centerx + radius, centery));
  const QPointF top    = transform.map(QPointF(centerx, centery + radius));

  // assumes that transform is not rotating
  const float rx = std::abs(center.x() - left.x());
  const float ry = std::abs(center.y() - top.y());

  setPolygonColor(attributes.color);
  setClipPath();
  painter->drawEllipse(center, rx, ry);
  unsetClipPath();
}

void DiPaintGLPainter::drawCircle(float centerx, float centery, float radius)
{
  if (transform.isRotating()) {
    DiGLPainter::drawCircle(centerx, centery, radius);
  } else {
    PolygonMode(gl_FRONT_AND_BACK, gl_LINE);
    painter->setRenderHint(QPainter::Antialiasing, attributes.antialiasing);
    paintCircle(centerx, centery, radius);
  }
}

void DiPaintGLPainter::fillCircle(float centerx, float centery, float radius)
{
  if (transform.isRotating()) {
    DiGLPainter::fillCircle(centerx, centery, radius);
  } else {
    ShadeModel(gl_FLAT);
    PolygonMode(gl_FRONT_AND_BACK, gl_FILL);
    paintCircle(centerx, centery, radius);
  }
}

void DiPaintGLPainter::paintRect(float x1, float y1, float x2, float y2)
{
  // assumes that transform is not rotating
  const QPointF p00 = transform.map(QPointF(x1, y1));
  const QPointF p11 = transform.map(QPointF(x2, y2));

  setPolygonColor(attributes.color);
  setClipPath();
  painter->drawRect(QRectF(p00, p11));
  unsetClipPath();
}

void DiPaintGLPainter::drawRect(float x1, float y1, float x2, float y2)
{
  if (transform.isRotating()) {
    DiGLPainter::drawRect(x1, y1, x2, y2);
  } else {
    PolygonMode(gl_FRONT_AND_BACK, gl_LINE);
    painter->setRenderHint(QPainter::Antialiasing, attributes.antialiasing);
    paintRect(x1, y1, x2, y2);
  }
}

void DiPaintGLPainter::fillRect(float x1, float y1, float x2, float y2)
{
  if (transform.isRotating()) {
    DiGLPainter::fillRect(x1, y1, x2, y2);
  } else {
    ShadeModel(gl_FLAT);
    PolygonMode(gl_FRONT_AND_BACK, gl_FILL);
    paintRect(x1, y1, x2, y2);
  }
}

void DiPaintGLPainter::drawLine(float x1, float y1, float x2, float y2)
{
  const QPointF p0 = transform.map(QPointF(x1, y1));
  const QPointF p1 = transform.map(QPointF(x2, y2));
  setPen();
  painter->setRenderHint(QPainter::Antialiasing, attributes.antialiasing);
  setClipPath();
  painter->drawLine(p0, p1);
  unsetClipPath();
}

void DiPaintGLPainter::drawPolyline(const QPolygonF& points)
{
  if (points.size() < 2) {
    METLIBS_LOG_ERROR("invalid polyline, size=" << points.size());
    return;
  }
  setPen();
  if (blend)
    painter->setCompositionMode(blendMode);
  else
    painter->setCompositionMode(QPainter::CompositionMode_Source);

  painter->setRenderHint(QPainter::Antialiasing, attributes.antialiasing);
  setClipPath();
  painter->drawPolyline(transform.map(points));
  unsetClipPath();
}

void DiPaintGLPainter::drawPolygon(const QPolygonF& points)
{
  if (points.size() < 3) {
    METLIBS_LOG_ERROR("invalid polygon, size=" << points.size());
    return;
  }
  const QPolygonF tpoints = transform.map(points);
  setPolygonColor(attributes.color);
  painter->setRenderHint(QPainter::Antialiasing, attributes.antialiasing);
  setClipPath();
  painter->drawPolygon(tpoints);
  unsetClipPath();
}

void DiPaintGLPainter::drawPolygons(const QList<QPolygonF>& polygons)
{
  QPainterPath path;
  Q_FOREACH(const QPolygonF& p, polygons) {
    if (p.size() < 3) {
      METLIBS_LOG_ERROR("invalid polygon, size=" << p.size());
      return;
    }
    path.addPolygon(transform.map(p));
  }
  setPolygonColor(attributes.color);
  painter->setRenderHint(QPainter::Antialiasing, attributes.antialiasing);
  setClipPath();
  painter->drawPath(path);
  unsetClipPath();
}

namespace {
inline size_t index(size_t w, size_t x, size_t y)
{
  return y*w + x;
}
} // namespace

void DiPaintGLPainter::drawReprojectedSubImage(const QImage& image, const QPolygonF& mapPositions,
    const diutil::Rect& part)
{
  IFDEBUG(METLIBS_LOG_SCOPE());
  if (part.x1 >= part.x2 || part.y1 >= part.y2) {
    IFDEBUG(METLIBS_LOG_DEBUG("invalid part " << part));
    return;
  }
  if (HIGH_QUALITY_BUT_SLOW) {
    painter->setTransform(QTransform());
    QPolygonF poly;
    for (int y = part.y1; y<part.y2; ++y) {
      const int iw1 = image.width() + 1, y0 = y*iw1, y1 = y0 + iw1;
      for (int x = part.x1; x<part.x2; ++x) {
        const QRgb p = image.pixel(x, y);
        if (qAlpha(p) == 0)
          continue;

        poly.clear();
        poly << mapPositions.at(y0 + x)
             << mapPositions.at(y0 + x + 1)
             << mapPositions.at(y1 + x + 1)
             << mapPositions.at(y1 + x);
        bool have_invalid = false;
        for (int i=0; !have_invalid && i<4; ++i)
          have_invalid |= diutil::is_undefined(poly.at(i));
        if (have_invalid)
          continue;

        painter->setBrush(QColor::fromRgba(p));
        painter->drawPolygon(poly);
      }
    }
    return;
  }

  // faster, but there might be empty pixels and some distortions
  const float MH_LIMIT = 0.5;
  const int SZ_LIMIT = 16;

  const int x0 = part.x1, x1 = part.x2, y0 = part.y1, y1 = part.y2;
  const size_t w = x1 - x0, h = y1 - y0, iw = image.width();
  const size_t xm = (x0+x1)/2, ym = (y0+y1)/2;
  const QPointF &p00 = mapPositions.at(index(iw+1, x0, y0)),
      &p01 = mapPositions.at(index(iw+1, x0, y1)),
      &p10 = mapPositions.at(index(iw+1, x1, y0)),
      &p11 = mapPositions.at(index(iw+1, x1, y1));

  bool paint = false;
  if (w <= SZ_LIMIT && h <= SZ_LIMIT) {
    IFDEBUG(METLIBS_LOG_DEBUG(LOGVAL(w) << LOGVAL(h) << LOGVAL(paint)));
    paint = true;
  } else if (MH_LIMIT > 0) {
    const QPointF &pm0 = mapPositions.at(index(iw+1, xm, y0)),
        &p0m = mapPositions.at(index(iw+1, x0, ym)),
        &p1m = mapPositions.at(index(iw+1, x1, ym)),
        &pm1 = mapPositions.at(index(iw+1, xm, y1)),
        &pmm = mapPositions.at(index(iw+1, xm, ym));

    const QPointF im0 = (p00 + p10)*0.5,
        i0m = (p00 + p01)*0.5,
        im1 = (p01 + p11)*0.5,
        i1m = (p10 + p11)*0.5,
        imm = (p00 + p01 + p10 + p11)*0.25;

    paint=((pm0-im0).manhattanLength() < MH_LIMIT
        && (pm1-im1).manhattanLength() < MH_LIMIT
        && (p0m-i0m).manhattanLength() < MH_LIMIT
        && (p1m-i1m).manhattanLength() < MH_LIMIT
        && (pmm-imm).manhattanLength() < MH_LIMIT);
    IFDEBUG(METLIBS_LOG_DEBUG("mh " << LOGVAL(paint)));
  }
  if (paint) {
    static Qt::ImageConversionFlags icf = Qt::DiffuseAlphaDither | Qt::NoOpaqueDetection;

    QPolygonF tgt;
    tgt << p00 << p10 << p11 << p01;
    if (w <= 1 && h <= 1) {
      const QRgb p = image.pixel(x0, y0);
      if (qAlpha(p) > 0) {
        painter->setBrush(QColor::fromRgba(p));
        painter->setTransform(QTransform());
        painter->drawPolygon(tgt);
      }
      return;
    } else {
      QTransform t;
      if (QTransform::squareToQuad(tgt, t)) {
        t = QTransform::fromScale(1.0/w, 1.0/h) * t;

        // somehow, Qt somehow likes to transform polygons in a
        // strange way, e.g. moving the third point to -2e8,-2e8; the
        // following is an attempt to work around this problem
        bool mapped_ok = true;
        const QPointF t_rect_corners[4] = {
          t.map(QPointF(x0,   y0)),
          t.map(QPointF(x0+w, y0)),
          t.map(QPointF(x0+w, y0+h)),
          t.map(QPointF(x0,   y0+h))
        };
        const QRectF r(x0, y0, w, h);
        const QPolygonF t_rect_poly = t.map(QPolygonF(r));
        for (int i=0; i<4; ++i) {
          qreal mh = (t_rect_corners[i] - t_rect_poly[i]).manhattanLength();
          if (mh > 1) {
            mapped_ok = false;
            break;
          }
        }
        if (mapped_ok) {
          painter->setTransform(t);
          painter->drawImage(QRectF(0, 0, w+1e-5, h+1e-5), image, r, icf);
          return;
        }
      }
    }
  }
  IFDEBUG(METLIBS_LOG_DEBUG("not painted" << LOGVAL(x0) << LOGVAL(xm) << LOGVAL(x1)
          << LOGVAL(y0) << LOGVAL(ym) << LOGVAL(y1)
          << LOGVAL(w) << LOGVAL(h)));
  if (w > h) {
    drawReprojectedSubImage(image, mapPositions, diutil::Rect(x0, y0, xm, y1));
    drawReprojectedSubImage(image, mapPositions, diutil::Rect(xm, y0, x1, y1));
  } else {
    drawReprojectedSubImage(image, mapPositions, diutil::Rect(x0, y0, x1, ym));
    drawReprojectedSubImage(image, mapPositions, diutil::Rect(x0, ym, x1, y1));
  }
}

void DiPaintGLPainter::drawReprojectedImage(const QImage& image, const float* mapPositionsXY,
    const diutil::Rect_v& imageparts, bool smooth)
{
  PolygonMode(gl_FRONT_AND_BACK, gl_FILL);
  Enable(gl_BLEND);
  BlendFunc(gl_SRC_ALPHA, gl_ONE_MINUS_SRC_ALPHA);
  ShadeModel(gl_FLAT);
  if (blend)
    painter->setCompositionMode(blendMode);
  else
    painter->setCompositionMode(QPainter::CompositionMode_Source);

  const size_t width = image.width(), height = image.height(),
      size11 = (width+1)*(height+1);

  QPolygonF positions;
  positions.reserve(size11);
  for (size_t i = 0; i < size11; ++i) {
    QPointF p(mapPositionsXY[2*i], mapPositionsXY[2*i+1]);
    if (!diutil::is_undefined(p))
      p = transform.map(p);
    positions << p;
  }

  const bool paa = painter->testRenderHint(QPainter::Antialiasing);
  painter->setRenderHint(QPainter::Antialiasing, false);
  const bool pst = painter->testRenderHint(QPainter::SmoothPixmapTransform);
  painter->setRenderHint(QPainter::SmoothPixmapTransform, smooth);
  painter->save(); // we will change the transformation matrix
  painter->setPen(Qt::NoPen);
  for (diutil::Rect_v::const_iterator it = imageparts.begin(); it != imageparts.end(); ++it) {
    drawReprojectedSubImage(image, positions, *it);
  }
  painter->restore();
  painter->setRenderHint(QPainter::Antialiasing, paa);
  painter->setRenderHint(QPainter::SmoothPixmapTransform, pst);
}

// ========================================================================

bool DiPaintGLPainter::drawText(const QString& str,
    const float x, const float y, const float a)
{
  if (!this->colorMask)
    return true;

  this->painter->save();
  // Set the clip path, but don't unset it - the state will be restored.
  this->setClipPath();

  DiPaintGLCanvas* c = (DiPaintGLCanvas*)canvas();
  const QFont& font = c->font();
  this->painter->setFont(font);
  QFontMetricsF fm = painter->fontMetrics();

  // No need to record this transformation.
  this->painter->setTransform(this->transform);
  this->painter->setPen(QPen(this->attributes.color));
  this->painter->translate(x, y);
  this->painter->rotate(a);
  // Unscale the text so that it appears at the intended size.
  this->painter->scale(c->fontScaleX(), c->fontScaleY());
  // Flip it vertically to take coordinate system differences into account.
  this->painter->setTransform(QTransform(1, 0, 0, 0, -1, 0, 0, 0, 1), true);
  this->painter->drawText(0, 0, str);
  this->painter->restore();
  return true;
}
