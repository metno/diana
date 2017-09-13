
#include "diRasterPlot.h"

#include "diPlot.h"

#define MILOGGER_CATEGORY "diana.RasterPlot"
#include <miLogger/miLogging.h>

RasterPlot::RasterPlot()
{
}

RasterPlot::~RasterPlot()
{
}

QImage RasterPlot::rasterPaint()
{
  METLIBS_LOG_SCOPE();

  StaticPlot* sp = rasterStaticPlot();
  const diutil::PointI size(sp->getPhysWidth(), sp->getPhysHeight());
  cached_ = QImage(size.x(), size.y(), QImage::Format_ARGB32);
  cached_.fill(Qt::transparent);

  GridReprojection::instance()->reproject(size, sp->getPlotSize(), sp->getMapArea().P(), rasterArea().P(), *this);
  return cached_;
}

void RasterPlot::pixelLine(const diutil::PointI& s, const diutil::PointD& xy0, const diutil::PointD& dxy, int w)
{
  rasterPixels(w, xy0, dxy, pixels(s));
}

QRgb* RasterPlot::pixels(const diutil::PointI& s)
{
  const int x0 = s.x(), y0 = cached_.size().height() - 1 - s.y();
  return reinterpret_cast<QRgb*>(cached_.scanLine(y0)) + x0;
}
