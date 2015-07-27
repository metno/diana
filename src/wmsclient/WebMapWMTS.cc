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

#include "WebMapWMTS.h"

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

#define MILOGGER_CATEGORY "diana.WebMapWMTS"
#include <miLogger/miLogging.h>

#ifdef HAVE_CONFIG_H
#include "config.h" // for PVERSION
#endif

namespace /* anonymous */ {

double pixelSpan(WebMapWMTSTileMatrixSet_cx matrixSet, WebMapWMTSTileMatrix_cx matrix)
{
  return matrix->scaleDenominator()
      * diutil::WMTS_M_PER_PIXEL
      / matrixSet->metersPerUnit();
}

WebMapWMTSTileMatrix_cx findMatrixForScale(WebMapWMTSTileMatrixSet_cx matrixSet, float denominator)
{
  METLIBS_LOG_SCOPE(LOGVAL(denominator));
  if (denominator <= 0)
    return 0;

  const double MAX_RATIO = 2.5;
  int bestIndex = -1;
  double bestRatio = 1;
  for (size_t i=0; i<matrixSet->countMatrices(); ++i) {
    const WebMapWMTSTileMatrix& matrix = matrixSet->matrix(i);
    double ratio = 0.5 * matrix.scaleDenominator() / denominator;
    METLIBS_LOG_DEBUG(LOGVAL(i) << LOGVAL(matrix.identifier())
        << LOGVAL(matrix.scaleDenominator()) << LOGVAL(ratio));
    if (ratio < 1/MAX_RATIO)
      continue;
    if (ratio < 1)
      ratio = 1 / ratio;
    if (ratio < MAX_RATIO && (bestIndex < 0 || ratio < bestRatio)) {
      bestRatio = ratio;
      bestIndex = i;
      METLIBS_LOG_DEBUG(LOGVAL(bestRatio));
    }
  }
  METLIBS_LOG_DEBUG(LOGVAL(bestIndex));
  if (bestIndex < 0)
    return 0;
  else
    return &matrixSet->matrix(bestIndex);
}

} // anonymous namespace

// ========================================================================

WebMapWMTSTileMatrix::WebMapWMTSTileMatrix(const std::string& id,
    double sd, double tmx, double tmy,
    size_t mw, size_t mh, size_t tw, size_t th)
  : mIdentifier(id)
  , mDenominator(sd)
  , mTileMinX(tmx)
  , mTileMaxY(tmy)
  , mMatrixWidth(mw)
  , mMatrixHeight(mh)
  , mTileWidth(tw)
  , mTileHeight(th)
{
}

// ========================================================================

WebMapWMTSTileMatrixSet::WebMapWMTSTileMatrixSet(const std::string& id,
    const std::string& crs)
  : mIdentifier(id)
  , mCRS(crs)
  , mProjection(diutil::projectionForCRS(crs))
  , mMetersPerUnit(diutil::metersPerUnit(mProjection))
{
}

// ========================================================================

WebMapWMTSRequest::WebMapWMTSRequest(WebMapWMTS_x service,
    WebMapWMTSLayer_cx layer,
    WebMapWMTSTileMatrixSet_cx matrixSet,
    WebMapWMTSTileMatrix_cx matrix)
  : mService(service)
  , mLayer(layer)
  , mMatrixSet(matrixSet)
  , mMatrix(matrix)
{
}

WebMapWMTSRequest::~WebMapWMTSRequest()
{
  diutil::delete_all_and_clear(mTiles);
}

void WebMapWMTSRequest::addTile(int tileX, int tileY)
{
  const double ps = pixelSpan(mMatrixSet, mMatrix),
      tileSpanX = mMatrix->tileWidth()  * ps,
      tileSpanY = mMatrix->tileHeight() * ps;
  const double tx0 = mMatrix->tileMinX() + tileSpanX * tileX,
      ty1 = mMatrix->tileMaxY() - tileSpanY * tileY;
  const Rectangle rect(tx0, ty1 - tileSpanY, tx0+tileSpanX, ty1);
  mTiles.push_back(new WebMapTile(tileX, tileY, rect));
}

void WebMapWMTSRequest::setDimensionValue(const std::string& dimIdentifier,
    const std::string& dimValue)
{
  mDimensionValues[dimIdentifier] = dimValue;
}

void WebMapWMTSRequest::submit()
{
  METLIBS_LOG_SCOPE();
  mUnfinished = mTiles.size();
  for (size_t i=0; i<mTiles.size(); ++i) {
    WebMapTile* tile = mTiles[i];
    connect(tile, SIGNAL(finished(WebMapTile*)),
        this, SLOT(tileFinished(WebMapTile*)));
    QNetworkReply* reply = mService->submitRequest(mLayer, mDimensionValues,
        mMatrixSet, mMatrix, tile->column(), tile->row());
    tile->submit(reply);
  }
}

