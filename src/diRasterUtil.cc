/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018 met.no

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

#include "diRasterUtil.h"

#include "diPainter.h"
#include "diPlotOptions.h"

#define MILOGGER_CATEGORY "diana.RasterUtil"
#include <miLogger/miLogging.h>

namespace diutil {

bool mini_maxi(const float* data, size_t n, float UNDEF, float& mini, float& maxi)
{
  bool is_set = false;
  for (size_t i = 0; i < n; ++i) {
    const float v = data[i];
    if (v == UNDEF)
      continue;
    if (!is_set || mini > v)
      mini = v;
    if (!is_set || maxi < v)
      maxi = v;
    is_set = true;
  }
  return is_set;
}

void mincut_maxcut(const float* image, size_t size, float UNDEF, float cut, float& mincut, float& maxcut)
{
  const int N = 256;
  int nindex[N];
  for (int i = 0; i < N; i++)
    nindex[i] = 0;
  int nundef = 0;

  const float cdiv = (N - 1) / (maxcut - mincut);

  for (size_t i = 0; i < size; i++) {
    if (image[i] == UNDEF) {
      nundef += 1;
    } else {
      const int j = (int)((image[i] - mincut) * cdiv);
      if (j >= 0 && j < N)
        nindex[j] += 1;
    }
  }

  const float npixel = size - nundef - nindex[0]; // number of pixels, drop index=0
  const int ncut = (int)(npixel * cut);           // number of pixels to cut   +1?
  int mini = 1;
  int maxi = N - 1;
  int ndropped = 0; // number of pixels dropped

  while (ndropped < ncut && mini < maxi) {
    if (nindex[mini] < nindex[maxi]) {
      ndropped += nindex[mini];
      mini++;
    } else {
      ndropped += nindex[maxi];
      maxi--;
    }
  }
  mincut = mini;
  maxcut = maxi;
}

void ColourLimits::initialize(const PlotOptions& poptions, float mini, float maxi)
{
  METLIBS_LOG_SCOPE();
  colours = &poptions.colours;
  limits = &poptions.limits;

  if (colours->size() < 2) {
    METLIBS_LOG_DEBUG("using default colours");
    colours_default.push_back(Colour::fromF(0.5, 0.5, 0.5));
    colours_default.push_back(Colour::fromF(0, 0, 0));
    colours_default.push_back(Colour::fromF(0, 1, 1));
    colours_default.push_back(Colour::fromF(1, 0, 0));
    colours = &colours_default;
  }

  if (limits->size() < 1) {
    // default, should be handled when reading setup, if allowed...
    const int nlim = 3;
    const float dlim = (maxi - mini) / colours->size();
    for (int i = 1; i <= nlim; i++) {
      limits_default.push_back(mini + dlim * i);
      METLIBS_LOG_DEBUG("using default limit[" << (i - 1) << "]=" << limits_default.back());
    }

    limits = &limits_default;
  }

  ncl = std::min(colours->size() - 1, limits->size());
  METLIBS_LOG_DEBUG(LOGVAL(ncl));
}

void ColourLimits::setColour(DiPainter* gl, float c) const
{
  if (ncl > 0) {
    size_t l = 0;
    while (l < ncl && c > (*limits)[l]) // TODO binary search
      l++;
    gl->setColour(colours->at(l), false);
  }
}

} // namespace diutil
