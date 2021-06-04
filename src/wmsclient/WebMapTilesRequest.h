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

#ifndef DIANA_WMSCLIENT_WEBMAPTILESREQUEST_H
#define DIANA_WMSCLIENT_WEBMAPTILESREQUEST_H

#include "WebMapRequest.h"

class WebMapTile;
class WebMapService;
class QNetworkReply;

class WebMapTilesRequest : public WebMapRequest
{
  Q_OBJECT;

public:
  WebMapTilesRequest(WebMapService* service);
  ~WebMapTilesRequest();

  /*! start fetching data */
  void submit() override;

  /*! stop fetching data */
  void abort() override;

  /*! number of tiles */
  size_t countTiles() const override { return mTiles.size(); }

  /*! rectangle of one tile, in tileProjection coordinates */
  const Rectangle& tileRect(size_t idx) const override;

  /*! image data of one tile; might have isNull() == true */
  const QImage& tileImage(size_t idx) const override;

protected:
  void addExpected();
  void addFinished(bool success);
  void addTile(WebMapTile*);
  virtual QNetworkReply* submitRequest(WebMapTile*) = 0;

private Q_SLOTS:
  void tileFinished(WebMapTile*);

protected:
  WebMapService* mService;

private:
  std::vector<WebMapTile*> mTiles;
  size_t mExpected;
  size_t mFinished;
  size_t mFinishedSuccess;
};

#endif // DIANA_WMSCLIENT_WEBMAPTILESREQUEST_H
