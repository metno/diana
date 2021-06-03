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

#ifndef WebMapRequest_h
#define WebMapRequest_h 1

#include "diField/diRectangle.h"

#include <QObject>

#include <memory>
#include <string>

class Projection;

class QImage;

class WebMapRequest : public QObject
{
  Q_OBJECT;

public:
  WebMapRequest();
  virtual ~WebMapRequest();

  /*! set dimension value; ignores non-existent dimensions; dimensions
   *  without explicitly specified value are set to a default value */
  virtual void setDimensionValue(const std::string& dimIdentifier, const std::string& dimValue) = 0;

  /*! start fetching data */
  virtual void submit() = 0;

  /*! stop fetching data */
  virtual void abort() = 0;

  /*! number of tiles */
  virtual size_t countTiles() const = 0;

  /*! rectangle of one tile, in tileProjection coordinates */
  virtual const Rectangle& tileRect(size_t idx) const = 0;

  /*! image data of one tile; might have isNull() == true */
  virtual const QImage& tileImage(size_t idx) const = 0;

  /*! tile index of the tile containing one point; might be == -1 if no such tile */
  virtual size_t tileIndex(float x, float y);

  /*! projection of all tiles */
  virtual const Projection& tileProjection() const = 0;

  /*! legend image; might have isNull() == true */
  virtual QImage legendImage() const;

public:
  Rectangle tilebbx;
  float x0, dx, y0, dy;

private:
  size_t lastTileIndex; //! cache for "tileIndex"

Q_SIGNALS:
  /*! the request is complete, ready for rendering, or aborted */
  void completed(bool success);
};

typedef WebMapRequest* WebMapRequest_x;
typedef std::shared_ptr<WebMapRequest> WebMapRequest_p;

#endif // WebMapRequest_h
