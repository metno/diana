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

#include "diProjection.h"

#include "../util/math_util.h"
#include "../util/openmp_tools.h"

#include <puDatatypes/miCoordinates.h> // for earth radius
#include <puTools/miString.h>

#include <float.h>
#include <cmath>
#include <values.h>

#include <functional>
#include <map>
#include <memory>
#include <stdexcept>

#define MILOGGER_CATEGORY "diField.Projection"
#include "miLogger/miLogging.h"

using namespace miutil;

// static
std::shared_ptr<Projection> Projection::sGeographic;

Projection::Projection()
{
}

Projection::Projection(const std::string& projStr)
{
  set_proj_definition(projStr);
}

bool Projection::set_proj_definition(const std::string& projStr)
{
  projDefinition = projStr;

  miutil::replace(projDefinition, "\"", "");
  projObject = std::shared_ptr<PJ>(pj_init_plus(projDefinition.c_str()), pj_free);
  if (!projObject)
    METLIBS_LOG_WARN("proj4 init error for '" << projDefinition << "': " << pj_strerrno(pj_errno));

  return (projObject != 0);
}

std::string Projection::getProjDefinitionExpanded() const
{
  static const std::string EMPTY;
  if (!projObject)
    return EMPTY;
  // get expanded definition, ie values from "+init=..." are included
  return pj_get_def(projObject.get(), 0);
}

Projection::~Projection()
{
}

bool Projection::operator==(const Projection &rhs) const
{
  return (projDefinition == rhs.projDefinition);
}

bool Projection::operator!=(const Projection &rhs) const
{
  return !(*this == rhs);
}

std::ostream& operator<<(std::ostream& output, const Projection& p)
{
  output << " proj4string=\"" << p.projDefinition<<"\"";
  return output;
}

bool Projection::convertPoints(const Projection& srcProj, size_t npos, float * x,
    float * y, bool silent) const
{
  if (!areDefined(srcProj, *this))
    return false;

  // use stack when converting a small number of points
  double *xd, *yd;
  const int N = 32;
  double xdN[N], ydN[N];
  std::unique_ptr<double[]> xdU, ydU;
  if (npos < N) {
    xd = xdN;
    yd = ydN;
  } else {
    xdU.reset(new double[npos]);
    ydU.reset(new double[npos]);
    xd = xdU.get();
    yd = ydU.get();
  }

  DIUTIL_OPENMP_PARALLEL(npos, for)
  for (size_t i = 0; i < npos; i++) {
    xd[i] = x[i];
    yd[i] = y[i];
  }

  if (!transformAndCheck(srcProj, npos, 1, xd, yd, silent))
    return false;

  DIUTIL_OPENMP_PARALLEL(npos, for)
  for (size_t i = 0; i < npos; i++) {
    x[i] = static_cast<float>(xd[i]);
    y[i] = static_cast<float>(yd[i]);
  }

  return true;
}

bool Projection::convertPoints(const Projection& srcProj, size_t npos, double* xd,
    double* yd, bool silent) const
{
  if (!areDefined(srcProj, *this))
    return false;

  return transformAndCheck(srcProj, npos, 1, xd, yd, silent);
}

bool Projection::convertPoints(const Projection& srcProj, size_t npos, diutil::PointF* xy, bool silent) const
{
  if (!areDefined(srcProj, *this))
    return false;

  // use stack when converting a small number of points
  double *xd, *yd;
  const int N = 32;
  double xdN[N], ydN[N];
  std::unique_ptr<double[]> xdU, ydU;
  if (npos < N) {
    xd = xdN;
    yd = ydN;
  } else {
    xdU.reset(new double[npos]);
    ydU.reset(new double[npos]);
    xd = xdU.get();
    yd = ydU.get();
  }

  DIUTIL_OPENMP_PARALLEL(npos, for)
  for (size_t i = 0; i < npos; i++) {
    xd[i] = xy[i].x();
    yd[i] = xy[i].y();
  }

  if (!transformAndCheck(srcProj, npos, 1, xd, yd, silent))
    return false;

  DIUTIL_OPENMP_PARALLEL(npos, for)
  for (size_t i = 0; i < npos; i++) {
    xy[i] = diutil::PointF(xd[i], yd[i]);
  }

  return true;
}

