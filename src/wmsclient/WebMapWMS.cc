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

#include "WebMapWMS.h"

#include "diUtilities.h"
#include "WebMapTile.h"
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

#define MILOGGER_CATEGORY "diana.WebMapWMS"
#include <miLogger/miLogging.h>

#ifdef HAVE_CONFIG_H
#include "config.h" // for PVERSION
#endif

namespace /* anonymous */ {

const int TILESIZE = 512;

int findZoomForScale(float z0denominator, float denominator)
{
  if (denominator <= 0)
    return 0;

  return std::max(0, (int) round(log(z0denominator / denominator) / log(2)) - 1);
}

bool hasAttributeValue(const QDomElement& e, const QString& a, const QStringList& values)
{
  if (!e.hasAttribute(a))
    return false;
  else
    return values.contains(e.attribute(a));
}

float toDecimalDegrees(const std::string& crs)
{
  if (crs == "EPSG:4326" || crs == "CRS:84")
    return RAD_TO_DEG;
  else
    return 1;
}

} // anonymous namespace

// ========================================================================

WebMapWmsCrsBoundingBox::WebMapWmsCrsBoundingBox(const std::string& c, const Rectangle& bb)
  : crs(c)
  , projection(diutil::projectionForCRS(crs))
  , metersPerUnit(diutil::metersPerUnit(projection))
  , boundingbox(bb)
{
}

// ========================================================================

WebMapWMSLayer::WebMapWMSLayer(const std::string& identifier)
  : WebMapLayer(identifier)
  , mMinZoom(0)
  , mMaxZoom(13)
{
}

// ========================================================================

WebMapWMSRequest::WebMapWMSRequest(WebMapWMS_x service,
    WebMapWMSLayer_cx layer, int crsIndex, int zoom)
  : mService(service)
  , mLayer(layer)
  , mCrsIndex(crsIndex)
  , mZoom(zoom)
  , mLegend(0)
{
}

WebMapWMSRequest::~WebMapWMSRequest()
{
  diutil::delete_all_and_clear(mTiles);
  delete mLegend;
}

void WebMapWMSRequest::setDimensionValue(const std::string& dimIdentifier,
    const std::string& dimValue)
{
  mDimensionValues[dimIdentifier] = dimValue;
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
  const Rectangle rect(tx0, ty1+dy, tx0+dx, ty1);
  mTiles.push_back(new WebMapTile(tileX, tileY, rect));
}

void WebMapWMSRequest::submit()
{
  METLIBS_LOG_SCOPE();
  mUnfinished = mTiles.size();

  if (!mLayer->legendUrl().empty()) {
    mLegend = new WebMapImage();
    connect(mLegend, SIGNAL(finishedImage(WebMapImage*)),
        this, SLOT(legendFinished(WebMapImage*)));
    mUnfinished += 1;
    mLegend->submit(mService->submitUrl(QUrl(QString::fromStdString(mLayer->legendUrl()))));
  }

  for (size_t i=0; i<mTiles.size(); ++i) {
    WebMapTile* tile = mTiles[i];
    connect(tile, SIGNAL(finished(WebMapTile*)),
        this, SLOT(tileFinished(WebMapTile*)));
    QNetworkReply* reply = mService->submitRequest(mLayer, mDimensionValues,
        mLayer->CRS(mCrsIndex), tile);
    tile->submit(reply);
  }
}

void WebMapWMSRequest::abort()
{
  METLIBS_LOG_SCOPE();
  if (mLegend)
    mLegend->abort();
  for (size_t i=0; i<mTiles.size(); ++i)
    mTiles[i]->abort();
}

bool WebMapWMSRequest::checkRedirect(WebMapImage* image)
{
  METLIBS_LOG_SCOPE();
  if (!image->reply())
    return false;

  METLIBS_LOG_DEBUG("http status=" << image->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());

  const QVariant vRedirect = image->reply()->attribute(QNetworkRequest::RedirectionTargetAttribute);
  if (vRedirect.isNull())
    return false;

  QUrl redirect = vRedirect.toUrl();
  if (redirect.isRelative())
    redirect = image->reply()->url().resolved(redirect);
  METLIBS_LOG_DEBUG("redirect to '" << redirect.toString().toStdString() << "'");
  image->submit(mService->submitUrl(redirect));
  return true;
}

