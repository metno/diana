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

#include "diUtilities.h"
#include "WebMapUtilities.h"

#include <puTools/miStringFunctions.h>

#include <QDomDocument>
#include <QDomElement>
#include <QNetworkReply>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QUrlQuery>
#endif

#include <sys/time.h>

#define MILOGGER_CATEGORY "diana.WebMapWMTS"
#include <miLogger/miLogging.h>

WebMapWMTS::WebMapWMTS(const std::string& identifier, const QUrl& url, QNetworkAccessManager* network)
  : WebMapService(identifier, network)
  , mServiceURL(url)
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
    const Rectangle& viewRect, const Projection& viewProj, double viewScale, int w, int h)
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
  METLIBS_LOG_DEBUG(LOGVAL(matrixSet->identifier()) << LOGVAL(p_tiles.getProj4DefinitionExpanded()));

  WebMapWMTSTileMatrix_cx matrix = matrixSet->findMatrixForScale(viewScale);
  if (!matrix) {
    METLIBS_LOG_DEBUG("no matrix");
    return 0;
  }
  METLIBS_LOG_DEBUG(LOGVAL(matrix->identifier()));

  std::unique_ptr<WebMapWMTSRequest> request(new WebMapWMTSRequest(this, layer, matrixSet, matrix));
  const float ps = matrixSet->pixelSpan(matrix),
      tileSpanX = matrix->tileWidth()  * ps,
      tileSpanY = matrix->tileHeight() * ps,
      // see OGC WMTS spec pdf page 23f (http://portal.opengeospatial.org/files/?artifact_id=35326)
      tileMaxX = matrix->tileMinX() + tileSpanX*matrix->matrixWidth(),
      tileMinY = matrix->tileMaxY() - tileSpanY*matrix->matrixHeight();

  request->x0 = matrix->tileMinX();
  request->dx = tileSpanX;
  request->y0 = matrix->tileMaxY();
  request->dy = -tileSpanY;
  request->tilebbx = Rectangle(matrix->tileMinX(), tileMinY, tileMaxX, matrix->tileMaxY());
  METLIBS_LOG_DEBUG(LOGVAL(request->tilebbx));

  diutil::tilexy_s tiles;
  diutil::select_pixel_tiles(tiles, w, h,
      matrix->matrixWidth(), request->x0, request->dx,
      matrix->matrixHeight(), request->y0, request->dy,
      request->tilebbx, p_tiles, viewRect, viewProj);

  METLIBS_LOG_DEBUG(LOGVAL(tiles.size()));
  for (diutil::tilexy_s::const_iterator it = tiles.begin(); it != tiles.end(); ++it)
    request->addTile(it->x, it->y);

  return request.release();
}

QNetworkReply* WebMapWMTS::submitRequest(WebMapWMTSLayer_cx layer, const std::map<std::string, std::string>& dimensionValues, const std::string& style,
                                         WebMapWMTSTileMatrixSet_cx matrixSet, WebMapWMTSTileMatrix_cx matrix, int tileX, int tileY)
{
  METLIBS_LOG_SCOPE();
  QUrl qurl;

  const std::string& style_ = style.empty() ? layer->defaultStyle() : style;
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
    miutil::replace(url, "{style}", style_);
    miutil::replace(url, "{TileMatrixSet}", matrixSet->identifier());
    miutil::replace(url, "{TileMatrix}", matrix->identifier());
    miutil::replace(url, "{TileCol}", miutil::from_number(tileX));
    miutil::replace(url, "{TileRow}", miutil::from_number(tileY));
    qurl = QUrl(QString::fromStdString(url));
  } else {
    // KVP-GET
    qurl = mGetTileKvpUrl;
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QUrlQuery urlq;
#else
    QUrl& urlq = qurl;
#endif
    urlq.addQueryItem("Service", "WMTS");
    urlq.addQueryItem("Request", "GetTile");
    urlq.addQueryItem("Version", "1.0.0");
    urlq.addQueryItem("Layer", diutil::sq(layer->identifier()));
    urlq.addQueryItem("Style", diutil::sq(style_));
    urlq.addQueryItem("Format", diutil::sq(layer->tileFormat()));

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
      urlq.addQueryItem(diutil::sq(dimKey), dimValue);
    }

    urlq.addQueryItem("TileMatrixSet", diutil::sq(matrixSet->identifier()));
    urlq.addQueryItem("TileMatrix", diutil::sq(matrix->identifier()));
    urlq.addQueryItem("TileCol", QString::number(tileX));
    urlq.addQueryItem("TileRow", QString::number(tileY));
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    qurl.setQuery(urlq);
#endif
  }
  METLIBS_LOG_DEBUG("url='" << qurl.toString().toStdString() << "'");

  return submitUrl(qurl);
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

  mRefeshReply = submitUrl(mServiceURL);
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
    const std::string crs = qs(eCRS.text());
    const Projection proj = diutil::projectionForCRS(crs);
    if (!proj.isDefined()) {
      METLIBS_LOG_DEBUG("unable to create Projection for CRS '" << crs << "'");
      continue;
    }
    std::unique_ptr<WebMapWMTSTileMatrixSet> ms(new WebMapWMTSTileMatrixSet(qs(eId.text()), proj));
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

    std::unique_ptr<WebMapWMTSLayer> layer(new WebMapWMTSLayer(qs(eId.text())));
    METLIBS_LOG_DEBUG(LOGVAL(layer->identifier()));

    layer->setTitle(qs(eLayer.firstChildElement("ows:Title").text()));

    std::string defaultStyle;
    std::vector<std::string> styles;
    QDOM_FOREACH_CHILD(eStyle, eLayer, "Style") {
      const std::string styleId = qs(eStyle.text());
      styles.push_back(styleId);
      const bool isDefault = (eStyle.attribute("isDefault") == "true");
      if (isDefault) {
        defaultStyle = styleId;
      }
    }
    if (defaultStyle.empty() && !styles.empty())
      defaultStyle = styles.front();
    layer->setStyles(styles, defaultStyle);

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
