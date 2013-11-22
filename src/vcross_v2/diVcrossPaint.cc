
#include "diVcrossPaint.h"

#include <GL/gl.h>
#if !defined(USE_PAINTGL)
#include <glText/glText.h>
#endif

#include <cmath>
#include <vector>

#define MILOGGER_CATEGORY "diana.VcrossPaint"
#include <miLogger/miLogging.h>

// struct PixelPoint {
//   float x;
//   float y;
//   PixelPoint(float xx, float yy)
//     : x(xx), y(yy) { }
//   PixelPoint()
//     : x(-1), y(-1) { }
//   bool valid() const
//     { return x>=0 and y>=0; }
// };
// 
// typedef PixelPoint DataPoint;
// 
// class PaintAxis {
// public:
//   virtual ~PaintAxis();
//   virtual DataPoint pixel2data(const PixelPoint& p) const = 0;
//   virtual PixelPoint data2pixel(const DataPoint& d) const = 0;
// };

PaintWindArrow::PaintWindArrow()
  : mWithArrowHead(true)
  , mSize(20)
{
}

void PaintWindArrow::paint(float u, float v, float gx, float gy) const
{
  // step and size for flags
  const float fStep = mSize / 10.0, fSize = mSize * 0.35;
  const float KN5_SCALE = 0.6;

  const float ff = sqrtf(u*u + v*v);
  if (ff <= 0.00001 or isnan(ff))
    return;
  
  // find no. of 50, 10 and 5 knot flags
  int n50 = 0, n10 = 0, n05 = 0;
  if (ff < 182.49) {
    n05 = int(ff * 0.2 + 0.5);
    n50 = n05 / 10;
    n05 -= n50 * 10;
    n10 = n05 / 2;
    n05 -= n10 * 2;
  } else if (ff < 190.) {
    n50 = 3;
    n10 = 3;
  } else if (ff < 205.) {
    n50 = 4;
  } else if (ff < 225.) {
    n50 = 4;
    n10 = 1;
  } else {
    n50 = 5;
  }

  const float unitX = u / ff,
      unitY = -v / ff; // -v because v is up, y coordinate increases down

  const float flagDX = fStep * unitX, flagDY = fStep * unitY;
  const float flagEndDX = fSize * unitY - flagDX, flagEndDY = -fSize * unitX - flagDY;

  std::vector<float> trianglePointsX, trianglePointsY;
  if (mWithArrowHead) {
    const float a = -1.5, s = a * 0.5;
    trianglePointsX.push_back(gx);
    trianglePointsY.push_back(gy);
    trianglePointsX.push_back(gx + a*flagDX + s*flagDY);
    trianglePointsY.push_back(gy + a*flagDY - s*flagDX);
    trianglePointsX.push_back(gx + a*flagDX - s*flagDY);
    trianglePointsY.push_back(gy + a*flagDY + s*flagDX);
  }
 
  // direction
  glBegin(GL_LINES);
  glVertex2f(gx, gy);
  // move to end of arrow
  gx -= mSize * unitX;
  gy -= mSize * unitY;
  glVertex2f(gx, gy);

  // 50-knot trianglePoints, store for plot below
  if (n50 > 0) {
    for (int n = 0; n < n50; n++) {
      trianglePointsX.push_back(gx);
      trianglePointsY.push_back(gy);
      gx += flagDX * 2.;
      gy += flagDY * 2.;
      trianglePointsX.push_back(gx + flagEndDX);
      trianglePointsY.push_back(gy + flagEndDY);
      trianglePointsX.push_back(gx);
      trianglePointsY.push_back(gy);
      // no gaps between 50-knot triangles
    }
    // add gap
    gx += flagDX;
    gy += flagDY;
  }

  // 10-knot lines
  for (int n = 0; n < n10; n++) {
    glVertex2f(gx, gy);
    glVertex2f(gx + flagEndDX, gy + flagEndDY);
    // add gap
    gx += flagDX;
    gy += flagDY;
  }

  // 5-knot lines
  if (n05 > 0) {
    if (n50 + n10 == 0) {
      // if only 5 knot line, add gap at end of arrow
      gx += flagDX;
      gy += flagDY;
    }
    glVertex2f(gx, gy);
    glVertex2f(gx + KN5_SCALE * flagEndDX, gy + KN5_SCALE * flagEndDY);
  }
  glEnd();

  // draw trinagles
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  const int nTrianglePoints = trianglePointsX.size();
  if (nTrianglePoints >= 3) {
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < nTrianglePoints; i += 3) {
      glVertex2f(trianglePointsX[i],     trianglePointsY[i]);
      glVertex2f(trianglePointsX[i + 1], trianglePointsY[i + 1]);
      glVertex2f(trianglePointsX[i + 2], trianglePointsY[i + 2]);
    }
    glEnd();
  }
}

// ########################################################################

PaintVector::PaintVector()
  : mWithArrowHead(true)
  , mScaleX(0.1)
  , mScaleY(mScaleX)
{
  METLIBS_LOG_SCOPE();
  glBegin(GL_LINES);
}

// ------------------------------------------------------------------------

PaintVector::~PaintVector()
{
  METLIBS_LOG_SCOPE();
  glEnd();
}

// ------------------------------------------------------------------------

void PaintVector::paint(float u, float v, float px, float py) const
{
  const float ff = sqrtf(u*u + v*v);
  if (ff <= 0.00001 or isnan(ff))
    return;
  
  // direction
  const float dx = mScaleX * u, dy = mScaleY * v;
  glVertex2f(px, py);
  px += dx;
  py += dy;
  glVertex2f(px, py);
  
  if (mWithArrowHead) {
    // arrow (drawn as two lines)
    const float a = -1/3., s = a / 2;
    glVertex2f(px, py);
    glVertex2f(px + a*dx + s*dy, py + a*dy - s*dx);
    glVertex2f(px, py);
    glVertex2f(px + a*dx - s*dy, py + a*dy + s*dx);
  }
}
