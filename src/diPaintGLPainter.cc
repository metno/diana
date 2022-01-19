/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2011-2021 met.no

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

DiPaintGLCanvas::DiPaintGLCanvas(QPaintDevice* device)
    : mDevice(device)
    , mFont(QFont(), mDevice)
    , mFontValid(false)
    , mFontScaleX(1)
    , mFontScaleY(1)
{
  METLIBS_LOG_SCOPE();
}

DiPaintGLCanvas::~DiPaintGLCanvas()
{
  for (int i=0; i<fontHandles.count(); ++i)
    QFontDatabase::removeApplicationFont(fontHandles.at(i));
}

bool DiPaintGLCanvas::selectFont(const std::string& family, diutil::FontFace face, float size)
{
  selectFont(family);
  setFontFace(face);
  setFontSize(size);
  return true;
}

bool DiPaintGLCanvas::selectFont(const std::string& family)
{
  METLIBS_LOG_SCOPE(LOGVAL(family));

  std::map<std::string, QString>::const_iterator it = fontMap.find(family);
  if (it == fontMap.end()) {
    mFontValid = false;
  } else {
    mFontValid = true;
    mFont.setFamily(it->second);
    mFont.setStyleStrategy(QFont::NoFontMerging);
  }
  return mFontValid;
}

bool DiPaintGLCanvas::hasFont(const std::string& family)
{
  return (fontMap.find(family) != fontMap.end());
}

bool DiPaintGLCanvas::setFontFace(diutil::FontFace face)
{
  if (diutil::isBoldFont(face))
    mFont.setWeight(QFont::Bold);
  else
    mFont.setWeight(QFont::Normal);
  mFont.setItalic(diutil::isItalicFont(face));
  return true;
}

bool DiPaintGLCanvas::setFontSize(float size)
{
  mFont.setPointSizeF(size);
  return true;
}

void DiPaintGLCanvas::setVpGlSize(int vpw, int vph, float glw, float glh)
{
  mFontScaleX = glw / vpw;
  mFontScaleY = glh / vph;
}

void DiPaintGLCanvas::parseFontSetup(const std::vector<std::string>& sect_fonts)
{
  for (int i=0; i<fontHandles.count(); ++i)
    QFontDatabase::removeApplicationFont(fontHandles.at(i));
  fontHandles.clear();
  fontMap.clear();

  DiGLCanvas::parseFontSetup(sect_fonts);
}

void DiPaintGLCanvas::defineFont(const std::string& fontfam, const std::string& fontfilename, diutil::FontFace face, bool /*use_bitmap*/)
{
  METLIBS_LOG_SCOPE(LOGVAL(fontfam) << LOGVAL(fontfilename));
  if (face != diutil::F_NORMAL)
    return;

  const int handle = QFontDatabase::addApplicationFont(QString::fromStdString(fontfilename));
  if (handle == -1) {
    METLIBS_LOG_WARN("Could not load font from '" << fontfilename << "'");
    return;
  }

  fontHandles << handle;

  const QStringList families = QFontDatabase::applicationFontFamilies(handle);
  for (const QString& family : families) {
    fontMap[fontfam] = family;
  }
}

