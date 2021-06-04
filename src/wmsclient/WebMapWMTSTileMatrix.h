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

#ifndef WebMapWMTSTileMatrix_h
#define WebMapWMTSTileMatrix_h 1

#include <string>

class WebMapWMTSTileMatrix
{
public:
  WebMapWMTSTileMatrix(const std::string& id, double sd, double tmx, double tmy, size_t mw, size_t mh, size_t tw, size_t th);

  /*! TileMatrix identifier */
  const std::string& identifier() const { return mIdentifier; }

  double scaleDenominator() const { return mDenominator; }

  double tileMinX() const { return mTileMinX; }

  double tileMaxY() const { return mTileMaxY; }

  size_t matrixWidth() const { return mMatrixWidth; }

  size_t matrixHeight() const { return mMatrixHeight; }

  size_t tileWidth() const { return mTileWidth; }

  size_t tileHeight() const { return mTileHeight; }

private:
  std::string mIdentifier;
  double mDenominator;
  double mTileMinX, mTileMaxY;
  size_t mMatrixWidth, mMatrixHeight;
  size_t mTileWidth, mTileHeight;
};

typedef WebMapWMTSTileMatrix* WebMapWMTSTileMatrix_x;
typedef const WebMapWMTSTileMatrix* WebMapWMTSTileMatrix_cx;

#endif // WebMapWMTSTileMatrix_h
