/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2018 met.no

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

#include "diana_config.h"

#include "diVprofPainter.h"

#include "diGlUtilities.h"
#include "diLinetype.h"
#include "diUtilities.h"
#include "diField/diPoint.h"

#include <mi_fieldcalc/math_util.h>

using diutil::PointF;

#define MILOGGER_CATEGORY "diana.VprofPainter"
#include <miLogger/miLogging.h>

VprofPainter::VprofPainter(DiGLPainter* GL)
    : gl(GL)
{
  METLIBS_LOG_SCOPE();
}

VprofPainter::~VprofPainter()
{
}

void VprofPainter::init(const Colour& clearcolour, const Rectangle& diagramsize, const PointF& plotsize)
{
  gl->clear(clearcolour);
  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_LINE);

  gl->Enable(DiGLPainter::gl_BLEND);
  gl->BlendFunc(DiGLPainter::gl_SRC_ALPHA, DiGLPainter::gl_ONE_MINUS_SRC_ALPHA);

  const float xscale = plotsize.x() / diagramsize.width();
  const float yscale = plotsize.y() / diagramsize.height();
  full = Rectangle(-diagramsize.x1 * xscale, -diagramsize.y1 * yscale, xscale, yscale);

  gl->LoadIdentity();
  gl->Ortho(0, plotsize.x(), 0, plotsize.y(), -1, 1);
}

void VprofPainter::setColour(const Colour& colour)
{
  gl->setColour(colour, true);
}

void VprofPainter::setLineStyle(const Colour& c, float lw)
{
  gl->setLineStyle(c, lw, true);
}

void VprofPainter::setLineStyle(const Colour& c, float lw, const Linetype& lt)
{
  gl->setLineStyle(c, lw, lt, true);
}

void VprofPainter::setLineStyle(const Linestyle& linestyle)
{
  setLineStyle(linestyle.colour, linestyle.linewidth, linestyle.linetype);
}

void VprofPainter::enableLineStipple(bool stipple)
{
  setLineStipple(stipple ? 0xFFC0 : 0xFFFF);
}

void VprofPainter::setLineStipple(short unsigned int stipple)
{
  if (stipple == 0xFFFF) {
    gl->Disable(DiGLPainter::gl_LINE_STIPPLE);
  } else {
    gl->Enable(DiGLPainter::gl_LINE_STIPPLE);
    gl->LineStipple(1, stipple);
  }
}

void VprofPainter::drawLine(const diutil::PointF& p1, const diutil::PointF& p2)
{
  float x1, y1, x2, y2;
  to_pixel(p1).unpack(x1, y1);
  to_pixel(p2).unpack(x2, y2);
  gl->drawLine(x1, y1, x2, y2);
}

void VprofPainter::drawRect(bool fill, const Rectangle& r)
{
  float x1, y1, x2, y2;
  to_pixel(PointF(r.x1, r.y1)).unpack(x1, y1);
  to_pixel(PointF(r.x2, r.y2)).unpack(x2, y2);
  gl->drawRect(fill, x1, y1, x2, y2);
}

void VprofPainter::drawArrow(const diutil::PointF& from, const diutil::PointF& to)
{
  float xa1, ya1, xa2, ya2;
  to_pixel(from).unpack(xa1, ya1);
  to_pixel(to).unpack(xa2, ya2);
  gl->drawArrow(xa1, ya1, xa2, ya2);
}

void VprofPainter::drawWindArrow(const diutil::PointF& uv_knots, const diutil::PointF& xy, float arrowSize, bool withArrowHead)
{
  float wx, wy;
  to_pixel(xy).unpack(wx, wy);
  gl->drawWindArrow(uv_knots.x(), uv_knots.y(), wx, wy, arrowSize * full.x2, withArrowHead);
}

void VprofPainter::drawText(const std::string& text, float x, float y, float angle)
{
  to_pixel(PointF(x, y)).unpack(x, y);
  gl->drawText(text, x, y, angle);
}

void VprofPainter::fpInitStr(const std::string& str, float x, float y, float angle, float size, const Colour& c, Alignment align, Font font)
{
  METLIBS_LOG_SCOPE();

  fpStrInfo strInfo;
  strInfo.str = str;
  strInfo.x = x;
  strInfo.y = y;
  strInfo.angle = angle;
  strInfo.c = c;
  strInfo.size = size;
  strInfo.align = align;
  strInfo.font = font;
  fpStr.push_back(strInfo);
}

void VprofPainter::fpDrawStr()
{
  METLIBS_LOG_SCOPE();
  gl->setFont(diutil::SCALEFONT);

  for (fpStrInfo& s : fpStr) {
    setFontsize(s.size);
    gl->setColour(s.c, false);
    if (s.align != ALIGN_LEFT) {
      const float w = getTextWidth(s.str);
      if (s.align == ALIGN_RIGHT)
        s.x -= w;
      else if (s.align == ALIGN_CENTER)
        s.x -= w * 0.5;
    }
    drawText(s.str, s.x, s.y, s.angle);
  }
  fpStr.clear();
}

float VprofPainter::getTextWidth(const std::string& text) const
{
  float w, h;
  gl->getTextSize(text, w, h);
  w /= full.x2;
  return w;
}

void VprofPainter::setFontsize(float chy)
{
  chy *= 0.8;
  const float fs = miutil::constrain_value(chy * full.y2, 5.0f, 35.0f);
  gl->setFontSize(fs);
}

PointF VprofPainter::to_pixel(const diutil::PointF& xy) const
{
  return PointF(full.x1 + xy.x() * full.x2, full.y1 + xy.y() * full.y2);
}

void VprofPainter::drawClipped(const std::vector<diutil::PointF>& canvas, const Rectangle& area)
{
  std::vector<float> x, y;
  x.reserve(canvas.size());
  y.reserve(canvas.size());
  for (const PointF& c : canvas) {
    const PointF pixel = to_pixel(c);
    x.push_back(pixel.x());
    y.push_back(pixel.y());
  }
  float clip[4];
  to_pixel(PointF(area.x1, area.y1)).unpack(clip[0], clip[2]);
  to_pixel(PointF(area.x2, area.y2)).unpack(clip[1], clip[3]);
  diutil::xyclip(x.size(), &x[0], &y[0], clip, gl);
}
