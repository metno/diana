/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2021 met.no

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

WebMapPlot::WebMapPlot(WebMapService* service, const std::string& layer)
    : mService(service)
    , mLayerId(layer)
    , mLayer(nullptr)
    , mTimeDimensionIdx(-1)
    , mTimeSelected(-2)
    , mTimeTolerance(-1)
    , mTimeOffset(0)
    , mMapTimeChanged(true)
    , mLegendOffsetX(-10)
    , mLegendOffsetY(10)
    , mAlphaOffset(0)
    , mAlphaScale(1)
    , mMakeGrey(false)
    , mPlotOrder(PO_LINES)
    , mRequest(nullptr)
{
  METLIBS_LOG_SCOPE(LOGVAL(mLayerId));
  if (mService) {
    if (mService->countLayers() > 0)
      serviceRefreshFinished();
    connect(mService, &WebMapService::refreshStarting, this, &WebMapPlot::serviceRefreshStarting);
    connect(mService, &WebMapService::refreshFinished, this, &WebMapPlot::serviceRefreshFinished);
    setRequestStatus(R_NONE);
  } else {
    setRequestStatus(R_FAILED); // no service
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

  if (mTimeDimensionIdx >= 0) {
    if (!mFixedTime.empty()) {
      diutil::appendText(text, "FIXED " + mFixedTime);
    } else {
      const std::string& timeId = mLayer->dimension(mTimeDimensionIdx).identifier();
      const auto itt = mDimensionValues.find(timeId);
      if (itt != mDimensionValues.end())
        diutil::appendText(text, itt->second);
    }
  }
  int idx = 0;
  for (const auto& dv : mDimensionValues) {
    METLIBS_LOG_DEBUG(LOGVAL(dv.first) << LOGVAL(dv.second));
    if (idx != mTimeDimensionIdx && !dv.second.empty())
      diutil::appendText(text, dv.first + "=" + dv.second);
    idx += 1;
  }

  const std::string att = attribution();
  if (!att.empty())
    diutil::appendText(text, "(" + att + ")");
}

void WebMapPlot::createRequest()
{
  METLIBS_LOG_SCOPE(LOGVAL(mLayerId));
  if (mRequest) {
    METLIBS_LOG_WARN("dropping existing request");
    dropRequest();
  }

  if (!mLayer)
    return;

  const int phys_w = getStaticPlot()->getPhysWidth();
  const int phys_h = getStaticPlot()->getPhysHeight();
  const double mapw = getStaticPlot()->getPlotSize().width(),
      m_per_unit = diutil::metersPerUnit(getStaticPlot()->getMapArea().P());
  const double viewScale = mapw * m_per_unit / phys_w
      / diutil::WMTS_M_PER_PIXEL;
  METLIBS_LOG_DEBUG("map.w=" << mapw << " m/unit=" << m_per_unit
                             << " phys.w=" << phys_w << " scale=" << viewScale);
  mRequest = mService->createRequest(mLayer->identifier(), getStaticPlot()->getPlotSize(), getStaticPlot()->getMapArea().P(), viewScale, phys_w, phys_h);
  if (!mRequest) {
    METLIBS_LOG_DEBUG("no request object created in webmap service");
    setRequestStatus(R_FAILED);
  } else {
    mRequest->setStyleName(mStyleName);
    for (auto& dv : mDimensionValues)
      mRequest->setDimensionValue(dv.first, dv.second);
    connect(mRequest, &WebMapRequest::completed, this, &WebMapPlot::requestCompleted);

    // submit() might emit completed() immediately and change status, therefore we set the expected status before calling submit
    setRequestStatus(R_SUBMITTED);
    mRequest->submit();
  }
}

void WebMapPlot::dropRequest()
{
  METLIBS_LOG_SCOPE(LOGVAL(mLayerId));
  if (mRequest) {
    disconnect(mRequest, &WebMapRequest::completed, this, &WebMapPlot::requestCompleted);
    mRequest->abort();
    mRequest->deleteLater();
  }
  mRequest = nullptr;
}

void WebMapPlot::setRequestStatus(RequestStatus rs)
{
  METLIBS_LOG_SCOPE(LOGVAL(mLayerId));
  const PlotStatusValue ps[R_MAX + 1] = {
      P_OK_DATA, // R_NONE
      P_WAITING, // R_REQUIRED
      P_WAITING, // R_SUBMITTED
      P_OK_DATA, // R_COMPLETED
      P_ERROR    // R_FAILED
  };
  mRequestStatus = rs;
  setStatus(ps[mRequestStatus]);
  METLIBS_LOG_DEBUG(LOGVAL(mRequestStatus) << LOGVAL(getStatus()));
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

float edgeOffset(float distance, int edge)
{
  if (distance >= 0)
    return distance;
  else
    return distance + edge;
}

} // namespace

void WebMapPlot::prepareData()
{
  METLIBS_LOG_SCOPE(LOGVAL(mLayerId) << LOGVAL(mRequestStatus));
  if (!isEnabled()) {
    METLIBS_LOG_DEBUG("disabled");
    return;
  }

  if (!mLayer) {
    METLIBS_LOG_DEBUG("no layer object");
    return;
  }

  if (mTimeDimensionIdx >= 0 && mMapTimeChanged) {
    const WebMapDimension& timeDim = mLayer->dimension(mTimeDimensionIdx);
    const std::string& timeId = timeDim.identifier();
    if (mFixedTime.empty()) {
      const miutil::miTime actualTime = miutil::addSec(mMapTime, mTimeOffset);
      METLIBS_LOG_DEBUG(LOGVAL(actualTime));

      TimeDimensionValues_t::const_iterator itBest = diutil::nearest_element(mTimeDimensionValues, actualTime, miutil::miTime::secDiff);
      if (mTimeTolerance >= 0 && itBest != mTimeDimensionValues.end()) { // reject if above max tolerance
        if (std::abs(miutil::miTime::secDiff(itBest->first, actualTime)) > mTimeTolerance)
          itBest = mTimeDimensionValues.end();
      }

      const int bestIndex = (itBest != mTimeDimensionValues.end()) ? itBest->second : -1;
      METLIBS_LOG_DEBUG(LOGVAL(bestIndex) << LOGVAL(mTimeSelected));
      if (bestIndex != mTimeSelected) {
        mTimeSelected = bestIndex;
        if (mTimeDimensionIdx >= 0 && mTimeSelected < 0) {
          METLIBS_LOG_DEBUG("time not found");
          dropRequest();
          setRequestStatus(R_FAILED);
          return; // has time axis, but time not found within tolerance
        }
        setRequestStatus(R_REQUIRED);
      }
      if (mTimeSelected >= 0)
        setDimensionValue(timeId, timeDim.value(mTimeSelected));
      else
        setDimensionValue(timeId, EMPTY_STRING);
    } else {
      setDimensionValue(timeId, mFixedTime);
    }
    mMapTimeChanged = false;
  }

  METLIBS_LOG_DEBUG(LOGVAL(mRequestStatus));
  if (mRequestStatus == R_REQUIRED)
    createRequest();

  /*Q_EMIT*/ update();
}

void WebMapPlot::plot(DiGLPainter* gl, PlotOrder porder)
{
  if (!isEnabled() || mRequestStatus != R_COMPLETED)
    return;

  if (porder == mPlotOrder) {
    METLIBS_LOG_SCOPE();
    gl->drawScreenImage(QPointF(0, 0), diutil::convertImage(mReprojected, mAlphaOffset, mAlphaScale, mMakeGrey));
  }

  if (porder == PO_LINES) {
    const QImage& li = mLegendImage;
    if (!li.isNull() && mLegendOffsetX != 0 && mLegendOffsetY != 0 && li.width() > 0 && li.height() > 0) {
      const float x = edgeOffset(mLegendOffsetX, getStaticPlot()->getPhysWidth() - li.width());
      const float y = edgeOffset(mLegendOffsetY, getStaticPlot()->getPhysHeight() - li.height());
      gl->drawScreenImage(QPointF(x, y), li);
    }
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

void WebMapPlot::requestCompleted(bool success)
{
  METLIBS_LOG_SCOPE(LOGVAL(mLayerId) << LOGVAL(success));
  if (success) {
    const StaticPlot* sp = getStaticPlot();
    const diutil::Values2<int> size(sp->getPhysWidth(), sp->getPhysHeight());
    QImage target(size.x(), size.y(), QImage::Format_ARGB32);
    target.fill(Qt::transparent);

    diutil::PointD xyt0(mRequest->x0, mRequest->y0), dxyt(mRequest->dx, mRequest->dy);
    METLIBS_LOG_DEBUG(LOGVAL(xyt0) << LOGVAL(dxyt));
    PaintTilesCB cb(target, mRequest, xyt0, dxyt);
    GridReprojection::instance()->reproject(size, sp->getPlotSize(), sp->getMapArea().P(), mRequest->tileProjection(), cb);

    mReprojected = target;
    mLegendImage = mRequest->legendImage();
    METLIBS_LOG_DEBUG(LOGVAL(mReprojected.width()) << LOGVAL(mReprojected.height()));

    setRequestStatus(R_COMPLETED);
  } else {
    mReprojected = mLegendImage = QImage();
    setRequestStatus(R_FAILED);
  }
  dropRequest();

  /*Q_EMIT*/ update();
}

void WebMapPlot::changeProjection(const Area& /*mapArea*/, const Rectangle& /*plotSize*/, const diutil::PointI& /*physSize*/)
{
  METLIBS_LOG_SCOPE(LOGVAL(mLayerId));
  dropRequest();
  setRequestStatus(R_REQUIRED);
  prepareData();

  /*Q_EMIT*/ update();
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
  if (mFixedTime.empty() && time != mMapTime) {
    mMapTime = time;
    mMapTimeChanged = true;
  }
  prepareData();
}

void WebMapPlot::setFixedTime(const std::string& fixedTime)
{
  METLIBS_LOG_SCOPE(LOGVAL(mLayerId));
  if (fixedTime != mFixedTime) {
    mFixedTime = fixedTime;
    mMapTimeChanged = true;
    dropRequest();
    setRequestStatus(R_REQUIRED);
  }
}

plottimes_t WebMapPlot::getTimes()
{
  METLIBS_LOG_SCOPE(LOGVAL(mLayerId) << LOGVAL(mRequestStatus));
  findLayerAndTimeDimension();

  plottimes_t times;
  for (const auto& ti : mTimeDimensionValues)
    times.insert(ti.first);
  return times;
}

void WebMapPlot::serviceRefreshStarting()
{
  METLIBS_LOG_SCOPE(LOGVAL(mLayerId) << LOGVAL(mRequestStatus));
  dropRequest();
  if (mRequestStatus == R_COMPLETED || mRequestStatus == R_FAILED || mRequestStatus == R_SUBMITTED)
    setRequestStatus(R_REQUIRED);

  mLayer = nullptr;
}

void WebMapPlot::serviceRefreshFinished()
{
  METLIBS_LOG_SCOPE(LOGVAL(mLayerId) << LOGVAL(mRequestStatus));
  findLayerAndTimeDimension();
  prepareData();
  /*Q_EMIT*/ update();
}

void WebMapPlot::findLayerAndTimeDimension()
{
  METLIBS_LOG_SCOPE(LOGVAL(mLayerId) << LOGVAL(mRequestStatus));
  if (mLayer)
    return;

  if (mService->countLayers() == 0) // FIXME replace with "service status"
    return;

  mLayer = mService->findLayerByIdentifier(mLayerId);
  mTimeDimensionIdx = -1;
  mTimeSelected = -2;
  mTimeDimensionValues.clear();
  mMapTimeChanged = true;

  if (!mLayer) {
    METLIBS_LOG_INFO("layer '" << mLayerId << "' not found");
    setRequestStatus(R_FAILED);
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
    METLIBS_LOG_DEBUG("no time dimension for layer '" << mLayerId << "'");
  } else {
    const WebMapDimension& timeDim = mLayer->dimension(mTimeDimensionIdx);
    for (size_t i = 0; i < timeDim.count(); ++i) {
      const miutil::miTime t = diutil::to_miTime(diutil::parseWmsIso8601(timeDim.value(i)));
      if (!t.undef())
        mTimeDimensionValues.insert(std::make_pair(t, i));
    }
  }
  setRequestStatus(R_REQUIRED);
}
