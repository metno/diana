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

#include "diProjection.h"
#include "util/geo_util.h"
#include "util/lru_cache.h"
#include "util/pair_hash.h"
#include "util/string_util.h"
#include "util/subprocess.h"

#include <mi_fieldcalc/math_util.h>
#include <mi_fieldcalc/openmp_tools.h>

#include <puDatatypes/miCoordinates.h> // for earth radius
#include <puTools/miString.h>

#include <QRegExp>

#include "diana_config.h"
#ifdef HAVE_PROJ_H
#include "proj.h"
#else //! HAVE_PROJ_H
#ifndef ACCEPT_USE_OF_DEPRECATED_PROJ_API_H
#define ACCEPT_USE_OF_DEPRECATED_PROJ_API_H
#endif
#include <proj_api.h>
#if !defined(PROJECTS_H)
typedef void PJ;
#endif
#endif //! HAVE_PROJ_H

#include <cassert>
#include <cfloat>
#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <stdexcept>

#define MILOGGER_CATEGORY "diField.Projection"
#include "miLogger/miLogging.h"

using namespace miutil;

namespace {
#ifdef HAVE_PROJ_H
const float DEG_TO_RAD = M_PI / 180;
const float RAD_TO_DEG = 180 / M_PI;

PJ_CONTEXT* ctx = nullptr;
void milogger_log_func(void*, int level, const char* msg)
{
  ::milogger::LoggerTag proj_logger(MILOGGER_CATEGORY ".proj");

  milogger::Severity severity;
  switch (level) {
  case PJ_LOG_ERROR:
    severity = milogger::ERROR;
    break;
  case PJ_LOG_DEBUG:
    severity = milogger::DEBUG;
    break;
  case PJ_LOG_TRACE:
    severity = milogger::VERBOSE;
    break;
  default:
    return;
  }

  if (auto record = proj_logger.createRecord(severity)) {
    record->stream() << msg;
    proj_logger.submitRecord(record);
  }
}
#endif // HAVE_PROJ_H

bool extractParameterFromWKT(const QString& wkt, const QString& key, double& value)
{
  QRegExp r_parameter("PARAMETER\\[\"" + key + "\", *([0-9.Ee+-]+)\\]");
  bool ok = false;
  if (r_parameter.indexIn(wkt) >= 0)
    value = r_parameter.cap(1).toDouble(&ok);
  return ok;
}

bool extractProj4ParameterFromWKT(QString& proj4, const QString& wkt, const QString& wktkey, const QString& proj4key, double dflt)
{
  double value;
  if (!extractParameterFromWKT(wkt, wktkey, value))
    return false;

  if (value != dflt)
    proj4 += QString(" +%1=%2").arg(proj4key).arg(value, 0, 'f');
  return true;
}

bool extractProj4ParameterFromWKT(QString& proj4, const QString& wkt, const QString& wktkey, const QString& proj4key)
{
  return extractProj4ParameterFromWKT(proj4, wkt, wktkey, proj4key, std::nan(""));
}

bool setProjectionViaGdalSRSInfo(Projection& p, QString wkt)
{
  const QString PROJECTION_LCC     = "PROJECTION[\"Lambert_Conformal_Conic\"]";
  const QString PROJECTION_LCC_2SP = "PROJECTION[\"Lambert_Conformal_Conic_2SP\"]";
  if (wkt.contains(PROJECTION_LCC) && wkt.contains("PARAMETER[\"standard_parallel_1\",") && wkt.contains("PARAMETER[\"standard_parallel_2\",")) {
    wkt.replace(PROJECTION_LCC, PROJECTION_LCC_2SP);
    METLIBS_LOG_INFO("trying to fix wkt for gdalsrsinfo, new wkt='" << wkt.toStdString() << "'");
  }

  QByteArray gdalStdOut;
  if (diutil::execute("gdalsrsinfo",
                      QStringList() << "-o"
                                    << "proj4" << wkt,
                      &gdalStdOut) != 0)
    return false;

  QString proj4(gdalStdOut);

  // gdalsrsinfo puts its proj4 output in quotes and adds some whitespace
  proj4 = proj4.trimmed();
  if (proj4.startsWith("'"))
    proj4 = proj4.mid(1);
  if (proj4.endsWith("'"))
    proj4 = proj4.mid(0, proj4.length() - 1);
  proj4 = proj4.trimmed();

  if (proj4.isEmpty())
    return false;
  METLIBS_LOG_DEBUG("gdalsrsinfo wkt='" << wkt.toStdString() << "', proj4='" << proj4.toStdString() << "'");
  return p.setProj4Definition(proj4.toStdString());
}

bool guessProjectionFromWKT(Projection& p, const QString& wkt)
{
  QString proj4;

  const QRegExp r_utm("\"WGS_1984_UTM_Zone_(\\d+).\"");
  if (r_utm.indexIn(wkt, 0) != -1) {
    proj4 = "+proj=utm +zone=" + r_utm.cap(1) + " +ellps=WGS84 +datum=WGS84 +units=m";

  } else if (wkt.contains("PROJECTION[\"Lambert_Conformal_Conic\"]")) {
    proj4 = QString("+proj=lcc");
    if (!extractProj4ParameterFromWKT(proj4, wkt, "standard_parallel_1", "lat_1"))
      return false;
    extractProj4ParameterFromWKT(proj4, wkt, "standard_parallel_2", "lat_2");
    extractProj4ParameterFromWKT(proj4, wkt, "central_meridian", "lon_0", 0);
    extractProj4ParameterFromWKT(proj4, wkt, "latitude_of_origin", "lat_0", 0);
    extractProj4ParameterFromWKT(proj4, wkt, "false_easting", "x_0", 0);
    extractProj4ParameterFromWKT(proj4, wkt, "false_northing", "y_0", 0);
    if (wkt.contains("GEOGCS[\"GCS_WGS_1984\"")) {
      proj4 += " +ellps=WGS84";
    } else {
      return false;
    }
    if (wkt.contains("UNIT[\"Meter\",1]")) {
      proj4 += " +units=m";
    } else {
      return false;
    }

  } else if (wkt.contains("PROJECTION[\"Geostationary_Satellite\"]")) {
    proj4 = QString("+proj=geos");
    if (!extractProj4ParameterFromWKT(proj4, wkt, "satellite_height", "h"))
      return false;
    extractProj4ParameterFromWKT(proj4, wkt, "central_meridian", "lon_0", 0);
    extractProj4ParameterFromWKT(proj4, wkt, "false_easting", "x_0", 0);
    extractProj4ParameterFromWKT(proj4, wkt, "false_northing", "y_0", 0);
  }

  if (!proj4.isEmpty()) {
    METLIBS_LOG_INFO("guessing that wkt='" << wkt.toStdString() << "' translates to proj4='" << proj4.toStdString() << "'");
    return p.setProj4Definition(proj4.toStdString());
  }

  if (wkt.startsWith("GEOGCS[\"GCS_WGS_1984\"")) {
    METLIBS_LOG_INFO("wild guessing geographic from wkt '" << wkt.toStdString() << "'");
    p = Projection::geographic();
    return true;
  }

  return false;
}

lru_cache<std::pair<std::string, std::string>, Transformation_cp, diutil::pair_hash<std::string, std::string>> transformations(20);

typedef std::shared_ptr<PJ> PJ_p;

#ifdef HAVE_PROJ_H

PJ_p wrap_PJ(PJ* pj)
{
  return PJ_p(pj, proj_destroy);
}

PJ_p norm_PJ(PJ_p pj)
{
  if (pj) {
    if (auto pn = wrap_PJ(proj_normalize_for_visualization(ctx, pj.get())))
      return pn;
  }
  return pj;
}

PJ_p make_PJ(const std::string& def)
{
  return wrap_PJ(proj_create(ctx, def.c_str()));
}

PJ_p make_PJ(const std::string& src, const std::string& dst)
{
  return norm_PJ(wrap_PJ(proj_create_crs_to_crs(ctx, src.c_str(), dst.c_str(), nullptr)));
}

class Proj6Transformation : public Transformation
{
public:
  Proj6Transformation(const std::string& src_def, const std::string& dst_def);

protected:
  bool defined() const override;
  bool transformAndCheck(bool fwd, size_t npos, size_t offset, double* x, double* y) const override;

private:
  PJ_p tf;
};

Proj6Transformation::Proj6Transformation(const std::string& src_def, const std::string& dst_def)
    : tf(make_PJ(src_def, dst_def))
{
  if (!tf)
    METLIBS_LOG_ERROR("could not create transformation from '" << src_def << "' to '" << dst_def << "'");
}

bool Proj6Transformation::defined() const
{
  return tf != nullptr;
}

bool Proj6Transformation::transformAndCheck(bool fwd, size_t npos, size_t offset, double* x, double* y) const
{
  if (!defined())
    return false;

  // actual transformation -- here we spend most of the time when large matrixes.
  const size_t stride = offset * sizeof(double);
  if (npos != proj_trans_generic(tf.get(), fwd ? PJ_FWD : PJ_INV, x, stride, npos, y, stride, npos, nullptr, 0, 0, nullptr, 0, 0)) {
    METLIBS_LOG_ERROR("error in proj_trans = " << proj_errno_string(proj_context_errno(ctx)));
    return false;
  } else {
    return true;
  }
}

#else  // !HAVE_PROJ_H

class Proj4Transformation : public Transformation
{
public:
  Proj4Transformation(PJ_p src, PJ_p dst, bool src_degree, bool dst_degree);

protected:
  bool defined() const override;
  bool transformAndCheck(bool fwd, size_t npos, size_t offset, double* x, double* y) const override;

private:
  PJ_p src;
  PJ_p dst;
  bool src_degree;
  bool dst_degree;
};

Proj4Transformation::Proj4Transformation(PJ_p src__, PJ_p dst_, bool src_degree_, bool dst_degree_)
    : src(src__)
    , dst(dst_)
    , src_degree(src_degree_)
    , dst_degree(dst_degree_)
{
}

bool Proj4Transformation::defined() const
{
  return src && dst;
}

bool Proj4Transformation::transformAndCheck(bool fwd, size_t npos, size_t offset, double* x, double* y) const
{
  if (!defined())
    return false;

  // actual transformation -- here we spend most of the time when large matrixes.
  if ((fwd && src_degree) || (!fwd && dst_degree)) {
    const int onpos = offset * npos;
    MIUTIL_OPENMP_PARALLEL(npos, for)
    for (int i = 0; i < onpos; i += offset) {
      x[i] *= DEG_TO_RAD;
      y[i] *= DEG_TO_RAD;
    }
  }

  double* z = nullptr;
  const int ret = pj_transform(fwd ? src.get() : dst.get(), fwd ? dst.get() : src.get(), npos, offset, x, y, z);
  if (ret != 0 && ret != -20) {
    // ret=-20 :"tolerance condition error"
    // ret=-14 : "latitude or longitude exceeded limits"
    if (ret != -14) {
      METLIBS_LOG_ERROR("error in pj_transform = " << pj_strerrno(ret) << "  " << ret);
    }
    return false;
  }

  if ((fwd && dst_degree) || (!fwd && src_degree)) {
    const int onpos = offset * npos;
    MIUTIL_OPENMP_PARALLEL(npos, for)
    for (int i = 0; i < onpos; i += offset) {
      if (x[i] != HUGE_VAL)
        x[i] *= RAD_TO_DEG;
      if (y[i] != HUGE_VAL)
        y[i] *= RAD_TO_DEG;
    }
  }
  return true;
}
#endif // !HAVE_PROJ_H

} // namespace

