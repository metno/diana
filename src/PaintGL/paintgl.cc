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

#include <QtGui>

#include "GL/gl.h"
#include "paintgl.h"

PaintGL *PaintGL::self = 0;

PaintGL::PaintGL()
{
    self = this;
    currentContext = 0;
}

PaintGL::~PaintGL()
{
    self = 0;
}

#define globalGL PaintGL::instance()
#define ctx globalGL->currentContext
#define TEXTURE_CACHE_SIZE 16

PaintGLContext::PaintGLContext()
    : painter(0)
{
}

PaintGLContext::~PaintGLContext()
{
    if (globalGL)
        ctx = 0;
}

PaintGLContext *PaintGLContext::currentContext()
{
    return ctx;
}

void PaintGLContext::makeCurrent()
{
    // Make this context the current one.
    ctx = this;

    useTexture = false;
    blend = false;
    blendMode = QPainter::CompositionMode_Source;
    pointSize = 1.0;

    clearColor = QColor(0, 0, 0, 0);
    colorMask = true;

    stencil.clear = 0;
    stencil.path = QPainterPath();
    stencil.fail = GL_KEEP;
    stencil.zfail = GL_KEEP;
    stencil.zpass = GL_KEEP;
    stencil.func = GL_ALWAYS;
    stencil.ref = 0;
    stencil.mask = ~0x0;
    stencil.clip = false;
    stencil.update = false;
    stencil.enabled = false;

    attributes.color = qRgba(255, 255, 255, 255);
    attributes.width = 1.0;
    attributes.polygonMode[GL_FRONT] = GL_FILL;
    attributes.polygonMode[GL_BACK] = GL_FILL;
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

    printing = false;
}

void PaintGLContext::begin(QPainter *painter)
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

bool PaintGLContext::isPainting() const
{
    return painter != 0;
}

void PaintGLContext::end()
{
    painter = 0;
}

void PaintGLContext::setPen()
{
    qreal width;
    if (printing)
        width = attributes.width/3.0;
    else
        width = attributes.width;

    QPen pen = QPen(QColor::fromRgba(attributes.color), width);
    pen.setCapStyle(Qt::FlatCap);
    pen.setCosmetic(true);
    if (attributes.lineStipple && !attributes.dashes.isEmpty()) {
        /* Set the dash pattern on the pen if defined, adjusting
           the length of each element to compensate for the line
           width. */
        QVector<qreal> dashes;
        if (width == 0) {
            if (printing)
                width = 0.5;
            else
                width = 1;
        }

        for (int i = 0; i < attributes.dashes.size(); ++i)
            dashes << attributes.dashes[i]/width;

        pen.setDashPattern(dashes);
        pen.setDashOffset(attributes.dashOffset/width);
    }

    painter->setPen(pen);
}

