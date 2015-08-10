
#include "diGlUtilities.h"

#include "diGLPainter.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.GlUtilities"
#include <miLogger/miLogging.h>

namespace diutil {

MapValuePosition mapValuePositionFromText(const std::string& p)
{
  const std::string pos = miutil::to_lower(p);
  if (pos == "left")
    return diutil::map_left;
  if (pos == "bottom")
    return diutil::map_bottom;
  if (pos == "right")
    return diutil::map_right;
  if (pos == "top")
    return diutil::map_top;
  if (pos == "both")
    return diutil::map_all;

  //obsolete syntax
  return (MapValuePosition)(atoi(pos.c_str()) + 1); // +1: MapValuePosition has map_none first
}

// ========================================================================

bool is_undefined(float v)
{
  const float LIMIT = 1e20;
  return v == HUGE_VAL || isnan(v) || fabsf(v) >= LIMIT || isinf(v);
}

PolylinePainter& PolylinePainter::addValid(float vx, float vy)
{
  if (!is_undefined(vx) && !is_undefined(vy))
    add(vx, vy);
  return *this;
}

void PolylinePainter::draw()
{
  mPainter->drawPolyline(mPolyline);
  mPolyline.clear();
}

void xyclip(int npos, const float *x, const float *y, const float xylim[4],
    MapValuePosition anno_position, const std::string& anno,
    MapValueAnno_v& anno_positions, DiGLPainter* gl)
{
  // ploting part(s) of the continuous line that is within the given
  // area, also the segments between 'neighboring point' both of which are
  // Outside the area.
  // (Color, line type and thickness must be set in advance)
  //
  // Graphics: OpenGL
  //
  //  input:
  //  ------
  //  x(npos),y(npos): line with 'npos' points (npos>1)
  //  xylim(1-4):      x1,x2,y1,y2 limits of given area

  PolylinePainter polyline(gl);

  int nint, nc, n, i, k1, k2;
  float xa, xb, ya, yb, x1, x2, y1, y2;
  float xc[4], yc[4];

  if (npos < 2)
    return;

  xa = xylim[0];
  xb = xylim[1];
  ya = xylim[2];
  yb = xylim[3];

  float xoffset = (xb - xa) / 200.0;
  float yoffset = (yb - ya) / 200.0;

  float xx = 0, yy = 0;
  if (x[0] < xa || x[0] > xb || y[0] < ya || y[0] > yb) {
    k2 = 0;
  } else {
    k2 = 1;
    nint = 0;
    xx = x[0];
    yy = y[0];
  }

  for (n = 1; n < npos; ++n) {
    k1 = k2;
    k2 = 1;

    if (x[n] < xa || x[n] > xb || y[n] < ya || y[n] > yb)
      k2 = 0;

    // check if 'n' and 'n-1' are inside area
    if (k1 + k2 == 2)
      continue;

    // k1+k2=1: point 'n' or 'n-1' are outside
    // k1+k2=0: checks if two neighboring points which are both outside the area
    //          still has a part of the line within the area.

    x1 = x[n - 1];
    y1 = y[n - 1];
    x2 = x[n];
    y2 = y[n];

    // checking if the 'n-1' and 'n' is outside at the same side
    if (k1 + k2 == 0 && ((x1 < xa && x2 < xa) || (x1 > xb && x2 > xb) || (y1
        < ya && y2 < ya) || (y1 > yb && y2 > yb)))
      continue;

    // checks all intersection possibilities
    nc = -1;
    if (x1 != x2) {
      nc++;
      xc[nc] = xa;
      yc[nc] = y1 + (y2 - y1) * (xa - x1) / (x2 - x1);
      if (yc[nc] < ya || yc[nc] > yb || (xa - x1) * (xa - x2) > 0.)
        nc--;
      nc++;
      xc[nc] = xb;
      yc[nc] = y1 + (y2 - y1) * (xb - x1) / (x2 - x1);
      if (yc[nc] < ya || yc[nc] > yb || (xb - x1) * (xb - x2) > 0.)
        nc--;
    }
    if (y1 != y2) {
      nc++;
      yc[nc] = ya;
      xc[nc] = x1 + (x2 - x1) * (ya - y1) / (y2 - y1);
      if (xc[nc] < xa || xc[nc] > xb || (ya - y1) * (ya - y2) > 0.)
        nc--;
      nc++;
      yc[nc] = yb;
      xc[nc] = x1 + (x2 - x1) * (yb - y1) / (y2 - y1);
      if (xc[nc] < xa || xc[nc] > xb || (yb - y1) * (yb - y2) > 0.)
        nc--;
    }

    if (k2 == 1) {
      // first point at a segment within the area
      nint = n - 1;
      xx = xc[0];
      yy = yc[0];
      if ((anno_position == map_all) ||
          (anno_position == map_left   && fabsf(xx - xa) < 0.001) ||
          (anno_position == map_bottom && fabsf(yy - ya) < 0.001))
      {
        anno_positions.push_back(MapValueAnno(anno, xx + xoffset, yy + yoffset));
      }
    } else if (k1 == 1) {
      // last point at a segment within the area
      polyline.reserve(n - nint - 1);
      polyline.addValid(xx, yy);
      for (i = nint + 1; i < n; i++)
        polyline.addValid(x[i], y[i]);
      polyline.addValid(xc[0], yc[0]);
      polyline.draw();
    } else if (nc > 0) {
      // two 'neighboring points' outside the area, but part of the line within
      polyline.addValid(xc[0], yc[0]);
      polyline.addValid(xc[1], yc[1]);
      polyline.draw();
    }
  }

  if (k2 == 1) {
    // last point is within the area
    polyline.reserve(n - nint - 1);
    polyline.addValid(xx, yy);
    for (i = nint + 1; i < npos; i++)
      polyline.addValid(x[i], y[i]);
    polyline.draw();
  }
}

void xyclip(int npos, const float *x, const float *y, const float xylim[4],
    DiGLPainter* gl)
{
  MapValueAnno_v dummy;
  xyclip(npos, x, y, xylim, map_none, std::string(), dummy, gl);
}

} // namespace diutil