bool Projection::convertPoints(const Projection& srcProj, size_t npos, diutil::PointD* xy, bool silent) const
{
  if (!areDefined(srcProj, *this))
    return false;

  static_assert(sizeof(diutil::PointD) == 2*sizeof(double), "sizeof PointD");
  double* xd = reinterpret_cast<double*>(xy);
  double* yd = xd + 1;
  assert(reinterpret_cast<double*>(&xy[1]) == (xd + 2));
  assert(yd[0] == xy[0].y());
  return transformAndCheck(srcProj, npos, 2, xd, yd, silent);
}

bool Projection::transformAndCheck(const Projection& src, size_t npos, size_t offset, double* x, double* y, bool silent) const
{
  // actual transformation -- here we spend most of the time when large matrixes.
  double* z = 0;
  const int ret = pj_transform(src.projObject.get(), projObject.get(), npos, offset, x, y, z);
  if (ret != 0 && ret !=-20) {
    //ret=-20 :"tolerance condition error"
    //ret=-14 : "latitude or longitude exceeded limits"
    if (!silent && ret != -14) {
      METLIBS_LOG_ERROR("error in pj_transform = " << pj_strerrno(ret) << "  " << ret);
    }
    return false;
  }
  return true;
}

// static
bool Projection::areDefined(const Projection& srcProj, const Projection& tgtProj)
{
  if (!srcProj.isDefined()) {
    METLIBS_LOG_ERROR("src projPJ not initialized, definition=" << srcProj);
    return false;
  }

  if (!tgtProj.isDefined()) {
    METLIBS_LOG_ERROR("tgt projPJ not initialized, definition=" << tgtProj);
    return false;
  }

  return true;
}

// Support Functions for convertVector
namespace {

struct uv {
  float u;
  float v;

  uv(float u_, float v_) :
    u(u_), v(v_)
  {
  }

  explicit uv(float angle) :
    u(std::sin(angle * DEG_TO_RAD)), v(std::cos(angle * DEG_TO_RAD))
  {
  }

  float angle() const
  {
    if (0 == u and 0 == v) {
      return HUGE_VAL;
    }

    if (0 == v) {
      if (0 < u)
        return 90;
      else
        return 270;
    }

    float ret = std::atan(u / v) * RAD_TO_DEG;
    if (v < 0)
      ret += 180;

    if (ret < 0)
      ret += 360;
    if (360 < ret)
      ret -= 360;

    return ret;
  }
};

/// Get the direction to north from point x, y
float * north(Projection p, size_t nvec, const float * x, const float * y)
{
  std::unique_ptr<float[]> north_x(new float[nvec]), north_y(new float[nvec]);
  std::copy(x, x + nvec, north_x.get());
  std::copy(y, y + nvec, north_y.get());

  p.convertToGeographic(nvec, north_x.get(), north_y.get());

  // convert to geographical, go a little north (lat += 0.1 deg), convert back
  const float deltaLat = 0.1f;

  DIUTIL_OPENMP_PARALLEL(nvec, for)
  for (size_t i = 0; i < nvec; i++) {
    north_y[i] = std::min<float>(north_y[i] + deltaLat, 90);
  }
  p.convertFromGeographic(nvec, north_x.get(), north_y.get());

  std::unique_ptr<float[]> ret(new float[nvec]);
  DIUTIL_OPENMP_PARALLEL(nvec, for)
  for (size_t i = 0; i < nvec; i++){
    ret[i] = uv(north_x[i] - x[i], north_y[i] - y[i]).angle();
  }
  return ret.release();
}

float turn(float angle_a, float angle_b)
{
  float angle = angle_a + angle_b;
  if (angle >= 360)
    angle -= 360;
  if (angle < 0)
    angle += 360;
  return angle;
}

} // namespace

