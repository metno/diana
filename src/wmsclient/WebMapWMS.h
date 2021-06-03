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

#ifndef WebMapWMS_h
#define WebMapWMS_h 1

#include "WebMapService.h"

#include "WebMapWMSLayer.h"
#include "WebMapWMSRequest.h"

#include <QUrl>

class QDomElement;

class WebMapWMS : public WebMapService
{
  Q_OBJECT

public:
  /* set with URL pointing to GetCapabilities. need to call refresh to
   * actually fetch data */
  WebMapWMS(const std::string& identifier, const QUrl& service, QNetworkAccessManager* network);

  ~WebMapWMS();

  /*! suitable refresh interval. negative for no refresh */
  int refreshInterval() const override;

  /*! create a request object for the specified layer. may be null,
   *  e.g. if unknown layer or; ownership is transferred to caller */
  WebMapRequest_x createRequest(const std::string& layer,
      const Rectangle& viewRect, const Projection& viewProj, double viewScale, int w, int h) override;

  QNetworkReply* submitRequest(WebMapWMSLayer_cx layer,
      const std::map<std::string, std::string>& dimensionValues,
      const std::string& crs, WebMapTile* tile);

  void refresh() override;

  const std::string& tileFormat() const
    { return mTileFormat; }

private:
  typedef std::map<std::string, Rectangle> crs_bbox_m;
  enum WmsVersion { WMS_111, WMS_130 };

  bool parseReply();
  bool parseLayer(QDomElement& eLayer, std::string style, std::string legendUrl, QStringList lCRS, crs_bbox_m crs_bboxes, WebMapDimension_v dimensions);
  void parseDimensionValues(WebMapDimension& dim, const QString& text, const QString& defaultValue);
  WebMapWmsCrsBoundingBox_cx findBestCRS(WebMapWMSLayer_cx layer, const Rectangle& viewRect, const Projection& viewProj) const;

private Q_SLOTS:
  void refreshReplyFinished();

private:
  QUrl mServiceURL;
  std::string mTileFormat;
  WmsVersion mVersion;

  long mNextRefresh;
  QNetworkReply* mRefeshReply;
};

typedef WebMapWMS* WebMapWMS_x;
typedef const WebMapWMS* WebMapWMS_cx;

#endif // WebMapWMS_h