bool DiPaintGLCanvas::getTextRect(const QString& str, float& x, float& y, float& w, float& h)
{
  if (str.length() == 0 || !mFontValid) {
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
  blend = false;
  blendMode = QPainter::CompositionMode_Source;

  clearColor = QColor(0, 0, 0, 0);
  colorMask = true;

  smooth = false;

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
  transformStack.clear();
  transform = QTransform();
  attributesStack.clear();
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
        if (blend) {
          // Optimisation: Diana only ever draws filled, blended quads without edges.
          int j = (i / 4) * 4;
          painter->setBrush(QColor::fromRgba(colors.at(j)));
        }
        painter->drawConvexPolygon(quad);
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

#define ENSURE_CTX_AND_PAINTER if (!this->painter) return;
#define ENSURE_CTX_AND_PAINTER_BOOL if (!this->painter) return false;

void DiPaintGLPainter::Begin(GLenum mode)
{
    this->stack.push(this->mode);
    this->mode = mode;
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

void DiPaintGLPainter::ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    // These are only enabled or disabled together in Diana.
    this->colorMask = red | green | blue | alpha;
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

DiGLPainter::GLboolean DiPaintGLPainter::IsEnabled(GLenum cap)
{
    switch (cap) {
    case gl_BLEND:
        return this->blend;
    case gl_TEXTURE_2D:
        return false;
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

void DiPaintGLPainter::Translatef(GLfloat x, GLfloat y, GLfloat z)
{
  Q_UNUSED(z);
  this->transform.translate(x, y);
}

void DiPaintGLPainter::Vertex2f(GLfloat x, GLfloat y)
{
  QPointF p = this->transform * QPointF(x, y);
  this->points.append(p);
  this->validPoints.append(!std::isnan(p.x()) && !std::isnan(p.y()));
  this->colors.append(this->attributes.color);
}

void DiPaintGLPainter::Vertex3f(GLfloat x, GLfloat y, GLfloat z)
{
  Q_UNUSED(z);
  Vertex2f(x, y);
}

void DiPaintGLPainter::Vertex3i(GLint x, GLint y, GLint z)
{
  Vertex3f(x, y, z);
}

void DiPaintGLPainter::Viewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
  this->viewport = QRect(x, y, width, height);
  this->setViewportTransform();

  if (this->isPainting())
    this->painter->setClipRect(this->viewport);
}

void DiPaintGLPainter::setFillMode(bool fill)
{
  if (fill) {
    ShadeModel(gl_FLAT);
    PolygonMode(gl_FRONT_AND_BACK, gl_FILL);
  } else {
    PolygonMode(gl_FRONT_AND_BACK, gl_LINE);
    painter->setRenderHint(QPainter::Antialiasing, attributes.antialiasing);
  }
}

void DiPaintGLPainter::drawCircle(bool fill, float centerx, float centery, float radius)
{
  if (transform.isRotating()) {
    DiGLPainter::drawCircle(fill, centerx, centery, radius);
    return;
  }

  setFillMode(fill);

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

void DiPaintGLPainter::drawRect(bool fill, float x1, float y1, float x2, float y2)
{
  if (transform.isRotating()) {
    DiGLPainter::drawRect(fill, x1, y1, x2, y2);
    return;
  }

  setFillMode(fill);

  // assumes that transform is not rotating
  const QPointF p00 = transform.map(QPointF(x1, y1));
  const QPointF p11 = transform.map(QPointF(x2, y2));

  // QRectF needs top-left and size; make sure we do not use bottom-left or so
  const QPointF ptl(std::min(p00.x(), p11.x()), std::min(p00.y(), p11.y()));
  const QSizeF siz(std::abs(p11.x() - p00.x()), std::abs(p11.y() - p00.y()));

  setPolygonColor(attributes.color);
  setClipPath();
  painter->drawRect(QRectF(ptl, siz));
  unsetClipPath();
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
  for (const QPolygonF& p : polygons) {
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

void DiPaintGLPainter::drawScreenImage(const QPointF& point, const QImage& image)
{
  painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
  painter->drawImage(point, image);
}

// ========================================================================

Colour DiPaintGLPainter::getColour()
{
  const QRgb c = this->attributes.color;
  return Colour(qRed(c), qGreen(c), qBlue(c), qAlpha(c));
}

// ========================================================================

void DiPaintGLPainter::setColour(const Colour& c, bool alpha)
{
  this->attributes.color = qRgba(c.R(), c.G(), c.B(), alpha ? c.A() : 255);
  if (this->painter && !this->blend)
    this->renderPrimitive();
}

bool DiPaintGLPainter::drawText(const QString& str,
    const QPointF& xy, const float a)
{
  if (!this->colorMask)
    return true;

  DiPaintGLCanvas* c = (DiPaintGLCanvas*)canvas();
  if (!c->fontValid())
    return false;

  this->painter->save();
  // Set the clip path, but don't unset it - the state will be restored.
  this->setClipPath();

  const QFont& font = c->font();
  this->painter->setFont(font);
  QFontMetricsF fm = painter->fontMetrics();

  // No need to record this transformation.
  this->painter->setTransform(this->transform);
  this->painter->setPen(QPen(this->attributes.color));
  this->painter->translate(xy.x(), xy.y());
  this->painter->rotate(a);
  // Unscale the text so that it appears at the intended size.
  this->painter->scale(c->fontScaleX(), c->fontScaleY());
  // Flip it vertically to take coordinate system differences into account.
  this->painter->setTransform(QTransform(1, 0, 0, 0, -1, 0, 0, 0, 1), true);
  this->painter->drawText(0, 0, str);
  this->painter->restore();
  return true;
}