void WebMapWMTSRequest::abort()
{
  METLIBS_LOG_SCOPE();
  for (size_t i=0; i<mTiles.size(); ++i)
    mTiles[i]->abort();
}

const Rectangle& WebMapWMTSRequest::tileRect(size_t idx) const
{
  return mTiles.at(idx)->rect();
}

const QImage& WebMapWMTSRequest::tileImage(size_t idx) const
{
  return mTiles.at(idx)->image();
}

void WebMapWMTSRequest::tileFinished(WebMapTile* tile)
{
  METLIBS_LOG_SCOPE();
  tile->loadImage(mLayer->tileFormat());
  mUnfinished -= 1;
  METLIBS_LOG_DEBUG(LOGVAL(mUnfinished));
  if (mUnfinished == 0)
    Q_EMIT completed();
}

// ========================================================================

WebMapWMTS::WebMapWMTS(const std::string& identifier, const QUrl& url, QNetworkAccessManager* network)
  : WebMapService(identifier)
  , mServiceURL(url)
  , mNetworkAccess(network)
  , mNextRefresh(0)
  , mRefeshReply(0)
{
}

WebMapWMTS::~WebMapWMTS()
{
  if (mRefeshReply)
    mRefeshReply->deleteLater();
  destroyLayers();
  destroyTileMatrixSets();
}

void WebMapWMTS::destroyTileMatrixSets()
{
  diutil::delete_all_and_clear(mTileMatrixSets);
}

int WebMapWMTS::refreshInterval() const
{
  return 3600;
}

WebMapRequest_x WebMapWMTS::createRequest(const std::string& layerIdentifier,
    const Rectangle& viewRect, const Projection& viewProj, double viewScale)
{
  METLIBS_LOG_SCOPE(LOGVAL(layerIdentifier) << LOGVAL(viewScale));
  WebMapWMTSLayer_cx layer = static_cast<WebMapWMTSLayer_cx>
      (findLayerByIdentifier(layerIdentifier));
  if (!layer) {
    METLIBS_LOG_DEBUG("layer not found");
    return 0;
  }

  WebMapWMTSTileMatrixSet_cx matrixSet = layer->matrixSet(layer->countCRS()-1);
  const Projection& p_tiles = matrixSet->projection();
  if (!p_tiles.isDefined()) {
    METLIBS_LOG_DEBUG("projection is undefined");
    return 0;
  }
  METLIBS_LOG_DEBUG(LOGVAL(matrixSet->identifier()) << LOGVAL(p_tiles.getProjDefinitionExpanded()));

  WebMapWMTSTileMatrix_cx matrix = findMatrixForScale(matrixSet, viewScale);
  if (!matrix) {
    METLIBS_LOG_DEBUG("no matrix");
    return 0;
  }
  METLIBS_LOG_DEBUG(LOGVAL(matrix->identifier()));

  const float ps = pixelSpan(matrixSet, matrix),
      tileSpanX = matrix->tileWidth()  * ps,
      tileSpanY = matrix->tileHeight() * ps;
  diutil::tilexy_s tiles;
  diutil::select_tiles(tiles,
      0, matrix->matrixWidth(),  matrix->tileMinX(), tileSpanX,
      0, matrix->matrixHeight(), matrix->tileMaxY(), -tileSpanY,
      p_tiles, viewRect, viewProj);

  std::auto_ptr<WebMapWMTSRequest> request(new WebMapWMTSRequest(this, layer, matrixSet, matrix));
  for (diutil::tilexy_s::const_iterator it = tiles.begin(); it != tiles.end(); ++it)
    request->addTile(it->x, it->y);

  return request.release();
}