bool Projection::convertVectors(const Projection& srcProj, size_t nvec,
    const float * from_x, const float * from_y,
    const float * to_x, const float * to_y,
    float * u, float * v) const
{
  const float udef = +1.e+35f;

  try {
    std::unique_ptr<float[]> from_north(north(srcProj, nvec, from_x, from_y)); // degrees
    std::unique_ptr<float[]> to_north(north(*this, nvec, to_x, to_y)); // degrees

    DIUTIL_OPENMP_PARALLEL(nvec, for)
    for (size_t i = 0; i < nvec; ++i) {
      if(u[i] != udef && v[i] != udef) {
        const float length = diutil::absval(u[i], v[i]);

        // the difference between angles in the two projections:
        float angle_diff = to_north[i] - from_north[i];

        float new_direction = turn(uv(u[i], v[i]).angle(), angle_diff);
        // float new_direction = to_north[ i ]; // This makes all directions be north.
        uv convert(new_direction);
        u[i] = convert.u * length;
        v[i] = convert.v * length;
      }
    }
    return true;
  } catch (std::exception & e) {
    METLIBS_LOG_ERROR("exception in convertVectors:" << e.what());
    return false;
  }
}

void Projection::calculateVectorRotationElements(const Projection& srcProj, int nvec,
    const float * from_x, const float * from_y,
    const float * to_x, const float * to_y,
    float * cosa, float * sina) const
{
  std::unique_ptr<float[]> from_north(north(srcProj, nvec, from_x, from_y)); // degrees
  std::unique_ptr<float[]> to_north  (north(*this,   nvec, to_x,   to_y)); // degrees

  DIUTIL_OPENMP_PARALLEL(nvec, for)
  for (int i = 0; i < nvec; ++i) {
    // the difference between angles in the two projections:
    if (from_north[i] == HUGE_VAL || to_north[i] == HUGE_VAL) {
      cosa[i] = HUGE_VAL;
      sina[i] = HUGE_VAL;
    } else {
      double angle_diff_rad = (from_north[i] - to_north[i])*DEG_TO_RAD;
      // return cos() and sin() of this angle
      cosa[i] = std::cos(angle_diff_rad);
      sina[i] = std::sin(angle_diff_rad);
    }
  }
}

bool Projection::convertVectors(const Projection& srcProj, int nvec,
    const float * to_x,  const float * to_y, float * u, float * v) const
{
  std::unique_ptr<float[]> from_x(new float[nvec]), from_y(new float[nvec]);
  std::copy(to_x, to_x + nvec, from_x.get());
  std::copy(to_y, to_y + nvec, from_y.get());
  srcProj.convertPoints(*this, nvec, from_x.get(), from_y.get()); // convert back to old projection

  return convertVectors(srcProj, nvec, from_x.get(), from_y.get(), to_x, to_y, u, v);
}

bool Projection::calculateVectorRotationElements(const Projection& srcProj, int nvec,
    const float * to_x, const float * to_y,
    float * cosa, float * sina) const
{
  std::unique_ptr<float[]> from_x(new float[nvec]), from_y(new float[nvec]);
  std::copy(to_x, to_x + nvec, from_x.get());
  std::copy(to_y, to_y + nvec, from_y.get());

  if (!srcProj.convertPoints(*this, nvec, from_x.get(), from_y.get())) // convert back to old projection
    return false;

  calculateVectorRotationElements(srcProj, nvec,
        from_x.get(), from_y.get(), to_x, to_y, cosa, sina);
  return true;
}

bool Projection::isLegal(float lon, float lat) const
{
  float px = lon;
  float py = lat;
  const bool ok = convertToGeographic(1, &px, &py);

  const float maxval = 1000000;
  return (ok && fabsf(px) < maxval && fabsf(py) < maxval);
}

