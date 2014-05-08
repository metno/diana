
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
typedef boost::shared_ptr<Axis> AxisPtr;

class VCAxisPositions : public DianaPositions {
public:
  VCAxisPositions(AxisPtr xaxis, AxisPtr yaxis,
      const std::vector<float>& xvalues, Values_cp zvalues)
    : mXpos(xaxis), mYpos(yaxis), mXval(xvalues), mYval(zvalues) { }

  virtual contouring::point_t position(size_t ix, size_t iy) const;

  AxisPtr mXpos, mYpos;
  std::vector<float> mXval;
  Values_cp mYval;
};

// ########################################################################

class VCContourField : public DianaFieldBase
{
public:
  VCContourField(Values_cp data, const DianaLevels& levels, const VCAxisPositions& positions)
    : DianaFieldBase(levels, positions), mData(data) { }
  
  size_t nx() const
    { return mData->npoint(); }
  
  size_t ny() const
    { return mData->nlevel(); }

protected:
  virtual float value(size_t ix, size_t iy) const;
  
private:
  Values_cp mData;
};

// ########################################################################

class VCLines : public contouring::lines_t {
public:
  VCLines(const DianaLevels& levels, const PlotOptions& poptions)
    : mLevels(levels), mPlotOptions(poptions) { }
  
  void add_contour_line(contouring::level_t level, const contouring::points_t& points, bool closed);

  void add_contour_polygon(contouring::level_t level, const contouring::points_t& points);

  void paint(QPainter& painter);

private:
  struct contour_t {
    QPolygonF line;
    bool polygon;
    contour_t(const QPolygonF& l, bool p)
      : line(l), polygon(p) { }
  };
  typedef std::vector<contour_t> contour_v;
  typedef std::map<contouring::level_t, contour_v> contour_m;

private:
  QPolygonF make_polygon(const contouring::points_t& cpoints);
  void paint_polygons(QPainter& painter);
  void paint_lines(QPainter& painter);
  void paint_coloured_lines(QPainter& painter, int linewidth, const Colour& colour,
      const Linetype& linetype, const contour_v& contours, contouring::level_t li, bool label);
  void paint_labels(QPainter& painter, const QPolygonF& points, contouring::level_t li);

private:
  const DianaLevels& mLevels;
  const PlotOptions& mPlotOptions;
  contour_m m_lines, m_polygons;
};

} // namespace detail
} // namespace vcross

#endif // VCROSS_DIVCROSSCONTOUR_H
