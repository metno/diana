/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diGridConverter.h"

#include "../util/openmp_tools.h"
#include <VcrossUtil.h> // minimize / maximize

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

Points& Points::operator=(const Points& rhs)
{
  if (this != &rhs) {
    area = rhs.area;
    map_proj = rhs.map_proj;

    npos = rhs.npos;
    delete[] x;
    delete[] y;
    if (npos) {
      x = new float[npos];
      y = new float[npos];
      memcpy(x, rhs.x, sizeof(float)*npos);
      memcpy(y, rhs.y, sizeof(float)*npos);
    }
  }
  return *this;
}


GridConverter::GridConverter() :
  pointbuffer(0), anglebuffer(0), mapfieldsbuffer(0)
{
}

GridConverter::GridConverter(const int s, const int smf) :
  pointbuffer(0), anglebuffer(0), mapfieldsbuffer(0)
{
  setBufferSize(s);
  setAngleBufferSize(s);
  setBufferSizeMapFields(smf);
}

GridConverter::GridConverter(const int s) :
  pointbuffer(0), anglebuffer(0), mapfieldsbuffer(0)
{
  setBufferSize(s);
  setAngleBufferSize(s);
}

GridConverter::~GridConverter()
{
  delete pointbuffer;
  delete anglebuffer;
  delete mapfieldsbuffer;
}

void GridConverter::setBufferSize(const int s)
{
  delete pointbuffer;
  pointbuffer = new ring<Points> (s);
}

void GridConverter::setAngleBufferSize(const int s)
{
  delete anglebuffer;
  anglebuffer = new ring<Points> (s);
}

void GridConverter::setBufferSizeMapFields(const int s)
{
  delete mapfieldsbuffer;
  mapfieldsbuffer = new ring<MapFields> (s);
}

bool GridConverter::doGetGridPoints(const GridArea& area, const Projection& map_proj,
    bool gridboxes, float**x, float**y, int& ipb)
{
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
    const Points& pi = (*pointbuffer)[i];
    if (pi.area == area && pi.map_proj == map_proj
        && pi.npos == npos && pi.gridboxes == gridboxes)
    {
      ipb = i;
      *x = pi.x;
      *y = pi.y;
      return true;
    }
  }

  // not found in cache

  ipb = 0;
  pointbuffer->push(Points());
  Points& p0 = (*pointbuffer)[0];

  p0.area = area;
  p0.map_proj = map_proj;
  p0.gridboxes = gridboxes;
  p0.npos = npos;
  p0.x = new float[npos];
  p0.y = new float[npos];
  p0.maprect = Rectangle();
  p0.ixmin = p0.ixmax = p0.iymin = p0.iymax = 0;

  const float gdxy = gridboxes ? 0.5 : 0; // offset by half cell iff using gridboxes

  DIUTIL_OPENMP_PARALLEL(npos, for)
  for (int iy = 0; iy < ny; iy++) {
    for (int ix = 0; ix < (nx-1); ix++) {
      int i = ix + iy*nx;
      p0.x[i] = area.fromGridX(ix - gdxy);
      p0.y[i] = area.fromGridY(iy - gdxy);
    }
    int i = iy*nx + nx - 1;
    // FIXME x[i]=nx-1 converts to x[i]=0 when transforming between geo-projections
    p0.x[i] = area.fromGridX(nx - gdxy - 1.1);
    p0.y[i] = area.fromGridY(iy - gdxy);
  }

  const Projection& pa = area.P();
  if (!map_proj.convertPoints(pa, npos, p0.x, p0.y)) {
    pointbuffer->pop();
    return false;
  }

  *x = p0.x;
  *y = p0.y;
  return true;
}

bool GridConverter::getGridPoints(const GridArea& area, const Area& map_area,
    bool gridboxes, float**x, float**y)
{
  int ipb;
  return doGetGridPoints(area, map_area.P(), gridboxes, x, y, ipb);
}

