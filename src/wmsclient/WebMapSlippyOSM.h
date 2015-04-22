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

#ifndef WebMapSlippyOSM_h
#define WebMapSlippyOSM_h 1

#include "WebMapService.h"
#include "WebMapTile.h"

#include <diField/diProjection.h>

#include <QImage>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;


class WebMapSlippyOSMLayer : public WebMapLayer {
public:
  WebMapSlippyOSMLayer(const std::string& identifier)
    : WebMapLayer(identifier) { }

  void setURLTemplate(const std::string& t)
    { mURLTemplate = t; }

  void setZoomRange(int minZoom, int maxZoom)
    { mMinZoom = minZoom; mMaxZoom = maxZoom; }

  void setTileFormat(const std::string& mime)
    { mTileFormat = mime; }

  int minZoom() const
    { return mMinZoom; }

  int maxZoom() const
    { return mMaxZoom; }

  const std::string& tileFormat() const
    { return mTileFormat; }

  size_t countCRS() const
    { return 1; }

  const std::string& CRS(size_t) const;

  const std::string& urlTemplate() const
    { return mURLTemplate; }

private:
  std::string mURLTemplate;
  int mMinZoom;
  int mMaxZoom;
  std::string mTileFormat;
};

typedef WebMapSlippyOSMLayer* WebMapSlippyOSMLayer_x;
typedef const WebMapSlippyOSMLayer* WebMapSlippyOSMLayer_cx;

// ========================================================================

class WebMapSlippyOSM;
typedef WebMapSlippyOSM* WebMapSlippyOSM_x;
typedef const WebMapSlippyOSM* WebMapSlippyOSM_cx;

class WebMapSlippyOSMRequest : public WebMapRequest {
  Q_OBJECT;

public:
  WebMapSlippyOSMRequest(WebMapSlippyOSM_x service, WebMapSlippyOSMLayer_cx layer, int zoom);

  ~WebMapSlippyOSMRequest();

  void addTile(int tileX, int tileY);

  void setDimensionValue(const std::string&, const std::string&) { }

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

  const Projection& tileProjection() const;

private Q_SLOTS:
  void tileFinished(WebMapTile*);

private:
  WebMapSlippyOSM_x mService;
  WebMapSlippyOSMLayer_cx mLayer;
  int mZoom;

  std::vector<WebMapTile*> mTiles;
  size_t mUnfinished;
};

typedef boost::shared_ptr<WebMapRequest> WebMapRequest_p;

// ========================================================================

class WebMapSlippyOSM : public WebMapService
{
  Q_OBJECT;

public:
  /* set with URL pointing to GetCapabilities. need to call refresh to
   * actually fetch data */
  WebMapSlippyOSM(const std::string& identifier, const QUrl& service, QNetworkAccessManager* network);

  ~WebMapSlippyOSM();

  /*! suitable refresh interval. negative for no refresh */
  int refreshInterval() const;

  /*! create a request object for the specified layer. may be null,
   *  e.g. if unknown layer or; ownership is transferred to caller */
  WebMapRequest_x createRequest(const std::string& layer,
      const Rectangle& viewRect, const Projection& viewProj, double viewScale);

  QNetworkReply* submitRequest(WebMapSlippyOSMLayer_cx layer,
      int zoom, int tileX, int tileY);

  void refresh();

  const Projection& projection() const
    { return mProjection; }

private:
  bool parseReply();

private Q_SLOTS:
  void refreshReplyFinished();

private:
  QUrl mServiceURL;
  QNetworkAccessManager* mNetworkAccess;
  long mNextRefresh;
  QNetworkReply* mRefeshReply;
  Projection mProjection;
};

#endif // WebMapSlippyOSM_h
