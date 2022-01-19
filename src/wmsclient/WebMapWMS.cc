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

#include "WebMapWMS.h"

#include "WebMapTile.h"
#include "WebMapUtilities.h"

#include <puTools/miStringFunctions.h>

#include <QDomDocument>
#include <QDomElement>
#include <QNetworkReply>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QUrlQuery>
#endif

#include <algorithm>

#include <sys/time.h>

#define MILOGGER_CATEGORY "diana.WebMapWMS"
#include <miLogger/miLogging.h>

namespace /* anonymous */ {

const int TILESIZE = 512;

const char CRS84[] = "CRS:84";
const char EPSG3857[] = "EPSG:3857";
const char EPSG4326[] = "EPSG:4326";
const char EPSG900913[] = "EPSG:900913";

int findZoomForScale(float z0denominator, float denominator)
{
  if (denominator <= 0)
    return 0;

  return std::max(0, (int) round(log(z0denominator / denominator) / log(2)) - 1);
}

int findZoom(WebMapWmsCrsBoundingBox_cx cb, double viewScale)
{
  const Rectangle& bb = cb->boundingbox;
  const float z0denominator = bb.width() * cb->metersPerUnit / TILESIZE / diutil::WMTS_M_PER_PIXEL;
  return findZoomForScale(z0denominator, viewScale);
}

bool hasAttributeValue(const QDomElement& e, const QString& a, const QStringList& values)
{
  if (!e.hasAttribute(a))
    return false;
  else
    return values.contains(e.attribute(a));
}

bool isGeographic(const std::string& crs)
{
  return (crs == EPSG4326 || crs == CRS84);
}

float toDecimalDegrees(const std::string& crs)
{
  if (isGeographic(crs))
    return RAD_TO_DEG;
  else
    return 1;
}

void swapWms130LatLon(const std::string& crs, float& x1, float& y1, float& x2, float& y2)
{
  if (crs == EPSG4326) {
    std::swap(x1, y1);
    std::swap(x2, y2);
  }
}

inline QString toQString(float number)
{
  return QString::number(number, 'f', -1);
}

} // anonymous namespace

WebMapWMS::WebMapWMS(const std::string& identifier, const QUrl& url, QNetworkAccessManager* network)
  : WebMapService(identifier, network)
  , mServiceURL(url)
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

WebMapWmsCrsBoundingBox_cx WebMapWMS::findBestCRS(WebMapWMSLayer_cx layer, const Rectangle& viewRect, const Projection& viewProj, double viewScale) const
{
  METLIBS_LOG_SCOPE(LOGVAL(viewRect) << LOGVAL(viewProj));
  const size_t n_crs = layer->countCRS();
  float best_distortion = -1;
  WebMapWmsCrsBoundingBox_cx best_cb = nullptr;
  for (size_t ci = 0; ci < n_crs; ++ci) {
    const WebMapWmsCrsBoundingBox& cb = layer->crsBoundingBox(ci);
    const int zoom = findZoom(&cb, viewScale);
    if (zoom >= layer->minZoom() && zoom <= layer->maxZoom()) {
      const float crs_distortion = diutil::distortion(cb.projection, viewProj, viewRect);
      if (best_distortion < 0 || crs_distortion < best_distortion) {
        best_distortion = crs_distortion;
        best_cb = &cb;
        METLIBS_LOG_DEBUG(LOGVAL(best_cb->crs));
      }
    }
  }
  return best_cb;
}