bool GridConverter::getGridPoints(const GridArea& area, const Area& map_area,
    const Rectangle& maprect, bool gridboxes, float**x, float**y,
    int& ix1, int& ix2, int& iy1, int& iy2)
{
  int ipb;
  if (!doGetGridPoints(area, map_area.P(), gridboxes, x, y, ipb))
    return false;

  const bool skiplimits = (maprect.width() == 0 || maprect.height() == 0);

  Points& pi = (*pointbuffer)[ipb];
  if (skiplimits) {
    ix1 = 0;
    ix2 = 0;
    iy1 = 0;
    iy2 = 0;
  } else if (pi.maprect == maprect) {
    ix1 = pi.ixmin;
    ix2 = pi.ixmax;
    iy1 = pi.iymin;
    iy2 = pi.iymax;
  } else {
    findGridLimits(area, maprect, gridboxes, pi.x, pi.y, ix1, ix2, iy1, iy2);
    pi.maprect = maprect;
    pi.ixmin = ix1;
    pi.ixmax = ix2;
    pi.iymin = iy1;
    pi.iymax = iy2;
  }

  return true;
}

// static
void GridConverter::doFindGridLimits(const GridArea& area, const Rectangle& maprect,
    bool gridboxes, const float* x, const float* y, size_t xy_offset,
    int& ix1, int& ix2, int& iy1, int& iy2)
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
      const int idx = xy_offset*(iy * nx + ix);
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
    int idx = xy_offset*iy*nx;
    bool was_inside = maprect.isinside(x[idx], y[idx]);
    int left = was_inside ? 0 : -1;
    int right = -1;
    if (was_inside)
      vcross::util::minimaximize(iy1, iy2, iy);
    IFDEBUG(METLIBS_LOG_DEBUG(LOGVAL(iy) << LOGVAL(was_inside)
            << LOGVAL(left) << LOGVAL(right) << LOGVAL(iy1) << LOGVAL(iy2)));

    for (int ix = 1; ix < nx; ix++) {
      idx += xy_offset;
      const bool inside = maprect.isinside(x[idx], y[idx]);
      if (inside) {
        vcross::util::minimaximize(iy1, iy2, iy);
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
      vcross::util::minimize(ix1, left);
      IFDEBUG(METLIBS_LOG_DEBUG(LOGVAL(ix1)));
    }
    // Expand to the right.
    if (right != -1) {
      vcross::util::maximize(ix2, right);
      IFDEBUG(METLIBS_LOG_DEBUG(LOGVAL(ix2)));
    }
  }

  IFDEBUG(METLIBS_LOG_DEBUG(LOGVAL(ix1) << LOGVAL(ix2) << LOGVAL(iy1) << LOGVAL(iy2)));
  ix1-=2;
  ix2+=2;
  iy1-=2;
  iy2+=2;

  vcross::util::maximize(ix1, 0);
  vcross::util::maximize(iy1, 0);
  vcross::util::minimize(ix2, nx - 1 - gdxy); // FIXME old GridConverter, excludes last column iff gridboxes
  vcross::util::minimize(iy2, ny - 1 - gdxy);
}

// convert set of points
bool GridConverter::getPoints(const Projection& projection, const Projection& map_projection,
    int npos, float* x, float* y) const
{
  return map_projection.convertPoints(projection, npos, x, y, false);
}

// get arrays of vector rotation elements
bool GridConverter::getVectorRotationElements(const Area& data_area,
    const Projection& map_proj, int nvec, const float *x, const float *y,
    float ** cosx, float ** sinx)
{
  if (!anglebuffer)
    setAngleBufferSize(defringsize_angles);

  // search buffer for existing point-set
  const int n = anglebuffer->size();
  for (int i=0; i < n; ++i) {
    const Points& pi = (*anglebuffer)[i];
    if (pi.area == data_area && pi.map_proj == map_proj && pi.npos == nvec) {
      *cosx = pi.x;
      *sinx = pi.y;
      return true;
    }
  }

  // need to calculate new rotation elements
  anglebuffer->push(Points()); // push a new points structure on the ring
  Points& p0 = (*anglebuffer)[0];
  p0.area = GridArea(data_area);
  p0.map_proj = map_proj;
  p0.npos = nvec;
  p0.x = new float[nvec];
  p0.y = new float[nvec];

  if (!map_proj.calculateVectorRotationElements(data_area.P(), nvec, x, y, p0.x, p0.y)) {
    anglebuffer->pop();
    return false;
  }
  *cosx = p0.x;
  *sinx = p0.y;
  return true;
}

