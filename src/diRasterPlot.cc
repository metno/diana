
#include "diRasterPlot.h"

#include "diPainter.h"
#include "diPlot.h"

#include <diField/VcrossUtil.h> // minimize + maximize

#define MILOGGER_CATEGORY "diana.RasterPlot"
#include <miLogger/miLogging.h>

RasterPlot::RasterPlot()
  : mScaleFactor(0), mNPositions(0)
{
}

RasterPlot::~RasterPlot()
{
}

void RasterPlot::rasterClear()
{
  mScaleFactor = 0;
  mNPositions = 0;
  mPositionsXY = boost::shared_array<float>();
  mImageScaled = QImage();
}

int RasterPlot::calculateScaleFactor()
{
  const GridArea& ra = rasterArea();
  StaticPlot* sp = rasterStaticPlot();

  float cx[2], cy[2];
  cx[0] = ra.R().x1;
  cy[0] = ra.R().y1;
  cx[1] = ra.R().x2;
  cy[1] = ra.R().y2;
  sp->ProjToMap(ra.P(), 2, cx, cy);

  const double gridW = ra.nx*sp->getPhysToMapScaleX()/double(cx[1] - cx[0]);
  const double gridH = ra.ny*sp->getPhysToMapScaleY()/double(cy[1] - cy[0]);
  return std::max(1, int(std::min(gridW, gridH)));
}

void RasterPlot::updateImage()
{
  const int scale = calculateScaleFactor();
  if (!mImageScaled.isNull() && scale == mScaleFactor)
    return;

  mScaleFactor = scale;
  mAreaScaled = rasterArea().scaled(mScaleFactor);
  mImageScaled = rasterScaledImage(mAreaScaled, mScaleFactor);
}

void RasterPlot::getGridPoints()
{
  METLIBS_LOG_TIME();
  StaticPlot* sp = rasterStaticPlot();
  const Area& mapArea = sp->getMapArea();
  const size_t npos = (mAreaScaled.nx+1) * (mAreaScaled.ny+1);
  if (npos == mNPositions && mPositionsXY && mapArea.P() == mMapProjection)
    return;

  const GridArea& ra = rasterArea();
  mMapProjection = mapArea.P();
  mSimilarProjection = !ra.P().isGeographic()
      && mMapProjection.isAlmostEqual(ra.P());
  METLIBS_LOG_DEBUG(LOGVAL(mSimilarProjection));

  if (npos != mNPositions) {
    mNPositions = npos;
    mPositionsXY = boost::shared_array<float>(new float[2*mNPositions]);
  }

  const bool center_on_gridpoint = true;
  if (mSimilarProjection) {
    float satX = ra.R().x1, satY = ra.R().y1;
    if (!sp->ProjToMap(ra.P(), 1, &satX, &satY))
      return;

    size_t im = 0;
    if (center_on_gridpoint) {
      satX -= 0.5*mAreaScaled.resolutionX;
      satY -= 0.5*mAreaScaled.resolutionY;
    }
    for (int iy=0; iy<=mAreaScaled.ny; ++iy) {
      const float mpy = satY + mAreaScaled.resolutionY * iy;
      for (int ix=0; ix<=mAreaScaled.nx; ++ix) {
        mPositionsXY[im++] = satX + mAreaScaled.resolutionX * ix;
        mPositionsXY[im++] = mpy;
      }
    }
  } else {
    float *x, *y;
    sp->gc.getGridPoints(mAreaScaled, mapArea, center_on_gridpoint, &x, &y);
    size_t im = 0;
    for (size_t i=0; i<mNPositions; ++i) {
      mPositionsXY[im++] = x[i];
      mPositionsXY[im++] = y[i];
    }
  }
}

