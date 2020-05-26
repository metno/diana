/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2014-2020 met.no

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

#ifndef diGlUtilities_h
#define diGlUtilities_h 1

#include "diPoint.h"

#include <QPolygonF>

#include <string>
#include <vector>

class Plot;
class DiGLPainter;
class DiPainter;

namespace diutil {

enum MapValuePosition {
  map_none, map_left, map_bottom, map_right, map_top, map_all
};

//! convert position "bottom", left" etc to MapValuePosition
MapValuePosition mapValuePositionFromText(const std::string& p);

/// Lat/Lon Value Annotation with position on map
struct MapValueAnno {
  std::string t;
  float x;
  float y;
  MapValueAnno(const std::string& tt, float xx, float yy)
    : t(tt), x(xx), y(yy) { }
};
typedef std::vector<MapValueAnno> MapValueAnno_v;

inline size_t index(int width, int ix, int iy)
{
  return iy * width + ix;
}

bool is_undefined(float v);

inline bool is_undefined(const XY& xy)
{ return is_undefined(xy.x()) || is_undefined(xy.y()); }

inline bool is_undefined(const QPointF& xy)
{ return is_undefined(xy.x()) || is_undefined(xy.y()); }

class PolylineBuilder
{
public:
  PolylineBuilder() {}
  void reserve(size_t s)
    { mPolyline.reserve(s); }

  void add(float vx, float vy) { mPolyline << QPointF(vx, vy); }

  void add(const float* x, const float* y, size_t idx) { add(x[idx], y[idx]); }

  void addValid(float vx, float vy);

  void addValid(const float* x, const float* y, size_t idx) { addValid(x[idx], y[idx]); }

  void clear();

  const QPolygonF& polyline() const { return mPolyline; }

private:
  QPolygonF mPolyline;
};

class PolylinePainter : public PolylineBuilder
{
public:
  PolylinePainter(DiPainter* p = nullptr)
      : mPainter(p) { }

  void draw();

private:
  DiPainter* mPainter;
};

void xyclip(int npos, const float *x, const float *y, const float xylim[4],
    MapValuePosition anno_position, const std::string& anno,
    MapValueAnno_v& anno_positions, DiGLPainter* gl);

void xyclip(int npos, const float *x, const float *y, const float xylim[4],
    DiGLPainter* gl);

class GlMatrixPushPop {
public:
  GlMatrixPushPop(DiGLPainter* gl)
    : mGL(gl)
    { PushMatrix(); }

  void PopMatrix();

  ~GlMatrixPushPop()
    { PopMatrix(); }

private:
  void PushMatrix();

  DiGLPainter* mGL;
};

} // namespace diutil

#endif // diGlUtilities_h
