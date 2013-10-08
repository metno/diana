
#ifndef VCROSS_DIVCROSSCONTOUR_HH
#define VCROSS_DIVCROSSCONTOUR_HH 1

#include "PolyContouring.h"

#include <diField/diVcrossData.h>

class FontManager;

namespace VcrossPlotDetail {
struct Axis;
typedef boost::shared_ptr<Axis> AxisPtr;

class VCContourField : public contouring::Field {
public:
  VCContourField(const VcrossData::values_t& data, AxisPtr xaxis, AxisPtr yaxis,
      const std::vector<float>& csdist, VcrossData::ZAxisPtr zax)
    : mData(data), mXpos(xaxis), mYpos(yaxis), mXval(csdist), mYval(zax) { }
    
  virtual ~VCContourField() { }

  virtual int nx() const
    { return mYval->mPoints; }
  
  virtual int ny() const
    { return mYval->mLevels; }

  virtual int nlevels() const
    { return mLevels.size(); }

  virtual int level_point(int ix, int iy) const;
  virtual int level_center(int cx, int cy) const;

  virtual contouring::Point point(int levelIndex, int x0, int y0, int x1, int y1) const;

  void setLevels(float lstart, float lstop, float lstep);
  const std::vector<float>& levels() const
    { return mLevels; }

  float getLevel(int idx) const
    { return mLevels[idx]; }
  
private:
  int index(int ix, int iy) const
    {  return iy*nx() + ix; }
  
  float value(int ix, int iy) const;
  
  contouring::Point position(int ix, int iy) const;
  
  int level_value(float value) const;

private:
  std::vector<float> mLevels;
  float mLstepInv;
  
  VcrossData::values_t mData;
  AxisPtr mXpos, mYpos;
  std::vector<float> mXval;
  VcrossData::ZAxisPtr mYval;
};

// ########################################################################

class VCContouring : public contouring::PolyContouring {
public:
  VCContouring(contouring::Field* field, FontManager* fm);
  ~VCContouring();
  virtual void emitLine(int li, contouring::Polyline& points, bool close);
private:
  FontManager* mFM;
};

} // namespace VcrossPlotDetail

#endif // VCROSS_DIVCROSSCONTOUR_HH
