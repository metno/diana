/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2014-2022 met.no

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

#ifndef VCROSS_DIVCROSSCONTOUR_H
#define VCROSS_DIVCROSSCONTOUR_H 1

#include "diPolyContouring.h"
#include "diPlotOptions.h"
#include <diField/VcrossData.h>
#include "poly_contouring.h"

#include <QtCore/QPointF>
#include <QtGui/QPainter>
#include <QtGui/QPolygonF>

namespace vcross {
namespace detail {

extern const float UNDEF_VALUE;

struct Axis;
typedef std::shared_ptr<Axis> AxisPtr;

class VCAxisPositions : public DianaPositions {
public:
  VCAxisPositions(AxisPtr xaxis, AxisPtr yaxis, const std::vector<float>& xvalues, diutil::Values_cp zvalues, bool timegraph);

  contouring::point_t position(size_t ix, size_t iy) const override;

private:
  AxisPtr mXpos, mYpos;
  std::vector<float> mXval;
  diutil::Values_cp mYval;
  int mYShapePositionXT, mYShapePositionZ;
};

// ########################################################################

class VCContourField : public DianaFieldBase
{
public:
  VCContourField(diutil::Values_cp data, const DianaLevels& levels, const VCAxisPositions& positions, bool timegraph);

  size_t nx() const override
    { return mData->shape().length(mShapePositionXT); }

  size_t ny() const override
    { return mData->shape().length(mShapePositionZ); }

protected:
  virtual float value(size_t ix, size_t iy) const override;

private:
  diutil::Values_cp mData;
  int mShapePositionXT, mShapePositionZ;
};

// ########################################################################

class VCLines : public DianaLines
{
public:
  VCLines(const PlotOptions& poptions, const DianaLevels& levels, QPainter& painter, const QRect& area);

protected:
  void paint_polygons() override;
  void paint_lines() override;
  void paint_labels() override;

  void setLine(const Colour& colour, const Linetype& linetype, int linewidth) override;
  void setFillColour(const Colour& colour) override;
  void setFillPattern(const std::string& pattern) override;
  void drawLine(const QPolygonF& lines) override;
  void drawPolygons(const point_vv& polygons) override;
  void drawLabels(const QPolygonF& points, contouring::level_t li) override;

private:
  void clip();
  void restore();
  QColor QCa(const Colour& colour);

private:
  QPainter& mPainter;
  QRect mArea;
};

} // namespace detail
} // namespace vcross

#endif // VCROSS_DIVCROSSCONTOUR_H
