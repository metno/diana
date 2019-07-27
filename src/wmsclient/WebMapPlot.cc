/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2019 met.no

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

#include "WebMapPlot.h"

#include "WebMapPainting.h"
#include "WebMapService.h"
#include "WebMapUtilities.h"
#include "diGLPainter.h"
#include "diPaintGLPainter.h"
#include "diPoint.h"
#include "diStaticPlot.h"

#include "diField/diGridReprojection.h"
#include "util/nearest_element.h"
#include "util/string_util.h"
#include "util/time_util.h"

#include <QImage>

#include <sys/time.h>

#define MILOGGER_CATEGORY "diana.WebMapPlot"
#include <miLogger/miLogging.h>

static const std::string EMPTY_STRING;
static const std::vector<std::string> EMPTY_STRING_V;
static const bool DEBUG_TILE_BORDERS = true;

WebMapPlot::WebMapPlot(WebMapService* service, const std::string& layer)
    : mService(service)
    , mLayerId(layer)
    , mLayer(0)
    , mTimeDimensionIdx(-1)
    , mTimeSelected(-1)
    , mTimeTolerance(-1)
    , mTimeOffset(0)
    , mAlphaOffset(0)
    , mAlphaScale(1)
    , mMakeGrey(false)
    , mPlotOrder(PO_LINES)
    , mRequest(0)
{
  if (mService) {
    serviceRefreshFinished();
    connect(mService, SIGNAL(refreshStarting()), this, SLOT(serviceRefreshStarting()));
    connect(mService, SIGNAL(refreshFinished()), this, SLOT(serviceRefreshFinished()));
  }
}

WebMapPlot::~WebMapPlot()
{
  dropRequest();
}

std::string WebMapPlot::title() const
{
  if (mLayer)
    return mLayer->title();
  else
    return std::string();
}

std::string WebMapPlot::attribution() const
{
  if (mLayer)
    return mLayer->attribution();
  else
    return std::string();
}

std::string WebMapPlot::getEnabledStateKey() const
{
  return "webmap_" + title();
}

void WebMapPlot::getAnnotation(std::string& text, Colour&) const
{
  METLIBS_LOG_SCOPE();
  text = title();
  METLIBS_LOG_DEBUG(LOGVAL(text) << LOGVAL(mDimensionValues.size()));

  for (auto&& dv : mDimensionValues) {
    METLIBS_LOG_DEBUG(LOGVAL(dv.first) << LOGVAL(dv.second));
    if (!dv.second.empty())
      diutil::appendText(text, dv.first + "=" + dv.second, " ");
  }

  const std::string att = attribution();
  if (!att.empty())
    diutil::appendText(text, "(" + att + ")");
}

void WebMapPlot::dropRequest()
{
  if (!mRequest)
    return;
  if (!mRequestCompleted) {
    disconnect(mRequest, SIGNAL(completed()), this, SIGNAL(update()));
    mRequest->abort();
    mRequest->deleteLater();
  } else {
    delete mRequest;
  }
  mRequest = 0;
}

namespace {

struct PaintTilesCB : public GridReprojectionCB {
  PaintTilesCB(QImage& target_, WebMapRequest_x request_,
               const diutil::PointD& xyt0_, const diutil::PointD& dxyt_)
    : target(target_)
    , request(request_)
    , xyt0(xyt0_)
    , dxyt(dxyt_)
  { }

  QImage& target;
  WebMapRequest_x request;

  diutil::PointD xyt0, dxyt;

  void pixelLine(const diutil::PointI& s, const diutil::PointD& xyf0, const diutil::PointD& dxyf, int n) override;
};

void PaintTilesCB::pixelLine(const diutil::PointI &s, const diutil::PointD& xyf0, const diutil::PointD& dxyf, int n)
{
  const int x0 = s.x(), y0 = target.size().height() - 1 - s.y();
  QRgb* rgb = reinterpret_cast<QRgb*>(target.scanLine(y0)) + x0;

  diutil::PointD fxy = xyf0;
  for (int i=0; i<n; ++i, fxy += dxyf) {
    const size_t tidx = request->tileIndex(fxy.x(), fxy.y());
    if (tidx == size_t(-1))
      continue;
    const QImage& timg = request->tileImage(tidx);
    if (timg.isNull())
      continue;
    const diutil::PointD ixy = (fxy - xyt0) / dxyt;
    float dummy = 0;
    const int ix = int(std::modf(ixy.x(), &dummy) * timg.width());
    const int iy = int(std::modf(ixy.y(), &dummy) * timg.height());
    rgb[i] = timg.pixel(ix, iy);
  }
}

} // namespace

