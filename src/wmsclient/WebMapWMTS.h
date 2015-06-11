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

#ifndef WebMapWMTS_h
#define WebMapWMTS_h 1

#include "WebMapService.h"

#include <diField/diProjection.h>

#include <QImage>
#include <QUrl>

#include <map>

class QNetworkAccessManager;
class QNetworkReply;

class WebMapTile;


class WebMapWMTSTileMatrix {
public:
  WebMapWMTSTileMatrix(const std::string& id, double sd,
      double tmx, double tmy,
      size_t mw, size_t mh, size_t tw, size_t th);

  /*! TileMatrix identifier */
  const std::string& identifier() const
    { return mIdentifier; }

  double scaleDenominator() const
    { return mDenominator; }

  double tileMinX() const
    { return mTileMinX; }

  double tileMaxY() const
    { return mTileMaxY; }

  size_t matrixWidth() const
    { return mMatrixWidth; }

  size_t matrixHeight() const
    { return mMatrixHeight; }

  size_t tileWidth() const
    { return mTileWidth; }

  size_t tileHeight() const
    { return mTileHeight; }

private:
  std::string mIdentifier;
  double mDenominator;
  double mTileMinX, mTileMaxY;
  size_t mMatrixWidth, mMatrixHeight;
  size_t mTileWidth, mTileHeight;
};

typedef WebMapWMTSTileMatrix* WebMapWMTSTileMatrix_x;
typedef const WebMapWMTSTileMatrix* WebMapWMTSTileMatrix_cx;

// ========================================================================

class WebMapWMTSTileMatrixSet {
public:
  WebMapWMTSTileMatrixSet(const std::string& id, const std::string& crs);

  void addMatrix(const WebMapWMTSTileMatrix& m)
    { mMatrices.push_back(m); }

  /*! TileMatrixSet identifier */
  const std::string& identifier() const
    { return mIdentifier; }

  const std::string& CRS() const
    { return mCRS; }

  const Projection& projection() const
    { return mProjection; }

  const double metersPerUnit() const
    { return mMetersPerUnit; }

  /*! number of tilematrices available */
  size_t countMatrices() const
    { return mMatrices.size(); }

  /*! access to a tilematrix */
  const WebMapWMTSTileMatrix& matrix(size_t idx) const
    { return mMatrices.at(idx); }

private:
  std::string mIdentifier;
  std::string mCRS;
  Projection mProjection;
  double mMetersPerUnit;
  std::vector<WebMapWMTSTileMatrix> mMatrices;
};

typedef WebMapWMTSTileMatrixSet* WebMapWMTSTileMatrixSet_x;
typedef const WebMapWMTSTileMatrixSet* WebMapWMTSTileMatrixSet_cx;

// ========================================================================

class WebMapWMTSLayer : public WebMapLayer {
public:
  WebMapWMTSLayer(const std::string& identifier)
    : WebMapLayer(identifier) { }

  void addMatrixSet(WebMapWMTSTileMatrixSet_cx mats)
    { mMatrixSets.push_back(mats); }

  void setURLTemplate(const std::string& t)
    { mURLTemplate = t; }

  void setTileFormat(const std::string& mime)
    { mTileFormat = mime; }

  void setDefaultStyle(const std::string& styleId)
    { mDefaultStyle = styleId; }

  size_t countCRS() const
    { return mMatrixSets.size(); }

  const std::string& CRS(size_t idx) const
    { return matrixSet(idx)->CRS(); }

  const WebMapWMTSTileMatrixSet_cx& matrixSet(size_t idx) const
    { return mMatrixSets.at(idx); }

  const std::string& urlTemplate() const
    { return mURLTemplate; }

  const std::string& tileFormat() const
    { return mTileFormat; }

  const std::string& defaultStyle() const
    { return mDefaultStyle; }

private:
  std::string mURLTemplate;
  std::vector<WebMapWMTSTileMatrixSet_cx> mMatrixSets;
  std::string mTileFormat;
  std::string mDefaultStyle;
};

typedef WebMapWMTSLayer* WebMapWMTSLayer_x;
typedef const WebMapWMTSLayer* WebMapWMTSLayer_cx;

// ========================================================================

class WebMapWMTS;
typedef WebMapWMTS* WebMapWMTS_x;
typedef const WebMapWMTS* WebMapWMTS_cx;

class WebMapWMTSRequest : public WebMapRequest {
  Q_OBJECT;

public:
  WebMapWMTSRequest(WebMapWMTS_x service, WebMapWMTSLayer_cx layer,
      WebMapWMTSTileMatrixSet_cx matrixSet, WebMapWMTSTileMatrix_cx matrix);

  ~WebMapWMTSRequest();

  void addTile(int tileX, int tileY);

  /*! set dimension value; ignores non-existent dimensions; dimensions
   *  without explicitly specified value are set to a default value */
  void setDimensionValue(const std::string& dimIdentifier,
      const std::string& dimValue);

  /*! start fetching data */
  void submit();

  /*! stop fetching data */
  void abort();

  /*! number of tiles */
  size_t countTiles() const
  { return mTiles.size(); }

  /*! rectangle of one tile, in tileProjection coordinates */
  const Rectangle& tileRect(size_t idx) const;

  /*! image data of one tile; might have isNull() == true */
  const QImage& tileImage(size_t idx) const;

  const Projection& tileProjection() const
    { return mMatrixSet->projection(); }

private Q_SLOTS:
  void tileFinished(WebMapTile*);

private:
  WebMapWMTS_x mService;
  WebMapWMTSLayer_cx mLayer;
  WebMapWMTSTileMatrixSet_cx mMatrixSet;
  WebMapWMTSTileMatrix_cx mMatrix;

  std::map<std::string, std::string> mDimensionValues;

  std::vector<WebMapTile*> mTiles;
  size_t mUnfinished;
};

typedef boost::shared_ptr<WebMapRequest> WebMapRequest_p;

// ========================================================================

class WebMapWMTS : public WebMapService
{
  Q_OBJECT;

public:
  /* set with URL pointing to GetCapabilities. need to call refresh to
   * actually fetch data */
  WebMapWMTS(const std::string& identifier,
      const QUrl& service, QNetworkAccessManager* network);

  ~WebMapWMTS();

  /*! suitable refresh interval. negative for no refresh */
  int refreshInterval() const;

  /*! create a request object for the specified layer. may be null,
   *  e.g. if unknown layer or; ownership is transferred to caller */
  WebMapRequest_x createRequest(const std::string& layer,
      const Rectangle& viewRect, const Projection& viewProj, double viewScale);

  QNetworkReply* submitRequest(WebMapWMTSLayer_cx layer,
      const std::map<std::string, std::string>& dimensionValues,
      WebMapWMTSTileMatrixSet_cx matrixSet, WebMapWMTSTileMatrix_cx matrix,
      int tileX, int tileY);

  void refresh();

private:
  void destroyTileMatrixSets();
  bool parseReply();

private Q_SLOTS:
  void refreshReplyFinished();

private:
  QUrl mServiceURL;
  QUrl mGetTileKvpUrl;
  QNetworkAccessManager* mNetworkAccess;
  long mNextRefresh;
  QNetworkReply* mRefeshReply;

  std::vector<WebMapWMTSTileMatrixSet_x> mTileMatrixSets;
};

#endif // WebMapWMTS_h
