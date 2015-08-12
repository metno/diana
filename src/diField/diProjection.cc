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

//#define DEBUGPRINT

//#define DEBUG_PROJ

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diProjection.h"

#include <puDatatypes/miCoordinates.h> // for earth radius
#include <puTools/miString.h>

#include <float.h>
#include <cmath>
#include <values.h>

#include <functional>
#include <map>
#include <stdexcept>

#define MILOGGER_CATEGORY "diField.Projection"
#include "miLogger/miLogging.h"

using namespace miutil;

// static
boost::shared_ptr<Projection> Projection::sGeographic;

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
  projObject = boost::shared_ptr<PJ>(pj_init_plus(projDefinition.c_str()), pj_free);
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

int Projection::convertPoints(const Projection& srcProj, int npos, float * x,
    float * y, bool silent) const
{
  METLIBS_LOG_SCOPE();
  if (!srcProj.projObject) {
    METLIBS_LOG_ERROR("src projPJ not initialized, definition=" << srcProj);
    return -1;
  }

  if (!projObject) {
    METLIBS_LOG_ERROR("tgt projPJ not initialized, definition=" << *this);
    return -1;
  }

  double * xd = new double[npos];
  double * yd = new double[npos];
  double * zd = 0;

  for (int i = 0; i < npos; i++) {
    xd[i] = x[i];
    yd[i] = y[i];
  }
  // Transformation
  int ret = pj_transform(srcProj.projObject.get(), projObject.get(), npos, 1, xd, yd, zd);
  if (ret != 0 && ret !=-20) {
    //ret=-20 :"tolerance condition error"
    //ret=-14 : "latitude or longitude exceeded limits"
    if (!silent && ret != -14) {
      METLIBS_LOG_ERROR("error in pj_transform = " << pj_strerrno(ret) << "  " << ret);
    }
    delete[] xd;
    delete[] yd;
    return -1;
  }

  for (int i = 0; i < npos; i++) {
    x[i] = static_cast<float> (xd[i]);
    y[i] = static_cast<float> (yd[i] );
  }

  // End convertPoints - clean up
  delete[] xd;
  delete[] yd;

  return 0;
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
float * north(Projection p, int nvec, const float * x, const float * y)
{
  float* north_x = new float[nvec];
  float* north_y = new float[nvec];
  std::copy(x, x + nvec, north_x);
  std::copy(y, y + nvec, north_y);

  p.convertToGeographic(nvec, north_x, north_y);

  for (int i = 0; i < nvec; i++){
    north_y[i] = std::min<float>(north_y[i] + 0.1, 90);
  }
  p.convertFromGeographic(nvec, north_x, north_y);
  float * ret = new float[nvec];

  for (int i = 0; i < nvec; i++){
    ret[i] = uv(north_x[i] - x[i], north_y[i] - y[i]).angle();
  }
  delete[] north_x;
  delete[] north_y;
  return ret;
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

int Projection::convertVectors(const Projection& srcProj, int nvec,
    const float * to_x,  const float * to_y, float * u, float * v) const
{
  int ierror = 0;
  float udef= +1.e+35;

  float * from_x = new float[nvec];
  float * from_y = new float[nvec];

  std::copy(to_x, to_x + nvec, from_x);
  std::copy(to_y, to_y + nvec, from_y);

  try {
    srcProj.convertPoints(*this, nvec, from_x, from_y); // convert back to old projection
    float * from_north = north(srcProj, nvec, from_x, from_y); // degrees
    float * to_north = north(*this, nvec, to_x, to_y); // degrees

    for (int i = 0; i < nvec; ++i) {
      if(u[i] != udef && v[i] != udef ) {
        const float length = std::sqrt(u[i]*u[i] + v[i]*v[i]);

        // the difference between angles in the two projections:
        float angle_diff = to_north[i] - from_north[i];

        float new_direction = turn(uv(u[i], v[i]).angle(), angle_diff);
        // float new_direction = to_north[ i ]; // This makes all directions be north.
        uv convert(new_direction);
        u[i] = convert.u * length;
        v[i] = convert.v * length;
      }
    }
    delete[] from_north;
    delete[] to_north;
  } catch (std::exception & e) {
    METLIBS_LOG_ERROR("exception in convertVectors:" << e.what());
    ierror = 1;
  }

  delete[] from_x;
  delete[] from_y;

  return ierror;
}

int Projection::calculateVectorRotationElements(const Projection& srcProj,
    int nvec, const float * to_x, const float * to_y, float * cosa,
    float * sina) const
{
  int err = 0;

  float * from_x = new float[nvec];
  float * from_y = new float[nvec];

  std::copy(to_x, to_x + nvec, from_x);
  std::copy(to_y, to_y + nvec, from_y);

  err = srcProj.convertPoints(*this, nvec, from_x, from_y); // convert back to old projection
  if(err!=0){
    return err;
  }
  float * from_north = north(srcProj, nvec, from_x, from_y); // degrees
  float * to_north = north(*this, nvec, to_x, to_y); // degrees

  for (int i = 0; i < nvec; ++i) {
    // the difference between angles in the two projections:
    if (from_north[i] == HUGE_VAL || to_north[i] == HUGE_VAL) {
      cosa[i] = HUGE_VAL;
      sina[i] = HUGE_VAL;
    } else {
      double angle_diff = from_north[i] - to_north[i];
      // return cos() and sin() of this angle
      cosa[i] = std::cos(angle_diff * DEG_TO_RAD);
      sina[i] = std::sin(angle_diff * DEG_TO_RAD);
    }
  }
  delete[] from_north;
  delete[] to_north;
  delete[] from_x;
  delete[] from_y;

  return err;
}

bool Projection::isLegal(float lon, float lat) const
{
  float px = lon;
  float py = lat;
  int ierror = convertToGeographic(1, &px, &py);

  const float maxval = 1000000;
  return (ierror == 0 && fabsf(px) < maxval && fabsf(py) < maxval);
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
  int ierror = convertFromGeographic(2, px, py);

  if (ierror) {
    METLIBS_LOG_ERROR("getMapLinesJumpLimit:" << ierror);
  } else {
    float dx = px[1] - px[0];
    float dy = py[1] - py[0];
    jumplimit = sqrtf(dx * dx + dy * dy) / 4;
  }
  return jumplimit;
}

int Projection::convertToGeographic(int n, float* x, float* y) const
{
  int ierror = geographic().convertPoints(*this, n, x, y);
  for (int i = 0; i < n; i++) {
    if (x[i] != HUGE_VAL)
      x[i] *= RAD_TO_DEG;
    if (y[i] != HUGE_VAL)
      y[i] *= RAD_TO_DEG;
  }
  return ierror;
}

int Projection::convertFromGeographic(int npos, float* x, float* y) const
{
  for (int i = 0; i < npos; i++) {
    if (x[i] == 0)
      x[i] = 0.01; //todo: Bug:  x[i]=0 converts to x[i]=nx-1 when transforming between geo-projections
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

  int n, i, j;
  int nt = 9;
  float *tx = new float[nt * nt];
  float *ty = new float[nt * nt];
  float dx = (maprect.x2 - maprect.x1) / float(nt - 1);
  float dy = (maprect.y2 - maprect.y1) / float(nt - 1);

  n = 0;
  for (j = 0; j < nt; j++) {
    for (i = 0; i < nt; i++) {
      tx[n] = maprect.x1 + dx * i;
      ty[n] = maprect.y1 + dy * j;
      n++;
    }
  }

  int ierror = convertToGeographic(n, tx, ty);

  if (ierror) {
    METLIBS_LOG_ERROR("calculateLatLonBoundingBox: " << ierror);
    delete[] tx;
    delete[] ty;
    return false;
  }

  for (i = 0; i < n; i++) {
    if (lonmin > tx[i])
      lonmin = tx[i];
    if (lonmax < tx[i])
      lonmax = tx[i];
    if (latmin > ty[i])
      latmin = ty[i];
    if (latmax < ty[i])
      latmax = ty[i];
  }

  delete[] tx;
  delete[] ty;
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

int Projection::getMapRatios(int nx, int ny, float gridResolutionX, float gridResolutionY,
    float* xmapr, float* ymapr, float* coriolis) const
{
  const int npos = nx * ny;
  float *x = new float[npos];
  float *y = new float[npos];
  int i = 0;
  for (int iy = 0; iy < ny; iy++) {
    for (int ix = 0; ix < nx; ix++) {
      x[i] = ix*gridResolutionX;
      y[i] = iy*gridResolutionY;
      i++;
    }
  }

  int ierror = convertToGeographic(npos, x, y);
  if (ierror < 0) {
    return ierror;
  }

  //HACK: in geographic projection wrapping the world x[0]=x[nx-1]=180
  //must set x[0]=-180 in order to get map ratios right
  for (int iy = 0; iy < ny; iy++) {
    if(x[nx*iy] > 179 && x[nx*iy] < 181) {
      x[nx*iy] = -180;
    }
  }

  if (coriolis) {
    for (int j = 0; j < npos; ++j) {
      coriolis[j] = 2.0 * 0.7292e-4 * sin(y[j] * DEG_TO_RAD);
    }
  }

  if (!(xmapr && ymapr))
    return ierror;

  i = 0;
  for (int iy = 0; iy < ny; iy++) {
    for (int ix = 0; ix < nx; ix++, i++) {
      if (iy == ny - 1) {
        ymapr[i] = ymapr[i - nx];
      } else {
        float dy = ((y[i + nx] - y[i]) * EARTH_RADIUS_M * DEG_TO_RAD);
        float dx = ((x[i + nx] - x[i]) * EARTH_RADIUS_M * DEG_TO_RAD) * cos(y[i] * DEG_TO_RAD);
        float dd = sqrt(pow(dy, 2) + pow(dx, 2));
        ymapr[i] = 1 / dd;
      }
      if (ix == nx - 1) {
        xmapr[i] = xmapr[i - 1];
      } else {
        float dy = ((y[i + 1] - y[i]) * EARTH_RADIUS_M * DEG_TO_RAD);
        float dx = ((x[i + 1] - x[i]) * EARTH_RADIUS_M * DEG_TO_RAD) * cos(y[i] * DEG_TO_RAD);
        float dd = sqrt(pow(dy, 2) + pow(dx, 2));
        xmapr[i] = 1 / dd;
      }
    }
  }
  return ierror;
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
  dlon = RAD_TO_DEG / EARTH_RADIUS_M / cos(lat * DEG_TO_RAD);
  return true;
}

// static
const Projection& Projection::geographic()
{
  if (!sGeographic)
    sGeographic = boost::shared_ptr<Projection>(new Projection("+proj=longlat  +ellps=WGS84 +towgs84=0,0,0 +no_defs"));
  return *sGeographic;
}