void WebMapPlot::plot(DiGLPainter* gl, PlotOrder porder)
{
  METLIBS_LOG_TIME();
  if (!isEnabled() || porder != mPlotOrder)
    return;
  if (!mLayer)
    return;

  if (mTimeDimensionIdx >= 0 && mTimeSelected < 0)
    return; // has time axis, but time not found within tolerance

  if (!mRequest) {
    METLIBS_LOG_DEBUG("about to request tiles...");
    const int phys_w = getStaticPlot()->getPhysWidth();
    const int phys_h = getStaticPlot()->getPhysHeight();
    const double mapw = getStaticPlot()->getPlotSize().width(),
        m_per_unit = diutil::metersPerUnit(getStaticPlot()->getMapArea().P());
    const double viewScale = mapw * m_per_unit / phys_w
        / diutil::WMTS_M_PER_PIXEL;
    METLIBS_LOG_DEBUG("map.w=" << mapw << " m/unit=" << m_per_unit
        << " phys.w=" << phys_w << " scale=" << viewScale);
    mRequestCompleted = false;
    mRequest = mService->createRequest(mLayer->identifier(),
        getStaticPlot()->getPlotSize(), getStaticPlot()->getMapArea().P(), viewScale, phys_w, phys_h);
    if (!mRequest) {
      METLIBS_LOG_DEBUG("no request object");
      return;
    }
    for (auto& dv : mDimensionValues)
      mRequest->setDimensionValue(dv.first, dv.second);
    if (mTimeDimensionIdx >= 0 && !mFixedTime.empty())
      mRequest->setDimensionValue(mLayer->dimension(mTimeDimensionIdx).identifier(), mFixedTime);
    connect(mRequest, SIGNAL(completed()), this, SLOT(requestCompleted()));
    mRequest->submit();
  } else if (mRequestCompleted) {
    METLIBS_LOG_DEBUG("about to plot tiles...");
    StaticPlot* sp = getStaticPlot();
    const diutil::Values2<int> size(sp->getPhysWidth(), sp->getPhysHeight());
    QImage target(size.x(), size.y(), QImage::Format_ARGB32);
    target.fill(Qt::transparent);

    diutil::PointD xyt0(mRequest->x0, mRequest->y0), dxyt(mRequest->dx, mRequest->dy);
    METLIBS_LOG_DEBUG(LOGVAL(xyt0) << LOGVAL(dxyt));
    PaintTilesCB cb(target, mRequest, xyt0, dxyt);
    GridReprojection::instance()->reproject(size, sp->getPlotSize(), sp->getMapArea().P(), mRequest->tileProjection(), cb);

    gl->drawScreenImage(QPointF(0,0), diutil::convertImage(target, mAlphaOffset, mAlphaScale, mMakeGrey));

    const QImage li = mRequest->legendImage();
    if (!li.isNull()) {
      METLIBS_LOG_DEBUG("about to plot legend...");
      const QPointF screenpos(getStaticPlot()->getPhysWidth() - 10 - li.width(),
                              getStaticPlot()->getPhysHeight() - 10 - li.height());
      gl->drawScreenImage(screenpos, li);
    }
  } else {
    METLIBS_LOG_DEBUG("waiting for tiles...");
  }
}

void WebMapPlot::setStyleAlpha(float offset, float scale)
{
  mAlphaOffset = offset;
  mAlphaScale = scale;
}

void WebMapPlot::setStyleGrey(bool makeGrey)
{
  mMakeGrey = makeGrey;
}

void WebMapPlot::setPlotOrder(PlotOrder po)
{
  mPlotOrder = po;
}

void WebMapPlot::requestCompleted()
{
  METLIBS_LOG_SCOPE();
  mRequestCompleted = true;
  Q_EMIT update();
}

void WebMapPlot::changeProjection(const Area& mapArea, const Rectangle& /*plotSize*/)
{
  if (mOldArea != mapArea) {
    mOldArea = mapArea;
    dropRequest();
  }
}

size_t WebMapPlot::countDimensions() const
{
  if (!mLayer)
    return 0;
  else
    return mLayer->countDimensions();
}

const std::string& WebMapPlot::dimensionTitle(size_t idx) const
{
  if (!mLayer || idx >= mLayer->countDimensions())
    return EMPTY_STRING;
  return mLayer->dimension(idx).title();
}