void WebMapWMSRequest::tileFinished(WebMapTile* tile)
{
  METLIBS_LOG_SCOPE();
  if (checkRedirect(tile))
    return;
  if (!tile->loadImage(mService->tileFormat()))
    tile->dummyImage(TILESIZE, TILESIZE);
  mUnfinished -= 1;
  METLIBS_LOG_DEBUG(LOGVAL(mUnfinished));
  if (mUnfinished == 0)
    Q_EMIT completed();
}

void WebMapWMSRequest::legendFinished(WebMapImage*)
{
  METLIBS_LOG_SCOPE();
  if (checkRedirect(mLegend))
    return;
  mLegend->loadImage("image/png");
  mUnfinished -= 1;
  METLIBS_LOG_DEBUG(LOGVAL(mUnfinished));
  if (mUnfinished == 0)
    Q_EMIT completed();
}

QImage WebMapWMSRequest::legendImage() const
{
  if (mLegend)
    return mLegend->image();
  else
    return QImage();
}

const Rectangle& WebMapWMSRequest::tileRect(size_t idx) const
{
  return mTiles.at(idx)->rect();
}

const QImage& WebMapWMSRequest::tileImage(size_t idx) const
{
  return mTiles.at(idx)->image();
}

// ========================================================================

WebMapWMS::WebMapWMS(const std::string& identifier, const QUrl& url, QNetworkAccessManager* network)
  : WebMapService(identifier)
  , mServiceURL(url)
  , mNetworkAccess(network)
  , mNextRefresh(0)
  , mRefeshReply(0)
{
  METLIBS_LOG_SCOPE();
}

WebMapWMS::~WebMapWMS()
{
  if (mRefeshReply)
    mRefeshReply->deleteLater();
}

int WebMapWMS::refreshInterval() const
{
  return 3600;
}

WebMapRequest_x WebMapWMS::createRequest(const std::string& layerIdentifier,
    const Rectangle& viewRect, const Projection& viewProj, double viewScale)
{
  METLIBS_LOG_SCOPE(LOGVAL(layerIdentifier));
  WebMapWMSLayer_cx layer = static_cast<WebMapWMSLayer_cx>
      (findLayerByIdentifier(layerIdentifier));
  if (!layer || layer->countCRS() == 0) {
    METLIBS_LOG_DEBUG("no layer, or no CRS");
    return 0;
  }

  const int crsIndex = 0;

  const WebMapWmsCrsBoundingBox& cb = layer->crsBoundingBox(crsIndex);
  const Rectangle& bb = cb.boundingbox;
  METLIBS_LOG_DEBUG(LOGVAL(cb.crs) << LOGVAL(bb) << LOGVAL(cb.projection.getProjDefinitionExpanded()) << LOGVAL(cb.metersPerUnit));

  const float z0denominator = bb.width() * cb.metersPerUnit / TILESIZE / diutil::WMTS_M_PER_PIXEL;
  const int zoom = findZoomForScale(z0denominator, viewScale);
  METLIBS_LOG_DEBUG(LOGVAL(z0denominator) << LOGVAL(viewScale) << LOGVAL(zoom));
  if (zoom < layer->minZoom() || zoom > layer->maxZoom())
    return 0;

  const int nx = (1<<zoom);
  const float x0 = bb.x1,
      dx = bb.width() / nx,
      y0 = bb.y2,
      dy = -dx;
  const int ny = (int) std::ceil(bb.height() / dx);
  METLIBS_LOG_DEBUG(LOGVAL(nx) << LOGVAL(ny) << LOGVAL(x0) << LOGVAL(dx) << LOGVAL(y0) << LOGVAL(dy));

  diutil::tilexy_s tiles;
  diutil::select_tiles(tiles, 0, nx, x0, dx, 0, ny, y0, dy,
      cb.projection, viewRect, viewProj);
  METLIBS_LOG_DEBUG(LOGVAL(tiles.size()));

  std::auto_ptr<WebMapWMSRequest> request(new WebMapWMSRequest(this, layer, crsIndex, zoom));
  for (diutil::tilexy_s::const_iterator it = tiles.begin(); it != tiles.end(); ++it)
    request->addTile(it->x, it->y);

  return request.release();
}

