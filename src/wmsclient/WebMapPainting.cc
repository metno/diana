/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015 MET Norway

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

#include "WebMapPainting.h"

#include "diColour.h"
#include "diGLPainter.h"

#include <QImage>

#define MILOGGER_CATEGORY "diana.WebMapPainting"
#include <miLogger/miLogging.h>

namespace diutil {

PixelData::~PixelData()
{
}

namespace /* anonymous */ {
void glVertexPosition(DiGLPainter* gl, const PixelData& pixels, int ix, int iy)
{
  float x, y;
  pixels.position(ix, iy, x, y);
  gl->Vertex2f(x, y);
}
} // anonymous namespace

void drawFillCell(DiGLPainter* gl, const PixelData& pixels)
{
  METLIBS_LOG_TIME();
  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
  gl->Enable(DiGLPainter::gl_BLEND);
  gl->BlendFunc(DiGLPainter::gl_SRC_ALPHA, DiGLPainter::gl_ONE_MINUS_SRC_ALPHA);
  gl->ShadeModel(DiGLPainter::gl_FLAT);

  Colour c;
  const Colour transparent(0, 0, 0, Colour::maxv);

  for (int iy = 0; iy < pixels.height(); ++iy) {
    gl->Begin(DiGLPainter::gl_QUAD_STRIP);
    glVertexPosition(gl, pixels, 0, iy);
    glVertexPosition(gl, pixels, 0, iy+1);

    for (int ix = 0; ix < pixels.width(); ++ix) {
      if (pixels.colour(ix, iy, c))
        gl->Color4ubv(c.RGBA());
      else
        gl->Color4ubv(transparent.RGBA());

      glVertexPosition(gl, pixels, ix+1, iy);
      glVertexPosition(gl, pixels, ix+1, iy+1);
    }
    gl->End();
  }
  gl->Disable(DiGLPainter::gl_BLEND);
}

// ========================================================================

QImageData::QImageData(const QImage* i, const float* vX, const float* vY)
  : image(i)
  , viewX(vX)
  , viewY(vY)
  , mColourTransform(0)
{
}

QImageData::~QImageData()
{
}

void QImageData::setColourTransform(ColourTransform_cx ct)
{
  mColourTransform = ct;
}

int QImageData::width() const
{
  return image->width();
}

int QImageData::height() const
{
  return image->height();
}

bool QImageData::colour(int ix, int iy, Colour& colour) const
{
  const QRgb pixel = image->pixel(ix, iy);
  colour.set(qRed(pixel), qGreen(pixel), qBlue(pixel), qAlpha(pixel));
  if (mColourTransform)
    mColourTransform->transform(colour);
  return true;
}

// ========================================================================

void SimpleColourTransform::transform(Colour& c) const
{
  const bool changeAlpha = (mAlphaOffset != 0 || mAlphaScale != 1);
  if (!mGrey && !changeAlpha)
    return;

  float r = c.fR();
  float g = c.fG();
  float b = c.fB();
  float a = c.fA();

  if (mGrey) {
    float avg = (r + b + g) / 3;
    r = b = g = avg;
  }
  if (changeAlpha)
    a = mAlphaOffset + a*mAlphaScale;
  c.setF(r, g, b, a);
}

static Qt::ImageConversionFlags icf = Qt::DiffuseAlphaDither | Qt::NoOpaqueDetection;

void ensureImageFormat(QImage& image, QImage::Format format)
{
  if (image.format() != format)
    image = image.convertToFormat(format, icf);
}

QImage convertImage(QImage image, float alpha_offset, float alpha_scale, bool make_grey)
{
  const bool change_alpha = (alpha_offset != 0 || alpha_scale != 1);
  if (change_alpha || make_grey) {
    ensureImageFormat(image, QImage::Format_ARGB32);
    for (int i=0; i<image.height(); ++i) {
      QRgb* rgb = reinterpret_cast<QRgb*>(image.scanLine(i));
      for (int j=0; j<image.width(); ++j) {
        QRgb& p = rgb[j];
        int r = qRed(p), g = qGreen(p), b = qBlue(p), a = qAlpha(p);
        if (make_grey)
          r = g = b = (r+g+b)/3;
        if (change_alpha)
          a = alpha_offset + alpha_scale*a;
        p = qRgba(r, g, b, a);
      }
    }
  }

  ensureImageFormat(image, QImage::Format_ARGB32_Premultiplied);
  return image;
}

} // namespace diutil
