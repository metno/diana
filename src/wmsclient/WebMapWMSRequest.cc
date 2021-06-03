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

#include "WebMapWMSRequest.h"

#include "WebMapTile.h"
#include "WebMapUtilities.h"
#include "WebMapWMS.h"
#include "WebMapWMSLayer.h"

#include <diField/diArea.h>

#define MILOGGER_CATEGORY "diana.WebMapWMSRequest"
#include <miLogger/miLogging.h>

WebMapWMSRequest::WebMapWMSRequest(WebMapWMS_x service, WebMapWMSLayer_cx layer, int crsIndex, int zoom)
    : WebMapTilesRequest(service)
    , mLayer(layer)
    , mCrsIndex(crsIndex)
    , mZoom(zoom)
    , mLegend(nullptr)
{
}

WebMapWMSRequest::~WebMapWMSRequest()
{
  delete mLegend;
}

void WebMapWMSRequest::setDimensionValue(const std::string& dimIdentifier, const std::string& dimValue)
{
  mDimensionValues[dimIdentifier] = dimValue;
}

const Projection& WebMapWMSRequest::tileProjection() const
{
  return mLayer->crsBoundingBox(mCrsIndex).projection;
}

void WebMapWMSRequest::addTile(int tileX, int tileY)
{
  METLIBS_LOG_SCOPE(LOGVAL(tileX) << LOGVAL(tileY));
  const Rectangle& bb = mLayer->crsBoundingBox(mCrsIndex).boundingbox;
  const int nxy = (1<<mZoom);
  const double x0 = bb.x1,
      dx = bb.width() / nxy,
      y0 = bb.y2,
      dy = -dx; // bb.height() / nxy;
  const double tx0 = x0 + dx*tileX,
      ty1 = y0 + dy*tileY;
  METLIBS_LOG_DEBUG(LOGVAL(nxy) << LOGVAL(x0) << LOGVAL(dx) << LOGVAL(y0) << LOGVAL(dy) << LOGVAL(tx0) << LOGVAL(ty1));
  const Rectangle rect(tx0, ty1 + dy, tx0 + dx, ty1);
  WebMapTilesRequest::addTile(new WebMapTile(tileX, tileY, rect));
}

void WebMapWMSRequest::submit()
{
  METLIBS_LOG_SCOPE();
  if (!mLayer->legendUrl().empty()) {
    mLegend = new WebMapImage();
    connect(mLegend, &WebMapImage::finishedImage, this, &WebMapWMSRequest::legendFinished);
    mLegend->submit(mService->submitUrl(QUrl(QString::fromStdString(mLayer->legendUrl()))));
    addExpected();
  }

  WebMapTilesRequest::submit();
}

QNetworkReply* WebMapWMSRequest::submitRequest(WebMapTile* tile)
{
  return static_cast<WebMapWMS*>(mService)->submitRequest(mLayer, mDimensionValues, mLayer->CRS(mCrsIndex), tile);
}

void WebMapWMSRequest::abort()
{
  if (mLegend)
    mLegend->abort();
  WebMapTilesRequest::abort();
}

void WebMapWMSRequest::legendFinished(WebMapImage*)
{
  METLIBS_LOG_SCOPE();
  if (diutil::checkRedirect(mService, mLegend))
    return;
  addFinished(mLegend->loadImage());
}

QImage WebMapWMSRequest::legendImage() const
{
  if (mLegend)
    return mLegend->image();
  else
    return QImage();
}
