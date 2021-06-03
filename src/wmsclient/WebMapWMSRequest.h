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

#ifndef WebMapWMSRequest_h
#define WebMapWMSRequest_h 1

#include "WebMapTilesRequest.h"

#include <QImage>

#include <map>

class QNetworkReply;

class WebMapImage;

class WebMapWMS;
typedef WebMapWMS* WebMapWMS_x;
typedef const WebMapWMS* WebMapWMS_cx;

class WebMapWMSLayer;
typedef WebMapWMSLayer* WebMapWMSLayer_x;
typedef const WebMapWMSLayer* WebMapWMSLayer_cx;

class WebMapWmsCrsBoundingBox;
typedef const WebMapWmsCrsBoundingBox* WebMapWmsCrsBoundingBox_cx;

class WebMapWMSRequest : public WebMapTilesRequest
{
  Q_OBJECT

public:
  WebMapWMSRequest(WebMapWMS_x service, WebMapWMSLayer_cx layer, WebMapWmsCrsBoundingBox_cx crs, int zoom);

  ~WebMapWMSRequest();

  void addTile(int tileX, int tileY);

  void setDimensionValue(const std::string& dimName, const std::string& dimValue) override;

  /*! start fetching data */
  void submit() override;

  /*! stop fetching data */
  void abort() override;

  const Projection& tileProjection() const override;

  QImage legendImage() const override;

protected:
  QNetworkReply* submitRequest(WebMapTile* tile) override;

private Q_SLOTS:
  void legendFinished(WebMapImage*);

private:
  WebMapWMSLayer_cx mLayer;
  WebMapWmsCrsBoundingBox_cx mCrs;
  int mZoom;
  std::map<std::string, std::string> mDimensionValues;

  WebMapImage* mLegend;
  QImage mLegendImage;
};

typedef std::shared_ptr<WebMapRequest> WebMapRequest_p;

#endif // WebMapWMSRequest_h
