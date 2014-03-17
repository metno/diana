
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

inline bool isUndefined(float v)
{
  return std::isnan(v) or v >= UNDEF_VALUE or v < -UNDEF_VALUE;
}

void VCContourField::setLevels(float lstep)
{
  float vMin = UNDEF_VALUE, vMax = UNDEF_VALUE;
  for (int ix=0; ix<nx(); ++ix) {
    for (int iy=0; iy<ny(); ++iy) {
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

float VCContourField::value(int ix, int iy) const
{
  const contouring::Point p = position(ix, iy);
  if (not (mXpos->legalPaint(p.x) and mYpos->legalPaint(p.y)))
    return UNDEF_VALUE;
  return mData->value(ix, iy);
}

contouring::Point VCContourField::position(int ix, int iy) const
{
  const float vx = mXval[ix], vy = mYval->value(ix, iy);
  return contouring::Point(mXpos->value2paint(vx), mYpos->value2paint(vy));
}

contouring::Point VCContourField::point(int levelIndex, int x0, int y0, int x1, int y1) const
{
  const float v0 = value(x0, y0);
  const float v1 = value(x1, y1);
  const float c = (mLevels[levelIndex]-v0)/(v1-v0);
  
  const contouring::Point p0 = position(x0, y0);
  const contouring::Point p1 = position(x1, y1);
  const float x = (1-c)*p0.x + c*p1.x; // FIXME interpolate before calling position?
  const float y = (1-c)*p0.y + c*p1.y;
  return contouring::Point(x, y);
}

int VCContourField::level_point(int ix, int iy) const
{
  const float v = value(ix, iy);
  if (isUndefined(v))
    return Field::UNDEFINED;
  return level_value(v);
}

int VCContourField::level_center(int cx, int cy) const
{
  const float v_00 = value(cx, cy), v_10 = value(cx+1, cy), v_01 = value(cx, cy+1), v_11 = value(cx+1, cy+1);
  if (isUndefined(v_00) or isUndefined(v_01) or isUndefined(v_10) or isUndefined(v_11))
    return Field::UNDEFINED;

  const float avg = 0.25*(v_00 + v_01 + v_10 + v_11);
  return level_value(avg);
}

int VCContourField::level_value(float value) const
{
#if 1
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

VCContouring::VCContouring(contouring::Field* field, QPainter& painter, const PlotOptions& poptions)
  : PolyContouring(field)
  , mPainter(painter)
  , mPlotOptions(poptions)
{
}

VCContouring::~VCContouring()
{
}

void VCContouring::emitLine(int li, contouring::Polyline& points, bool close)
{
  const bool highlight = ((li % 5) == 0);
  const bool label = highlight;

  QPen pen(vcross::util::QC(mPlotOptions.linecolour));
  int lw = mPlotOptions.linewidth;
  if (highlight)
    lw += 1;
  pen.setWidth(lw);

  VCContourField* vcf = static_cast<VCContourField*>(mField);;
  { // draw line
    if (mPlotOptions.linetype.stipple)
      vcross::util::setDash(pen, mPlotOptions.linetype.factor, mPlotOptions.linetype.bmap);
    mPainter.setPen(pen);
    
    QPolygonF line;
    BOOST_FOREACH(const contouring::Point& p, points) {
      line << QPointF(p.x, p.y);
    }
    if (close)
      mPainter.drawPolygon(line);
    else
      mPainter.drawPolyline(line);
  }
  if (label) { // draw label
    size_t idx = 2 + 5*(li % 5), step = idx;
    contouring::Polyline::const_iterator it = points.begin();
    while (idx+2 < points.size()) {
      std::advance(it, step);
      const contouring::Point &p0 = *it, &p1 = *(++it);
      step = 60;
      idx += step+1;
      
      const QString qtxt = QString::number(vcf->getLevel(li));
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

} // namespace vcross
} // namespace detail