// convert u,v vector coordinates for points x,y
bool GridConverter::getVectors(const Area& data_area, const Projection& map_proj,
    int nvec, const float *x, const float *y, float* u, float* v)
{
  float * cosx = 0;
  float * sinx = 0;
  if (!getVectorRotationElements(data_area, map_proj, nvec, x, y, &cosx, &sinx))
    return false;

  DIUTIL_OPENMP_PARALLEL(nvec, for)
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
bool GridConverter::getDirectionVectors(const Area& map_area, const bool turn,
    int nvec, const float *x, const float *y, float *u, float *v)
{
  Rectangle r;
  Area geo_area(Projection::geographic(), r);

  // make vector-components (east/west and north/south) in geographic grid,
  // to be rotated to the map grid
  // u,v is dd,ff coming in
  const float zturn = turn ? -1 : 1;
  DIUTIL_OPENMP_PARALLEL(nvec, for)
  for (int i=0; i<nvec; ++i) {
    if (u[i] != undef && v[i] != undef) {
      float dd   = u[i] * DEG_TO_RAD;
      float ff   = v[i] * zturn;
      u[i] = ff * sinf(dd);
      v[i] = ff * cosf(dd);
    }
  }

  return getVectors(geo_area, map_area.P(), nvec, x, y, u, v);
}

// convert true north direction and velocity (dd=u,ff=v)
// to u,v vector coordinates for one point
// Specific point given by index
bool GridConverter::getDirectionVector(const Area& map_area, const bool turn,
    int nvec, const float *x, const float *y, int index, float & u, float & v)
{
  if (index < 0 || index >= nvec)
    return false;
  return getDirectionVectors(map_area, turn, 1, &x[index], &y[index], &u, &v);
}

bool GridConverter::geo2xy(const Area& area, int npos, float* x, float* y)
{
  return area.P().convertFromGeographic(npos, x, y);
}

bool GridConverter::xy2geo(const Area& area, int npos, float* x, float* y)
{
  return area.P().convertToGeographic(npos, x, y);
}

bool GridConverter::geov2xy(const Area& area, int npos,
    const float* x, const float* y, float *u, float *v)
{
  return area.P().convertVectors(Projection::geographic(), npos, x, y, u, v);
}

bool GridConverter::getMapFields(const GridArea& area,
    const float** xmapr, const float**ymapr, const float** coriolis)
{
  if (!mapfieldsbuffer)
    setBufferSizeMapFields(defringsize_mapfields);

  // search buffer for existing data
  int n = mapfieldsbuffer->size();
  METLIBS_LOG_DEBUG("++ Map.Ringbuffer's size is now: " << n);

  const int npos = area.gridSize();

  int ipbm = -1;
  int i = 0;
  while (i < n && (*mapfieldsbuffer)[i].area != area)
    i++;
  if (i < n)
    ipbm = i;

  MapFields* mf;
  if (ipbm < 0) {
    mapfieldsbuffer->push(MapFields()); // push a new MapFields structure on the ring
    mf = &(*mapfieldsbuffer)[0];
    mf->area = area;
    mf->xmapr = 0;
    mf->ymapr = 0;
    mf->coriolis = 0;
    ipbm = 0;
  } else {
    mf = &(*mapfieldsbuffer)[ipbm];
  }

  if (!mf->xmapr) {
    mf->xmapr = new float[npos];
    mf->ymapr = new float[npos];
    mf->coriolis = new float[npos];
    if (!area.P().getMapRatios(area.nx, area.ny, area.R().x1, area.R().y1, area.resolutionX, area.resolutionY,
            mf->xmapr, mf->ymapr, mf->coriolis))
    {
      METLIBS_LOG_ERROR("getMapRatios problem");
      return false;
    }
  }

  if (xmapr && ymapr) {
    *xmapr = mf->xmapr;
    *ymapr = mf->ymapr;
  }

  if (coriolis)
    *coriolis = mf->coriolis;

  return true;
}
