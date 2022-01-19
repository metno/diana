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

#include "WebMapRequest.h"

#include <QImage>

#define MILOGGER_CATEGORY "diana.WebMapRequest"
#include <miLogger/miLogging.h>

static const size_t INVALID_IDX = size_t(-1);

WebMapRequest::WebMapRequest()
    : lastTileIndex(INVALID_IDX)
{
}

WebMapRequest::~WebMapRequest() {}

QImage WebMapRequest::legendImage() const
{
  return QImage();
}

size_t WebMapRequest::tileIndex(float x, float y)
{
  if (lastTileIndex == INVALID_IDX || !tileRect(lastTileIndex).isinside(x, y)) {
    lastTileIndex = INVALID_IDX;
    for (size_t i = 0; i < countTiles(); ++i) {
      if (tileRect(i).isinside(x, y)) {
        lastTileIndex = i;
        break;
      }
    }
  }
  return lastTileIndex;
}