WebMapRequest_x WebMapWMS::createRequest(const std::string& layerIdentifier,
    const Rectangle& viewRect, const Projection& viewProj, double viewScale, int w, int h)
{
  METLIBS_LOG_SCOPE(LOGVAL(layerIdentifier));
  WebMapWMSLayer_cx layer = static_cast<WebMapWMSLayer_cx>(findLayerByIdentifier(layerIdentifier));
  if (!layer) {
    METLIBS_LOG_DEBUG("no layer, cannot create request");
    return 0;
  }

  const WebMapWmsCrsBoundingBox_cx cb = findBestCRS(layer, viewRect, viewProj, viewScale);
  if (!cb) {
    METLIBS_LOG_DEBUG("no CRS for for layer '" << layer->identifier() << "'");
    return nullptr;
  }

  const int zoom = findZoom(cb, viewScale);
  std::unique_ptr<WebMapWMSRequest> request(new WebMapWMSRequest(this, layer, cb, zoom));
  const int nx = (1<<zoom);
  const Rectangle& bb = cb->boundingbox;
  request->x0 = bb.x1;
  request->dx = bb.width() / nx;
  request->y0 = bb.y2;
  request->dy = -request->dx;
  const int ny = (int) std::ceil(bb.height() / request->dx);
  METLIBS_LOG_DEBUG(LOGVAL(nx) << LOGVAL(ny) << LOGVAL(request->x0) << LOGVAL(request->dx) << LOGVAL(request->y0) << LOGVAL(request->dy));

  diutil::tilexy_s tiles;
  diutil::select_pixel_tiles(tiles, w, h,
                             nx, request->x0, request->dx,
                             ny, request->y0, request->dy,
                             cb->boundingbox, cb->projection, viewRect, viewProj);
  METLIBS_LOG_DEBUG(LOGVAL(tiles.size()));

  for (diutil::tilexy_s::const_iterator it = tiles.begin(); it != tiles.end(); ++it)
    request->addTile(it->x, it->y);

  return request.release();
}

QNetworkReply* WebMapWMS::submitRequest(WebMapWMSLayer_cx layer, const std::map<std::string, std::string>& dimensionValues, const std::string& style,
                                        const std::string& crs, WebMapTile* tile)
{
  QUrl qurl = mServiceURL;
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  QUrlQuery urlq;
#else
  QUrl& urlq = qurl;
#endif
  urlq.addQueryItem("SERVICE", "WMS");
  urlq.addQueryItem("REQUEST", "GetMap");
  urlq.addQueryItem("VERSION", (mVersion == WMS_130) ? "1.3.0" : "1.1.1");
  urlq.addQueryItem("TRANSPARENT", "TRUE");
  urlq.addQueryItem("LAYERS", diutil::sq(layer->identifier()));
  urlq.addQueryItem("FORMAT", diutil::sq(tileFormat()));
  urlq.addQueryItem("WIDTH", QString::number(TILESIZE));
  urlq.addQueryItem("HEIGHT", QString::number(TILESIZE));

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
    urlq.addQueryItem(diutil::sq(dimKey), dimValue);
  }

  if (!style.empty())
    urlq.addQueryItem("STYLES", diutil::sq(style));

  const QString aCRS = ((mVersion == WMS_111) ? "SRS" : "CRS");
  urlq.addQueryItem(aCRS, QString::fromStdString(crs));

  const Rectangle& r = tile->rect();
  const float f = toDecimalDegrees(crs);
  float minx = f*r.x1, miny = f*r.y1, maxx = f*r.x2, maxy = f*r.y2;
  if (mVersion == WMS_130)
    swapWms130LatLon(crs, minx, miny, maxx, maxy);
  QString bbox = toQString(minx) + "," + toQString(miny) + "," + toQString(maxx) + "," + toQString(maxy);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  urlq.addQueryItem("BBOX", bbox /*.replace("+", "%2B")*/ .toUtf8());
  qurl.setQuery(urlq);
#else // Qt < 5.0
  urlq.addEncodedQueryItem("BBOX", bbox.replace("+", "%2B").toUtf8());
