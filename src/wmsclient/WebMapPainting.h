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

#ifndef WebMapPainting_h
#define WebMapPainting_h 1

class Colour;
class QImage;
class DiGLPainter;

namespace diutil {

class PixelData {
public:
  virtual ~PixelData();

  virtual int width() const = 0;
  virtual int height() const = 0;

  virtual bool colour(int ix, int iy, Colour& colour) const = 0;
  virtual void position(int ix, int iy, float&x, float& y) const = 0;
};

void drawFillCell(DiGLPainter* gl, const PixelData& pixels);

// ========================================================================

class ColourTransform {
public:
  virtual ~ColourTransform() { }

  virtual void transform(Colour& c) const = 0;
};

typedef ColourTransform* ColourTransform_x;
typedef const ColourTransform* ColourTransform_cx;

// ========================================================================

class QImageData : public PixelData
{
public:
  QImageData(const QImage* image, const float* vX, const float* vY);
  ~QImageData();

  int width() const;

  int height() const;

  bool colour(int ix, int iy, Colour& colour) const;

  void position(int ix, int iy, float&x, float& y) const
    { const int idx = (width()+1)*iy + ix; x = viewX[idx]; y = viewY[idx]; }

  void setColourTransform(ColourTransform_cx ct);

private:
  const QImage* image;
  const float* viewX;
  const float* viewY;
  ColourTransform_cx mColourTransform;
};

// ========================================================================

class SimpleColourTransform : public ColourTransform
{
public:
  SimpleColourTransform()
    : mAlphaOffset(0), mAlphaScale(1), mGrey(false) { }

  SimpleColourTransform(float alpha_offset, float alpha_scale, bool make_grey)
    : mAlphaOffset(alpha_offset), mAlphaScale(alpha_scale), mGrey(make_grey) { }

  void setStyleAlpha(float offset, float scale)
    { mAlphaOffset = offset; mAlphaScale = scale; }

  void setStyleGrey(bool makeGrey)
    { mGrey = makeGrey; }

  void transform(Colour& c) const;

private:
  float mAlphaOffset;
  float mAlphaScale;
  bool mGrey;
};

QImage convertImage(QImage image, float alpha_offset, float alpha_scale, bool make_grey);

} // namespace diutil

#endif // WebMapPainting_h