bool Projection::isAlmostEqual(const Projection& p) const
{
  std::vector<std::string> thisProjectionParts = miutil::split(projDefinition, 0, " ");
  std::vector<std::string> pProjectionParts = miutil::split(p.getProjDefinition(), 0, " ");

  std::map<std::string, std::string> partMap;

  //Loop through this proj string, put key and value in map (except x_0 and y_0)
  for( size_t i=0; i<thisProjectionParts.size(); ++i ) {
    std::vector<std::string> token = miutil::split(thisProjectionParts[i], "=");
    if ( token.size() == 2 && token[0] != "+x_0" && token[0] != "+y_0" ){
      partMap[token[0]] = token[1];
    }
  }

  //Loop through the proj string of p, compare key and value, remove equal parts
  for( size_t i=0; i<pProjectionParts.size(); ++i ) {
    std::vector<std::string> token = miutil::split(pProjectionParts[i], "=");
    if ( token.size() == 2 && token[0] != "+x_0" && token[0] != "+y_0" ){
      std::map<std::string,std::string>::iterator it =partMap.find(token[0]);
      if ( it == partMap.end() || partMap[token[0]] != token[1] ) {
        return false;
      }
      partMap.erase(it);
    }
  }

  //if all parts, except x_0 and y_0, are equal, return true
  if ( partMap.size() ==0 ) {
    return true;
  }

  //Not all parts are equal
  return false;
}

float Projection::getMapLinesJumpLimit() const
{
  float jumplimit = 100000000;
  // some projections do not benefit from using jumplimits
  if (miutil::contains(projDefinition, "+proj=stere"))
    return jumplimit;

  // find position of two geographic positions
  float px[2] = { 0.0, 0.0 };
  float py[2] = { 45.0, -45.0 };

  if (!convertFromGeographic(2, px, py))
    METLIBS_LOG_ERROR("getMapLinesJumpLimit");

  float dx = px[1] - px[0];
  float dy = py[1] - py[0];
  return diutil::absval(dx , dy) / 4;
}

bool Projection::convertToGeographic(int n, float* x, float* y) const
{
  const bool ok = geographic().convertPoints(*this, n, x, y);

  DIUTIL_OPENMP_PARALLEL(n, for)
  for (int i = 0; i < n; i++) {
    if (x[i] != HUGE_VAL)
      x[i] *= RAD_TO_DEG;
    if (y[i] != HUGE_VAL)
      y[i] *= RAD_TO_DEG;
  }
  return ok ;
}

bool Projection::convertFromGeographic(int npos, float* x, float* y) const
{
  DIUTIL_OPENMP_PARALLEL(npos, for)
  for (int i = 0; i < npos; i++) {
    if (x[i] == 0)
      x[i] = 0.01f; //todo: Bug:  x[i]=0 converts to x[i]=nx-1 when transforming between geo-projections
    x[i] *=  DEG_TO_RAD;
    y[i] *=  DEG_TO_RAD;
  }
  return convertPoints(geographic(), npos, x, y);
}

void Projection::setDefault()
{
  set_proj_definition("+proj=ob_tran +o_proj=longlat +lon_0=0 +o_lat_p=25 +x_0=0.811578 +y_0=0.637045 +ellps=WGS84 +towgs84=0,0,0 +no_defs");
}

bool Projection::calculateLatLonBoundingBox(const Rectangle & maprect,
    float & lonmin, float & lonmax, float & latmin, float & latmax) const
{
  // find (approx.) geographic area on map

  lonmin = FLT_MAX;
  lonmax = -FLT_MAX;
  latmin = FLT_MAX;
  latmax = -FLT_MAX;

  const size_t nt = 9, ntnt = nt*nt;
  std::unique_ptr<float[]> tx(new float[ntnt]), ty(new float[ntnt]);
  float dx = (maprect.x2 - maprect.x1) / float(nt - 1);
  float dy = (maprect.y2 - maprect.y1) / float(nt - 1);

  DIUTIL_OPENMP_PARALLEL(ntnt, for)
  for (size_t j = 0; j < nt; j++) {
    for (size_t i = 0; i < nt; i++) {
      size_t n = i + j*nt;
      tx[n] = maprect.x1 + dx * i;
      ty[n] = maprect.y1 + dy * j;
    }
  }

  if (!convertToGeographic(ntnt, tx.get(), ty.get())) {
    METLIBS_LOG_ERROR("calculateLatLonBoundingBox");
    return false;
  }

  for (size_t i = 0; i < ntnt; i++) {
    if (lonmin > tx[i])
      lonmin = tx[i];
    if (lonmax < tx[i])
      lonmax = tx[i];
    if (latmin > ty[i])
      latmin = ty[i];
    if (latmax < ty[i])
      latmax = ty[i];
  }

  return true;
}

