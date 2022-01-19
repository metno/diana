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

#ifndef WebMapWMTSRequest_h
#define WebMapWMTSRequest_h 1

#include "WebMapTilesRequest.h"

#include "WebMapWMTSTileMatrixSet.h"

#include <map>

class WebMapWMTS;
typedef WebMapWMTS* WebMapWMTS_x;
typedef const WebMapWMTS* WebMapWMTS_cx;

class WebMapWMTSLayer;
typedef WebMapWMTSLayer* WebMapWMTSLayer_x;
typedef const WebMapWMTSLayer* WebMapWMTSLayer_cx;

class WebMapWMTSRequest : public WebMapTilesRequest
{
  Q_OBJECT

public:
  WebMapWMTSRequest(WebMapWMTS_x service, WebMapWMTSLayer_cx layer, WebMapWMTSTileMatrixSet_cx matrixSet, WebMapWMTSTileMatrix_cx matrix);

  ~WebMapWMTSRequest();

  void addTile(int tileX, int tileY);

  /*! set dimension value; ignores non-existent dimensions; dimensions
   *  without explicitly specified value are set to a default value */
  void setDimensionValue(const std::string& dimIdentifier, const std::string& dimValue) override;

  const Projection& tileProjection() const override { return mMatrixSet->projection(); }

protected:
  QNetworkReply* submitRequest(WebMapTile* tile) override;

private:
  WebMapWMTSLayer_cx mLayer;
  WebMapWMTSTileMatrixSet_cx mMatrixSet;
  WebMapWMTSTileMatrix_cx mMatrix;

  std::map<std::string, std::string> mDimensionValues;
};

typedef std::shared_ptr<WebMapRequest> WebMapRequest_p;

#endif // WebMapWMTSRequest_h