void PaintGLContext::setPolygonColor(const QRgb &color)
{
    switch (attributes.polygonMode[GL_FRONT]) {
    case GL_FILL:
        painter->setPen(Qt::NoPen);
        if (attributes.polygonStipple && !attributes.mask.isNull()) {
            QVector<QRgb> colours;
            colours << qRgba(0, 0, 0, 0) << attributes.color;
            attributes.mask.setColorTable(colours);
            painter->setBrush(attributes.mask);
        } else
            painter->setBrush(QColor::fromRgba(color));
        break;
    case GL_LINE: {
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

void PaintGLContext::plotSubdivided(const QPointF quad[], const QRgb color[], int divisions)
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

void PaintGLContext::renderPrimitive()
{
    if (points.size() == 0)
        return;

    static QPointF poly[4];
    static QRgb color[4];

    switch (mode) {
    case GL_POINTS:
        setPen();
        if (colorMask) {
            for (int i = 0; i < points.size(); ++i) {
                painter->drawPoint(points.at(i));
            }
        }
        break;
    case GL_LINES:
        setPen();
        painter->setRenderHint(QPainter::Antialiasing, attributes.antialiasing);

        if (colorMask) {
            for (int i = 0; i < points.size() - 1; i += 2) {
                painter->drawLine(points.at(i), points.at(i+1));
            }
        }
        break;
    case GL_LINE_LOOP:
        setPen();
        painter->setRenderHint(QPainter::Antialiasing, attributes.antialiasing);
        points.append(points.at(0));
        if (colorMask)
            painter->drawPolyline(points);
        break;
    case GL_LINE_STRIP: {
        setPen();
        painter->setRenderHint(QPainter::Antialiasing, attributes.antialiasing);
        if (colorMask)
            painter->drawPolyline(points);
        break;
    }
    case GL_TRIANGLES: {
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
    case GL_TRIANGLE_STRIP: {
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
    case GL_TRIANGLE_FAN: {
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
    case GL_QUADS: {
        // Optimisation: Diana only ever draws filled, blended quads without edges.
        if (blend)
            painter->setPen(Qt::NoPen);
        else
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
    case GL_QUAD_STRIP: {
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
    case GL_POLYGON: {
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

void PaintGLContext::setViewportTransform()
{
    QTransform t;
    t = transform.translate(viewport.left(), viewport.top());
    t = t.scale(viewport.width()/window.width(), viewport.height()/window.height());
    t = t.translate(-window.left(), -window.top());
    transform = t;
}

void PaintGLContext::setClipPath()
{
    if (stencil.enabled && stencil.clip && !stencil.path.isEmpty()) {
        QPainterPath p;
        p.addRect(viewport);
        QPainterPath clipPath = p - stencil.path;
        painter->setClipPath(clipPath);
    }
}

void PaintGLContext::unsetClipPath()
{
    if (stencil.enabled && stencil.clip)
        painter->setClipRect(viewport);
}

GLuint PaintGLContext::bindTexture(const QImage &image)
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

    glGenTextures(1, &t);
    textures[t] = image2;
    textureCache[key] = t;

    return t;
}

void PaintGLContext::drawTexture(const QPointF &pos, GLuint texture)
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

#define ENSURE_CTX if (!globalGL || !ctx) return;
#define ENSURE_CTX_BOOL if (!globalGL || !ctx) return false;
#define ENSURE_CTX_INT if (!globalGL || !ctx) return 0;
#define ENSURE_CTX_AND_PAINTER if (!globalGL || !ctx || !ctx->painter) return;
#define ENSURE_CTX_AND_PAINTER_BOOL if (!globalGL || !ctx || !ctx->painter) return false;

void glBegin(GLenum mode)
{
    ENSURE_CTX
    ctx->stack.push(ctx->mode);
    ctx->mode = mode;
}

void glBindTexture(GLenum target, GLuint texture)
{
    ENSURE_CTX

    // Assume target == GL_TEXTURE_2D
    ctx->currentTexture = texture;
}

void glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig,
              GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
    ENSURE_CTX

    // Diana only uses this to displace bitmaps.
    ctx->bitmapMove = ctx->transform * QPointF(xmove, ymove);
}

void glBlendFunc(GLenum sfactor, GLenum dfactor)
{
    ENSURE_CTX_AND_PAINTER
    if (sfactor == GL_SRC_ALPHA && dfactor == GL_ONE_MINUS_SRC_ALPHA)
        ctx->blendMode = QPainter::CompositionMode_SourceOver;
}

void glCallList(GLuint list)
{
    ENSURE_CTX_AND_PAINTER
    if (!ctx->colorMask) return;

    QPicture picture = ctx->lists[list];

    ctx->painter->save();
    ctx->painter->setTransform(ctx->transform);
    ctx->painter->drawPicture(0, 0, picture);
    ctx->painter->restore();
}

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    ENSURE_CTX
    ctx->clearColor = QColor(red * 255, green * 255, blue * 255, alpha * 255);
}

void glClear(GLbitfield mask)
{
    ENSURE_CTX

    if (mask & GL_COLOR_BUFFER_BIT) {
        if (ctx->isPainting() && ctx->colorMask)
            ctx->painter->fillRect(0, 0, ctx->painter->device()->width(),
                                         ctx->painter->device()->height(), ctx->clearColor);
        else
            ctx->clear = true; // Is this used?
    }
    if (mask & GL_STENCIL_BUFFER_BIT) {
        if (ctx->isPainting())
            ctx->stencil.path = QPainterPath();
        else
            ctx->clear = true; // Is this used?
    }
}

void glClearStencil(GLint s)
{
    ENSURE_CTX
    ctx->stencil.clear = s;
}

void glColor3d(GLdouble red, GLdouble green, GLdouble blue)
{
    ENSURE_CTX
    ctx->attributes.color = qRgba(red * 255, green * 255, blue * 255, 255);
    if (ctx->painter && !ctx->blend)
        ctx->renderPrimitive();
}

void glColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
    ENSURE_CTX
    ctx->attributes.color = qRgba(red * 255, green * 255, blue * 255, 255);
    if (ctx->painter && !ctx->blend)
        ctx->renderPrimitive();
}

void glColor3fv(const GLfloat *v)
{
    ENSURE_CTX
    ctx->attributes.color = qRgba(v[0] * 255, v[1] * 255, v[2] * 255, 255);
    if (ctx->painter && !ctx->blend)
        ctx->renderPrimitive();
}

void glColor3ub(GLubyte red, GLubyte green, GLubyte blue)
{
    ENSURE_CTX
    ctx->attributes.color = qRgba(red, green, blue, 255);
    if (ctx->painter && !ctx->blend)
        ctx->renderPrimitive();
}

void glColor3ubv(const GLubyte *v)
{
    ENSURE_CTX
    ctx->attributes.color = qRgba(v[0], v[1], v[2], 255);
    if (ctx->painter && !ctx->blend)
        ctx->renderPrimitive();
}

void glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
    ENSURE_CTX
    ctx->attributes.color = qRgba(red * 255, green * 255, blue * 255, alpha * 255);
    if (ctx->painter && !ctx->blend)
        ctx->renderPrimitive();
}

void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    ENSURE_CTX
    ctx->attributes.color = qRgba(red * 255, green * 255, blue * 255, alpha * 255);
    if (ctx->painter && !ctx->blend)
        ctx->renderPrimitive();
}

void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    ENSURE_CTX
    ctx->attributes.color = qRgba(red, green, blue, alpha);
    if (ctx->painter && !ctx->blend)
        ctx->renderPrimitive();
}

void glColor4fv(const GLfloat *v)
{
    ENSURE_CTX
    ctx->attributes.color = qRgba(v[0] * 255, v[1] * 255, v[2] * 255, v[3] * 255);
    if (ctx->painter && !ctx->blend)
        ctx->renderPrimitive();
}

void glColor4ubv(const GLubyte *v)
{
    ENSURE_CTX
    ctx->attributes.color = qRgba(v[0], v[1], v[2], v[3]);
    if (ctx->painter && !ctx->blend)
        ctx->renderPrimitive();
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    ENSURE_CTX

    // These are only enabled or disabled together in Diana.
    ctx->colorMask = red | green | blue | alpha;
}

void glDeleteLists(GLuint list, GLsizei range)
{
    ENSURE_CTX
    for (GLuint i = list; i < list + range; ++i) {
        ctx->lists.remove(i - 1);
        ctx->listTransforms.remove(i - 1);
    }
}

void glDeleteTextures(GLsizei n, const GLuint *textures)
{
    ENSURE_CTX
    for (int i = 0; i < n; ++i)
        ctx->textures.remove(textures[i]);
}

void glDepthMask(GLboolean flag)
{
    // Unimplemented - this may be required for correct frame plotting.
}

void glDisable(GLenum cap)
{
    ENSURE_CTX
    switch (cap) {
    case GL_BLEND:
        ctx->blend = false;
        break;
    case GL_TEXTURE_2D:
        ctx->useTexture = false;
        break;
    case GL_LINE_STIPPLE:
        ctx->attributes.lineStipple = false;
        break;
    case GL_MULTISAMPLE:
        ctx->attributes.antialiasing = false;
        break;
    case GL_POLYGON_STIPPLE:
        ctx->attributes.polygonStipple = false;
        break;
    case GL_STENCIL_TEST:
        ctx->stencil.enabled = false;
        break;
    default:
        break;
    }
}

void glDisableClientState(GLenum cap)
{
  ENSURE_CTX
  ctx->clientState &= ~cap;
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
  ENSURE_CTX_AND_PAINTER

  if (!(ctx->clientState & GL_VERTEX_ARRAY))
    return;

  glBegin(mode);
  if (ctx->vertexSize == 2 && ctx->vertexType == GL_DOUBLE) {
    GLdouble *ptr = (GLdouble *)(ctx->vertexPointer);
    for (GLint i = first * 2; i < (first + count) * 2; i += 2)
      glVertex2dv(&ptr[i]);
  }
  glEnd();
}

void glDrawBuffer(GLenum mode)
{
    /* Unimplemented - code using this function is unlikely to be used when
       this OpenGL wrapper is in use. */
}

void glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type,
                  const GLvoid *pixels)
{
    ENSURE_CTX_AND_PAINTER
    // Assuming type == GL_UNSIGNED_BYTE

    if (!ctx->colorMask) return;

    int sx = ctx->pixelStore[GL_UNPACK_SKIP_PIXELS];
    int sy = ctx->pixelStore[GL_UNPACK_SKIP_ROWS];
    int sr = ctx->pixelStore[GL_UNPACK_ROW_LENGTH];

    QImage image = QImage((const uchar *)pixels + (sr * 4 * sy) + (sx * 4), width, height, sr * 4, QImage::Format_ARGB32).rgbSwapped();
    QImage destImage;

    // Process the image according to the transfer function parameters.
    if (ctx->attributes.scaled) {
        destImage = QImage(image.size(), QImage::Format_ARGB32);
        destImage.fill(ctx->attributes.scale);

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
    if (ctx->attributes.biased) { 
        QImage biasImage(image.size(), QImage::Format_ARGB32);
        biasImage.fill(ctx->attributes.bias);

        QPainter biasPainter;
        biasPainter.begin(&destImage);
        biasPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        biasPainter.drawImage(0, 0, biasImage);
        biasPainter.end();
    }

    ctx->painter->save();
    // Set the clip path, but don't unset it - the state will be restored.
    ctx->setClipPath();

    // It seems that we need to explicitly set the composition mode.
    ctx->painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    // No need to record the following transformation because we will only use it once.
    ctx->painter->resetTransform();
    ctx->painter->translate(ctx->rasterPos);
    ctx->painter->scale(ctx->attributes.pixelZoom.x(), -ctx->attributes.pixelZoom.y());
    ctx->painter->drawImage(0, 0, destImage);
    ctx->painter->restore();

    // Update the raster position.
    ctx->rasterPos += ctx->bitmapMove;
}

void glEdgeFlag(GLboolean flag)
{
    /* Unimplemented - this seems to be called only with GL_TRUE in Diana and
       should be true by default. */
}

void glEnable(GLenum cap)
{
    ENSURE_CTX_AND_PAINTER
    switch (cap) {
    case GL_BLEND:
        ctx->blend = true;
        break;
    case GL_TEXTURE_2D:
        ctx->useTexture = true;
        break;
    case GL_LINE_STIPPLE:
        ctx->attributes.lineStipple = true;
        break;
    case GL_MULTISAMPLE:
        ctx->attributes.antialiasing = true;
        break;
    case GL_POLYGON_STIPPLE:
        ctx->attributes.polygonStipple = true;
        break;
    case GL_STENCIL_TEST:
        ctx->stencil.enabled = true;
        break;
    default:
        break;
    }
}

void glEnableClientState(GLenum cap)
{
  ENSURE_CTX
  ctx->clientState |= cap;
}

void glEnd()
{
    ENSURE_CTX_AND_PAINTER

    ctx->setClipPath();
    ctx->renderPrimitive();
    ctx->unsetClipPath();

    ctx->mode = ctx->stack.pop();
}

void glEndList()
{
    ENSURE_CTX
    RenderItem item = ctx->renderStack.pop();
    ctx->painter->end();
    delete ctx->painter;

    ctx->painter = item.painter;

    // Restore the transformation matrix to the one stored before
    // the list began.
    ctx->transform = ctx->listTransforms[item.list];

    if (item.mode == GL_COMPILE_AND_EXECUTE)
        glCallList(item.list);
}

void glFlush()
{
    ENSURE_CTX_AND_PAINTER
    ctx->renderPrimitive();
}

GLuint glGenLists(GLsizei range)
{
    ENSURE_CTX_INT

    // Diana only ever asks for one list at a time, so we can do something simple.
    GLuint next = 1;

    while (ctx->lists.contains(next))
        ++next;

    ctx->lists[next] = QPicture();
    ctx->listTransforms[next] = ctx->transform;
    return next;
}

void glGenTextures(GLsizei n, GLuint *textures)
{
    ENSURE_CTX

    GLuint min = 0;
    if (!ctx->textures.isEmpty()) {

        foreach (GLuint i, ctx->textures.keys())
            if (min == 0 || i < min) min = i;

        // If the requested number of textures will fit below the minimum texture
        // ID in the hash then simply use the values below the minimum.
        // Start assigning values from 1, not 0.
        if (min > (GLuint)n) {
            for (GLuint i = 1; i <= (GLuint)n; ++i) {
                ctx->textures[i] = QImage();
                textures[i - 1] = i;
            }
            return;
        }
    }

    // Find the highest ID and use values above that.
    GLuint max = min;
    foreach (GLuint i, ctx->textures.keys())
        if (i > max) max = i;

    for (GLuint i = 0; i < (GLuint)n; ++i) {
        ctx->textures[max + 1 + i] = QImage();
        textures[i] = max + i + 1;
    }
}

void glGetFloatv(GLenum pname, GLfloat *params)
{
    ENSURE_CTX
    switch (pname) {
    case GL_CURRENT_COLOR:
    {
        QRgb c = ctx->attributes.color;
        params[0] = qRed(c)/255.0;
        params[1] = qGreen(c)/255.0;
        params[2] = qBlue(c)/255.0;
        params[3] = qAlpha(c)/255.0;
        break;
    }
    case GL_LINE_WIDTH:
        params[0] = ctx->attributes.width;
        break;
    default:
        break;
    }
}

void glGetIntegerv(GLenum pname, GLint *params)
{
    ENSURE_CTX_AND_PAINTER
    switch (pname) {
    case GL_VIEWPORT:
        params[0] = ctx->painter->window().left();
        params[1] = ctx->painter->window().top();
        params[2] = ctx->painter->window().width();
        params[3] = ctx->painter->window().height();
        break;
    case GL_MAX_VIEWPORT_DIMS:
        params[0] = 65535;
        params[1] = 65535;
        break;
    default:
        break;
    }
}

void glIndexi(GLint c)
{
    /* Unimplemented - either explicitly documented as unused in Diana or
       used in code that is unlikely to be executed. */
}

GLboolean glIsEnabled(GLenum cap)
{
    switch (cap) {
    case GL_BLEND:
        return ctx->blend;
    case GL_TEXTURE_2D:
        return ctx->useTexture;
    case GL_LINE_STIPPLE:
        return ctx->attributes.lineStipple;
    case GL_MULTISAMPLE:
        return ctx->attributes.antialiasing;
    case GL_POLYGON_STIPPLE:
        return ctx->attributes.polygonStipple;
    case GL_STENCIL_TEST:
        return ctx->stencil.enabled;
    default:
        return false;
    }
}

GLboolean glIsList(GLuint list)
{
    ENSURE_CTX_INT

    if (ctx->lists.contains(list))
        return 1;
    else
        return 0;
}

void glLineStipple(GLint factor, GLushort pattern)
{
    ENSURE_CTX

    QVector<qreal> dashes;
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

    // If there is only one element then discard it and use an empty vector.
    // This will result in a solid line being plotted.
    if (dashes.size() == 1)
        dashes.clear();
    else {
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
            ctx->attributes.dashOffset = total - dashes.last();
        } else
            ctx->attributes.dashOffset = 0;
    }

    ctx->attributes.dashes = dashes;
}

void glLineWidth(GLfloat width)
{
    ENSURE_CTX_AND_PAINTER
    ctx->attributes.width = width;
}

void glLoadIdentity()
{
    ENSURE_CTX_AND_PAINTER
    ctx->transform = QTransform();
}

void glNewList(GLuint list, GLenum mode)
{
    ENSURE_CTX

    RenderItem item;
    item.mode = mode;
    item.painter = ctx->painter;
    item.list = list;
    ctx->renderStack.push(item);

    ctx->painter = new QPainter;
    ctx->painter->begin(&ctx->lists[list]);
    if (item.painter)
        ctx->painter->setRenderHints(item.painter->renderHints());

    // For display lists, reset the transformation to the identity matrix.
    // This works around issues with transformation errors in PDF, PS and
    // SVG output when rendering circles defined as display lists in ObsPlot.
    ctx->transform = QTransform();
}

void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
             GLdouble near_val, GLdouble far_val)
{
    ENSURE_CTX

    ctx->window = QRectF(left, top, right - left, bottom - top);
    ctx->setViewportTransform();
}

void glPixelStorei(GLenum pname, GLint param)
{
    ENSURE_CTX
    ctx->pixelStore[pname] = param;
}

void glPixelTransferf(GLenum pname, GLfloat param)
{
    ENSURE_CTX
    switch (pname) {
    case GL_RED_BIAS:
        ctx->attributes.bias.setRedF(param);
        ctx->attributes.biased |= (param != 0.0);
        break;
    case GL_GREEN_BIAS:
        ctx->attributes.bias.setGreenF(param);
        ctx->attributes.biased |= (param != 0.0);
        break;
    case GL_BLUE_BIAS:
        ctx->attributes.bias.setBlueF(param);
        ctx->attributes.biased |= (param != 0.0);
        break;
    case GL_ALPHA_SCALE:
        ctx->attributes.scale.setAlphaF(param);
        ctx->attributes.scaled |= (param != 1.0);
        break;
    }
}

void glPixelZoom(GLfloat xfactor, GLfloat yfactor)
{
    ENSURE_CTX
    ctx->attributes.pixelZoom = QPointF(xfactor, yfactor);
}

void glPointSize(GLfloat size)
{
    ENSURE_CTX
    ctx->pointSize = size;
}

void glPolygonMode(GLenum face, GLenum mode)
{
    ENSURE_CTX

    if (face == GL_FRONT_AND_BACK) {
        glPolygonMode(GL_FRONT, mode);
        glPolygonMode(GL_BACK, mode);
        return;
    }

    switch (mode) {
    case GL_FILL:
        ctx->attributes.polygonMode[face] = GL_FILL;
        break;
    case GL_LINE:
        ctx->attributes.polygonMode[face] = GL_LINE;
        break;
    default:
        break;
    }
}

void glPolygonStipple(const GLubyte *mask)
{
    ENSURE_CTX

    ctx->attributes.mask = QImage(mask, 32, 32, QImage::Format_Mono);
    QVector<QRgb> colours;
    colours << qRgba(0, 0, 0, 0) << ctx->attributes.color;
    ctx->attributes.mask.setColorTable(colours);
}

void glPopMatrix()
{
    ENSURE_CTX
    ctx->transform = ctx->transformStack.pop();
}

void glPopAttrib(void)
{
    ENSURE_CTX
    ctx->attributes = ctx->attributesStack.pop();
}

void glPushAttrib(GLbitfield mask)
{
    ENSURE_CTX
    if (mask & GL_LINE_BIT)
      ctx->attributesStack.push(ctx->attributes);
    else if (mask & GL_POLYGON_BIT)
      ctx->attributesStack.push(ctx->attributes);
    else if (mask & GL_COLOR_BUFFER_BIT)
      ctx->attributesStack.push(ctx->attributes);
    else if (mask & GL_CURRENT_BIT)
      ctx->attributesStack.push(ctx->attributes);
    else if (mask & GL_PIXEL_MODE_BIT)
      ctx->attributesStack.push(ctx->attributes);
}

void glPushMatrix()
{
    ENSURE_CTX
    ctx->transformStack.push(ctx->transform);
}

void glRasterPos2f(GLfloat x, GLfloat y)
{
    ENSURE_CTX
    ctx->rasterPos = ctx->transform * QPointF(x, y);
}

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                  GLenum format, GLenum type, GLvoid *pixels)
{
    /* Unimplemented - code using this function is unlikely to be used when
       this OpenGL wrapper is in use. */
}

void glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    glBegin(GL_POLYGON);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
}

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    ENSURE_CTX

    // Diana only rotates about the z axis.
    ctx->transform = ctx->transform.rotate(angle);
}