#endif

  METLIBS_LOG_DEBUG("url='" << qurl.toEncoded().constData() << "' x=" << tile->column() << " y=" << tile->row());
  return submitUrl(qurl);
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

  mRefeshReply = submitUrl(mServiceURL);
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
    METLIBS_LOG_WARN("request error " << qs(mRefeshReply->errorString())
                                      << ", HTTP status=" << mRefeshReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
    return false;
  }

  QDomDocument doc;
  if (!doc.setContent(mRefeshReply)) {
    METLIBS_LOG_WARN("capabilities document xml error");
    return false;
  }

  QDomElement eDoc = doc.documentElement();
  if (eDoc.attribute("version") == "1.1.1") {
    mVersion = WMS_111;
  } else if (eDoc.attribute("version") == "1.3.0") {
    mVersion = WMS_130;
  } else {
    METLIBS_LOG_WARN("unsupported WMS version");
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

  const std::vector<std::string> top_styles, top_legendUrls;
  const crs_bbox_m top_crs_bboxes;
  const WebMapDimension_v top_dimensions;
  QDOM_FOREACH_CHILD(eLayer, eCapability, "Layer") {
    if (!parseLayer(eLayer, top_styles, top_legendUrls, QStringList(), top_crs_bboxes, top_dimensions))
      return false;
  }

  return true;
}

