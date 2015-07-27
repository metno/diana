/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015 MET Norway

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

#include <diField/diArea.h>
#include <puTools/miStringFunctions.h>

#include <QDomDocument>
#include <QDomElement>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>

#include <sys/time.h>

#define MILOGGER_CATEGORY "diana.WebMapSlippyOSM"
#include <miLogger/miLogging.h>

#ifdef HAVE_CONFIG_H
#include "config.h" // for PVERSION
#endif

namespace /* anonymous */ {

const std::string EPSG_3857 = "EPSG:3857";

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

// ========================================================================

WebMapSlippyOSMRequest::WebMapSlippyOSMRequest(WebMapSlippyOSM_x service,
    WebMapSlippyOSMLayer_cx layer, int zoom)
  : mService(service)
  , mLayer(layer)
  , mZoom(zoom)
{
}

WebMapSlippyOSMRequest::~WebMapSlippyOSMRequest()
{
  diutil::delete_all_and_clear(mTiles);
}

void WebMapSlippyOSMRequest::addTile(int tileX, int tileY)
{
  const int nxy = (1<<mZoom);
  const double x0 = -EARTH_CICRUMFERENCE_M/2,
      dx = EARTH_CICRUMFERENCE_M / nxy,
      y0 = -x0,
      dy = -dx;
  const double tx0 = x0 + dx*tileX,
      ty1 = y0 + dy*tileY;
  const Rectangle rect(tx0, ty1+dy, tx0+dx, ty1);
  mTiles.push_back(new WebMapTile(tileX, tileY, rect));
}

void WebMapSlippyOSMRequest::submit()
{
  METLIBS_LOG_SCOPE();
  mUnfinished = mTiles.size();
  for (size_t i=0; i<mTiles.size(); ++i) {
    WebMapTile* tile = mTiles[i];
    connect(tile, SIGNAL(finished(WebMapTile*)),
        this, SLOT(tileFinished(WebMapTile*)));
    QNetworkReply* reply = mService->submitRequest(mLayer, mZoom,
        tile->column(), tile->row());
    tile->submit(reply);
  }
}

void WebMapSlippyOSMRequest::abort()
{
  METLIBS_LOG_SCOPE();
  for (size_t i=0; i<mTiles.size(); ++i)
    mTiles[i]->abort();
}

void WebMapSlippyOSMRequest::tileFinished(WebMapTile* tile)
{
  METLIBS_LOG_SCOPE();
  tile->loadImage(mLayer->tileFormat());
  mUnfinished -= 1;
  METLIBS_LOG_DEBUG(LOGVAL(mUnfinished));
  if (mUnfinished == 0)
    Q_EMIT completed();
}

const Rectangle& WebMapSlippyOSMRequest::tileRect(size_t idx) const
{
  return mTiles.at(idx)->rect();
}

const QImage& WebMapSlippyOSMRequest::tileImage(size_t idx) const
{
  return mTiles.at(idx)->image();
}

const Projection& WebMapSlippyOSMRequest::tileProjection() const
{
  return mService->projection();
}

// ========================================================================

const std::string& WebMapSlippyOSMLayer::CRS(size_t) const
{
  return EPSG_3857;
}

// ========================================================================

WebMapSlippyOSM::WebMapSlippyOSM(const std::string& identifier, const QUrl& url, QNetworkAccessManager* network)
  : WebMapService(identifier)
  , mServiceURL(url)
  , mNetworkAccess(network)
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
    const Rectangle& viewRect, const Projection& viewProj, double viewScale)
{
  METLIBS_LOG_SCOPE();
  WebMapSlippyOSMLayer_cx layer = static_cast<WebMapSlippyOSMLayer_cx>
      (findLayerByIdentifier(layerIdentifier));
  if (!layer)
    return 0;

  int zoom = findZoomForScale(viewScale);
  if (zoom < layer->minZoom() || zoom > layer->maxZoom())
    return 0;

  const int nxy = (1<<zoom);
  const float x0 = -EARTH_CICRUMFERENCE_M/2,
      dx = EARTH_CICRUMFERENCE_M / nxy,
      y0 = -x0,
      dy = -dx;

  diutil::tilexy_s tiles;
  diutil::select_tiles(tiles, 0, nxy, x0, dx, 0, nxy, y0, dy,
      mProjection, viewRect, viewProj);

  std::auto_ptr<WebMapSlippyOSMRequest> request(new WebMapSlippyOSMRequest(this, layer, zoom));
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

#if 1
  QNetworkRequest nr(QUrl(QString::fromStdString(url)));
  nr.setRawHeader("User-Agent", "diana " PVERSION);
  return mNetworkAccess->get(nr);
#else
  return 0;
#endif
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

  mRefeshReply = mNetworkAccess->get(QNetworkRequest(mServiceURL));
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

    std::auto_ptr<WebMapSlippyOSMLayer> layer(new WebMapSlippyOSMLayer(sId));
    layer->setTitle(sTitle);
    layer->setAttribution(sAttribution);
    layer->setURLTemplate(sTemplate);
    layer->setZoomRange(sMinZoom, sMaxZoom);
    layer->setTileFormat(sFormat);
    mLayers.push_back(layer.release());
  }

  return true;
}