void glScalef(GLfloat x, GLfloat y, GLfloat z)
{
    Q_UNUSED(z)
    ENSURE_CTX
    ctx->transform = ctx->transform.scale(x, y);
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
}

void glShadeModel(GLenum mode)
{
    ENSURE_CTX

    switch (mode) {
    case GL_FLAT:
        ctx->smooth = false;
        break;
    case GL_SMOOTH:
    default:
        ctx->smooth = true;
        break;
    }
}

void glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
    ENSURE_CTX
    ctx->stencil.func = func;
    ctx->stencil.ref = ref;
    ctx->stencil.mask = mask;
}

void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
    ENSURE_CTX
    ctx->stencil.fail = fail;
    ctx->stencil.zfail = zfail;
    ctx->stencil.zpass = zpass;

    // Special cases for Diana. We perform a test here because glStencilOp
    // is typically called after glStencilFunc in Diana code. Both tend to
    // be called with a limited range of input parameters.
    if (ctx->stencil.func == GL_ALWAYS && ctx->stencil.ref == 1 &&
        ctx->stencil.mask == 1 && zpass == GL_REPLACE) {

        ctx->stencil.update = true;
        ctx->stencil.clip = false;

    } else if (ctx->stencil.func == GL_EQUAL && ctx->stencil.ref == 0 &&
               zpass == GL_KEEP) {

        ctx->stencil.update = false;
        ctx->stencil.clip = true;

    } else if (ctx->stencil.func == GL_NOTEQUAL && ctx->stencil.ref == 1 &&
               zpass == GL_KEEP) {

        ctx->stencil.update = false;
        ctx->stencil.clip = true;
    }
}