QNetworkReply* WebMapWMS::submitRequest(WebMapWMSLayer_cx layer,
    const std::map<std::string, std::string>& dimensionValues,
    const std::string& crs, WebMapTile* tile)
{
  QUrl qurl = mServiceURL;
  qurl.addQueryItem("SERVICE", "WMS");
  qurl.addQueryItem("REQUEST", "GetMap");
  qurl.addQueryItem("VERSION", (mVersion == WMS_130) ? "1.3.0" : "1.1.1");
  qurl.addQueryItem("TRANSPARENT", "TRUE");
  qurl.addQueryItem("LAYERS", diutil::sq(layer->identifier()));
  qurl.addQueryItem("STYLES", diutil::sq(layer->defaultStyle()));
  qurl.addQueryItem("FORMAT", diutil::sq(tileFormat()));

  for (size_t d = 0; d<layer->countDimensions(); ++d) {
    const WebMapDimension& dim = layer->dimension(d);
    std::string dimKey = miutil::to_lower(dim.identifier());
    if (dimKey != "time" && dimKey != "elevation")
      dimKey = "dim_" + dim.identifier(); // FIXME this is just an assumption
    QString dimValue = diutil::sq(dim.defaultValue());
    std::map<std::string, std::string>::const_iterator itD
        = dimensionValues.find(dim.identifier());
    if (itD != dimensionValues.end()) {
      if (std::find(dim.values().begin(), dim.values().end(), itD->second) != dim.values().end())
        dimValue = diutil::sq(itD->second);
    }
    qurl.addQueryItem(diutil::sq(dimKey), dimValue);
  }
  qurl.addQueryItem("WIDTH", QString::number(TILESIZE));
  qurl.addQueryItem("HEIGHT", QString::number(TILESIZE));

  const QString aCRS = ((mVersion == WMS_111) ? "SRS" : "CRS");
  qurl.addQueryItem(aCRS, QString::fromStdString(crs));

  const Rectangle& r = tile->rect();
  const float f = toDecimalDegrees(crs);
  qurl.addEncodedQueryItem("BBOX", QString("%1,%2,%3,%4")
      .arg(f*r.x1).arg(f*r.y1).arg(f*r.x2).arg(f*r.y2).replace("+", "%2B").toUtf8());

  METLIBS_LOG_DEBUG("url='" << qurl.toEncoded().constData() << "' x=" << tile->column() << " y=" << tile->row());
  return submitUrl(qurl);
}

QNetworkReply* WebMapWMS::submitUrl(const QUrl& url)
{
#if 1
  QNetworkRequest nr(url);
  nr.setRawHeader("User-Agent", "diana " PVERSION);
  return mNetworkAccess->get(nr);
#else
  return 0;
#endif
}

void WebMapWMS::refresh()
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

void WebMapWMS::refreshReplyFinished()
{
  METLIBS_LOG_SCOPE();
  destroyLayers();

  parseReply();

  mRefeshReply->deleteLater();
  mRefeshReply = 0;

  Q_EMIT refreshFinished();
}

