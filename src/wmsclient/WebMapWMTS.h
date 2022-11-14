/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2022 met.no

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

#include "WebMapWMTSLayer.h"
#include "WebMapWMTSRequest.h"
#include "WebMapWMTSTileMatrixSet.h"

#include <QUrl>

class WebMapWMTS : public WebMapService
{
  Q_OBJECT

public:
  /* set with URL pointing to GetCapabilities. need to call refresh to
   * actually fetch data */
  WebMapWMTS(const std::string& identifier,
      const QUrl& url, QNetworkAccessManager* network);

  ~WebMapWMTS();

  /*! suitable refresh interval. negative for no refresh */
  int refreshInterval() const override;

  /*! create a request object for the specified layer. may be null,
   *  e.g. if unknown layer or; ownership is transferred to caller */
  WebMapRequest_x createRequest(const std::string& layer,
      const Rectangle& viewRect, const Projection& viewProj, double viewScale, int w, int h) override;

  QNetworkReply* submitRequest(WebMapWMTSLayer_cx layer, const std::map<std::string, std::string>& dimensionValues, const std::string& style,
                               WebMapWMTSTileMatrixSet_cx matrixSet, WebMapWMTSTileMatrix_cx matrix, int tileX, int tileY);

  void refresh() override;

private:
  void destroyTileMatrixSets();
  bool parseReply();
  std::pair<const WebMapWMTSTileMatrixSet*, const WebMapWMTSTileMatrix*> findBestCRS(const WebMapWMTSLayer* layer, const Rectangle& viewRect,
                                                                                     const Projection& viewProj, double viewScale) const;

private Q_SLOTS:
  void refreshReplyFinished();

private:
  QUrl mServiceURL;
  QUrl mGetTileKvpUrl;
  long mNextRefresh;
  QNetworkReply* mRefeshReply;

  std::vector<WebMapWMTSTileMatrixSet_x> mTileMatrixSets;
};

typedef WebMapWMTS* WebMapWMTS_x;
typedef const WebMapWMTS* WebMapWMTS_cx;

#endif // WebMapWMTS_h
