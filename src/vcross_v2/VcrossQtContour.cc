/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2020 met.no

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

#include "VcrossQtContour.h"

#include "VcrossQtAxis.h"
#include "VcrossQtUtil.h"
#include "diField/VcrossUtil.h"
#include "qtUtility.h"

#include <mi_fieldcalc/math_util.h>

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

VCContourField::VCContourField(Values_cp data, const DianaLevels& levels,
    const VCAxisPositions& positions, bool timegraph)
  : DianaFieldBase(levels, positions)
  , mData(data)
  , mShapePositionXT(mData->shape().position(timegraph ? Values::TIME : Values::GEO_X))
  , mShapePositionZ(mData->shape().position(Values::GEO_Z))
{
}

float VCContourField::value(size_t ix, size_t iy) const
{
  Values::ShapeIndex idx(mData->shape());
  idx.set(mShapePositionXT, ix);
  idx.set(mShapePositionZ, iy);
  return mData->value(idx);
}

// ########################################################################

VCAxisPositions::VCAxisPositions(AxisPtr xaxis, AxisPtr yaxis,
    const std::vector<float>& xvalues, Values_cp zvalues, bool timegraph)
  : mXpos(xaxis)
  , mYpos(yaxis)
  , mXval(xvalues)
  , mYval(zvalues)
  , mYShapePositionXT(mYval->shape().position(timegraph ? Values::TIME : Values::GEO_X))
  , mYShapePositionZ(mYval->shape().position(Values::GEO_Z))
{
}

contouring::point_t VCAxisPositions::position(size_t ix, size_t iy) const
{
  Values::ShapeIndex idx_y(mYval->shape());
  idx_y.set(mYShapePositionXT, ix);
  idx_y.set(mYShapePositionZ,  iy);

  const float vx = mXval[ix], vy = mYval->value(idx_y);
  return contouring::point_t(mXpos->value2paint(vx, false), mYpos->value2paint(vy, false));
}

// ########################################################################

VCLines::VCLines(const PlotOptions& poptions, const DianaLevels& levels, QPainter& painter, const QRect& area)
  : DianaLines(poptions, levels), mPainter(painter), mArea(area)
{
}

void VCLines::paint_polygons()
{
  clip();
  DianaLines::paint_polygons();
  restore();
}

void VCLines::paint_lines()
{
  clip();
  DianaLines::paint_lines();
  restore();
}

void VCLines::paint_labels()
{
  const float scale = mPlotOptions.labelSize;
  if (scale < 0.05)
    return;

  QFont origFont;
  const bool changeFont = (scale <= 0.95 or scale >= 1.05);
  if (changeFont) {
    origFont = mPainter.font();
    QFont scaledFont(origFont);
    if (scaledFont.pointSizeF() > 0)
      scaledFont.setPointSizeF(scaledFont.pointSizeF() * scale);
    else
      scaledFont.setPointSize((int)(scaledFont.pointSize() * scale));
    mPainter.setFont(scaledFont);
  }

  DianaLines::paint_labels();

  if (changeFont)
    mPainter.setFont(origFont);
}

void VCLines::setLine(const Colour& colour, const Linetype& linetype, int linewidth)
{
  if (linetype.bmap != 0) {
    QPen pen(diutil::QC(colour), linewidth);
    vcross::util::setDash(pen, linetype);
    mPainter.setPen(pen);
  } else {
    mPainter.setPen(Qt::NoPen);
  }
  mPainter.setBrush(Qt::NoBrush);
}

void VCLines::setFillColour(const Colour& colour)
{
  mPainter.setPen(Qt::NoPen);
  mPainter.setBrush(QCa(colour));
}

void VCLines::setFillPattern(const std::string& pattern)
{
  // FIXME implement this
}

void VCLines::drawPolygons(const point_vv& polygons)
{
  for (const auto& p : polygons)
    mPainter.drawPolygon(p);
}

void VCLines::drawLine(const QPolygonF& line)
{
  if (line.size() >= 4 and line.front() == line.back())
    mPainter.drawPolygon(line);
  else
    mPainter.drawPolyline(line);
}

void VCLines::drawLabels(const QPolygonF& points, contouring::level_t li)
{
  if (points.size() < 10)
    return;

  mPainter.setPen(diutil::QC(mPlotOptions.linecolour));
  mPainter.setBrush(Qt::NoBrush);

  const QString lbl = QString::number(mLevels.value_for_level(li));

  const float lbl_w = mPainter.fontMetrics().width(lbl), lbl_h = mPainter.fontMetrics().height();
  if (lbl_h <= 0 or lbl_w <= 0)
    return;
  const float lbl_w2 = lbl_w * lbl_w;

  int idx = (size_t)(0.1*(1 + (std::abs(li+100000) % 5))) * points.size();
  for (; idx + 1 < points.size(); idx += 5) {
    QPointF p0 = points.at(idx), p1;
    const int idx0 = idx;
    for (idx += 1; idx < points.size(); ++idx) {
      p1 = points.at(idx);
      const float dy = p1.y() - p0.y(), dx = p1.x() - p0.x();
      if (miutil::absval2(dx, dy) >= lbl_w2)
        break;
    }
    if (idx >= points.size())
      break;

    if (not (mArea.contains(p0.x(), p0.y()) and mArea.contains(p1.x(), p1.y()))) {
      idx += 20;
      continue;
    }

    if (p1.x() < p0.x())
      std::swap(p0, p1);
    const float angle_deg = atan2f(p1.y() - p0.y(), p1.x() - p0.x()) * 180. / M_PI;

    // check that line is somewhat straight under label
    int idx2 = idx0 + 1;
    for (; idx2 < idx; ++idx2) {
      const QPointF& p2 = points.at(idx2);
      const float a = atan2f(p2.y() - p0.y(), p2.x() - p0.x()) * 180. / M_PI;
      if (std::abs(a - angle_deg) > 15)
        break;
    }
    if (idx2 < idx)
      continue;

    // label angle seems ok, find position
    mPainter.save();
    mPainter.translate(p0.x(), p0.y());
    mPainter.rotate(angle_deg);
    
    const QRectF rtext(-lbl_w/2, -lbl_h/2, lbl_w, lbl_h);
    mPainter.fillRect(rtext, Qt::white); // FIXME adapt to plot background color
    mPainter.drawText(rtext, Qt::AlignCenter, lbl);
    mPainter.restore();

    idx += 10*(idx - idx0);
  }
}

void VCLines::clip()
{
  mPainter.save();
  mPainter.setClipRegion(mArea);
}

void VCLines::restore()
{
  mPainter.restore();
}

QColor VCLines::QCa(const Colour& colour)
{
  QColor qcolor(diutil::QC(colour));
  qcolor.setAlpha(mPlotOptions.alpha);
  return qcolor;
}

} // namespace vcross
} // namespace detail