Transformation::~Transformation() {}

bool Transformation::forward(size_t npos, float* x, float* y) const
{
  return transform(true, npos, x, y);
}

bool Transformation::forward(size_t npos, double* x, double* y) const
{
  return transform(true, npos, x, y);
}

bool Transformation::forward(size_t npos, diutil::PointF* xy) const
{
  return transform(true, npos, xy);
}

bool Transformation::forward(size_t npos, diutil::PointD* xy) const
{
  return transform(true, npos, xy);
}

bool Transformation::inverse(size_t npos, float* x, float* y) const
{
  return transform(false, npos, x, y);
}

bool Transformation::inverse(size_t npos, double* x, double* y) const
{
  return transform(false, npos, x, y);
}

bool Transformation::inverse(size_t npos, diutil::PointF* xy) const
{
  return transform(false, npos, xy);
}

bool Transformation::inverse(size_t npos, diutil::PointD* xy) const
{
  return transform(false, npos, xy);
}

bool Transformation::transform(bool fwd, size_t npos, float* x, float* y) const
{
  if (!defined())
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

  MIUTIL_OPENMP_PARALLEL(npos, for)
  for (size_t i = 0; i < npos; i++) {
    xd[i] = x[i];
    yd[i] = y[i];
  }

  if (!transformAndCheck(fwd, npos, 1, xd, yd))
    return false;

  MIUTIL_OPENMP_PARALLEL(npos, for)
  for (size_t i = 0; i < npos; i++) {
    x[i] = static_cast<float>(xd[i]);
    y[i] = static_cast<float>(yd[i]);
  }

  return true;
}

