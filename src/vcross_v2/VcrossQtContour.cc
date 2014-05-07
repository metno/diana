
#include "VcrossQtContour.h"

#include "VcrossQtAxis.h"
#include "VcrossQtUtil.h"
#include <diField/VcrossUtil.h>

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

float VCContourField::value(size_t ix, size_t iy) const
{
  const contouring::point_t p = position(ix, iy);
  const VCAxisPositions& ax = static_cast<const VCAxisPositions&>(positions());
  if (not (ax.mXpos->legalPaint(p.x) and ax.mYpos->legalPaint(p.y)))
    return UNDEF_VALUE;
  return mData->value(ix, iy);
}

contouring::point_t VCAxisPositions::position(size_t ix, size_t iy) const
{
  const float vx = mXval[ix], vy = mYval->value(ix, iy);
  return contouring::point_t(mXpos->value2paint(vx), mYpos->value2paint(vy));
}

// ########################################################################

void VCLines::paint(QPainter& painter)
{
  paint_polygons(painter);
  paint_lines(painter);
}

void VCLines::paint_lines(QPainter& painter)
{
  for (contour_m::const_iterator it = m_lines.begin(); it != m_lines.end(); ++it) {
    if (it->first == DianaLevels::UNDEF_LEVEL) {
      if (mPlotOptions.undefMasking)
        paint_coloured_lines(painter, mPlotOptions.undefLinewidth, mPlotOptions.undefColour, mPlotOptions.undefLinetype,
            it->second, it->first, false);
    } else {
      paint_coloured_lines(painter, mPlotOptions.linewidth, mPlotOptions.linecolour, mPlotOptions.linetype,
          it->second, it->first, true);//mPlotOptions.valueLabel);
    }
  }
}

void VCLines::paint_coloured_lines(QPainter& painter, int linewidth, const Colour& colour,
    const Linetype& linetype, const contour_v& contours, contouring::level_t level, bool label)
{
  QPen pen(vcross::util::QC(colour), linewidth);
  vcross::util::setDash(pen, linetype);

  painter.setPen(pen);
  painter.setBrush(Qt::NoBrush);

  for (contour_v::const_iterator itC = contours.begin(); itC != contours.end(); ++itC) {
    const QPolygonF& line = itC->line;
    if (itC->polygon)
      painter.drawPolygon(line);
    else
      painter.drawPolyline(line);
    
    if (label) { // draw label
      const int step_spacing = 10;
      int idx = step_spacing/2 + step_spacing*(level % step_spacing);
      for (; idx+1 < line.size(); idx += 5) {
        const QPointF p0 = line[idx], p1 = line[idx+1];
        if (p0.x() == p1.x() or p0.y() == p1.y())
          continue;
        if (p0.x() < 0 or p1.x() < 0 or p0.y() < 0 or p1.y() < 0)
          continue;

        float angle = atan2f(p1.y() - p0.y(), p1.x() - p0.x()) * 180/M_PI;
        if (angle > 90)
          angle  -= 180;
        else if(angle < -90)
          angle  += 180;
        if (angle < -45 or angle > 45)
          continue;

        if (angle >= -10 and angle <= 10)
          angle = 0;

        const QString qtxt = QString::number(mLevels.value_for_level(level));
        const float w = painter.fontMetrics().width(qtxt), h = painter.fontMetrics().height();

        painter.save();
        painter.translate(p0.x(), p0.y());
        painter.rotate(angle);
        
        const QRectF rtext(-w/2, -h/2, w, h);
        painter.fillRect(rtext, Qt::white); // FIXME adapt to plot background color
        painter.drawText(rtext, Qt::AlignCenter, qtxt);
        painter.restore();

        idx += 55;
      }
    }
  }
}

void VCLines::paint_polygons(QPainter& painter)
{
  QBrush brush;

  const int ncolours = mPlotOptions.palettecolours.size();
  const int ncolours_cold = mPlotOptions.palettecolours_cold.size();

  for (contour_m::const_iterator it = m_polygons.begin(); it != m_polygons.end(); ++it) {
    contouring::level_t level = it->first;
    const contour_v& contours = it->second;
    if (level == DianaLevels::UNDEF_LEVEL) {
      if (mPlotOptions.undefMasking != 1)
        return;
      brush = vcross::util::QC(mPlotOptions.undefColour);
    } else if (level <= 0 and ncolours_cold) {
      const int idx = find_index(mPlotOptions.repeat, ncolours_cold, -level);
      brush = vcross::util::QC(mPlotOptions.palettecolours_cold[idx]);
    } else {
      const int idx = find_index(mPlotOptions.repeat, ncolours, std::max(level - 1, 0));
      brush = vcross::util::QC(mPlotOptions.palettecolours[idx]);
    }
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(brush);
    
    for (contour_v::const_iterator itC = contours.begin(); itC != contours.end(); ++itC)
      painter.drawPolygon(itC->line);
  }
}

QPolygonF VCLines::make_polygon(const contouring::points_t& cpoints)
{
  QPolygonF line;
  for (contouring::points_t::const_iterator it = cpoints.begin(); it != cpoints.end(); ++it)
    line << QPointF(it->x, it->y);
  return line;
}

void VCLines::add_contour_line(contouring::level_t li, const contouring::points_t& cpoints, bool closed)
{
  if (li == DianaLevels::UNDEF_LEVEL and mPlotOptions.undefMasking != 2)
    return;
  if (li != DianaLevels::UNDEF_LEVEL and not mPlotOptions.options_1)
    return;

  m_lines[li].push_back(contour_t(make_polygon(cpoints), closed));
}

void VCLines::add_contour_polygon(contouring::level_t level, const contouring::points_t& cpoints)
{
  if (level == DianaLevels::UNDEF_LEVEL and mPlotOptions.undefMasking != 1)
    return;
  if (level != DianaLevels::UNDEF_LEVEL and mPlotOptions.palettecolours.empty() and mPlotOptions.palettecolours_cold.empty())
    return;

  m_polygons[level].push_back(contour_t(make_polygon(cpoints), true));
}

} // namespace vcross
} // namespace detail
