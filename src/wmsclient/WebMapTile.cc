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

#include "WebMapTile.h"

#include <QNetworkReply>

#include <QFont>
#include <QPainter>
#include <QTimer>

#define MILOGGER_CATEGORY "diana.WebMapWMS"
#include <miLogger/miLogging.h>

WebMapTile::WebMapTile(int column, int row, const Rectangle& rect)
  : mColumn(column)
  , mRow(row)
  , mRect(rect)
  , mReply(0)
{
}

WebMapTile::~WebMapTile()
{
  dropRequest();
}

void WebMapTile::submit(QNetworkReply* r)
{
  METLIBS_LOG_SCOPE();
  dropRequest();
  mReply = r;
  if (mReply) {
    METLIBS_LOG_DEBUG("waiting for " << mReply->url().toString().toStdString() << "'");
    connect(mReply, SIGNAL(finished()), this, SLOT(replyFinished()));
  } else {
    METLIBS_LOG_DEBUG("no QNetworkReply");
#if 0
    QTimer::singleShot(10, this, SLOT(replyFinished()));
#else
    replyFinished();
#endif
  }
}

void WebMapTile::abort()
{
  dropRequest();
}

void WebMapTile::dropRequest()
{
  if (mReply) {
    disconnect(mReply, SIGNAL(finished()), this, SLOT(replyFinished()));
    mReply->abort();
    mReply->deleteLater();
  }
  mReply = 0;
}

bool WebMapTile::loadImage(const char* format)
{
  METLIBS_LOG_SCOPE(LOGVAL(mColumn) << LOGVAL(mRow) << LOGVAL(format));
  if (mReply) {
    METLIBS_LOG_DEBUG(LOGVAL(mReply->isFinished()) << LOGVAL(mReply->error()));

    const QString ct = mReply->header(QNetworkRequest::ContentTypeHeader).toString();
    METLIBS_LOG_DEBUG("url='" << mReply->url().toString().toStdString() << "' Content-Type=" << ct.toStdString() << "'");
    bool ok = false;
    const char* fmt = 0;
    if (ct == "image/png")
      fmt = "PNG";
    else if (ct == "image/jpeg")
      fmt = "JPEG";
    if (fmt != 0) {
#if 0
      ok = mImage.load(mReply, fmt);
#else
      const QByteArray data = mReply->readAll();
      ok = mImage.loadFromData(data, fmt);
#endif
    }
    METLIBS_LOG_DEBUG(LOGVAL(mImage.width()) << LOGVAL(mImage.height()) << LOGVAL(ok));
  }
  return !mImage.isNull();
}

void WebMapTile::dummyImage(int tw, int th)
{
  mImage = QImage(tw, th, QImage::Format_ARGB32);
  mImage.fill(Qt::transparent);
  QPainter p(&mImage);
  const float fontScale = 3;
  QFont f = p.font();
  int fpx = f.pixelSize();
  if (fpx > 0)
    f.setPixelSize(fontScale*fpx);
    else
      f.setPointSizeF(fontScale*f.pointSizeF());
  p.setFont(f);
  p.fillRect(mImage.rect(), QBrush(QColor((mColumn&1) ? 80 : 255, (mRow&1) ? 80 : 255, 128, 128)));
  p.drawText(mImage.rect(), Qt::AlignLeft | Qt::AlignVCenter, QString("x=%1 y=%2").arg(mColumn).arg(mRow));
  p.end();
}

void WebMapTile::replyFinished()
{
  Q_EMIT finished(this);
}
