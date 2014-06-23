
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
  return mData->value(ix, iy);
}

contouring::point_t VCAxisPositions::position(size_t ix, size_t iy) const
{
  const float vx = mXval[ix], vy = mYval->value(ix, iy);
  return contouring::point_t(mXpos->value2paint(vx, false), mYpos->value2paint(vy, false));
}

// ########################################################################

void VCLines::paint(QPainter& painter, const QRect& area)
{
  painter.save();
  painter.setClipRegion(area);
  paint_polygons(painter);
  paint_lines(painter);
  painter.restore();

  paint_all_labels(painter, mPlotOptions.linewidth, mPlotOptions.linecolour,
      mPlotOptions.linetype, area);
}

void VCLines::paint_lines(QPainter& painter)
{
  for (contour_m::const_iterator it = m_lines.begin(); it != m_lines.end(); ++it) {
    if (it->first == DianaLevels::UNDEF_LEVEL) {
      if (mPlotOptions.undefMasking)
        paint_coloured_lines(painter, mPlotOptions.undefLinewidth, mPlotOptions.undefColour, mPlotOptions.undefLinetype,
            it->second, it->first);
    } else {
      paint_coloured_lines(painter, mPlotOptions.linewidth, mPlotOptions.linecolour, mPlotOptions.linetype,
          it->second, it->first);//mPlotOptions.valueLabel);
    }
  }
}

void VCLines::paint_coloured_lines(QPainter& painter, int linewidth, const Colour& colour,
    const Linetype& linetype, const contour_v& contours, contouring::level_t level)
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
  }
}

void VCLines::paint_all_labels(QPainter& painter, int linewidth, const Colour& colour,
    const Linetype& linetype, const QRect& area)
{
  QPen pen(vcross::util::QC(colour), linewidth);
  vcross::util::setDash(pen, linetype);

  painter.setPen(pen);
  painter.setBrush(Qt::NoBrush);

  for (contour_m::const_iterator it = m_lines.begin(); it != m_lines.end(); ++it) {
    if (it->first != DianaLevels::UNDEF_LEVEL) {
      
      const contour_v& contours = it->second;
      for (contour_v::const_iterator itC = contours.begin(); itC != contours.end(); ++itC) {
        const QPolygonF& line = itC->line;
        paint_line_labels(painter, line, it->first, area);
      }
    }
  }
}

void VCLines::paint_line_labels(QPainter& painter, const QPolygonF& points, contouring::level_t li,
    const QRect& area)
{
  if (points.size() < 10)
    return;

  const QString lbl = QString::number(mLevels.value_for_level(li));

  const float lbl_w = painter.fontMetrics().width(lbl), lbl_h = painter.fontMetrics().height();
  if (lbl_h <= 0 or lbl_w <= 0)
    return;
  const float lbl_w2 = lbl_w * lbl_w;

  int idx = int(0.1*(1 + (std::abs(li+100000) % 5))) * points.size();
  for (; idx + 1 < points.size(); idx += 5) {
    QPointF p0 = points.at(idx), p1;
    const int idx0 = idx;
    for (idx += 1; idx < points.size(); ++idx) {
      p1 = points.at(idx);
      const float dy = p1.y() - p0.y(), dx = p1.x() - p0.x();
      if (dx*dx + dy*dy >= lbl_w2)
        break;
    }
    if (idx >= points.size())
      break;

    if (not (area.contains(p0.x(), p0.y()) and area.contains(p1.x(), p1.y()))) {
      idx += 20;
      continue;
    }

    if (p1.x() < p0.x())
      std::swap(p0, p1);
    const float angle_deg = atan2f(p1.y() - p0.y(), p1.x() - p0.x()) * 180. / M_PI;
    if (angle_deg < -45 or angle_deg > 45)
      continue;

    // check that line is somewhat straight under label
    int idx2 = idx0 + 1;
    for (; idx2 < idx; ++idx2) {
      const QPointF p2 = points.at(idx2);
      const float a = atan2f(p2.y() - p0.y(), p2.x() - p0.x()) * 180. / M_PI;
      if (std::abs(a - angle_deg) > 15)
        break;
    }
    if (idx2 < idx)
      continue;

    // label angle seems ok, find position
    painter.save();
    painter.translate(p0.x(), p0.y());
    painter.rotate(angle_deg);
    
    const QRectF rtext(-lbl_w/2, -lbl_h/2, lbl_w, lbl_h);
    painter.fillRect(rtext, Qt::white); // FIXME adapt to plot background color
    painter.drawText(rtext, Qt::AlignCenter, lbl);
    painter.restore();

    idx += 10*(idx - idx0);
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
      const int idx = find_index(mPlotOptions.repeat, ncolours, level - 1);
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