void glTexCoord2f(GLfloat s, GLfloat t)
{
    ENSURE_CTX
    // Unimplemented - unused in Diana.
}

void glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
}

void glTexImage2D(GLenum target, GLint level, GLint internalFormat,
                  GLsizei width, GLsizei height, GLint border,
                  GLenum format, GLenum type, const GLvoid *pixels)
{
    ENSURE_CTX

    QImage texture = QImage(width, height, QImage::Format_ARGB32);
    // Assuming type == GL_UNSIGNED_BYTE
    int length = width * height;
    texture.loadFromData((const uchar *)pixels, length);
    ctx->textures[ctx->currentTexture] = texture;
}

void glTexParameteri(GLenum target, GLenum pname, GLint param)
{
    // Unimplemented - unused in Diana.
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
    Q_UNUSED(z)
    ENSURE_CTX
    ctx->transform = ctx->transform.translate(x, y);
}

void glVertex2dv(const GLdouble *v)
{
    ENSURE_CTX
    QPointF p = ctx->transform * QPointF(v[0], v[1]);
    ctx->points.append(p);
    ctx->validPoints.append(!isnan(p.x()) && !isnan(p.y()));
    ctx->colors.append(ctx->attributes.color);
}

void glVertex2f(GLfloat x, GLfloat y)
{
    ENSURE_CTX
    QPointF p = ctx->transform * QPointF(x, y);
    ctx->points.append(p);
    ctx->validPoints.append(!isnan(p.x()) && !isnan(p.y()));
    ctx->colors.append(ctx->attributes.color);
}