QNetworkReply* WebMapWMTS::submitRequest(WebMapWMTSLayer_cx layer,
    const std::map<std::string, std::string>& dimensionValues,
    WebMapWMTSTileMatrixSet_cx matrixSet, WebMapWMTSTileMatrix_cx matrix,
    int tileX, int tileY)
{
  METLIBS_LOG_SCOPE();
  QUrl qurl;

  if (!layer->urlTemplate().empty()) {
    std::string url = layer->urlTemplate();
    for (size_t d = 0; d<layer->countDimensions(); ++d) {
      const WebMapDimension& dim = layer->dimension(d);
      const std::string templateKey = "{" + dim.identifier() + "}";
      std::map<std::string, std::string>::const_iterator it
          = dimensionValues.find(dim.identifier());
      if (it != dimensionValues.end())
        miutil::replace(url, templateKey, it->second);
      else
        miutil::replace(url, templateKey, dim.defaultValue());
    }
    miutil::replace(url, "{style}", layer->defaultStyle());
    miutil::replace(url, "{TileMatrixSet}", matrixSet->identifier());
    miutil::replace(url, "{TileMatrix}", matrix->identifier());
    miutil::replace(url, "{TileCol}", miutil::from_number(tileX));
    miutil::replace(url, "{TileRow}", miutil::from_number(tileY));
    qurl = QUrl(QString::fromStdString(url));
  } else {
    // KVP-GET
    qurl = mGetTileKvpUrl;
    qurl.addQueryItem("Service", "WMTS");
    qurl.addQueryItem("Request", "GetTile");
    qurl.addQueryItem("Version", "1.0.0");
    qurl.addQueryItem("Layer", diutil::sq(layer->identifier()));
    qurl.addQueryItem("Style", diutil::sq(layer->defaultStyle()));
    qurl.addQueryItem("Format", diutil::sq(layer->tileFormat()));

    // the WMTS standard is not very clear about how to specify sample dimensions
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

    qurl.addQueryItem("TileMatrixSet", diutil::sq(matrixSet->identifier()));
    qurl.addQueryItem("TileMatrix", diutil::sq(matrix->identifier()));
    qurl.addQueryItem("TileCol", QString::number(tileX));
    qurl.addQueryItem("TileRow", QString::number(tileY));
  }
  METLIBS_LOG_DEBUG("url='" << qurl.toString().toStdString() << "'");

#if 1
  QNetworkRequest nr(qurl);
  nr.setRawHeader("User-Agent", "diana " PVERSION);
  return mNetworkAccess->get(nr);
#else
  return 0;
#endif
}

