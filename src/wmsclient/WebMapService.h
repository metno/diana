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

#ifndef WebMapService_h
#define WebMapService_h 1

#include "WebMapLayer.h"
#include "WebMapRequest.h"

#include <QObject>

#include <string>
#include <vector>

class Projection;
class Rectangle;

class QNetworkAccessManager;
class QNetworkReply;

class WebMapService : public QObject {
  Q_OBJECT;

protected:
  WebMapService(const std::string& identifier, QNetworkAccessManager* network);

public:
  virtual ~WebMapService();

  void setBasicAuth(const std::string& basicauth)
    { mBasicAuth = basicauth; }

  void addExtraQueryItem(const std::string& k, const std::string& v)
    { mExtraQueryItems.push_back(std::make_pair(k, v)); }

  const std::string& identifier() const
    { return mIdentifier; }

  /*! human-readable title. only valid after refreshComplete */
  const std::string& title(/*language*/) const
    { return mTitle; }

  /*! suitable refresh interval in seconds. negative for no refresh */
  virtual int refreshInterval() const;

  /*! number of layers available */
  size_t countLayers() const
    { return mLayers.size(); }

  /*! access to a layer */
  WebMapLayer_cx layer(size_t idx) const
    { return mLayers.at(idx); }

  WebMapLayer_cx findLayerByIdentifier(const std::string& identifier);

  /*! create a request object for the specified layer. may be null,
   *  e.g. if unknown layer or */
  virtual WebMapRequest_x createRequest(const std::string& layer,
      const Rectangle& viewRect, const Projection& viewProj, double viewScale, int w, int h) = 0;

  QNetworkReply* submitUrl(QUrl url);

public Q_SLOTS:
  /* trigger reload of capabilities */
  virtual void refresh();

Q_SIGNALS:
  /*! the service capabilities will be refreshed */
  void refreshStarting();

  /*! the service capabilities have been refreshed */
  void refreshFinished();

protected:
  void destroyLayers();

protected:
  std::string mIdentifier;
  std::string mTitle;
  std::string mBasicAuth;
  std::vector< std::pair< std::string,std::string > > mExtraQueryItems;
  std::vector<WebMapLayer_cx> mLayers;

private:
  QNetworkAccessManager* mNetworkAccess;
};

#endif // WebMapService_h
