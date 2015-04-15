
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

const int TILESIZE = 256;

int findZoomForScale(float z0denominator, float denominator)
{
  if (denominator <= 0)
    return 0;

  return (int) round(log(z0denominator / denominator) / log(2));
}

bool hasAttributeValue(const QDomElement& e, const QString& a, const QStringList& values)
{
  if (!e.hasAttribute(a))
    return false;
  else
    return values.contains(e.attribute(a));
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
{
}

WebMapWMSRequest::~WebMapWMSRequest()
{
  diutil::delete_all_and_clear(mTiles);
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
  for (size_t i=0; i<mTiles.size(); ++i)
    mTiles[i]->abort();
}

void WebMapWMSRequest::tileFinished(WebMapTile* tile)
{
  METLIBS_LOG_SCOPE();
  if (!tile->loadImage(mService->tileFormat()))
    tile->dummyImage(TILESIZE, TILESIZE);
  mUnfinished -= 1;
  if (mUnfinished == 0)
    Q_EMIT completed();
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
  const double x0 = bb.x1,
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
    QString dimValue;
    std::map<std::string, std::string>::const_iterator it
        = dimensionValues.find(dim.identifier());
    if (it != dimensionValues.end())
      dimValue = diutil::sq(it->second);
    else
      dimValue = diutil::sq(dim.defaultValue());
    qurl.addQueryItem(diutil::sq(dimKey), dimValue);
  }
  qurl.addQueryItem("WIDTH", QString::number(TILESIZE));
  qurl.addQueryItem("HEIGHT", QString::number(TILESIZE));

  const QString aCRS = ((mVersion == WMS_111) ? "SRS" : "CRS");
  qurl.addQueryItem(aCRS, QString::fromStdString(crs));

  const Rectangle& r = tile->rect();
  qurl.addEncodedQueryItem("BBOX", QString("%1,%2,%3,%4").arg(r.x1).arg(r.y1).arg(r.x2).arg(r.y2).replace("+", "%2B").toUtf8());

  METLIBS_LOG_DEBUG("url='" << qurl.toEncoded().constData() << "' x=" << tile->column() << " y=" << tile->row());

#if 1
  QNetworkRequest nr(qurl);
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
    METLIBS_LOG_DEBUG("request error");
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

  const std::string top_style;
  const crs_bbox_m top_crs_bboxes;
  const WebMapDimension_v top_dimensions;
  QDOM_FOREACH_CHILD(eLayer, eCapability, "Layer") {
    if (!parseLayer(eLayer, top_style, top_crs_bboxes, top_dimensions))
      return false;
  }

  return true;
}

bool WebMapWMS::parseLayer(QDomElement& eLayer, std::string style, crs_bbox_m crs_bboxes,
    std::vector<WebMapDimension> dimensions)
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
    const float minx = eBoundingBox.attribute("minx").toFloat();
    const float miny = eBoundingBox.attribute("miny").toFloat();
    const float maxx = eBoundingBox.attribute("maxx").toFloat();
    const float maxy = eBoundingBox.attribute("maxy").toFloat();
    crs_bboxes[sCRS] = Rectangle(minx, miny, maxx, maxy);
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
        minx = eBBox.firstChildElement("westBoundLongitude").text().toFloat() * DEG_TO_RAD;
        maxx = eBBox.firstChildElement("eastBoundLongitude").text().toFloat() * DEG_TO_RAD;
        miny = eBBox.firstChildElement("southBoundLatitude").text().toFloat() * DEG_TO_RAD;
        maxy = eBBox.firstChildElement("northBoundLatitude").text().toFloat() * DEG_TO_RAD;
      } else {
        QDomElement eBBox = eLayer.firstChildElement("LatLonBoundingBox");
        const QStringList values = eBBox.text().split(",");
        minx = values.at(0).toFloat() * DEG_TO_RAD;
        miny = values.at(1).toFloat() * DEG_TO_RAD;
        maxx = values.at(2).toFloat() * DEG_TO_RAD;
        maxy = values.at(3).toFloat() * DEG_TO_RAD;
      }
    } else {
      continue;
    }
    crs_bboxes[sCRS] = Rectangle(minx, miny, maxx, maxy);
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
    if (isElevation && !diutil::startswith(sUnits, "EPSG:")) {
      METLIBS_LOG_DEBUG("elevation dimension with bad unit '" << sUnits << "'");
      return false;
    }

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
  if (!eStyle1Name.isNull())
    style = qs(eStyle1Name.text());

  const std::string sLayerName = qs(eLayerName.text());
  const bool goodName = !sLayerName.empty();
  const bool unusedName = findLayerByIdentifier(sLayerName) == 0;
  const bool tileable =
      !hasAttributeValue(eLayer, "noSubsets", (QStringList() << "true" << "1"))
      && !hasAttributeValue(eLayer, "fixedWidth", (QStringList() << "0"))
      && !hasAttributeValue(eLayer, "fixedHeight", (QStringList() << "0"));

  METLIBS_LOG_DEBUG(LOGVAL(hasContent) << LOGVAL(goodName) << LOGVAL(unusedName) << LOGVAL(tileable));

  if (hasContent && goodName && unusedName && tileable) {
    METLIBS_LOG_DEBUG("adding layer '" << sLayerName << "'");
    std::auto_ptr<WebMapWMSLayer> layer(new WebMapWMSLayer(sLayerName));
    layer->setTitle(qs(eLayer.firstChildElement("Title").text()));
    for (crs_bbox_m::const_iterator it = crs_bboxes.begin(); it != crs_bboxes.end(); ++it)
      layer->addCRS(it->first, it->second);
    layer->setDefaultStyle(style);
    layer->setDimensions(dimensions);
    mLayers.push_back(layer.release());
  } else {
    METLIBS_LOG_DEBUG("skipping layer '" << sLayerName << "'");
  }

  QDOM_FOREACH_CHILD(eChildLayer, eLayer, "Layer") {
    if (!parseLayer(eChildLayer, style, crs_bboxes, dimensions))
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
  const QStringList values = text.split(QRegExp(",\\s*"));
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
