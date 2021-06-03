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

#include "WebMapImage.h"

#include "WebMapUtilities.h"

#include <QNetworkReply>

#define MILOGGER_CATEGORY "diana.WebMapImage"
#include <miLogger/miLogging.h>

using diutil::qs;

WebMapImage::WebMapImage()
    : mReply(nullptr)
{
}

WebMapImage::~WebMapImage()
{
  dropRequest();
}

void WebMapImage::submit(QNetworkReply* r)
{
  dropRequest();
  mReply = r;
  if (mReply) {
    METLIBS_LOG_DEBUG("waiting for " << mReply->url().toString().toStdString() << "'");
    connect(mReply, &QNetworkReply::finished, this, &WebMapImage::replyFinished);
  } else {
    METLIBS_LOG_ERROR("submit without QNetworkReply");
    replyFinished();
  }
}

void WebMapImage::abort()
{
  dropRequest();
}

void WebMapImage::dropRequest()
{
  if (mReply) {
    disconnect(mReply, &QNetworkReply::finished, this, &WebMapImage::replyFinished);
    mReply->abort();
    mReply->deleteLater();
  }
  mReply = nullptr;
}

bool WebMapImage::loadImage()
{
  METLIBS_LOG_SCOPE("url='" << qs(mReply->url().toString()));
  if (!mReply) {
    METLIBS_LOG_WARN("url='" << qs(mReply->url().toString()) << " has not network reply");
    return false;
  }

  if (!mReply->isFinished()) {
    METLIBS_LOG_WARN("url='" << qs(mReply->url().toString()) << " is not finished");
    return false;
  }

  if (mReply->error() != QNetworkReply::NoError) {
    METLIBS_LOG_WARN("url='" << qs(mReply->url().toString()) << " has error: " << qs(mReply->errorString()));
    return false;
  }

  const QString ct = mReply->header(QNetworkRequest::ContentTypeHeader).toString();
  METLIBS_LOG_DEBUG("Content-Type='" << ct.toStdString() << "'");
  const char* fmt = 0;
  if (ct == "image/png")
    fmt = "PNG";
  else if (ct == "image/jpeg")
    fmt = "JPEG";
  else if (ct.startsWith("text")) {
    QString text = QString::fromUtf8(mReply->readAll().constData());
    METLIBS_LOG_WARN("url '" << mReply->url().toString().toStdString() << "' resulted in text reply: " << diutil::qs(text));
    return false;
  }

  const QByteArray data = mReply->readAll();
  if (!mImage.loadFromData(data, fmt) || mImage.isNull()) {
    METLIBS_LOG_WARN("url '" << mReply->url().toString().toStdString() << "' result image could not be loaded");
    return false;
  }

  return true;
}

void WebMapImage::replyFinished()
{
  Q_EMIT finishedImage(this);
}
