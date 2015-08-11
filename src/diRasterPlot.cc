
#include "diRasterPlot.h"

#include "diGlUtilities.h"
#include "diPainter.h"
#include "diPlot.h"

#include <diField/VcrossUtil.h> // minimize + maximize

#define MILOGGER_CATEGORY "diana.RasterPlot"
#include <miLogger/miLogging.h>

static const bool center_on_gridpoint = true;

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

namespace {
// FIXME copied from WebMapWMS
int findZoomForScale(float z0denominator, float denominator)
{
  if (denominator <= 0)
    return 0;
  return std::max(0, (int) round(log(z0denominator / denominator) / log(2)) - 1);
}
} // namespace

int RasterPlot::calculateScaleFactor()
{
  METLIBS_LOG_SCOPE();
  const GridArea& ra = rasterArea();
  StaticPlot* sp = rasterStaticPlot();

  float cx[2], cy[2];
  cx[0] = ra.R().x1;
  cy[0] = ra.R().y1;
  cx[1] = ra.R().x2;
  cy[1] = ra.R().y2;
  sp->ProjToMap(ra.P(), 2, cx, cy);

  if (!diutil::is_undefined(cx[0]) && !diutil::is_undefined(cx[1])
      && !diutil::is_undefined(cy[0]) && !diutil::is_undefined(cy[1]))
  {
    const float dcx = cx[1] - cx[0], dcy = cy[1] - cy[0];
    const float msx = sp->getPhysToMapScaleX(), msy = sp->getPhysToMapScaleY();
    const double gridW = ra.nx*msx/dcx;
    const double gridH = ra.ny*msy/dcy;
    METLIBS_LOG_DEBUG(LOGVAL(ra.nx) << LOGVAL(msx) << LOGVAL(dcx)
        << LOGVAL(ra.ny) << LOGVAL(msy) << LOGVAL(dcy));
    METLIBS_LOG_DEBUG(LOGVAL(gridW) << LOGVAL(gridH));
    return std::max(1, int(std::min(gridW, gridH)));
  } else {
#if 0 // this is not working properly
    // FIXME this is copied from WebMapPlot
    const double mapw = sp->getPlotSize().width(),
        m_per_unit = diutil::metersPerUnit(sp->getMapArea().P()),
        phys_w = sp->getPhysWidth();
    const double viewScale = mapw * m_per_unit / phys_w
        / diutil::WMTS_M_PER_PIXEL;

    // FIXME this is copied from WebMapWMS
    const float z0denominator = ra.R().width() * diutil::metersPerUnit(ra.P()) / diutil::WMTS_M_PER_PIXEL;
    const int zoom = findZoomForScale(z0denominator, viewScale);
    const int scale = 1<<zoom;
    METLIBS_LOG_DEBUG(LOGVAL(zoom) << LOGVAL(scale));
    return scale;
#else
    return 1;
#endif
  }
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

diutil::Rect_v RasterPlot::checkVisible()
{
  METLIBS_LOG_TIME();

  StaticPlot* sp = rasterStaticPlot();
  const Rectangle& mapRect = sp->getMapSize();
  const GridArea& mas = mAreaScaled;
  METLIBS_LOG_DEBUG(LOGVAL(mapRect) << LOGVAL(mScaleFactor));

  if (mSimilarProjection) {
    const float satX1 = mPositionsXY[0], satY1 = mPositionsXY[1];
    const int ix1 = std::max((int)floor((mapRect.x1 - satX1)/mas.resolutionX), 0);
    const int ix2 = std::min((int) ceil((mapRect.x2 - satX1)/mas.resolutionX), mas.nx);
    const int iy1 = std::max((int)floor((mapRect.y1 - satY1)/mas.resolutionY), 0);
    const int iy2 = std::min((int) ceil((mapRect.y2 - satY1)/mas.resolutionY), mas.ny);
    METLIBS_LOG_DEBUG("similar" << LOGVAL(ix1) << LOGVAL(ix2) << LOGVAL(iy1) << LOGVAL(iy2));

    return diutil::Rect_v(1, diutil::Rect(ix1, iy1, ix2, iy2));
  } else {
    // different projections, need to search indices
#if 0
    // old version selecting only one rectangle
    diutil::Rect_v rects(1);
    diutil::Rect& r = rects.back();
    GridConverter::findGridLimits(mAreaScaled, mapRect, center_on_gridpoint,
        mPositionsXY.get(), r.x1, r.x2, r.y1, r.y2);
    return rects;
#else
    // new version selecting a list of rectangles
    const int bxy = center_on_gridpoint ? 1 : 0;

    // FIXME at present, we do not have the possibility to check if we
    // go outside the projections "borders" (e.g., wrap around) when
    // we substract 0.5*resX/Y; therefore, we omit the outermost grid
    // data and hope that the grid itself does not cross any borders
    const int x1 = bxy, x2 = mas.nx-bxy, y1 = bxy, y2 = mas.ny-bxy;

    return diutil::select_tiles(diutil::Rect(x1, y1, x2, y2),
        mAreaScaled, mPositionsXY, mapRect, false);
#endif
  }
}

void RasterPlot::rasterPaint(DiPainter* gl)
{
  METLIBS_LOG_TIME();

  updateImage();
  if (mImageScaled.isNull())
    return;

  // TODO only needed when projection or map scale changes
  getGridPoints();

  // TODO only needed after getGridPoints or when view rect changes
  const diutil::Rect_v cells = checkVisible();

  gl->drawReprojectedImage(mImageScaled, mPositionsXY.get(), cells, false);
}

// ========================================================================

namespace diutil {

bool tile_front(const XY& left, const XY& mid, const XY& right)
{
  const XY d1 = (left - mid), d2 = (right - mid);
  return (d1.x() * d2.y()) > (d2.x() * d1.y());
}

namespace detail {

enum tile_overlap {
  OVERLAP_NONE = 0,
  OVERLAP_PARTIAL = 1,
  OVERLAP_COMPLETE = 2
};

inline XY gridPosition(const GridArea& grid, boost::shared_array<float> gridPositionsXY, int ix, int iy)
{
  int idx = diutil::index(grid.nx+1, ix, iy);
  return XY(gridPositionsXY[2*idx], gridPositionsXY[2*idx+1]);
}

struct check_point {
  bool have_valid;
  Rectangle bbx;
  check_point() : have_valid(false) { }
  bool operator()(const XY& b);
};

bool check_point::operator()(const XY& b)
{
  bool valid = !diutil::is_undefined(b);
  if (valid) {
    if (have_valid) {
      vcross::util::minimaximize(bbx.x1, bbx.x2, b.x());
      vcross::util::minimaximize(bbx.y1, bbx.y2, b.y());
    } else {
      bbx.x1 = bbx.x2 = b.x();
      bbx.y1 = bbx.y2 = b.y();
    }
    have_valid = true;
  }
  return valid;
}

tile_overlap select_tile_rects(Rect_v& tiles, const Rect& rect,
    const GridArea& grid, boost::shared_array<float> gridPositionsXY,
    const Rectangle& r_view, bool select_front)
{
  assert(!rect.empty());

  const int rnx =  rect.width(), rny = rect.height();
  if (rnx == 1 && rny == 1) {
    const XY cBL(gridPosition(grid, gridPositionsXY, rect.x1, rect.y1));
    const XY cBR(gridPosition(grid, gridPositionsXY, rect.x1, rect.y2));
    const XY cTL(gridPosition(grid, gridPositionsXY, rect.x2, rect.y1));
    const XY cTR(gridPosition(grid, gridPositionsXY, rect.x2, rect.y2));
    check_point check;
    bool accept_coord = check(cBL) && check(cTR) && check(cBR) && check(cTL)
        && r_view.intersects(check.bbx);
    if (accept_coord) {
      bool good_side = (tile_front(cBL, cTL, cTR) == select_front)
          && (tile_front(cTL, cTR, cBR) == select_front)
          && (tile_front(cTR, cBR, cBL) == select_front)
          && (tile_front(cBR, cBL, cTL) == select_front);
#if 0 // this is a hack to suppress rectangles that are streched very much
      if (good_side) {
        const float dxb = fabsf(cBL.x() - cBR.x()), dxt = fabsf(cTL.x() - cTR.x());
        const float dyr = fabsf(cTR.x() - cBR.x()), dyl = fabsf(cTL.x() - cBL.x());
        const float dx  = std::max(dxb, dxt), dy = std::max(dyr, dyl);
        // FIXME 0.3 could be too large for WMS tiles, while it might be okay for model grids or satellite data
        good_side = (dx < 0.3*r_view.width() && dy < 0.3*r_view.height());
      }
#endif
      if (good_side)
        return OVERLAP_COMPLETE;
    }
    return OVERLAP_NONE;
  }

  Rect child_a = rect, child_b = rect;
  if (rny > rnx) {
    // keep x, divide on y
    child_a.y2 = rect.y1 + rny / 2;
    child_b.y1 = child_a.y2;
  } else {
    // divide on x, keep y
    child_a.x2 = rect.x1 + rnx / 2;
    child_b.x1 = child_a.x2;
  }

  tile_overlap ov_a = select_tile_rects(tiles, child_a, grid,  gridPositionsXY, r_view, select_front);
  tile_overlap ov_b = select_tile_rects(tiles, child_b, grid,  gridPositionsXY, r_view, select_front);

  if (ov_a == OVERLAP_COMPLETE and ov_b == OVERLAP_COMPLETE)
    return OVERLAP_COMPLETE;
  if (ov_a == OVERLAP_NONE and ov_b == OVERLAP_NONE)
    return OVERLAP_NONE;
  if (ov_a == OVERLAP_COMPLETE)
    tiles.push_back(child_a);
  if (ov_b == OVERLAP_COMPLETE)
    tiles.push_back(child_b);
  return OVERLAP_PARTIAL;
}

} // namespace detail

Rect_v select_tiles(const Rect& rect, const GridArea& grid,
    boost::shared_array<float> gridPositionsXY,
    const Rectangle& r_view, bool select_front)
{
  METLIBS_LOG_TIME(LOGVAL(r_view));
  Rect_v tiles;
  if (!rect.empty()) {
    detail::tile_overlap ov = detail::select_tile_rects(tiles, rect, grid, gridPositionsXY, r_view, select_front);
    if (ov == detail::OVERLAP_COMPLETE && !rect.empty())
      tiles.push_back(rect);
  }
  return tiles;
}

} // namespace diutil
