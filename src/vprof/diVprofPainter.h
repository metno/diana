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

#ifndef VPROFPAINTER_H
#define VPROFPAINTER_H

#include "diGLPainter.h"
#include "diLinestyle.h"
#include "diVprofBox.h"

#include <vector>

/**
   \brief Plots the Vertical Profile diagram background without data
*/
class VprofPainter
{
public:
  enum Alignment { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT };
  enum Font { FONT_DEFAULT, FONT_SCALED };

private:
  struct fpStrInfo
  {
    std::string str;
    float x, y, angle;
    float size;
    Colour c;
    Alignment align;
    Font font;
  };

public:
  VprofPainter(DiGLPainter* gl);
  ~VprofPainter();

  void drawClipped(const std::vector<diutil::PointF>& canvas, const Rectangle& area);

  void drawLine(const diutil::PointF& p1, const diutil::PointF& p2);
  void drawLine(float x1, float y1, float x2, float y2) { drawLine(diutil::PointF(x1, y1), diutil::PointF(x2, y2)); }

  void drawRect(bool fill, const Rectangle& r);

  void drawArrow(const diutil::PointF& from, const diutil::PointF& to);
  void drawWindArrow(const diutil::PointF& uv_knots, const diutil::PointF& xy, float arrowSize, bool withArrowHead);

  void fpInitStr(const std::string& str, float x, float y, float angle, float size, const Colour& c, Alignment align = ALIGN_LEFT, Font font = FONT_DEFAULT);
  void fpDrawStr();

  float getTextWidth(const std::string& text) const;
  void setFontsize(float chy);
  void drawText(const std::string& text, float x, float y, float angle = 0);

  /** Clear and setup.
   * \param clearcolour background clear colour
   * \param diagramsize size of diagram
   * \param plotsize size of plot area
   */
  void init(const Colour& clearcolour, const Rectangle& diagramsize, const diutil::PointF& plotsize);

  void setColour(const Colour& colour);
  void setLineStyle(const Colour& colour, float linewidth);
  void setLineStyle(const Colour& colour, float linewidth, const Linetype& lt);
  void setLineStyle(const Linestyle& linestyle);
  void setLineStipple(short unsigned int stipple);
  void enableLineStipple(bool stipple);

private:
  diutil::PointF to_pixel(const diutil::PointF& xy) const;
  diutil::PointF to_pixel(float x, float y) const { return to_pixel(diutil::PointF(x, y)); }

private:
  DiGLPainter* gl;
  Rectangle full;
  std::vector<fpStrInfo> fpStr;
};

#endif // VPROFPAINTER_H
