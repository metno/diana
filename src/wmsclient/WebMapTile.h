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

#ifndef WebMapTile_h
#define WebMapTile_h 1

#include <diField/diRectangle.h>

#include <QImage>
#include <QObject>

class QNetworkReply;


class WebMapImage : public QObject {
  Q_OBJECT;

public:
  WebMapImage();

  ~WebMapImage();

  const QImage& image() const
    { return mImage; }

  void submit(QNetworkReply* reply);

  void abort();

  bool loadImage(const char* format);
  bool loadImage(const std::string& format)
    { return loadImage(format.c_str()); }

  const QNetworkReply* reply() const
    { return mReply; }

protected Q_SLOTS:
  virtual void replyFinished();

Q_SIGNALS:
  void finishedImage(WebMapImage* self);

private:
  void dropRequest();

protected:
  QImage mImage;
  QNetworkReply* mReply;
};

// ========================================================================

class WebMapTile : public WebMapImage {
  Q_OBJECT;

public:
  WebMapTile(int column, int row, const Rectangle& rect);

  ~WebMapTile();

  int column() const
    { return mColumn; }
  int row() const
    { return mRow; }

  const Rectangle& rect() const
    { return mRect; }

  void dummyImage(int tw, int th);

protected /*Q_SLOTS*/:
  void replyFinished() /* Q_DECL_OVERRIDE */;

Q_SIGNALS:
  void finished(WebMapTile* self);

protected:
  int mColumn;
  int mRow;
  Rectangle mRect;
};

#endif // WebMapTile_h
