/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2022 met.no

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

#include "diana_config.h"

#include "diGridConverter.h"

#include <mi_fieldcalc/math_util.h>
#include <mi_fieldcalc/openmp_tools.h>

#include <cmath>
#include <memory.h>

#define MILOGGER_CATEGORY "diField.GridConverter"
#include "miLogger/miLogging.h"

#if 0
#define IFDEBUG(x) x
#else
#define IFDEBUG(x) do { } while (false)
#endif

static const float undef = +1.e+35;

static const float DEG_TO_RAD = M_PI / 180;

MapFields::MapFields()
  : xmapr(0)
  , ymapr(0)
  , coriolis(0)
{
}

MapFields::~MapFields()
{
  delete[] xmapr;
  delete[] ymapr;
  delete[] coriolis;
}

Points::Points()
    : npos(0)
    , gridboxes(false)
    , x(0)
    , y(0)
    , ixmin(0)
    , ixmax(0)
    , iymin(0)
    , iymax(0)
{
}

Points::~Points()
{
  delete[] x;
  delete[] y;
}

GridConverter::GridConverter() {}

GridConverter::~GridConverter()
{
}

void GridConverter::setBufferSize(const int s)
{
  pointbuffer.reset(new ring<Points_p>(s));
}

void GridConverter::setAngleBufferSize(const int s)
{
  anglebuffer.reset(new ring<Points_p>(s));
}

void GridConverter::setBufferSizeMapFields(const int s)
{
  mapfieldsbuffer.reset(new ring<MapFields_p>(s));
}

Points_p GridConverter::doGetGridPoints(const GridArea& area, const Projection& map_proj, bool gridboxes)
{
  std::lock_guard<std::mutex> lock(point_mutex);
  if (!pointbuffer)
    setBufferSize(defringsize);

  int nx = area.nx, ny = area.ny;
  if (gridboxes) {
    nx++;
    ny++;
  }
  const int npos = nx*ny;

  const int n = pointbuffer->size();
  for (int i=0; i < n; ++i) {
    Points_p pi = (*pointbuffer)[i];
    if (pi->area == area && pi->map_proj == map_proj && pi->npos == npos && pi->gridboxes == gridboxes) {
      return pi;
    }
  }

  // not found in cache

  std::unique_ptr<float[]> x(new float[npos]);
  std::unique_ptr<float[]> y(new float[npos]);

  const float gdxy = gridboxes ? 0.5 : 0; // offset by half cell iff using gridboxes

  MIUTIL_OPENMP_PARALLEL(npos, for)
  for (int iy = 0; iy < ny; iy++) {
    for (int ix = 0; ix < (nx-1); ix++) {
      int i = ix + iy*nx;
      x[i] = area.fromGridX(ix - gdxy);
      y[i] = area.fromGridY(iy - gdxy);
    }
    int i = iy*nx + nx - 1;
    // FIXME x[i]=nx-1 converts to x[i]=0 when transforming between geo-projections
    x[i] = area.fromGridX(nx - gdxy - 1.1);
    y[i] = area.fromGridY(iy - gdxy);
  }

  const Projection& pa = area.P();
  if (!map_proj.convertPoints(pa, npos, x.get(), y.get()))
    return nullptr;

  Points_p p0 = std::make_shared<Points>();
  p0->area = area;
  p0->map_proj = map_proj;
  p0->gridboxes = gridboxes;
  p0->npos = npos;
  p0->x = x.release();
  p0->y = y.release();
  p0->maprect = Rectangle();
  p0->ixmin = p0->ixmax = p0->iymin = p0->iymax = 0;
  pointbuffer->push(p0);
  return p0;
}

Points_cp GridConverter::getGridPoints(const GridArea& area, const Area& map_area, bool gridboxes)
{
  return doGetGridPoints(area, map_area.P(), gridboxes);
}

Points_cp GridConverter::getGridPoints(const GridArea& area, const Area& map_area, const Rectangle& maprect, bool gridboxes, int& ix1, int& ix2, int& iy1,
                                       int& iy2)
{
  Points_p pi = doGetGridPoints(area, map_area.P(), gridboxes);
  if (!pi)
    return nullptr;

  const bool skiplimits = (maprect.width() == 0 || maprect.height() == 0);

  if (skiplimits) {
    ix1 = 0;
    ix2 = 0;
    iy1 = 0;
    iy2 = 0;
  } else if (pi->maprect == maprect) {
    ix1 = pi->ixmin;
    ix2 = pi->ixmax;
    iy1 = pi->iymin;
    iy2 = pi->iymax;
  } else {
    findGridLimits(area, maprect, gridboxes, pi->x, pi->y, ix1, ix2, iy1, iy2);
    std::lock_guard<std::mutex> lock(point_mutex);
    // FIXME still bad if receivers use more than x and y
    pi->maprect = maprect;
    pi->ixmin = ix1;
    pi->ixmax = ix2;
    pi->iymin = iy1;
    pi->iymax = iy2;
  }

  return pi;
}

