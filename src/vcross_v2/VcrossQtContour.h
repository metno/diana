
#ifndef VCROSS_DIVCROSSCONTOUR_H
#define VCROSS_DIVCROSSCONTOUR_H 1

#include "diPolyContouring.h"
#include "diPlotOptions.h"
#include <diField/VcrossData.h>
#include "poly_contouring.hh"

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
  VCAxisPositions(AxisPtr xaxis, AxisPtr yaxis,
      const std::vector<float>& xvalues, Values_cp zvalues,
      bool timegraph);

  contouring::point_t position(size_t ix, size_t iy) const override;

private:
  AxisPtr mXpos, mYpos;
  std::vector<float> mXval;
  Values_cp mYval;
  int mYShapePositionXT, mYShapePositionZ;
};

// ########################################################################

class VCContourField : public DianaFieldBase
{
public:
  VCContourField(Values_cp data, const DianaLevels& levels,
      const VCAxisPositions& positions, bool timegraph);

  size_t nx() const override
    { return mData->shape().length(mShapePositionXT); }

  size_t ny() const override
    { return mData->shape().length(mShapePositionZ); }

protected:
  virtual float value(size_t ix, size_t iy) const override;

private:
  Values_cp mData;
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
  void drawLine(const point_v& lines) override;
  void drawPolygons(const point_vv& polygons) override;
  void drawLabels(const point_v& points, contouring::level_t li) override;

private:
  void clip();
  void restore();
  QPolygonF make_polygon(const point_v& cpoints);
  QColor QCa(const Colour& colour);

private:
  QPainter& mPainter;
  QRect mArea;
};

} // namespace detail
} // namespace vcross

#endif // VCROSS_DIVCROSSCONTOUR_H