void glVertex2i(GLint x, GLint y)
{
    ENSURE_CTX
    glVertex2f(x, y);
}

void glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
    Q_UNUSED(z)
    ENSURE_CTX
    QPointF p = ctx->transform * QPointF(x, y);
    ctx->points.append(p);
    ctx->validPoints.append(!isnan(p.x()) && !isnan(p.y()));
    ctx->colors.append(ctx->attributes.color);
}

void glVertex3i(GLint x, GLint y, GLint z)
{
    ENSURE_CTX
    glVertex3f(x, y, z);
}

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
  ENSURE_CTX
  ctx->vertexSize = size;
  ctx->vertexType = type;
  ctx->vertexStride = stride;
  ctx->vertexPointer = ptr;
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    ENSURE_CTX
    ctx->viewport = QRect(x, y, width, height);
    ctx->setViewportTransform();

    if (ctx->isPainting())
        ctx->painter->setClipRect(ctx->viewport);
}



bool glText::testDefineFonts(std::string path)
{
    return true;
}

bool glText::defineFonts(const std::string pattern, const std::string family, const std::string psname)
{
    return true;
}

bool glText::defineFont(const std::string font, const std::string fontfilename, const glText::FontFace, const int, const std::string, const float, const float)
{
    int handle = QFontDatabase::addApplicationFont(QString::fromStdString(fontfilename));
    if (handle == -1)
        return false;

    QStringList families = QFontDatabase::applicationFontFamilies(handle);
    if (families.isEmpty())
        return false;

    foreach (QString family, families)
        fontMap[QString::fromStdString(font)] = family;

    return true;
}

