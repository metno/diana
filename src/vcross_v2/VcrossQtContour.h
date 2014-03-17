
#ifndef VCROSS_DIVCROSSCONTOUR_H
#define VCROSS_DIVCROSSCONTOUR_H 1

#include "PolyContouring.h"
#include <diField/VcrossData.h>

class QPainter;
class PlotOptions;

namespace vcross {
namespace detail {

struct Axis;
typedef boost::shared_ptr<Axis> AxisPtr;

class VCContourField : public contouring::Field {
public:
  VCContourField(Values_cp data, AxisPtr xaxis, AxisPtr yaxis,
      const std::vector<float>& xvalues, Values_cp zvalues)
    : mData(data), mXpos(xaxis), mYpos(yaxis), mXval(xvalues), mYval(zvalues) { }
  
  virtual ~VCContourField() { }

  virtual int nx() const
    { return mData->npoint(); }
  
  virtual int ny() const
    { return mData->nlevel(); }

  virtual int nlevels() const
    { return mLevels.size(); }

  virtual int level_point(int ix, int iy) const;
  virtual int level_center(int cx, int cy) const;

  virtual contouring::Point point(int levelIndex, int x0, int y0, int x1, int y1) const;

  void setLevels(float lstep);
  void setLevels(float lstart, float lstop, float lstep);
  const std::vector<float>& levels() const
    { return mLevels; }

  float getLevel(int idx) const
    { return mLevels[idx]; }
  
private:
  float value(int ix, int iy) const;
  
  contouring::Point position(int ix, int iy) const;
  
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

class VCContouring : public contouring::PolyContouring {
public:
  VCContouring(contouring::Field* field, QPainter& p, const PlotOptions& poptions);
  ~VCContouring();
  virtual void emitLine(int li, contouring::Polyline& points, bool close);
private:
  QPainter& mPainter;
  const PlotOptions& mPlotOptions;
};

} // namespace detail
} // namespace vcross

#endif // VCROSS_DIVCROSSCONTOUR_H
