/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2022 met.no

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

#include "diStreamlineGenerator.h"

#include "diField/diField.h"
#include "diUtilities.h"

#include <mi_fieldcalc/math_util.h>

#include <cmath>

#define MILOGGER_CATEGORY "diana.Streamlines"
#include <miLogger/miLogging.h>

namespace {
float bilinear(float f00, float f10, float f01, float f11, float x, float y)
{
  return f00 * (1 - x) * (1 - y) + f10 * x * (1 - y) + f01 * (1 - x) * y + f11 * x * y;
}

inline bool valid(Field_cp f, const diutil::PointF& p)
{
  const auto x = p.x(), y = p.y();
  return !std::isnan(x) && !std::isnan(y) && x >= 0 && x <= f->area.nx - 2 && y >= 0 && y <= f->area.ny - 2;
}

template <typename T>
int sign(T val)
{
  return (T(0) < val) - (val < T(0));
}

inline float at(Field_cp f, int x, int y)
{
  if (x >= 0 && x < f->area.nx && y >= 0 && y < f->area.ny)
    return f->data[y * f->area.nx + x];
  else
    return std::nanf("");
}

struct p_less
{
  bool operator()(const diutil::PointF& a, const diutil::PointF& b) const
  {
    if (a.x() < b.x())
      return true;
    if (a.x() > b.x())
      return false;
    return a.y() < b.y();
  }
};
} // namespace

// ------------------------------------------------------------------------

StreamlineGenerator::StreamlineGenerator(Field_cp u, Field_cp v)
    : u_(u)
    , v_(v)
{
}

diutil::PointF StreamlineGenerator::interpolate(const diutil::PointF& pos) const
{
  float fix, fiy;
  const float fx = std::modf(pos.x(), &fix);
  const float fy = std::modf(pos.y(), &fiy);
  const int ix = static_cast<int>(fix);
  const int iy = static_cast<int>(fiy);
  const float u = bilinear(at(u_, ix, iy), at(u_, ix + 1, iy), at(u_, ix, iy + 1), at(u_, ix + 1, iy + 1), fx, fy);
  const float v = bilinear(at(v_, ix, iy), at(v_, ix + 1, iy), at(v_, ix, iy + 1), at(v_, ix + 1, iy + 1), fx, fy);
  if (std::isnan(v) || std::isnan(v)) {
    METLIBS_LOG_ERROR(LOGVAL(pos) << LOGVAL(ix) << LOGVAL(iy) << LOGVAL(fx) << LOGVAL(fy) << LOGVAL(u_->area.nx) << LOGVAL(u_->area.ny));
    METLIBS_LOG_ERROR(LOGVAL(at(u_, ix, iy)) << LOGVAL(at(u_, ix + 1, iy)) << LOGVAL(at(u_, ix, iy + 1)) << LOGVAL(at(u_, ix + 1, iy + 1)));
    METLIBS_LOG_ERROR(LOGVAL(at(v_, ix, iy)) << LOGVAL(at(v_, ix + 1, iy)) << LOGVAL(at(v_, ix, iy + 1)) << LOGVAL(at(v_, ix + 1, iy + 1)));
  }
  return diutil::PointF(u, v);
}

StreamlineGenerator::SLP_v StreamlineGenerator::generate(const diutil::PointF& from, const size_t count)
{
  SLP_v sl;
  std::set<diutil::PointF, p_less> positions; // loop detection
  const float eps = 0.5;                      // minimum speed
  diutil::PointF p = from;
  for (size_t i = 0; i < count && valid(u_, p); ++i) {
    const diutil::PointF uv = interpolate(p);
    const float speed = std::sqrt(miutil::square(uv.x()) + miutil::square(uv.y()));

    if (speed < eps)
      break;
    if (positions.find(p) != positions.end())
      break;

    positions.insert(p);
    sl.push_back({p, speed});

    // calculate next point
    float dx, dy;
#if 1
    const float gridstep = 2;
    if (std::abs(uv.x()) > std::abs(uv.y())) {
      // step along x
      dx = gridstep * sign(uv.x());
      dy = uv.y() * dx / uv.x();
    } else {
      // step along y
      dy = gridstep * sign(uv.y());
      dx = uv.x() * dy / uv.y();
    }
#else
    dx = uv.x() * 0.25;
    dy = uv.y() * 0.25;
#endif
#if 0
    METLIBS_LOG_DEBUG(LOGVAL(p) << LOGVAL(sl.back().pos)
                                << LOGVAL(uv.x()) << LOGVAL(uv.y())
                                << LOGVAL(speed)
                                << LOGVAL(dx) << LOGVAL(dy));
#endif

    p.rx() += dx;
    p.ry() += dy;
  }
  return sl;
}
