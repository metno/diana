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

#include "WebMapWMTSTileMatrixSet.h"

#include "WebMapUtilities.h"

#define MILOGGER_CATEGORY "diana.WebMapWMTSTileMatrixSet"
#include <miLogger/miLogging.h>

WebMapWMTSTileMatrixSet::WebMapWMTSTileMatrixSet(const std::string& id, const std::string& crs)
    : mIdentifier(id)
    , mCRS(crs)
    , mProjection(diutil::projectionForCRS(crs))
    , mMetersPerUnit(diutil::metersPerUnit(mProjection))
{
}

WebMapWMTSTileMatrix_cx WebMapWMTSTileMatrixSet::findMatrixForScale(float denominator) const
{
  METLIBS_LOG_SCOPE(LOGVAL(denominator));
  if (denominator <= 0)
    return 0;

  const double MAX_RATIO = 2.5;
  WebMapWMTSTileMatrix_cx best = nullptr;
  double bestRatio = 1;
  for (const WebMapWMTSTileMatrix& matrix : mMatrices) {
    double ratio = 0.5 * matrix.scaleDenominator() / denominator;
    METLIBS_LOG_DEBUG(LOGVAL(matrix.identifier()) << LOGVAL(matrix.scaleDenominator()) << LOGVAL(ratio));
    if (ratio < 1 / MAX_RATIO)
      continue;
    if (ratio < 1)
      ratio = 1 / ratio;
    if (ratio < MAX_RATIO && (!best || ratio < bestRatio)) {
      bestRatio = ratio;
      best = &matrix;
      METLIBS_LOG_DEBUG(LOGVAL(bestRatio));
    }
  }
  return best;
}

double WebMapWMTSTileMatrixSet::pixelSpan(WebMapWMTSTileMatrix_cx matrix) const
{
  return matrix->scaleDenominator()
         * diutil::WMTS_M_PER_PIXEL
         / metersPerUnit();
}