bool WebMapWMS::parseReply()
{
  METLIBS_LOG_SCOPE();
  using diutil::qs;

  if (mRefeshReply->error() != QNetworkReply::NoError) {
    METLIBS_LOG_DEBUG("request error " <<  mRefeshReply->error()
        << ", HTTP status=" << mRefeshReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
    return false;
  }

  QDomDocument doc;
  if (!doc.setContent(mRefeshReply)) {
    METLIBS_LOG_DEBUG("document error");
    return false;
  }

  QDomElement eDoc = doc.documentElement();
  if (eDoc.attribute("version") == "1.1.1") {
    mVersion = WMS_111;
  } else if (eDoc.attribute("version") == "1.3.0") {
    mVersion = WMS_130;
  } else {
    METLIBS_LOG_DEBUG("unsupported version");
    return false;
  }
  METLIBS_LOG_DEBUG(LOGVAL(mVersion));

  QDomElement eService = eDoc.firstChildElement("Service");
  mTitle = qs(eService.firstChildElement("Title").text());
  METLIBS_LOG_DEBUG(LOGVAL(mTitle));

  QDomElement eMaxWidth = eService.firstChildElement("MaxWidth");
  QDomElement eMaxHeight = eService.firstChildElement("MaxHeight");
  if ((!eMaxWidth.isNull() && eMaxWidth.text().toInt() < TILESIZE)
      || (!eMaxHeight.isNull() && eMaxHeight.text().toInt() < TILESIZE))
  {
    METLIBS_LOG_DEBUG("unsupported max width/height");
    return false;
  }

  QDomElement eCapability = eDoc.firstChildElement("Capability");
  {
    QDomElement eRequest = eCapability.firstChildElement("Request");
    QDomElement eGetMap = eRequest.firstChildElement("GetMap");

    std::set<std::string> formats;
    QDOM_FOREACH_CHILD(eFormat, eGetMap, "Format") {
      formats.insert(qs(eFormat.text()));
    }
    // prefer png, alternatively jpeg
    std::set<std::string>::const_iterator it = formats.find("image/png");
    if (it == formats.end())
      it = formats.find("image/jpeg");
    if (it != formats.end()) {
      mTileFormat = *it;
    } else {
      METLIBS_LOG_DEBUG("unsupported tile format");
      return false;
    }

    QDomElement eDCPType = eGetMap.firstChildElement("DCPType");
    QDomElement eHTTP = eDCPType.firstChildElement("HTTP");
    QDomElement eGet = eHTTP.firstChildElement("Get");
    if (eGet.isNull()) {
      METLIBS_LOG_DEBUG("unsupported GetMap, only HTTP GET is supported");
      return false;
    }

    QDomElement eOnlineResource = eGet.firstChildElement("OnlineResource");
    mServiceURL = eOnlineResource.attribute("xlink:href");
  }

  const std::string top_style, top_legendUrl;
  const crs_bbox_m top_crs_bboxes;
  const WebMapDimension_v top_dimensions;
  QDOM_FOREACH_CHILD(eLayer, eCapability, "Layer") {
    if (!parseLayer(eLayer, top_style, top_legendUrl, top_crs_bboxes, top_dimensions))
      return false;
  }

  return true;
}

bool WebMapWMS::parseLayer(QDomElement& eLayer, std::string style, std::string legendUrl,
    crs_bbox_m crs_bboxes, std::vector<WebMapDimension> dimensions)
{
  METLIBS_LOG_SCOPE();
  using diutil::qs;

  QDomElement eLayerName = eLayer.firstChildElement("Name");
  METLIBS_LOG_DEBUG("layer name='" << eLayerName.text().toStdString() << "'");
  const bool hasContent = !eLayerName.isNull();

  const QString aCRS = ((mVersion == WMS_111) ? "SRS" : "CRS");

  // loop over BoundingBox elements to extract explicit bbox'es
  QDOM_FOREACH_CHILD(eBoundingBox, eLayer, "BoundingBox") {
    std::string sCRS = qs(eBoundingBox.attribute(aCRS));
    METLIBS_LOG_DEBUG("explicit bbox check" << LOGVAL(sCRS));
    if (sCRS == "EPSG:900913")
      sCRS = "EPSG:3857";
    const float f = 1 / toDecimalDegrees(sCRS);
    const float minx = f * eBoundingBox.attribute("minx").toFloat();
    const float miny = f * eBoundingBox.attribute("miny").toFloat();
    const float maxx = f * eBoundingBox.attribute("maxx").toFloat();
    const float maxy = f * eBoundingBox.attribute("maxy").toFloat();
    crs_bboxes[sCRS] = Rectangle(minx, miny, maxx, maxy);
    METLIBS_LOG_DEBUG(LOGVAL(sCRS) << LOGVAL(minx) << LOGVAL(maxx) << LOGVAL(miny) << LOGVAL(maxy));
  }

  // loop over SRS/CRS elements to extract known bbox'es
  QStringList lCRS;
  QDOM_FOREACH_CHILD(eCRS, eLayer, aCRS) {
    if (mVersion == WMS_111)
      lCRS << eCRS.text().split(" ", QString::SkipEmptyParts);
    else
      lCRS << eCRS.text();
  }
  if (lCRS.contains("EPSG:900913")) {
    lCRS.removeAll("EPSG:900913");
    lCRS << "EPSG:3857";
  }
  Q_FOREACH(const QString& crs, lCRS) {
    float minx, miny, maxx, maxy;
    const std::string sCRS = crs.toStdString();
    METLIBS_LOG_DEBUG("known bbox check" << LOGVAL(sCRS));
    crs_bbox_m::iterator it = crs_bboxes.find(sCRS);
    if (sCRS == "EPSG:3857") {
      static const float M = 2.00375e+07;
      if (it != crs_bboxes.end()) {
        minx = std::max(-M, it->second.x1);
        miny = std::max(-M, it->second.y1);
        maxx = std::min( M, it->second.x2);
        maxy = std::min( M, it->second.y2);
      } else {
        minx = miny = -M;
        maxx = maxy =  M;
      }
    } else if (sCRS == "EPSG:4326" && it == crs_bboxes.end()) {
      if (mVersion == WMS_130) {
        QDomElement eBBox = eLayer.firstChildElement("EX_GeographicBoundingBox");
        if (eBBox.isNull())
          continue;
        minx = eBBox.firstChildElement("westBoundLongitude").text().toFloat() * DEG_TO_RAD;
        maxx = eBBox.firstChildElement("eastBoundLongitude").text().toFloat() * DEG_TO_RAD;
        miny = eBBox.firstChildElement("southBoundLatitude").text().toFloat() * DEG_TO_RAD;
        maxy = eBBox.firstChildElement("northBoundLatitude").text().toFloat() * DEG_TO_RAD;
      } else {
        QDomElement eBBox = eLayer.firstChildElement("LatLonBoundingBox");
        if (eBBox.isNull())
          continue;
        minx = eBBox.attribute("minx").toFloat() * DEG_TO_RAD;
        miny = eBBox.attribute("miny").toFloat() * DEG_TO_RAD;
        maxx = eBBox.attribute("maxx").toFloat() * DEG_TO_RAD;
        maxy = eBBox.attribute("maxy").toFloat() * DEG_TO_RAD;
      }
    } else if (sCRS == "CRS:84" && it == crs_bboxes.end()) {
      maxx = 180 * DEG_TO_RAD;
      minx = -maxx;
      maxy = 90 * DEG_TO_RAD;
      miny = -maxy;
    } else {
      continue;
    }
    const Rectangle bb(minx, miny, maxx, maxy);
    if (bb.width() > 0 && bb.height() > 0)
      crs_bboxes[sCRS] = bb;
  }

  // TODO implement ScaleHint for WMS 1.1.1 and Min/MaxScaleDenominator for WMS 1.3.0

  QDOM_FOREACH_CHILD(eDim, eLayer, "Dimension") {
    const std::string sName = diutil::qs(eDim.attribute("name"));
    const std::string sUnits = qs(eDim.attribute("units"));
    METLIBS_LOG_DEBUG(LOGVAL(sName) << LOGVAL(sUnits));

    const std::string lName = miutil::to_lower(sName);
    const bool isTime = (lName == "time");
    const bool isElevation = (lName == "elevation");
    if (isTime && sUnits != "ISO8601") {
      METLIBS_LOG_DEBUG("time dimension with bad unit '" << sUnits << "'");
      return false;
    }
#if 0
    if (isElevation && !diutil::startswith(sUnits, "EPSG:")) {
      METLIBS_LOG_DEBUG("elevation dimension with bad unit '" << sUnits << "'");
      return false;
    }
#endif

    size_t iDim = 0;
    for (; iDim < dimensions.size(); ++iDim)
      if (dimensions[iDim].identifier() == sName)
        break;
    if (iDim == dimensions.size())
      dimensions.push_back(WebMapDimension(sName));
    WebMapDimension& dim = dimensions[iDim];
    if (mVersion == WMS_130) {
      parseDimensionValues(dim, eDim.text(), eDim.attribute("default"));
    } else if (mVersion == WMS_111) {
      // values come in "Extent"
    }
  }

  if (mVersion == WMS_111) {
    QDOM_FOREACH_CHILD(eExtent, eLayer, "Extent") {
      const std::string sName = qs(eExtent.attribute("name"));
      // find corresponding WebMapDimension object
      size_t iDim = 0;
      for (; iDim < dimensions.size(); ++iDim)
        if (dimensions[iDim].identifier() == sName)
          break;
      if (iDim == dimensions.size()) {
        METLIBS_LOG_DEBUG("extent given for unknown dimension '" << sName << "'");
        return false;
      }
      parseDimensionValues(dimensions[iDim], eExtent.text(), eExtent.attribute("default"));
    }
  }

  QDomElement eStyle1 = eLayer.firstChildElement("Style");
  QDomElement eStyle1Name = eStyle1.firstChildElement("Name");
  if (!eStyle1Name.isNull()) {
    style = qs(eStyle1Name.text());
    QDomElement eStyle1LegendUrl = eStyle1.firstChildElement("LegendURL").firstChildElement("OnlineResource");
    if (!eStyle1LegendUrl.isNull())
      legendUrl = qs(eStyle1LegendUrl.attribute("xlink:href"));
  }

  const std::string sLayerName = qs(eLayerName.text());
  const bool goodName = !sLayerName.empty();
  const bool unusedName = findLayerByIdentifier(sLayerName) == 0;
  const bool tileable =
      !hasAttributeValue(eLayer, "noSubsets", (QStringList() << "true" << "1"))
      && !hasAttributeValue(eLayer, "fixedWidth", (QStringList() << "0"))
      && !hasAttributeValue(eLayer, "fixedHeight", (QStringList() << "0"));
  const bool hasCRS = !crs_bboxes.empty();

  METLIBS_LOG_DEBUG(LOGVAL(hasContent) << LOGVAL(goodName) << LOGVAL(unusedName) << LOGVAL(tileable) << LOGVAL(hasCRS));

  if (hasContent && goodName && unusedName && tileable && hasCRS) {
    METLIBS_LOG_DEBUG("adding layer '" << sLayerName << "'");
    std::auto_ptr<WebMapWMSLayer> layer(new WebMapWMSLayer(sLayerName));
    layer->setTitle(qs(eLayer.firstChildElement("Title").text()));
    for (crs_bbox_m::const_iterator it = crs_bboxes.begin(); it != crs_bboxes.end(); ++it)
      layer->addCRS(it->first, it->second);
    layer->setDefaultStyle(style, legendUrl);
    layer->setDimensions(dimensions);
    mLayers.push_back(layer.release());
  } else {
    METLIBS_LOG_DEBUG("skipping layer '" << sLayerName << "'");
  }

  QDOM_FOREACH_CHILD(eChildLayer, eLayer, "Layer") {
    if (!parseLayer(eChildLayer, style, legendUrl, crs_bboxes, dimensions))
      return false;
  }

  return true;
}

void WebMapWMS::parseDimensionValues(WebMapDimension& dim, const QString& text, const QString& defaultValue)
{
  METLIBS_LOG_SCOPE(LOGVAL(dim.identifier()));
  using diutil::qs;

  dim.clearValues();

  const bool isTime = dim.isTime();
  const QStringList values = text.trimmed().split(QRegExp(",\\s*"));
  Q_FOREACH(const QString& v, values) {
    if (v.contains("/")) {
      QStringList expanded;
      if (isTime)
        expanded = diutil::expandWmsTimes(v);
      else
        expanded = diutil::expandWmsValues(v);
      Q_FOREACH(const QString& ev, expanded) {
        dim.addValue(qs(ev), ev == defaultValue);
      }
    } else {
      dim.addValue(qs(v), v == defaultValue);
    }
  }
}
