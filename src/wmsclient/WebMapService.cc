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

#include "WebMapService.h"

#include "WebMapUtilities.h"
#include "diField/diRectangle.h"
#include "diUtilities.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QUrlQuery>
#endif

#include "diana_config.h"

#define MILOGGER_CATEGORY "diana.WebMapService"
#include <miLogger/miLogging.h>

WebMapService::WebMapService(const std::string& identifier, QNetworkAccessManager* network)
    : mIdentifier(identifier)
    , mExcludeLayersWithChildren(false)
    , mNetworkAccess(network)
{
}

WebMapService::~WebMapService()
{
  destroyLayers();
}

void WebMapService::destroyLayers()
{
  diutil::delete_all_and_clear(mLayers);
}

QNetworkReply* WebMapService::submitUrl(QUrl url)
{
  if (!mExtraQueryItems.empty()) {
    QUrlQuery urlq(url.query());
    for (auto&& kv : mExtraQueryItems)
      urlq.addQueryItem(diutil::sq(kv.first), diutil::sq(kv.second));
    url.setQuery(urlq);
  }

  QNetworkRequest nr(url);
  nr.setRawHeader("User-Agent", "diana " PVERSION);

  if (!mBasicAuth.empty()) {
    QString concatenated = QString::fromStdString(mBasicAuth);
    QByteArray data = concatenated.toUtf8().toBase64();
    QString headerData = "Basic " + data;
    nr.setRawHeader("Authorization", headerData.toLocal8Bit());
  }

  return mNetworkAccess->get(nr);
}

int WebMapService::refreshInterval() const
{
  return -1;
}

void WebMapService::refresh()
{
  Q_EMIT refreshStarting();
  Q_EMIT refreshFinished();
}

WebMapLayer_cx WebMapService::findLayerByIdentifier(const std::string& identifier)
{
  for (size_t i=0; i<countLayers(); ++i) {
    if (layer(i)->identifier() == identifier)
      return layer(i);
  }
  return 0;
}