bool Transformation::transform(bool fwd, size_t npos, double* xd, double* yd) const
{
  return transformAndCheck(fwd, npos, 1, xd, yd);
}

bool Transformation::transform(bool fwd, size_t npos, diutil::PointF* xy) const
{
  if (!defined())
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

  MIUTIL_OPENMP_PARALLEL(npos, for)
  for (size_t i = 0; i < npos; i++) {
    xd[i] = xy[i].x();
    yd[i] = xy[i].y();
  }

  if (!transformAndCheck(fwd, npos, 1, xd, yd))
    return false;

  MIUTIL_OPENMP_PARALLEL(npos, for)
  for (size_t i = 0; i < npos; i++) {
    xy[i] = diutil::PointF(xd[i], yd[i]);
  }

  return true;
}

bool Transformation::transform(bool fwd, size_t npos, diutil::PointD* xy) const
{
  static_assert(sizeof(diutil::PointD) == 2*sizeof(double), "sizeof PointD");
  double* xd = reinterpret_cast<double*>(xy);
  double* yd = xd + 1;
  assert(reinterpret_cast<double*>(&xy[1]) == (xd + 2));
  assert(yd[0] == xy[0].y());

  return transformAndCheck(fwd, npos, 2, xd, yd);
}

// static
std::shared_ptr<Projection> Projection::sGeographic;

