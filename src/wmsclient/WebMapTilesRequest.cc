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

#include "WebMapTilesRequest.h"

#include "WebMapTile.h"
#include "WebMapUtilities.h"
#include "diUtilities.h"

#define MILOGGER_CATEGORY "diana.WebMapTilesRequest"
#include <miLogger/miLogging.h>

WebMapTilesRequest::WebMapTilesRequest(WebMapService* service)
    : mService(service)
    , mExpected(0)
    , mFinished(0)
    , mFinishedSuccess(0)
{
}

WebMapTilesRequest::~WebMapTilesRequest()
{
  diutil::delete_all_and_clear(mTiles);
}

void WebMapTilesRequest::addTile(WebMapTile* tile)
{
  if (mFinished == 0 && mTiles.size() < 128) {
    mTiles.push_back(tile);
    addExpected();
  } else {
    METLIBS_LOG_WARN("too many tiles");
    delete tile;
    diutil::delete_all_and_clear(mTiles);
    mFinished = 1;
  }
}

void WebMapTilesRequest::submit()
{
  if (mFinished != 0) {
    Q_EMIT completed(false);
    return;
  }
  for (WebMapTile* tile : mTiles) {
    connect(tile, &WebMapTile::finished, this, &WebMapTilesRequest::tileFinished);
    tile->submit(submitRequest(tile));
  }
}

void WebMapTilesRequest::abort()
{
  for (WebMapTile* tile : mTiles) {
    disconnect(tile, &WebMapTile::finished, this, &WebMapTilesRequest::tileFinished);
    tile->abort();
  }
}

void WebMapTilesRequest::tileFinished(WebMapTile* tile)
{
  if (!diutil::checkRedirect(mService, tile))
    addFinished(tile->loadImage());
}

void WebMapTilesRequest::addExpected()
{
  mExpected += 1;
}

void WebMapTilesRequest::addFinished(bool success)
{
  mFinished += 1;
  if (success)
    mFinishedSuccess += 1;

  if (mFinished == mExpected)
    Q_EMIT completed(mFinishedSuccess == mExpected);
}

const Rectangle& WebMapTilesRequest::tileRect(size_t idx) const
{
  return mTiles.at(idx)->rect();
}

const QImage& WebMapTilesRequest::tileImage(size_t idx) const
{
  return mTiles.at(idx)->image();
}
