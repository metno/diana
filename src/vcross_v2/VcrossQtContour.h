
#ifndef VCROSS_DIVCROSSCONTOUR_H
#define VCROSS_DIVCROSSCONTOUR_H 1

#include <diField/VcrossData.h>
#include "poly_contouring.hh"

class QPainter;
class PlotOptions;
#include <QtCore/QPointF>

namespace vcross {
namespace detail {

struct Axis;
typedef boost::shared_ptr<Axis> AxisPtr;

class VCContourField : public contouring::field_t
{
public:
  static const contouring::level_t UNDEF_LEVEL = -1;

  VCContourField(Values_cp data, AxisPtr xaxis, AxisPtr yaxis,
      const std::vector<float>& xvalues, Values_cp zvalues)
    : mData(data), mXpos(xaxis), mYpos(yaxis), mXval(xvalues), mYval(zvalues) { }
  
  ~VCContourField() { }

  size_t nx() const
    { return mData->npoint(); }
  
  size_t ny() const
    { return mData->nlevel(); }

  int nlevels() const
    { return mLevels.size(); }

  contouring::level_t grid_level(size_t ix, size_t iy) const;

  contouring::point_t grid_point(size_t x, size_t y) const
    { return position(x, y); }
  contouring::point_t line_point(contouring::level_t level, size_t x0, size_t y0, size_t x1, size_t y1) const;

  contouring::level_t undefined_level() const
    { return UNDEF_LEVEL; }

  void setLevels(float lstep);
  void setLevels(float lstart, float lstop, float lstep);
  const std::vector<float>& levels() const
    { return mLevels; }

  float getLevel(int idx) const
    { return mLevels[idx]; }
  
private:
  float value(size_t ix, size_t iy) const;
  
  contouring::point_t position(size_t ix, size_t iy) const;
  
  int level_value(float value) const;

private:
  std::vector<float> mLevels;
  float mLstepInv;
  
  Values_cp mData;
  AxisPtr mXpos, mYpos;
  std::vector<float> mXval;
  Values_cp mYval;
};

// ########################################################################

class VCLines : public contouring::lines_t {
public:
  VCLines(const VCContourField& field, QPainter& painter, const PlotOptions& poptions)
    : mField(field), mPainter(painter), mPlotOptions(poptions) { }
  
  void add_contour_line(contouring::level_t level, const contouring::points_t& points, bool closed);

  void add_contour_polygon(contouring::level_t level, const contouring::points_t& points);

private:
  const VCContourField& mField;
  QPainter& mPainter;
  const PlotOptions& mPlotOptions;
};

} // namespace detail
} // namespace vcross

#endif // VCROSS_DIVCROSSCONTOUR_H
