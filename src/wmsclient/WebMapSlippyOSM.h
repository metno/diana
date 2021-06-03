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

#ifndef WebMapSlippyOSM_h
#define WebMapSlippyOSM_h 1

#include "WebMapService.h"

#include <diField/diProjection.h>

#include <QUrl>

class WebMapSlippyOSMLayer;
typedef WebMapSlippyOSMLayer* WebMapSlippyOSMLayer_x;
typedef const WebMapSlippyOSMLayer* WebMapSlippyOSMLayer_cx;

class WebMapSlippyOSM : public WebMapService
{
  Q_OBJECT

public:
  /* set with URL pointing to GetCapabilities. need to call refresh to
   * actually fetch data */
  WebMapSlippyOSM(const std::string& identifier, const QUrl& service, QNetworkAccessManager* network);

  ~WebMapSlippyOSM();

  /*! suitable refresh interval. negative for no refresh */
  int refreshInterval() const override;

  /*! create a request object for the specified layer. may be null,
   *  e.g. if unknown layer or; ownership is transferred to caller */
  WebMapRequest_x createRequest(const std::string& layer,
      const Rectangle& viewRect, const Projection& viewProj, double viewScale, int w, int h) override;

  QNetworkReply* submitRequest(WebMapSlippyOSMLayer_cx layer,
      int zoom, int tileX, int tileY);

  void refresh() override;

  const Projection& projection() const
    { return mProjection; }

private:
  bool parseReply();

private Q_SLOTS:
  void refreshReplyFinished();

private:
  QUrl mServiceURL;
  long mNextRefresh;
  QNetworkReply* mRefeshReply;
  Projection mProjection;
};

#endif // WebMapSlippyOSM_h
