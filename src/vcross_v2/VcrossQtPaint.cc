
#include "VcrossQtPaint.h"

#include <diField/VcrossUtil.h>

#include <QtGui/QPainter>

#include <cmath>
#include <vector>

#define MILOGGER_CATEGORY "diana.VcrossPaint"
#include <miLogger/miLogging.h>

namespace vcross {

PaintWindArrow::PaintWindArrow()
  : mWithArrowHead(true)
  , mSize(20)
{
}

void PaintWindArrow::paint(QPainter& painter, float u, float v, float gx, float gy) const
{
  // step and size for flags
  const float fStep = mSize / 10.0, fSize = mSize * 0.35;
  const float KN5_SCALE = 0.6;

  const float ff = sqrtf(u*u + v*v);
  if (ff <= 0.00001 or isnan(ff))
    return;
  
  const util::WindArrowFeathers waf = util::countFeathers(ff);

  const float unitX = u / ff,
      unitY = -v / ff; // -v because v is up, y coordinate increases down

  const float flagDX = fStep * unitX, flagDY = fStep * unitY;
  const float flagEndDX = fSize * unitY - flagDX, flagEndDY = -fSize * unitX - flagDY;

  std::vector<QPointF> trianglePoints;
  if (mWithArrowHead) {
    const float a = -1.5, s = a * 0.5;
    trianglePoints.push_back(QPointF(gx, gy));
    trianglePoints.push_back(QPointF(gx + a*flagDX + s*flagDY, gy + a*flagDY - s*flagDX));
    trianglePoints.push_back(QPointF(gx + a*flagDX - s*flagDY, gy + a*flagDY + s*flagDX));
  }

  QVector<QLineF> lines;
 
  // direction
  QPointF p1(gx, gy);
  // move to end of arrow
  gx -= mSize * unitX;
  gy -= mSize * unitY;
  QPointF p2(gx, gy);
  lines.push_back(QLineF(p1, p2));

  // 50-knot trianglePoints, store for plot below
  if (waf.n50 > 0) {
    for (int n = 0; n < waf.n50; n++) {
      trianglePoints.push_back(QPointF(gx, gy));
      gx += flagDX * 2.;
      gy += flagDY * 2.;
      trianglePoints.push_back(QPointF(gx + flagEndDX, gy + flagEndDY));
      trianglePoints.push_back(QPointF(gx, gy));
      // no gaps between 50-knot triangles
    }
    // add gap
    gx += flagDX;
    gy += flagDY;
  }

  // 10-knot lines
  for (int n = 0; n < waf.n10; n++) {
    lines.push_back(QLineF(gx, gy, gx + flagEndDX, gy + flagEndDY));
    // add gap
    gx += flagDX;
    gy += flagDY;
  }

  // 5-knot lines
  if (waf.n05 > 0) {
    if (waf.n50 == 0 and waf.n10 == 0) {
      // if only 5 knot line, add gap at end of arrow
      gx += flagDX;
      gy += flagDY;
    }
    lines.push_back(QLineF(gx, gy, gx + KN5_SCALE * flagEndDX, gy + KN5_SCALE * flagEndDY));
  }
  painter.drawLines(lines);

  // draw triangles
  const int nTrianglePoints = trianglePoints.size();
  if (nTrianglePoints >= 3) {
    for (int i = 0; i < nTrianglePoints; i += 3) {
      const QPointF* triangle = &trianglePoints[i];
      painter.drawPolygon(triangle, 3);
    }
  }
}

// ########################################################################

PaintVector::PaintVector()
  : mScaleX(1)
  , mScaleY(mScaleX)
  , mThickArrowScale(0.1)
{
  METLIBS_LOG_SCOPE();
}

// ------------------------------------------------------------------------

PaintVector::~PaintVector()
{
  METLIBS_LOG_SCOPE();
}

// ------------------------------------------------------------------------

// static
void PaintVector::paintArrow(QPainter& painter, float px, float py, float ex, float ey)
{
  // direction
  painter.drawLine(px, py, ex, ey);

  // arrow (drawn as two lines)
  const float dx = ex - px, dy = ey - py;
  const float a = -1/3., s = a / 2;
  painter.drawLine(ex, ey, ex + a*dx + s*dy, ey + a*dy - s*dx);
  painter.drawLine(ex, ey, ex + a*dx - s*dy, ey + a*dy + s*dx);
}

// ------------------------------------------------------------------------

void PaintVector::paint(QPainter& painter, float u, float v, float px, float py) const
{
  const float ff = sqrtf(u*u + v*v);
  if (ff <= 0.00001 or isnan(ff))
    return;

  QPen pen = painter.pen();
  if (mThickArrowScale > 0) {
    QPen pen2(pen);
    pen2.setWidthF(ff * mThickArrowScale);
    painter.setPen(pen2);
  }

  paintArrow(painter, px, py, px + mScale*mScaleX*u, py - mScale*mScaleY*v);

  if (mThickArrowScale > 0)
    painter.setPen(pen);
}

// ------------------------------------------------------------------------

void PaintVector::setScaleXY(float sx, float sy)
{
  mScaleX = (sx == 0 ? 1 : sx);
  mScaleY = (sy == 0 ? 1 : sy);
}

void PaintVector::setScale(float sxy)
{
  mScale = sxy;
  if (mScale <= 0)
    mScale = 1;
}

} // namespace vcross