void RasterPlot::checkVisible(int& ix1, int& ix2, int& iy1, int& iy2)
{
  METLIBS_LOG_TIME();
  const bool center_on_gridpoint = true;
  const Rectangle& mapRect = rasterStaticPlot()->getMapSize();
  METLIBS_LOG_DEBUG(LOGVAL(mapRect));
  if (mSimilarProjection) {
    const float satX1 = mPositionsXY[0], satY1 = mPositionsXY[1];
    ix1 = std::max((int)floor((mapRect.x1 - satX1)/mAreaScaled.resolutionX), 0);
    ix2 = std::min((int) ceil((mapRect.x2 - satX1)/mAreaScaled.resolutionX), mAreaScaled.nx);
    iy1 = std::max((int)floor((mapRect.y1 - satY1)/mAreaScaled.resolutionY), 0);
    iy2 = std::min((int) ceil((mapRect.y2 - satY1)/mAreaScaled.resolutionY), mAreaScaled.ny);
    METLIBS_LOG_DEBUG("similar" << LOGVAL(ix1) << LOGVAL(ix2) << LOGVAL(iy1) << LOGVAL(iy2));
  } else {
    // different projections, need to search indices
#if 1
    METLIBS_LOG_DEBUG("calling GridConverter");
    GridConverter::findGridLimits(mAreaScaled, mapRect, center_on_gridpoint,
        mPositionsXY.get(), ix1, ix2, iy1, iy2);
#else
    ix1 = mAreaScaled.nx;
    iy1 = mAreaScaled.ny;
    ix2 = iy2 = 0;
    size_t im = 0;
    for (int iy=0; iy<=mAreaScaled.ny; ++iy) {
      for (int ix=0; ix<=mAreaScaled.nx; ++ix, im += 2) {
        if (ix1 > ix || ix2 < ix || iy1 > iy || iy2 < iy) {
          if (mapRect.isinside(mPositionsXY[im], mPositionsXY[im+1])) {
            vcross::util::minimaximize(ix1, ix2, ix);
            vcross::util::minimaximize(iy1, iy2, iy);
          }
        }
      }
    }
#endif
  }
  if (center_on_gridpoint) {
    // FIXME old GridConverter, excludes last column iff gridboxes
    vcross::util::minimize(ix2, mAreaScaled.nx-1);
    vcross::util::minimize(iy2, mAreaScaled.ny-1);
  }
  METLIBS_LOG_DEBUG(LOGVAL(ix1) << LOGVAL(ix2) << LOGVAL(iy1) << LOGVAL(iy2));
}

void RasterPlot::rasterPaint(DiPainter* gl)
{
  METLIBS_LOG_TIME();

  updateImage();
  if (mImageScaled.isNull())
    return;

  // TODO only needed when projection or map scale changes
  getGridPoints();

  // TODO only needed when map window changes
  int ix1, ix2, iy1, iy2;
  checkVisible(ix1, ix2, iy1, iy2);
  if (ix1>ix2 || iy1>iy2)
    return;

  if (ix1 == 0 && ix2 == mAreaScaled.nx && iy1 == 0 && iy2 == mAreaScaled.ny) {
    gl->drawReprojectedImage(mImageScaled, mPositionsXY.get(), false);
  } else {
    // TODO making a new copy is only necessary if the map window changed
    const size_t cw = ix2-ix1, ch = iy2-iy1, cs = (cw+1)*(ch+1),
        dim = 2*(mAreaScaled.nx-cw);
    QImage copy = mImageScaled.copy(ix1, iy1, cw, ch);
    boost::shared_array<float> cxy = boost::shared_array<float>(new float[2*cs]);
    size_t ic = 0, im = 2*(iy1*(mAreaScaled.nx+1) + ix1);
    for (int iy=iy1; iy<=iy2; ++iy, im += dim) {
      for (int ix=ix1; ix<=ix2; ++ix) {
        cxy[ic++] = mPositionsXY[im++];
        cxy[ic++] = mPositionsXY[im++];
      }
    }
    METLIBS_LOG_TIME("paint sub-image");
    gl->drawReprojectedImage(copy, cxy.get(), false);
  }
}
