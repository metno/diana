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

#include "WebMapWMTS.h"

#include "WebMapTile.h"

#define MILOGGER_CATEGORY "diana.WebMapWMTSRequest"
#include <miLogger/miLogging.h>

WebMapWMTSRequest::WebMapWMTSRequest(WebMapWMTS_x service, WebMapWMTSLayer_cx layer, WebMapWMTSTileMatrixSet_cx matrixSet, WebMapWMTSTileMatrix_cx matrix)
    : WebMapTilesRequest(service)
    , mLayer(layer)
    , mMatrixSet(matrixSet)
    , mMatrix(matrix)
{
}

WebMapWMTSRequest::~WebMapWMTSRequest() {}

void WebMapWMTSRequest::addTile(int tileX, int tileY)
{
  const double ps = mMatrixSet->pixelSpan(mMatrix),
      tileSpanX = mMatrix->tileWidth()  * ps,
      tileSpanY = mMatrix->tileHeight() * ps;
  const double tx0 = mMatrix->tileMinX() + tileSpanX * tileX,
      ty1 = mMatrix->tileMaxY() - tileSpanY * tileY;
  const Rectangle rect(tx0, ty1 - tileSpanY, tx0+tileSpanX, ty1);
  WebMapTilesRequest::addTile(new WebMapTile(tileX, tileY, rect));
}

void WebMapWMTSRequest::setDimensionValue(const std::string& dimIdentifier, const std::string& dimValue)
{
  mDimensionValues[dimIdentifier] = dimValue;
}

QNetworkReply* WebMapWMTSRequest::submitRequest(WebMapTile* tile)
{
  std::string style = styleName();
  if (style.empty() && mLayer->countStyles() > 0) {
    style = mLayer->style(0);
  }
  return static_cast<WebMapWMTS_x>(mService)->submitRequest(mLayer, mDimensionValues, style, mMatrixSet, mMatrix, tile->column(), tile->row());
}
