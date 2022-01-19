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

#include "WebMapTile.h"

#include <QFont>
#include <QPainter>

#define MILOGGER_CATEGORY "diana.WebMapTile"
#include <miLogger/miLogging.h>

WebMapTile::WebMapTile(int column, int row, const Rectangle& rect)
  : mColumn(column)
  , mRow(row)
  , mRect(rect)
{
}

WebMapTile::~WebMapTile()
{
}

void WebMapTile::dummyImage(int tw, int th)
{
  METLIBS_LOG_SCOPE();
  const bool noimage = mImage.isNull();
  if (noimage) {
    mImage = QImage(tw, th, QImage::Format_ARGB32);
    mImage.fill(Qt::transparent);
  }
  QPainter p(&mImage);
  const float fontScale = 3;
  QFont f = p.font();
  int fpx = f.pixelSize();
  if (fpx > 0)
    f.setPixelSize(fontScale*fpx);
  else
    f.setPointSizeF(fontScale*f.pointSizeF());
  p.setFont(f);
  const QColor tilecolor((mColumn&1) ? 80 : 255, (mRow&1) ? 80 : 255, 128, 128);
  if (noimage)
    p.fillRect(mImage.rect(), QBrush(tilecolor));
  else
    p.setPen(QPen(tilecolor));
  p.drawText(mImage.rect(), Qt::AlignLeft | Qt::AlignVCenter, QString("x=%1 y=%2").arg(mColumn).arg(mRow));
  p.end();
}

void WebMapTile::replyFinished()
{
  WebMapImage::replyFinished();
  Q_EMIT finished(this);
}
