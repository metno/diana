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

#ifndef WebMapWMTSTileMatrixSet_h
#define WebMapWMTSTileMatrixSet_h 1

#include "WebMapWMTSTileMatrix.h"

#include <diField/diProjection.h>

#include <string>
#include <vector>

class WebMapWMTSTileMatrixSet
{
public:
  WebMapWMTSTileMatrixSet(const std::string& id, const std::string& crs);

  void addMatrix(const WebMapWMTSTileMatrix& m) { mMatrices.push_back(m); }

  /*! TileMatrixSet identifier */
  const std::string& identifier() const { return mIdentifier; }

  const Projection& projection() const { return mProjection; }

  double metersPerUnit() const { return mMetersPerUnit; }

  /*! number of tilematrices available */
  size_t countMatrices() const { return mMatrices.size(); }

  /*! access to a tilematrix */
  const WebMapWMTSTileMatrix& matrix(size_t idx) const { return mMatrices.at(idx); }

  double pixelSpan(WebMapWMTSTileMatrix_cx matrix) const;
  WebMapWMTSTileMatrix_cx findMatrixForScale(float denominator) const;

private:
  std::string mIdentifier;
  Projection mProjection;
  double mMetersPerUnit;
  std::vector<WebMapWMTSTileMatrix> mMatrices;
};

typedef WebMapWMTSTileMatrixSet* WebMapWMTSTileMatrixSet_x;
typedef const WebMapWMTSTileMatrixSet* WebMapWMTSTileMatrixSet_cx;

#endif // WebMapWMTSTileMatrixSet_h
