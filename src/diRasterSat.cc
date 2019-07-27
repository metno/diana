/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2019 met.no

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

#include "diRasterSat.h"

//#define REMEMBER 1

RasterSat::RasterSat(const PlotArea& pa, const GridArea& ga, const unsigned char* rgba)
    : pa_(pa)
    , ga_(ga)
    , rgba_(rgba)
    , nx(ga_.nx)
    , ny(ga_.ny)
    , fxy0(ga_.R().x1, ga_.R().y1)
    , res_inv(1 / ga_.resolutionX, 1 / ga_.resolutionY)
{
}

RasterSat::~RasterSat() {}

void RasterSat::rasterPixels(int n, const diutil::PointD& xy0, const diutil::PointD& dxy, QRgb* pixels)
{
  diutil::PointD ixy = (xy0 - fxy0) * res_inv;
  const diutil::PointD step = dxy * res_inv;
#ifdef REMEMBER
  int li = -1, lx = -1, ly = -1;
#endif
  for (int i = 0; i < n; ++i, ixy += step) {
    const double dxy = 0.5;
    const int ix = int(ixy.x() - dxy), iy = int(ixy.y() + dxy);
    if (ix < 0 || ix >= nx || iy < 0 || iy >= ny)
      continue;
#ifdef REMEMBER
    if (li >= 0 && ix == lx && iy == ly) {
      pixels[i] = pixels[li];
    } else {
#endif
      const unsigned char* p = &rgba_[(ix + iy * nx) * 4];
      pixels[i] = qRgba(p[0], p[1], p[2], p[3]);
#ifdef REMEMBER
      li = i;
      lx = ix;
      ly = iy;
    }
#endif
  }
}