struct Projection::P
{
  std::string proj4Definition;
  enum LonLat { LL_UNKNOWN, LL_NO, LL_YES };
  LonLat isLonLat;
#ifndef HAVE_PROJ_H
  PJ_p proj4PJ;
#endif // !HAVE_PROJ_H

  P() : isLonLat(LL_UNKNOWN) {}
};

Projection::Projection()
    : p_(new P)
{
#ifdef HAVE_PROJ_H
  if (!ctx) {
    ctx = proj_context_create();
    proj_log_func(ctx, nullptr, milogger_log_func);
  }
#endif // HAVE_PROJ_H
}

Projection::Projection(const Projection& o)
    : p_(new P(*o.p_))
{
}

Projection& Projection::operator=(const Projection& o)
{
  Projection p(o);
  std::swap(p_, p.p_);
  return *this;
}

Projection::Projection(const std::string& projStr)
    : Projection()
{
  setFromString(projStr);
}

bool Projection::setFromString(const std::string& crsdef)
{
  if (crsdef.empty()) {
    p_.reset(new P);
    return true;
  }

  if (miutil::contains(crsdef, "PROJCS[") || miutil::contains(crsdef, "GEOGCS[")) {
    return setFromWKT(crsdef);
  }

  static const std::regex RE_EPSG(" *(\\+init=)?([eE][pP][sS][gG]:)([1-9][0-9]+) *");
  std::smatch match_epsg;
  if (std::regex_search(crsdef, match_epsg, RE_EPSG)) {
    return setFromEPSG(match_epsg.str(3));
  }

  return setProj4Definition(crsdef);
}

bool Projection::setFromEPSG(const std::string& epsg)
{
  std::string code = epsg;

  // clang-format off
  static std::map<std::string, std::string> EPSG_REPLACE = { // EPSG code replacement
    { "900913", "3857"},
  };
  {
    const auto it = EPSG_REPLACE.find(code);
    if (it != EPSG_REPLACE.end())
      code = it->second;
  }

#ifdef HAVE_PROJ_H
  static const std::string EPSG_INIT_PREFIX = "EPSG:";
#else // !HAVE_PROJ_H
  static std::map<std::string, std::string> EPSG_PROJ = { // EPSG code -> proj init
    // DWD, see https://epsg.io/4839
    { "4389", "+proj=lcc +lat_1=48.66666666666666 +lat_2=53.66666666666666 +lat_0=51 +lon_0=10.5 +x_0=0 +y_0=0"
              " +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs"},
  };
  // clang-format on

  const auto it = EPSG_PROJ.find(code);
  if (it != EPSG_PROJ.end()) {
    return setProj4Definition(it->second);
  }

  static const std::string EPSG_INIT_PREFIX = "+init=epsg:";
#endif // !HAVE_PROJ_H

  static const std::set<int> EPSG_LONLAT{
#include "diProjectionEPSGLonlat.icc"
  };

  const auto ok = setProj4Definition(EPSG_INIT_PREFIX + code);
  if (ok) {
    p_->isLonLat = EPSG_LONLAT.count(miutil::to_int(code)) ? P::LL_YES : P::LL_NO;
  }
  return ok;
}