const std::vector<std::string>& WebMapPlot::dimensionValues(size_t idx) const
{
  if (!mLayer || idx >= mLayer->countDimensions())
    return EMPTY_STRING_V;
  return mLayer->dimension(idx).values();
}

void WebMapPlot::setDimensionValue(const std::string& dimId, const std::string& dimValue)
{
  METLIBS_LOG_SCOPE(LOGVAL(dimId) << LOGVAL(dimValue));
  if (!dimValue.empty())
    mDimensionValues[dimId] = dimValue;
  else
    mDimensionValues.erase(dimId);
}

void WebMapPlot::changeTime(const miutil::miTime& time)
{
  METLIBS_LOG_SCOPE(LOGVAL(time) << LOGVAL(mTimeDimensionIdx));
  if (!mLayer || mTimeDimensionIdx < 0) {
    METLIBS_LOG_DEBUG("no layer, or no time dimension");
    return;
  }

  const miutil::miTime actualTime = miutil::addSec(time, mTimeOffset);
  METLIBS_LOG_DEBUG(LOGVAL(actualTime));

  TimeDimensionValues_t::const_iterator itBest = diutil::nearest_element(mTimeDimensionValues, actualTime, miutil::miTime::secDiff);
  if (mTimeTolerance >= 0 && itBest != mTimeDimensionValues.end()) { // reject if above max tolerance
    if (std::abs(miutil::miTime::secDiff(itBest->first, actualTime)) > mTimeTolerance)
      itBest = mTimeDimensionValues.end();
  }

  const int bestIndex = (itBest != mTimeDimensionValues.end()) ? itBest->second : -1;
  METLIBS_LOG_DEBUG(LOGVAL(bestIndex) << LOGVAL(mTimeSelected));
  if (bestIndex == mTimeSelected)
    return;
  mTimeSelected = bestIndex;

  const WebMapDimension& timeDim = mLayer->dimension(mTimeDimensionIdx);
  const std::string& timeId = timeDim.identifier();
  if (mTimeSelected >= 0) {
    const std::string& value = timeDim.value(mTimeSelected);
    METLIBS_LOG_DEBUG(LOGVAL(mTimeSelected) << "=> '" << value << "'");
    setDimensionValue(timeId, value);
  } else {
    METLIBS_LOG_DEBUG("time = empty");
    setDimensionValue(timeId, EMPTY_STRING);
  }
  dropRequest();
}

void WebMapPlot::setFixedTime(const std::string& fixedTime)
{
  mFixedTime = fixedTime;
}

plottimes_t WebMapPlot::getTimes()
{
  findLayerAndTimeDimension();

  plottimes_t times;
  for (const auto& ti : mTimeDimensionValues)
    times.insert(ti.first);
  return times;
}

void WebMapPlot::serviceRefreshStarting()
{
  METLIBS_LOG_SCOPE();
  dropRequest();
  mLayer = 0;
}

void WebMapPlot::serviceRefreshFinished()
{
  findLayerAndTimeDimension();
  Q_EMIT update();
}

void WebMapPlot::findLayerAndTimeDimension()
{
  METLIBS_LOG_SCOPE(LOGVAL(mLayerId));
  if (mLayer)
    return;

  mLayer = mService->findLayerByIdentifier(mLayerId);
  mTimeDimensionIdx = -1;
  mTimeSelected = -1;
  mTimeDimensionValues.clear();

  if (!mLayer) {
    METLIBS_LOG_INFO("layer '" << mLayerId << "' not found");
    return;
  }

  // search time dimension
  for (size_t i = 0; i < mLayer->countDimensions(); ++i) {
    METLIBS_LOG_DEBUG(LOGVAL(mLayer->dimension(i).identifier()));
    if (mLayer->dimension(i).isTime()) {
      mTimeDimensionIdx = i;
      METLIBS_LOG_DEBUG(LOGVAL(mTimeDimensionIdx));
      break;
    }
  }
  if (mTimeDimensionIdx < 0) {
    METLIBS_LOG_INFO("time dimension for layer '" << mLayerId << "' not found");
    return;
  }

  const WebMapDimension& timeDim = mLayer->dimension(mTimeDimensionIdx);
  for (size_t i = 0; i < timeDim.count(); ++i) {
    const miutil::miTime t = diutil::to_miTime(diutil::parseWmsIso8601(timeDim.value(i)));
    if (!t.undef())
      mTimeDimensionValues.insert(std::make_pair(t, i));
  }
}
