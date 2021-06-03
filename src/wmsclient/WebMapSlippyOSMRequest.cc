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

#include "WebMapSlippyOSMRequest.h"

#include "WebMapSlippyOSM.h"
#include "WebMapTile.h"
#include "WebMapUtilities.h"

#include <diField/diArea.h>

#define MILOGGER_CATEGORY "diana.WebMapSlippyOSMRequest"
#include <miLogger/miLogging.h>

namespace /* anonymous */ {
const double EARTH_CICRUMFERENCE_M = diutil::WMTS_EARTH_RADIUS_M * 2 * M_PI;
} // anonymous namespace

WebMapSlippyOSMRequest::WebMapSlippyOSMRequest(WebMapSlippyOSM_x service, WebMapSlippyOSMLayer_cx layer, int zoom)
    : WebMapTilesRequest(service)
    , mLayer(layer)
    , mZoom(zoom)
{
}

WebMapSlippyOSMRequest::~WebMapSlippyOSMRequest()
{
}

void WebMapSlippyOSMRequest::addTile(int tileX, int tileY)
{
  METLIBS_LOG_SCOPE();
  const int nxy = (1<<mZoom);
  const double x0 = -EARTH_CICRUMFERENCE_M/2,
      dx = EARTH_CICRUMFERENCE_M / nxy,
      y0 = -x0,
      dy = -dx;
  const double tx0 = x0 + dx*tileX,
      ty1 = y0 + dy*tileY;
  const Rectangle rect(tx0, ty1+dy, tx0+dx, ty1);
  WebMapTilesRequest::addTile(new WebMapTile(tileX, tileY, rect));
}

QNetworkReply* WebMapSlippyOSMRequest::submitRequest(WebMapTile* tile)
{
  return static_cast<WebMapSlippyOSM_x>(mService)->submitRequest(mLayer, mZoom, tile->column(), tile->row());
}

const Projection& WebMapSlippyOSMRequest::tileProjection() const
{
  return static_cast<WebMapSlippyOSM_x>(mService)->projection();
}