bool Projection::setProj4Definition(const std::string& proj4str)
{
  p_->proj4Definition = miutil::trimmed(proj4str);

#ifdef HAVE_PROJ_H
  const auto pj = make_PJ(p_->proj4Definition);
  if (!pj) {
    METLIBS_LOG_WARN("proj init error for '" << p_->proj4Definition << "': " << proj_errno_string(proj_context_errno(ctx)));
    p_->proj4Definition.clear();
  }
#else  // !HAVE_PROJ_H
  p_->proj4PJ = std::shared_ptr<PJ>(pj_init_plus(p_->proj4Definition.c_str()), pj_free);
  if (!p_->proj4PJ)
    METLIBS_LOG_WARN("proj4 init error for '" << p_->proj4Definition << "': " << pj_strerrno(pj_errno));
#endif // !HAVE_PROJ_H
  return isDefined();
}

bool Projection::setFromWKT(const std::string& wkt)
{
#ifdef HAVE_PROJ_H
  return setProj4Definition(wkt);
#else  // !HAVE_PROJ_H
  const QString qwkt = QString::fromStdString(wkt);
  if (setProjectionViaGdalSRSInfo(*this, qwkt))
    return true;
  if (guessProjectionFromWKT(*this, qwkt))
    return true;
  return false;
#endif // !HAVE_PROJ_H
}

const std::string& Projection::getProj4Definition() const
{
  return p_->proj4Definition;
}

std::string Projection::getProj4DefinitionExpanded() const
{
#ifdef HAVE_PROJ_H
  return getProj4Definition(); // no expansion
#else  // !HAVE_PROJ_H
  static const std::string EMPTY;
  if (!p_->proj4PJ)
    return EMPTY;
  // get expanded definition, ie values from "+init=..." are included
  return pj_get_def(p_->proj4PJ.get(), 0);
#endif  // !HAVE_PROJ_H
}

/// return true if projection is defined
bool Projection::isDefined() const
{
#ifdef HAVE_PROJ_H
  return !p_->proj4Definition.empty();
#else  // !HAVE_PROJ_H
  return p_->proj4PJ != 0;
#endif // !HAVE_PROJ_H
}

Projection::~Projection() {}

bool Projection::operator==(const Projection& rhs) const
{
  return (p_->proj4Definition == rhs.p_->proj4Definition);
}

bool Projection::operator!=(const Projection& rhs) const
{
  return !(*this == rhs);
}

std::ostream& operator<<(std::ostream& output, const Projection& p)
{
  output << " proj4string=\"" << p.p_->proj4Definition << "\"";
  return output;
}

Transformation_cp Projection::transformationFrom(const Projection& src) const
{
  if (!areDefined(src, *this))
    return {};

#if 0
  std::unique_lock<std::mutex> lock(mutex_);
#endif
  const auto key = std::make_pair(src.getProj4Definition(), getProj4Definition());
  if (transformations.has(key)) {
    return transformations.get(key);
  }

  METLIBS_LOG_DEBUG("transformations cache miss '" << src.getProj4Definition() << "' => '" << getProj4Definition() << "'");
#ifdef PROJ_H
  Transformation_cp tf = std::make_shared<Proj6Transformation>(src.getProj4Definition(), getProj4Definition());
#else
  Transformation_cp tf = std::make_shared<Proj4Transformation>(src.p_->proj4PJ, p_->proj4PJ, src.isDegree(), isDegree());
#endif
  transformations.put(key, tf);
  return tf;
}

bool Projection::convertPoints(const Projection& srcProj, size_t npos, float* x, float* y) const
{
  if (!areDefined(srcProj, *this))
    return false;

  return transformationFrom(srcProj)->forward(npos, x, y);
}