// static
void GridConverter::findGridLimits(const GridArea& area, const Rectangle& maprect, bool gridboxes, const float* x, const float* y, int& ix1, int& ix2, int& iy1,
                                   int& iy2)
{
  IFDEBUG(METLIBS_LOG_SCOPE(LOGVAL(area) << LOGVAL(maprect) << LOGVAL(gridboxes)));
  const int gdxy = gridboxes ? 1 : 0;
  const int nx = area.nx + gdxy, ny = area.ny + gdxy;
  IFDEBUG(METLIBS_LOG_DEBUG(LOGVAL(nx) << LOGVAL(ny)));

  // find needed (field) area to cover map area

  // check field corners
  int numinside = 0;
  const int dix = std::max(nx - 1, 1), diy = std::max(ny - 1, 1);
  for (int iy = 0; iy < ny; iy += diy) {
    for (int ix = 0; ix < nx; ix += dix) {
      const int idx = iy * nx + ix;
      if (maprect.isinside(x[idx], y[idx]))
        numinside++;
    }
  }
  IFDEBUG(METLIBS_LOG_DEBUG(LOGVAL(numinside)));
  if (numinside >= 3) {
    ix1 = 0;
    iy1 = 0;
    ix2 = nx - 1 - gdxy; // FIXME old GridConverter, excludes last column iff gridboxes
    iy2 = ny - 1 - gdxy;
    return;
  }

  ix1 = nx;
  ix2 = -1;
  iy1 = ny;
  iy2 = -1;

  for (int iy = 0; iy < ny; iy++) {
    int idx = iy * nx;
    bool was_inside = maprect.isinside(x[idx], y[idx]);
    int left = was_inside ? 0 : -1;
    int right = -1;
    if (was_inside)
      miutil::minimaximize(iy1, iy2, iy);
    IFDEBUG(METLIBS_LOG_DEBUG(LOGVAL(iy) << LOGVAL(was_inside)
            << LOGVAL(left) << LOGVAL(right) << LOGVAL(iy1) << LOGVAL(iy2)));

    for (int ix = 1; ix < nx; ix++) {
      idx += 1;
      const bool inside = maprect.isinside(x[idx], y[idx]);
      if (inside) {
        miutil::minimaximize(iy1, iy2, iy);
        if (left == -1)
          left = ix;
        IFDEBUG(METLIBS_LOG_DEBUG(LOGVAL(ix) << LOGVAL(left) << LOGVAL(iy1) << LOGVAL(iy2)));
      }
      if (was_inside && !inside) {
        right = ix - 1;
        IFDEBUG(METLIBS_LOG_DEBUG(LOGVAL(ix) << LOGVAL(right)));
      }

      was_inside = inside;
    }
    // Handle the cases where the map and field areas overlap partially or completely.
    if (left == -1 && right != -1)
      left = 0;                     // left edges of the field and map areas may coincide
    if (left != -1 && right == -1)
      right = nx - 1;               // right edges of the field and map areas may coincide
    if (left == -1 && right == -1 && was_inside) {
      left = 0;                     // the whole span coincides with the map area
      right = nx - 1;
    }
    IFDEBUG(METLIBS_LOG_DEBUG(LOGVAL(left) << LOGVAL(right)));

    // Expand to the left.
    if (left != -1) {
      miutil::minimize(ix1, left);
      IFDEBUG(METLIBS_LOG_DEBUG(LOGVAL(ix1)));
    }
    // Expand to the right.
    if (right != -1) {
      miutil::maximize(ix2, right);
      IFDEBUG(METLIBS_LOG_DEBUG(LOGVAL(ix2)));
    }
  }

  IFDEBUG(METLIBS_LOG_DEBUG(LOGVAL(ix1) << LOGVAL(ix2) << LOGVAL(iy1) << LOGVAL(iy2)));
  ix1-=2;
  ix2+=2;
  iy1-=2;
  iy2+=2;

  miutil::maximize(ix1, 0);
  miutil::maximize(iy1, 0);
  miutil::minimize(ix2, nx - 1 - gdxy); // FIXME old GridConverter, excludes last column iff gridboxes
  miutil::minimize(iy2, ny - 1 - gdxy);
}