bool glText::set(const std::string name, const glText::FontFace face, const float size)
{
    ENSURE_CTX_BOOL

    setFont(name);
    setFontFace(face);
    setFontSize(size);
    return true;
}

bool glText::setFont(const std::string name)
{
    ENSURE_CTX_BOOL

    ctx->font.setFamily(fontMap[QString::fromStdString(name)]);
    ctx->font.setStyleStrategy(QFont::NoFontMerging);
    return true;
}

bool glText::setFontFace(const glText::FontFace face)
{
    ENSURE_CTX_BOOL

    if (face & 1)
        ctx->font.setWeight(QFont::Bold);
    else
        ctx->font.setWeight(QFont::Normal);
    ctx->font.setItalic((face & 2) != 0);
    return true;
}

bool glText::setFontSize(const float size)
{
    ENSURE_CTX_BOOL

    ctx->font.setPointSizeF(size);
    return true;
}

bool glText::drawChar(const int c, const float x, const float y,
                      const float a)
{
    // Use the same code as the glTextTT class.
    char s[2];
    s[0] = (char) c;
    s[1] = '\0';

    return drawStr(s, x, y, a);
}

bool glText::drawStr(const char* s, const float x, const float y,
                     const float a)
{
    ENSURE_CTX_AND_PAINTER_BOOL
    if (!ctx->colorMask) return true;

    ctx->painter->save();
    // Set the clip path, but don't unset it - the state will be restored.
    ctx->setClipPath();

    float xscale = pow(pow(ctx->transform.m11(), 2) + pow(ctx->transform.m12(), 2), 0.5);
    float yscale = pow(pow(ctx->transform.m21(), 2) + pow(ctx->transform.m22(), 2), 0.5);

    ctx->painter->setFont(ctx->font);
    QString str = QString::fromLatin1(s);
    QFontMetricsF fm(ctx->font, ctx->painter->device());
    float h = fm.boundingRect(str).height();

    // No need to record this transformation.
    ctx->painter->setTransform(ctx->transform);
    ctx->painter->setPen(QPen(ctx->attributes.color));
    ctx->painter->translate(x, y);
    ctx->painter->rotate(a);
    // Unscale the text so that it appears at the intended size.
    ctx->painter->scale(1.0/xscale, 1.0/yscale);
    // Flip it vertically to take coordinate system differences into account.
    ctx->painter->translate(0, -h/2);
    ctx->painter->setTransform(QTransform(1, 0, 0, 0, -1, 0, 0, 0, 1), true);
    ctx->painter->drawText(0, -h/2, str);
    ctx->painter->restore();
    return true;
}

