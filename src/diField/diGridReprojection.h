#ifndef DI_GRID_REPROJECTION_H
#define DI_GRID_REPROJECTION_H

#include "diPoint.h"
#include <memory>

class Projection;

class GridReprojectionCB {
public:
  virtual ~GridReprojectionCB();
  virtual void pixelLine(const diutil::PointI &s0, const diutil::PointD& xyf0, const diutil::PointD& dxyf, int w) = 0;
  virtual void pixelQuad(const diutil::PointI &s0, const diutil::PointD& pxy00, const diutil::PointD& pxy10,
                         const diutil::PointD& pxy01, const diutil::PointD& pxy11, int w);
  virtual void linearQuad(const diutil::PointI& s0, const diutil::PointD& fxy00, const diutil::PointD& fxy10,
                          const diutil::PointD& fxy01, const diutil::PointD& fxy11, const diutil::PointI& wh);
};

class GridReprojection;
typedef std::shared_ptr<GridReprojection> GridReprojection_p;

class GridReprojection {
public:
  virtual ~GridReprojection();

  virtual void reproject(const diutil::PointI &size, const Rectangle& mr,
                         const Projection& p_map, const Projection& p_data,
                         GridReprojectionCB& cb) = 0;

  static GridReprojection_p instance();
  static void instance(GridReprojection_p i);

private:
  static GridReprojection_p instance_;
};

#endif // DI_GRID_REPROJECTION_H