bool Projection::convertPoints(const Projection& srcProj, size_t npos, double* xd, double* yd) const
{
  if (!areDefined(srcProj, *this))
    return false;

  return transformationFrom(srcProj)->forward(npos, xd, yd);
}

bool Projection::convertPoints(const Projection& srcProj, size_t npos, diutil::PointF* xy) const
{
  if (!areDefined(srcProj, *this))
    return false;

  return transformationFrom(srcProj)->forward(npos, xy);
}

bool Projection::convertPoints(const Projection& srcProj, size_t npos, diutil::PointD* xy) const
{
  if (!areDefined(srcProj, *this))
    return false;

  return transformationFrom(srcProj)->forward(npos, xy);
}

// static
bool Projection::areDefined(const Projection& srcProj, const Projection& tgtProj)
{
  if (!srcProj.isDefined()) {
    METLIBS_LOG_ERROR("src proj not initialized, definition=" << srcProj << "'");
    return false;
  }

  if (!tgtProj.isDefined()) {
    METLIBS_LOG_ERROR("tgt proj not initialized, definition='" << tgtProj << "'");
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

  /*!
   * \param angle in degrees
   */
  explicit uv(float angle) :
    u(std::sin(angle * DEG_TO_RAD)), v(std::cos(angle * DEG_TO_RAD))
  {
  }

  /*!
   * \return angle in degrees
   */
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

/*! Get the direction to north from point x, y
 * \return rotation angles in degrees
 */
float* north(const Projection& p, size_t nvec, const float* x, const float* y)
{
  std::unique_ptr<float[]> north_x(new float[nvec]), north_y(new float[nvec]);
  std::copy(x, x + nvec, north_x.get());
  std::copy(y, y + nvec, north_y.get());

  const auto tf = p.transformationFrom(Projection::geographic());
  tf->inverse(nvec, north_x.get(), north_y.get());

  // convert to geographical, go a little north (lat += 0.1 deg), convert back
  const float deltaLat = 0.1f;

  MIUTIL_OPENMP_PARALLEL(nvec, for)
  for (size_t i = 0; i < nvec; i++) {
    north_y[i] = std::min<float>(north_y[i] + deltaLat, 90);
  }
  tf->forward(nvec, north_x.get(), north_y.get());

  std::unique_ptr<float[]> ret(new float[nvec]);
  MIUTIL_OPENMP_PARALLEL(nvec, for)
  for (size_t i = 0; i < nvec; i++){
    ret[i] = uv(north_x[i] - x[i], north_y[i] - y[i]).angle();
  }
  return ret.release();
}

//! All angles in degrees
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

    MIUTIL_OPENMP_PARALLEL(nvec, for)
    for (size_t i = 0; i < nvec; ++i) {
      if(u[i] != udef && v[i] != udef) {
        const float length = miutil::absval(u[i], v[i]);

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

// from_x and from_y are in srcProj's coordinates
// to_x and to_y are in this projection's coordinates
void Projection::calculateVectorRotationElements(const Projection& srcProj, int nvec,
    const float * from_x, const float * from_y,
    const float * to_x, const float * to_y,
    float * cosa, float * sina) const
{
  std::unique_ptr<float[]> from_north(north(srcProj, nvec, from_x, from_y)); // degrees
  std::unique_ptr<float[]> to_north  (north(*this,   nvec, to_x,   to_y)); // degrees

  MIUTIL_OPENMP_PARALLEL(nvec, for)
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

// x and y are in this projection's coordinates
bool Projection::calculateVectorRotationElements(const Projection& srcProj, int nvec,
    const float * to_x, const float * to_y,
    float * cosa, float * sina) const
{
  std::unique_ptr<float[]> from_x(new float[nvec]), from_y(new float[nvec]);
  std::copy(to_x, to_x + nvec, from_x.get());
  std::copy(to_y, to_y + nvec, from_y.get());

  if (!srcProj.convertPoints(*this, nvec, from_x.get(), from_y.get())) // convert back to srcProj
    return false;

  calculateVectorRotationElements(srcProj, nvec, from_x.get(), from_y.get(), to_x, to_y, cosa, sina);
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
  std::vector<std::string> thisProjectionParts = miutil::split(p_->proj4Definition, 0, " ");
  std::vector<std::string> pProjectionParts = miutil::split(p.getProj4Definition(), 0, " ");

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
  if (miutil::contains(p_->proj4Definition, "+proj=stere"))
    return jumplimit;

  // find position of two geographic positions
  float px[2] = { 0.0, 0.0 };
  float py[2] = { 45.0, -45.0 };

  if (!convertFromGeographic(2, px, py))
    METLIBS_LOG_ERROR("getMapLinesJumpLimit");

  float dx = px[1] - px[0];
  float dy = py[1] - py[0];
  return miutil::absval(dx, dy) / 4;
}

bool Projection::convertToGeographic(int n, float* x, float* y) const
{
  return geographic().convertPoints(*this, n, x, y);
}

bool Projection::convertFromGeographic(int npos, float* x, float* y) const
{
  MIUTIL_OPENMP_PARALLEL(npos, for)
  for (int i = 0; i < npos; i++) {
    if (x[i] == 0)
      x[i] = 0.01f; //todo: Bug:  x[i]=0 converts to x[i]=nx-1 when transforming between geo-projections
  }
  return convertPoints(geographic(), npos, x, y);
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

  MIUTIL_OPENMP_PARALLEL(ntnt, for)
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
    miutil::minimaximize(lonmin, lonmax, tx[i]);
    miutil::minimaximize(latmin, latmax, ty[i]);
  }

  return true;
}

bool Projection::adjustedLatLonBoundingBox(const Rectangle & maprect,
    float & lonmin, float & lonmax, float & latmin, float & latmax) const
{
  // when UTM: keep inside a strict lat/lon-rectangle
  if (miutil::contains(p_->proj4Definition, "+proj=utm")) {
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

  MIUTIL_OPENMP_PARALLEL(npos, for)
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
  MIUTIL_OPENMP_PARALLEL(ny, for)
  for (int iy = 0; iy < ny; iy++) {
    size_t index = nx*iy;
    if(x[index] > 179 && x[index] < 181) {
      x[index] = -180;
    }
  }

  if (coriolis) {
    const float EARTH_OMEGA = 0.7292e-4f; // rad/s, 2*pi / (23*3600 + 56*60 + 4.1), see https://en.wikipedia.org/wiki/Coriolis_frequency
    const float cfactor = 2.0f * EARTH_OMEGA;
    MIUTIL_OPENMP_PARALLEL(npos, for)
    for (int j = 0; j < npos; ++j) {
      coriolis[j] = cfactor * sin(y[j] * DEG_TO_RAD);
    }
  }

  if (!(xmapr && ymapr))
    return false;

  MIUTIL_OPENMP_PARALLEL(nx, for)
  for (int ix = 0; ix < nx; ix++) {
    int i = ix;
    for (int iy = 0; iy < ny-1; iy++, i += nx)
      ymapr[i] = 1 / diutil::GreatCircleDistance(y[i + nx], y[i], x[i + nx], x[i]);
    ymapr[i] = ymapr[i - nx];
  }
  MIUTIL_OPENMP_PARALLEL(ny, for)
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
  if (!isDefined())
    return false;

  if (p_->isLonLat != P::LL_UNKNOWN)
    return p_->isLonLat == P::LL_YES;

  const auto& pd = p_->proj4Definition;
  if (miutil::contains(pd, "+proj=longlat") || miutil::contains(pd, "+proj=ob_tran +o_proj=longlat +lon_0=0 +o_lat_p=90")) {
    return true;
  }
#if !defined(HAVE_PROJ_H)
  if (pj_is_latlong(p_->proj4PJ.get()))
    return true;
#endif // !HAVE_PROJ_H

  return false;
}

bool Projection::isDegree() const
{
  if (!isDefined())
    return false;

  if (isGeographic())
    return true;

  const std::string& pd = p_->proj4Definition;
  if (miutil::contains(pd, "+proj=ob_tran") && miutil::contains(pd, "+o_proj=longlat"))
    return true;

#if defined(HAVE_PROJ_H) && ((1000 * PROJ_VERSION_MAJOR + PROJ_VERSION_MINOR) >= 7002)
  const auto pj = make_PJ(p_->proj4Definition, "EPSG:3003");
  if (pj && proj_degree_input(pj.get(), PJ_FWD))
    return true;
#endif // HAVE_PROJ_H

  return false;
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
    sGeographic = std::make_shared<Projection>("+proj=longlat +ellps=WGS84 +towgs84=0,0,0 +no_defs");
  return *sGeographic;
}
