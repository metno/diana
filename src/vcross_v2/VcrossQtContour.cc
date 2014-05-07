
#include "VcrossQtContour.h"

#include "VcrossQtAxis.h"
#include "VcrossQtUtil.h"
#include <diField/VcrossUtil.h>
#include "diPlotOptions.h"

#include <QtGui/QPainter>

#include <boost/foreach.hpp>

#include <cmath>

#define MILOGGER_CATEGORY "diana.VcrossContour"
#include <miLogger/miLogging.h>

namespace vcross {
namespace detail {

const float UNDEF_VALUE = 1e30;
const contouring::level_t VCContourField::UNDEF_LEVEL;

inline bool isUndefined(float v)
{
  return std::isnan(v) or v >= UNDEF_VALUE or v < -UNDEF_VALUE;
}

void VCContourField::setLevels(float lstep)
{
  float vMin = UNDEF_VALUE, vMax = UNDEF_VALUE;
  for (size_t ix=0; ix<nx(); ++ix) {
    for (size_t iy=0; iy<ny(); ++iy) {
      const float v = mData->value(ix, iy);
      if (isUndefined(v))
        continue;
      if (isUndefined(vMin) or v < vMin)
        vMin = v;
      if (isUndefined(vMax) or v > vMax)
        vMax = v;
    }
  }
  
  if (not isUndefined(vMin)) {
    const float lstart = ((int)(vMin / lstep)+1) * lstep;
    const float lstop  = ((int)(vMax / lstep)+1) * lstep;
    setLevels(lstart, lstop, lstep);
  }
}

void VCContourField::setLevels(float lstart, float lstop, float lstep)
{
  mLstepInv  = 1/lstep;
  mLevels.clear();
  for (float l=lstart; l<=lstop; l+=lstep)
    mLevels.push_back(l);
}

float VCContourField::value(size_t ix, size_t iy) const
{
  const contouring::point_t p = position(ix, iy);
  if (not (mXpos->legalPaint(p.x) and mYpos->legalPaint(p.y)))
    return UNDEF_VALUE;
  return mData->value(ix, iy);
}

contouring::point_t VCContourField::position(size_t ix, size_t iy) const
{
  const float vx = mXval[ix], vy = mYval->value(ix, iy);
  return contouring::point_t(mXpos->value2paint(vx), mYpos->value2paint(vy));
}

contouring::point_t VCContourField::line_point(int levelIndex, size_t x0, size_t y0, size_t x1, size_t y1) const
{
  const float v0 = value(x0, y0);
  const float v1 = value(x1, y1);
  const float c = (mLevels[levelIndex]-v0)/(v1-v0);
  
  const contouring::point_t p0 = position(x0, y0);
  const contouring::point_t p1 = position(x1, y1);
  const float x = (1-c)*p0.x + c*p1.x; // FIXME interpolate before calling position?
  const float y = (1-c)*p0.y + c*p1.y;
  return contouring::point_t(x, y);
}

contouring::level_t VCContourField::grid_level(size_t ix, size_t iy) const
{
  const float v = value(ix, iy);
  if (isUndefined(v))
    return VCContourField::UNDEF_LEVEL;
  return level_value(v);
}

contouring::level_t VCContourField::level_value(float value) const
{
#if 0
  if (value < mLevels.front())
    return 0;
  if (value >= mLevels.back())
    return mLevels.size();
  return (int)((value-mLevels.front()) * mLstepInv - 1000) + 1000;
#else
  return std::lower_bound(mLevels.begin(), mLevels.end(), value) - mLevels.begin();
#endif
}

// ########################################################################

void VCLines::add_contour_line(contouring::level_t level, const contouring::points_t& points, bool closed)
{
  const bool highlight = ((level % 5) == 0);
  const bool label = highlight;

  QPen pen(vcross::util::QC(mPlotOptions.linecolour));
  int lw = mPlotOptions.linewidth;
  if (highlight)
    lw += 1;
  pen.setWidth(lw);

  if (mPlotOptions.linetype.stipple)
    vcross::util::setDash(pen, mPlotOptions.linetype.factor, mPlotOptions.linetype.bmap);

  mPainter.setPen(pen);
  mPainter.setBrush(Qt::NoBrush);

  QPolygonF line;
  for (contouring::points_t::const_iterator it = points.begin(); it != points.end(); ++it)
    line << QPointF(it->x, it->y);

  if (closed)
    mPainter.drawPolygon(line);
  else
    mPainter.drawPolyline(line);

  if (label) { // draw label
    size_t idx = 2 + 5*(level % 5), step = idx;
    contouring::points_t::const_iterator it = points.begin();
    while (idx+2 < points.size()) {
      std::advance(it, step);
      const contouring::point_t &p0 = *it, &p1 = *(++it);
      step = 60;
      idx += step+1;
      
      const QString qtxt = QString::number(mField.getLevel(level));
      float angle = atan2f(p1.y - p0.y, p1.x - p0.x) * 180/M_PI;
      if (angle > 90)
        angle  -= 180;
      else if(angle < -90)
        angle  += 180;
      if (angle >= -10 and angle <= 10)
        angle = 0;
      const float w = mPainter.fontMetrics().width(qtxt), h = mPainter.fontMetrics().height();
      mPainter.save();
      
      mPainter.translate(p0.x, p0.y);
      mPainter.rotate(angle);
      
      const QRectF rtext(-w/2, -h/2, w, h);
      mPainter.fillRect(rtext, Qt::white); // FIXME adapt to plot background color
      mPainter.drawText(rtext, Qt::AlignCenter, qtxt);
      mPainter.restore();
    }
  }
}

void VCLines::add_contour_polygon(contouring::level_t level, const contouring::points_t& points)
{
  QBrush brush;
  if (level != mField.undefined_level()) {
    const size_t npalette = mPlotOptions.palettecolours.size();
    if (npalette > 0) {
      brush = vcross::util::QC(mPlotOptions.palettecolours[level % npalette]);
    } else if (false) {
      const size_t nlevels = mField.levels().size();
      float fraction = 0.5;
      if (nlevels > 0 and level >= 0 and level <= (int)nlevels)
        fraction = float(level)/nlevels;
      brush = QColor(int(255*fraction), 0, (0x80-int(0x80*fraction)), 64);
    }
  } else {
    brush = Qt::green;
  }

  mPainter.setPen(Qt::NoPen);
  mPainter.setBrush(brush);

  QPolygonF line;
  for (contouring::points_t::const_iterator it = points.begin(); it != points.end(); ++it)
    line << QPointF(it->x, it->y);

  mPainter.drawPolygon(line);
}

} // namespace vcross
} // namespace detail