bool WebMapWMS::parseLayer(QDomElement& eLayer, std::vector<std::string> styles, std::vector<std::string> legendUrls, QStringList lCRS, crs_bbox_m crs_bboxes,
                           std::vector<WebMapDimension> dimensions)
{
  METLIBS_LOG_SCOPE();
  using diutil::qs;

  QDomElement eLayerName = eLayer.firstChildElement("Name");
  const std::string sLayerName = qs(eLayerName.text());
  METLIBS_LOG_DEBUG("layer name='" << sLayerName << "'");
  const bool hasContent = !sLayerName.empty();

  const QString aCRS = ((mVersion == WMS_111) ? "SRS" : "CRS");

  // loop over BoundingBox elements to extract explicit bbox'es
  QDOM_FOREACH_CHILD(eBoundingBox, eLayer, "BoundingBox") {
    std::string sCRS = qs(eBoundingBox.attribute(aCRS));
    METLIBS_LOG_DEBUG("explicit bbox check" << LOGVAL(sCRS));
    if (sCRS == EPSG900913)
      sCRS = EPSG3857;
    const float f = 1 / toDecimalDegrees(sCRS);
    float minx = eBoundingBox.attribute("minx").toFloat();
    float miny = eBoundingBox.attribute("miny").toFloat();
    float maxx = eBoundingBox.attribute("maxx").toFloat();
    float maxy = eBoundingBox.attribute("maxy").toFloat();
    if (mVersion == WMS_130)
      swapWms130LatLon(sCRS, minx, miny, maxx, maxy);
    crs_bboxes[sCRS] = Rectangle(minx*f, miny*f, maxx*f, maxy*f);
    METLIBS_LOG_DEBUG(LOGVAL(sCRS) << LOGVAL(minx) << LOGVAL(maxx) << LOGVAL(miny) << LOGVAL(maxy));
  }

  // loop over SRS/CRS elements to extract known bbox'es
  QDOM_FOREACH_CHILD(eCRS, eLayer, aCRS) {
    if (mVersion == WMS_111)
      lCRS << eCRS.text().split(" ", QString::SkipEmptyParts);
    else
      lCRS << eCRS.text();
  }
  if (lCRS.contains(EPSG900913)) {
    lCRS.removeAll(EPSG900913);
    lCRS << EPSG3857;
  }
  for (const QString& crs : lCRS) {
    float minx, miny, maxx, maxy;
    const std::string sCRS = qs(crs);
    METLIBS_LOG_DEBUG("known bbox check" << LOGVAL(sCRS));
    crs_bbox_m::iterator it = crs_bboxes.find(sCRS);
    if (sCRS == EPSG3857) {
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
    } else if (sCRS == EPSG4326 && it == crs_bboxes.end()) {
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
    } else if (sCRS == CRS84 && it == crs_bboxes.end()) {
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
#ifdef WMS_DO_NOT_IGNORE_ELEVATION
    const bool isElevation = (lName == "elevation");
#endif
    if (isTime && sUnits != "ISO8601") {
      METLIBS_LOG_DEBUG("time dimension with bad unit '" << sUnits << "'");
      return false;
    }
#ifdef WMS_DO_NOT_IGNORE_ELEVATION
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
    dim.setUnits(sUnits);
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

  if (!eLayer.firstChildElement("Style").isNull()) {
    styles.clear();
    legendUrls.clear();
    QDOM_FOREACH_CHILD(eStyle, eLayer, "Style")
    {
      QDomElement eStyleName = eStyle.firstChildElement("Name");
      if (!eStyleName.isNull()) {
        std::string style = qs(eStyleName.text());
        std::string legendUrl;
        QDomElement eStyleLegendUrl = eStyle.firstChildElement("LegendURL").firstChildElement("OnlineResource");
        if (!eStyleLegendUrl.isNull())
          legendUrl = qs(eStyleLegendUrl.attribute("xlink:href"));

        styles.push_back(style);
        legendUrls.push_back(legendUrl);
      }
    }
  }

  const bool goodName = !sLayerName.empty();
  const bool unusedName = findLayerByIdentifier(sLayerName) == 0;
  const bool tileable =
      !hasAttributeValue(eLayer, "noSubsets", (QStringList() << "true" << "1"))
      && !hasAttributeValue(eLayer, "fixedWidth", (QStringList() << "0"))
      && !hasAttributeValue(eLayer, "fixedHeight", (QStringList() << "0"));
  const bool hasCRS = !crs_bboxes.empty();
  const bool hasChildrenExcluded = excludeLayersWithChildren() && (!eLayer.firstChildElement("Layer").isNull());

  METLIBS_LOG_DEBUG(LOGVAL(hasContent) << LOGVAL(goodName) << LOGVAL(unusedName) << LOGVAL(tileable) << LOGVAL(hasCRS));

  if (hasContent && goodName && unusedName && tileable && hasCRS && !hasChildrenExcluded) {
    METLIBS_LOG_DEBUG("adding layer '" << sLayerName << "'");
    std::unique_ptr<WebMapWMSLayer> layer(new WebMapWMSLayer(sLayerName));
    layer->setTitle(qs(eLayer.firstChildElement("Title").text()));
    for (const QString& crs : lCRS) {
      const std::string sCRS = qs(crs);
      crs_bbox_m::iterator it = crs_bboxes.find(sCRS);
      if (it != crs_bboxes.end()) {
        const Projection proj = diutil::projectionForCRS(it->first);
        if (proj.isDefined()) {
          layer->addCRS(it->first, proj, it->second);
        } else {
          METLIBS_LOG_DEBUG("unable to create Projection for CRS '" << it->first << "'");
        }
      }
    }
    layer->setStyles(styles, legendUrls);
    layer->setDimensions(dimensions);
    mLayers.push_back(layer.release());
  } else {
    METLIBS_LOG_DEBUG("skipping layer '" << sLayerName << "'");
  }

  QDOM_FOREACH_CHILD(eChildLayer, eLayer, "Layer") {
    if (!parseLayer(eChildLayer, styles, legendUrls, lCRS, crs_bboxes, dimensions))
      return false;
  }

  return true;
}

void WebMapWMS::parseDimensionValues(WebMapDimension& dim, const QString& text, const QString& defaultValue)
{
  METLIBS_LOG_SCOPE(LOGVAL(dim.identifier()));
  using diutil::qs;

  dim.clearValues();

  const bool isTime = dim.isTimeDimension();
  const QStringList values = text.trimmed().split(QRegExp(",\\s*"));
  for (const QString& v : values) {
    if (v.contains("/")) {
      QStringList expanded;
      if (isTime)
        expanded = diutil::expandWmsTimes(v);
      else
        expanded = diutil::expandWmsValues(v);
      for (const QString& ev : expanded) {
        dim.addValue(qs(ev), ev == defaultValue);
      }
    } else {
      dim.addValue(qs(v), v == defaultValue);
    }
  }
}