bool glText::getCharSize(const int c, float& w, float& h)
{
    // Use the same code as the glTextTT class.
    char s[2];
    s[0] = (char) c;
    s[1] = '\0';

    return getStringSize(s, w, h);
}

bool glText::getMaxCharSize(float& w, float& h)
{
    // Use the same code as the glTextTT class.
    getCharSize('M', w, h);
    return true;
}

bool glText::getStringSize(const char* s, float& w, float& h)
{
    ENSURE_CTX_AND_PAINTER_BOOL

    QString str = QString::fromLatin1(s);

    float xscale = pow(pow(ctx->transform.m11(), 2) + pow(ctx->transform.m12(), 2), 0.5);
    float yscale = pow(pow(ctx->transform.m21(), 2) + pow(ctx->transform.m22(), 2), 0.5);

    QFontMetricsF fm(ctx->font, ctx->painter->device());
    //QRectF rect = ctx->transform.inverted().mapRect(QRectF(0, 0, fm.width(s), fm.height()));
    QRectF rect = fm.boundingRect(str);
    w = rect.width() / xscale;
    h = rect.height() * 0.8 / yscale;
    if (w == 0 || str.trimmed().isEmpty())
        h = 0;

    return true;
}

PaintGLWidget::PaintGLWidget(QWidget *parent, bool antialiasing)
    : QWidget(parent), initialized(false), antialiasing(antialiasing)
{
    glContext = new PaintGLContext();
}