// get arrays of vector rotation elements
// x and y are in map_proj coordinates
Points_p GridConverter::getVectorRotationElements(const Area& data_area, const Projection& map_proj, int nvec, const float* x, const float* y)
{
  std::lock_guard<std::mutex> lock(angle_mutex);
  if (!anglebuffer)
    setAngleBufferSize(defringsize_angles);

  // search buffer for existing point-set
  const int n = anglebuffer->size();
  for (int i=0; i < n; ++i) {
    Points_p pi = (*anglebuffer)[i];
    if ((Area)pi->area == data_area && pi->map_proj == map_proj && pi->npos == nvec) {
      return pi;
    }
  }

  // need to calculate new rotation elements
  std::unique_ptr<float[]> cosx(new float[nvec]);
  std::unique_ptr<float[]> sinx(new float[nvec]);
  if (!map_proj.calculateVectorRotationElements(data_area.P(), nvec, x, y, cosx.get(), sinx.get()))
    return nullptr;

  Points_p p0 = std::make_shared<Points>();
  p0->area = GridArea(data_area);
  p0->map_proj = map_proj;
  p0->npos = nvec;
  p0->x = cosx.release();
  p0->y = sinx.release();
  anglebuffer->push(p0);
  return p0;
}

// convert u,v vector coordinates for points x,y
bool GridConverter::getVectors(const Area& data_area, const Projection& map_proj,
    int nvec, const float *x, const float *y, float* u, float* v)
{
  Points_cp p = getVectorRotationElements(data_area, map_proj, nvec, x, y);
  if (!p)
    return false;

  const float* cosx = p->x;
  const float* sinx = p->y;

  MIUTIL_OPENMP_PARALLEL(nvec, for)
  for (int i = 0; i < nvec; ++i) {
    if (u[i] != undef && v[i] != undef) {
      if (cosx[i] == HUGE_VAL || sinx[i] == HUGE_VAL) {
        u[i] = undef;
        v[i] = undef;
      } else {
        const float ui = u[i], vi = v[i];
        u[i] = cosx[i] * ui - sinx[i] * vi;
        v[i] = sinx[i] * ui + cosx[i] * vi;
      }
    }
  }

  return true;
}

// convert true north direction and velocity (dd=u,ff=v)
// to u,v vector coordinates for points x,y
bool GridConverter::getDirectionVectors(const Area& map_area, int nvec, const float* x, const float* y, float* u, float* v)
{
  // make vector-components (east/west and north/south) in geographic grid,
  // to be rotated to the map grid
  // u,v is dd,ff coming in
  MIUTIL_OPENMP_PARALLEL(nvec, for)
  for (int i=0; i<nvec; ++i) {
    if (u[i] != undef && v[i] != undef) {
      float dd = u[i] * DEG_TO_RAD;
      float ff = v[i];
      u[i] = ff * sinf(dd);
      v[i] = ff * cosf(dd);
    }
  }

  const Area geo_area(Projection::geographic(), Rectangle());
  return getVectors(geo_area, map_area.P(), nvec, x, y, u, v);
}

MapFields_cp GridConverter::getMapFields(const GridArea& area)
{
  std::lock_guard<std::mutex> lock(mapfields_mutex);
  if (!mapfieldsbuffer)
    setBufferSizeMapFields(defringsize_mapfields);

  // search buffer for existing data
  int n = mapfieldsbuffer->size();
  METLIBS_LOG_DEBUG("++ Map.Ringbuffer's size is now: " << n);

  const int npos = area.gridSize();

  for (int i = 0; i < n; ++i) {
    MapFields_p mf = (*mapfieldsbuffer)[i];
    if (mf->area == area)
      return mf;
  }

  std::unique_ptr<float> xmapr(new float[npos]);
  std::unique_ptr<float> ymapr(new float[npos]);
  std::unique_ptr<float> coriolis(new float[npos]);
  if (!area.P().getMapRatios(area.nx, area.ny, area.R().x1, area.R().y1, area.resolutionX, area.resolutionY, xmapr.get(), ymapr.get(), coriolis.get())) {
    METLIBS_LOG_ERROR("getMapRatios problem");
    return nullptr;
  }

  MapFields_p mf = std::make_shared<MapFields>();
  mf->area = area;
  mf->xmapr = xmapr.release();
  mf->ymapr = ymapr.release();
  mf->coriolis = coriolis.release();
  mapfieldsbuffer->push(mf);
  return mf;
}