bool Projection::adjustedLatLonBoundingBox(const Rectangle & maprect,
    float & lonmin, float & lonmax, float & latmin, float & latmax) const
{
  // when UTM: keep inside a strict lat/lon-rectangle
  if (miutil::contains(projDefinition, "+proj=utm")) {
    if (!calculateLatLonBoundingBox(maprect, lonmin, lonmax, latmin, latmax)) {
      return false;
    }
    return true;
  }

  lonmin = -180;
  lonmax = 180;
  latmin = -90;
  latmax = 90;

  return true;
}

bool Projection::getMapRatios(int nx, int ny, float x0, float y0, float gridResolutionX, float gridResolutionY,
    float* xmapr, float* ymapr, float* coriolis) const
{
  if (nx < 2 || ny < 2)
    return false;

  const int npos = nx * ny;
  std::unique_ptr<float[]> x(new float[npos]), y(new float[npos]);

  DIUTIL_OPENMP_PARALLEL(npos, for)
  for (int iy = 0; iy < ny; iy++) {
    for (int ix = 0; ix < nx; ix++) {
      size_t i = ix + iy*nx;
      x[i] = x0 + ix*gridResolutionX;
      y[i] = y0 + iy*gridResolutionY;
    }
  }

  if (!convertToGeographic(npos, x.get(), y.get()))
    return false;

  //HACK: in geographic projection wrapping the world x[0]=x[nx-1]=180
  //must set x[0]=-180 in order to get map ratios right
  DIUTIL_OPENMP_PARALLEL(ny, for)
  for (int iy = 0; iy < ny; iy++) {
    size_t index = nx*iy;
    if(x[index] > 179 && x[index] < 181) {
      x[index] = -180;
    }
  }

  if (coriolis) {
    const float EARTH_OMEGA = 0.7292e-4f; // rad/s, 2*pi / (23*3600 + 56*60 + 4.1), see https://en.wikipedia.org/wiki/Coriolis_frequency
    const float cfactor = 2.0f * EARTH_OMEGA;
    DIUTIL_OPENMP_PARALLEL(npos, for)
    for (int j = 0; j < npos; ++j) {
      coriolis[j] = cfactor * sin(y[j] * DEG_TO_RAD);
    }
  }

  if (!(xmapr && ymapr))
    return false;

  DIUTIL_OPENMP_PARALLEL(nx, for)
  for (int ix = 0; ix < nx; ix++) {
    int i = ix;
    for (int iy = 0; iy < ny-1; iy++, i += nx)
      ymapr[i] = 1 / diutil::GreatCircleDistance(y[i + nx], y[i], x[i + nx], x[i]);
    ymapr[i] = ymapr[i - nx];
  }
  DIUTIL_OPENMP_PARALLEL(ny, for)
  for (int iy = 0; iy < ny; iy++) {
    int i = iy*nx;
    for (int ix = 0; ix < nx-1; ix++, i += 1)
      xmapr[i] = 1 / diutil::GreatCircleDistance(y[i + 1], y[i], x[i + 1], x[i]);
    xmapr[i] = xmapr[i - 1];
  }
  return true;
}

bool Projection::isGeographic() const
{
  return (miutil::contains(projDefinition, "+proj=eqc")
      || miutil::contains(projDefinition, "+proj=longlat")
      || miutil::contains(projDefinition, "+proj=ob_tran +o_proj=longlat +lon_0=0 +o_lat_p=90")
      || pj_is_latlong(projObject.get()));
}

// static
bool Projection::getLatLonIncrement(float lat, float /*lon*/, float& dlat, float& dlon)
{
  dlat = RAD_TO_DEG / EARTH_RADIUS_M;
  dlon = dlat / cos(lat * DEG_TO_RAD);
  return true;
}

// static
const Projection& Projection::geographic()
{
  if (!sGeographic)
    sGeographic = std::shared_ptr<Projection>(new Projection("+proj=longlat  +ellps=WGS84 +towgs84=0,0,0 +no_defs"));
  return *sGeographic;
}