PaintGLWidget::~PaintGLWidget()
{
    delete glContext;
}

QImage PaintGLWidget::grabFrameBuffer(bool withAlpha)
{
    makeCurrent();
    if (!initialized) {
        initializeGL();
        initialized = true;
    }

    QImage image;
    if (withAlpha)
        image = QImage(size(), QImage::Format_ARGB32);
    else
        image = QImage(size(), QImage::Format_RGB32);

    QPainter painter;
    painter.begin(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    paint(&painter);
    painter.end();

    return image;
}

bool PaintGLWidget::isValid()
{
    return true;
}

void PaintGLWidget::makeCurrent()
{
    ctx = glContext;
}

void PaintGLWidget::swapBuffers()
{
    if (!glContext->isPainting())
        update();
}

void PaintGLWidget::updateGL()
{
    update();
}

/**
  * This function is intended to be implemented in a subclass.
  */
void PaintGLWidget::initializeGL()
{
}

/**
  * This function is intended to be implemented in a subclass.
  */
void PaintGLWidget::paintGL()
{
}

/**
  * This function is intended to be implemented in a subclass.
  */
void PaintGLWidget::resizeGL(int width, int height)
{
}

void PaintGLWidget::paintEvent(QPaintEvent* event)
{
    makeCurrent();
    if (!initialized) {
        initializeGL();
        initialized = true;
    }

    QPainter painter;
    painter.begin(this);
    glContext->makeCurrent();
    glContext->begin(&painter);
    if (antialiasing)
        painter.setRenderHint(QPainter::Antialiasing);

    paintGL();

    glContext->end();
    painter.end();
}

void PaintGLWidget::resizeEvent(QResizeEvent* event)
{
    makeCurrent();

    int w = event->size().width();
    int h = event->size().height();

    resizeGL(w, h);
}

void PaintGLWidget::setAutoBufferSwap(bool enable)
{
    Q_UNUSED(enable)
}

void PaintGLWidget::paint(QPainter *painter)
{
    glContext->makeCurrent();
    glContext->begin(painter);
    paintGL();
    glContext->end();
}

void PaintGLWidget::print(QPrinter* device)
{
  makeCurrent();
  if (!initialized) {
      initializeGL();
      initialized = true;
  }

  QPicture picture;
  QPainter painter;
  painter.begin(&picture);
  paint(&painter);
  painter.end();

  painter.begin(device);
  painter.translate(device->width()/2.0, device->height()/2.0);
  double scale = qMin(device->width()/double(width()), device->height()/double(height()));
  if (scale < 1.0)
    painter.scale(scale, scale);
  painter.translate(-width()/2, -height()/2);
  painter.setClipRect(0, 0, width(), height());
  painter.drawPicture(0, 0, picture);
  painter.end();
}

void PaintGLWidget::renderText(int x, int y, const QString &str, const QFont &font, int listBase)
{
  glPushMatrix();
  glContext->transform = QTransform();
  glContext->painter->drawText(x, y, str);
  glPopMatrix();
}

QImage PaintGLWidget::convertToGLFormat(const QImage &image)
{
  return image.transformed(QTransform().scale(1, -1)).rgbSwapped();
}