void WebMapWMTS::refresh()
{
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

void WebMapWMTS::refreshReplyFinished()
{
  METLIBS_LOG_SCOPE();
  destroyLayers();
  destroyTileMatrixSets();

  parseReply();

  mRefeshReply->deleteLater();
  mRefeshReply = 0;

  Q_EMIT refreshFinished();
}

bool WebMapWMTS::parseReply()
{
  METLIBS_LOG_SCOPE();
  using diutil::qs;

  if (mRefeshReply->error() != QNetworkReply::NoError) {
    METLIBS_LOG_DEBUG("capabilities request error");
    return false;
  }

  QDomDocument doc;
  if (!doc.setContent(mRefeshReply)) {
    METLIBS_LOG_DEBUG("capabilities document error");
    return false;
  }

  QDomElement eCap = doc.documentElement();
  QDomElement eService = eCap.firstChildElement("ows:ServiceIdentification");
  QDomElement eServiceType = eService.firstChildElement("ows:ServiceType");
  if (eServiceType.text() != "OGC WMTS")
    return false;
  QDomElement eServiceTypeVersion = eService.firstChildElement("ows:ServiceTypeVersion");
  if (eServiceTypeVersion.text() != "1.0.0")
    return false;

  QDomElement eServiceTitle = eService.firstChildElement("ows:Title");
  mTitle = qs(eServiceTitle.text());

  QDomElement eOperationsMetaData = eCap.firstChildElement("ows:OperationsMetadata");
  if (!eOperationsMetaData.isNull()) {
    QDOM_FOREACH_CHILD(eOperation, eOperationsMetaData, "ows:Operation") {
      if (eOperation.attribute("name") != "GetTile")
        continue;
      QDomElement eDCP = eOperation.firstChildElement("ows:DCP");
      QDomElement eHTTP = eDCP.firstChildElement("ows:HTTP");
      QDomElement eGet = eHTTP.firstChildElement("ows:Get");
      if (eGet.isNull())
        continue;
      QDomElement eConstraint = eGet.firstChildElement("ows:Constraint");
      QDomElement eAllowedValues = eConstraint.firstChildElement("ows:AllowedValues");
      QDomElement eValue = eAllowedValues.firstChildElement("ows:Value");
      if (eValue.text() == "KVP") {
        mGetTileKvpUrl = QUrl(eGet.attribute("xlink:href"));
        METLIBS_LOG_DEBUG("kvp='" << eGet.attribute("xlink:href").toStdString());
        break;
      }
    }
  }

  QDomElement eContents = eCap.firstChildElement("Contents");
  QDOM_FOREACH_CHILD(eTileMatrixSet, eContents, "TileMatrixSet") {
    QDomElement eId = eTileMatrixSet.firstChildElement("ows:Identifier");
    QDomElement eCRS = eTileMatrixSet.firstChildElement("ows:SupportedCRS");
    std::auto_ptr<WebMapWMTSTileMatrixSet> ms
        (new WebMapWMTSTileMatrixSet(qs(eId.text()), qs(eCRS.text())));
    QDOM_FOREACH_CHILD(eTileMatrix, eTileMatrixSet, "TileMatrix") {
      QDomElement eMId = eTileMatrix.firstChildElement("ows:Identifier");
      QDomElement eScale = eTileMatrix.firstChildElement("ScaleDenominator");
      QDomElement eTopLeft = eTileMatrix.firstChildElement("TopLeftCorner");
      QDomElement eTileWidth = eTileMatrix.firstChildElement("TileWidth");
      QDomElement eTileHeight = eTileMatrix.firstChildElement("TileHeight");
      QDomElement eMatrixWidth = eTileMatrix.firstChildElement("MatrixWidth");
      QDomElement eMatrixHeight = eTileMatrix.firstChildElement("MatrixHeight");
      QStringList topleft = eTopLeft.text().split(" ");
      ms->addMatrix(WebMapWMTSTileMatrix(qs(eMId.text()), eScale.text().toDouble(),
              topleft[0].toDouble(), topleft[1].toDouble(),
              eMatrixWidth.text().toInt(), eMatrixHeight.text().toInt(),
              eTileWidth.text().toInt(), eTileHeight.text().toInt()));
    }
    mTileMatrixSets.push_back(ms.release());
  }

  QDOM_FOREACH_CHILD(eLayer, eContents, "Layer") {
    QDomElement eId = eLayer.firstChildElement("ows:Identifier");

    std::auto_ptr<WebMapWMTSLayer> layer(new WebMapWMTSLayer(qs(eId.text())));
    METLIBS_LOG_DEBUG(LOGVAL(layer->identifier()));

    layer->setTitle(qs(eLayer.firstChildElement("ows:Title").text()));

    std::string defaultStyle, anyStyle;
    QDOM_FOREACH_CHILD(eStyle, eLayer, "Style") {
      const bool isDefault = (eStyle.attribute("isDefault") == "true");
      if (!isDefault && !anyStyle.empty())
        continue;
      const std::string styleId = qs(eStyle.text());
      if (isDefault) {
        defaultStyle = styleId;
        break;
      } else {
        anyStyle = styleId;
      }
    }
    METLIBS_LOG_DEBUG(LOGVAL(defaultStyle) << LOGVAL(anyStyle));
    layer->setDefaultStyle((!defaultStyle.empty()) ? defaultStyle : anyStyle);

    QDOM_FOREACH_CHILD(eDimension, eLayer, "Dimension") {
      QDomElement eDId = eDimension.firstChildElement("ows:Identifier");
      WebMapDimension dim(qs(eDId.text()));

      QDomElement eDefault = eDimension.firstChildElement("Default");
      QDOM_FOREACH_CHILD(eValue, eDimension, "Value") {
        dim.addValue(qs(eValue.text()), eValue.text() == eDefault.text());
      }
      layer->addDimension(dim);
    }

    QDOM_FOREACH_CHILD(eTileMatrixLink, eLayer, "TileMatrixSetLink") {
      QDomElement eSetId = eTileMatrixLink.firstChildElement("TileMatrixSet");
      for (size_t i=0; i<mTileMatrixSets.size(); ++i) {
        if (mTileMatrixSets[i]->identifier() == qs(eSetId.text())) {
          layer->addMatrixSet(mTileMatrixSets[i]);
          break;
        }
      }
    }

    QDOM_FOREACH_CHILD(eResourceUrl, eLayer, "ResourceURL") {
      if (eResourceUrl.attribute("resourceType") != "tile")
        continue;

      layer->setURLTemplate(qs(eResourceUrl.attribute("template")));

      const std::string format = qs(eResourceUrl.attribute("format"));
      if (format == "image/png" || format == "image/jpeg") {
        layer->setTileFormat(format);
        break;
      }
    }
    METLIBS_LOG_DEBUG(LOGVAL(layer->urlTemplate()));
    if (layer->urlTemplate().empty()) {
      std::set<std::string> formats;
      QDOM_FOREACH_CHILD(eFormat, eLayer, "Format") {
        formats.insert(qs(eFormat.text()));
      }
      // prefer png, alternatively jpeg
      std::set<std::string>::const_iterator it = formats.find("image/png");
      if (it == formats.end())
        it = formats.find("image/jpeg");
      if (it != formats.end())
        layer->setTileFormat(*it);
    }
    METLIBS_LOG_DEBUG(LOGVAL(layer->tileFormat()));

    mLayers.push_back(layer.release());
  }

  return true;
}
