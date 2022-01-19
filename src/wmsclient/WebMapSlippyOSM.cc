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

#include "WebMapSlippyOSM.h"

#include "diUtilities.h"
#include "WebMapUtilities.h"
#include "WebMapSlippyOSMLayer.h"
#include "WebMapSlippyOSMRequest.h"

#include <diField/diArea.h>
#include <puTools/miStringFunctions.h>

#include <QDomDocument>
#include <QDomElement>
#include <QNetworkReply>
#include <QStringList>

#include <sys/time.h>

#define MILOGGER_CATEGORY "diana.WebMapSlippyOSM"
#include <miLogger/miLogging.h>

namespace /* anonymous */ {

const int TILESIZE = 256;

const double EARTH_CICRUMFERENCE_M = diutil::WMTS_EARTH_RADIUS_M * 2 * M_PI;

const double ZOOM_0_DENOMINATOR =
    // zoom 0 divides earth circumference in TILESIZE pixels
    (EARTH_CICRUMFERENCE_M / TILESIZE)
    // scale denominator is above divided by standard meter per pixel
    // (WMS 1.3.0 spec 7.2.4.6.9)
    / diutil::WMTS_M_PER_PIXEL;

int findZoomForScale(float denominator)
{
  METLIBS_LOG_SCOPE(LOGVAL(denominator));
  if (denominator <= 0)
    return 0;

  return std::max(0, (int) round(log(ZOOM_0_DENOMINATOR / denominator) / log(2)) - 1);
}

} // anonymous namespace

WebMapSlippyOSM::WebMapSlippyOSM(const std::string& identifier, const QUrl& url, QNetworkAccessManager* network)
  : WebMapService(identifier, network)
  , mServiceURL(url)
  , mNextRefresh(0)
  , mRefeshReply(0)
  , mProjection("+init=epsg:3857")
{
  METLIBS_LOG_SCOPE();
}

WebMapSlippyOSM::~WebMapSlippyOSM()
{
  if (mRefeshReply)
    mRefeshReply->deleteLater();
}

int WebMapSlippyOSM::refreshInterval() const
{
  return 3600;
}

WebMapRequest_x WebMapSlippyOSM::createRequest(const std::string& layerIdentifier,
    const Rectangle& viewRect, const Projection& viewProj, double viewScale, int w, int h)
{
  METLIBS_LOG_SCOPE();
  WebMapSlippyOSMLayer_cx layer = static_cast<WebMapSlippyOSMLayer_cx>
      (findLayerByIdentifier(layerIdentifier));
  if (!layer)
    return 0;

  int zoom = findZoomForScale(viewScale);
  if (zoom < layer->minZoom() || zoom > layer->maxZoom())
    return 0;

  std::unique_ptr<WebMapSlippyOSMRequest> request(new WebMapSlippyOSMRequest(this, layer, zoom));
  const int nxy = (1<<zoom);
  request->x0 = -EARTH_CICRUMFERENCE_M/2;
  request->dx = EARTH_CICRUMFERENCE_M / nxy;
  request->y0 = -request->x0;
  request->dy = -request->dx;
  request->tilebbx = Rectangle(request->x0, -request->y0, -request->x0, request->y0);

  diutil::tilexy_s tiles;
  diutil::select_pixel_tiles(tiles, w, h,
                             nxy, request->x0, request->dx,
                             nxy, request->y0, request->dy,
                             request->tilebbx, mProjection, viewRect, viewProj);
  METLIBS_LOG_DEBUG(LOGVAL(tiles.size()));

  for (diutil::tilexy_s::const_iterator it = tiles.begin(); it != tiles.end(); ++it)
    request->addTile(it->x, it->y);

  return request.release();
}

QNetworkReply* WebMapSlippyOSM::submitRequest(WebMapSlippyOSMLayer_cx layer,
    int zoom, int tileX, int tileY)
{
  METLIBS_LOG_SCOPE();
  std::string url = layer->urlTemplate();
  miutil::replace(url, "{zoom}", miutil::from_number(zoom));
  miutil::replace(url, "{x}", miutil::from_number(tileX));
  miutil::replace(url, "{y}", miutil::from_number(tileY));
  METLIBS_LOG_DEBUG(LOGVAL(url));

  return submitUrl(QUrl(QString::fromStdString(url)));
}

void WebMapSlippyOSM::refresh()
{
  METLIBS_LOG_SCOPE(LOGVAL(mNextRefresh));
  if (mRefeshReply)
    return;

  timeval now;
  gettimeofday(&now, 0);

  if (mNextRefresh >= now.tv_sec)
    return;

  mNextRefresh = now.tv_sec + refreshInterval();
  Q_EMIT refreshStarting();

  mRefeshReply = submitUrl(mServiceURL);
  connect(mRefeshReply, SIGNAL(finished()), this, SLOT(refreshReplyFinished()));
}

void WebMapSlippyOSM::refreshReplyFinished()
{
  METLIBS_LOG_SCOPE();
  destroyLayers();

  parseReply();

  mRefeshReply->deleteLater();
  mRefeshReply = 0;

  Q_EMIT refreshFinished();
}

bool WebMapSlippyOSM::parseReply()
{
  METLIBS_LOG_SCOPE();
  using diutil::qs;

  if (mRefeshReply->error() != QNetworkReply::NoError) {
    METLIBS_LOG_DEBUG("SlippyMaps request error");
    return false;
  }

  QDomDocument doc;
  if (!doc.setContent(mRefeshReply)) {
    METLIBS_LOG_DEBUG("SlippyMaps document error");
    return false;
  }

  QDomElement eSlippy = doc.documentElement();
  if (eSlippy.attribute("version") != "1.0.0") {
    METLIBS_LOG_DEBUG("unsupported version");
    return false;
  }

  mTitle = qs(eSlippy.firstChildElement("title").text());
  METLIBS_LOG_DEBUG(LOGVAL(mTitle));

  QDomElement eServices = eSlippy.firstChildElement("services");
  QDOM_FOREACH_CHILD(eService, eServices, "service") {
    const std::string sId = qs(eService.attribute("id"));
    METLIBS_LOG_DEBUG(LOGVAL(sId));
    const std::string sFormat = qs(eService.attribute("format"));
    const std::string sTemplate = qs(eService.attribute("url_template"));
    int sMinZoom = eService.attribute("min_zoom").toInt();
    int sMaxZoom = eService.attribute("max_zoom").toInt();

    const std::string sTitle = qs(eService.firstChildElement("title").text());
    const std::string sAttribution = qs(eService.firstChildElement("attribution").text());

    std::unique_ptr<WebMapSlippyOSMLayer> layer(new WebMapSlippyOSMLayer(sId));
    layer->setTitle(sTitle);
    layer->setAttribution(sAttribution);
    layer->setURLTemplate(sTemplate);
    layer->setZoomRange(sMinZoom, sMaxZoom);
    layer->setTileFormat(sFormat);
    mLayers.push_back(layer.release());
  }

  return true;
}
