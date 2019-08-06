/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#include "diVprofCalculations.h"

#include "diVprofSimpleData.h"
#include "diVprofUtils.h"
#include "diField/VcrossUtil.h"
#include "diUtilities.h"

#include <mi_fieldcalc/FieldCalculations.h>
#include <mi_fieldcalc/MetConstants.h>
#include <mi_fieldcalc/math_util.h>

#include <algorithm>
#include <cmath>

#define MILOGGER_CATEGORY "diana.VprofCalculations"
#include <miLogger/miLogging.h>

namespace {
const double RAD_TO_DEG = 180 / M_PI;
} // namespace

namespace vprof {

bool check_same_z(VprofGraphData_cp a, VprofGraphData_cp b)
{
  if (a->length() != b->length())
    return false;
  for (size_t i = 0; i < a->length(); ++i) {
    if (a->z(i) != b->z(i))
      return false;
  }
  return true;
}

VprofGraphData_cp relhum(VprofGraphData_cp tt, VprofGraphData_cp td)
{
  METLIBS_LOG_SCOPE();
  using namespace miutil::constants;

  if (!all_valid(tt) || !all_valid(td) || !check_same_z(tt, td))
    return VprofGraphData_cp();

  const size_t nlev = tt->length();

  // use FieldCalculations::cvhum, pretending we have a horizontal field with y-dim of length 1
  std::unique_ptr<float[]> v_tt(new float[nlev]), v_td(new float[nlev]), v_rh(new float[nlev]);
  for (size_t k = 0; k < nlev; k++) {
    v_tt[k] = vprof::invalid_to_undef(tt->x(k));
    v_td[k] = vprof::invalid_to_undef(td->x(k));
  }
  const int compute = 5;
  miutil::ValuesDefined defined = miutil::combineDefined(tt->defined(), td->defined());
  miutil::fieldcalc::cvhum(nlev, 1, v_tt.get(), v_td.get(), "celsius", compute, v_rh.get(), defined, vprof::UNDEF);

  VprofSimpleData_p rh = std::make_shared<VprofSimpleData>(vprof::VP_RELATIVE_HUMIDITY, tt->zUnit(), "%");
  rh->reserve(nlev);
  for (size_t k = 0; k < nlev; k++)
    rh->add(tt->z(k), v_rh[k]);

  return rh;
}

VprofGraphData_cp cloudbase(VprofGraphData_cp tt, VprofGraphData_cp td)
{
  if (!all_valid(tt) || !all_valid(td) || !check_same_z(tt, td))
    return VprofGraphData_cp();

  VprofSimpleData_p cloudbase = std::make_shared<VprofSimpleData>(vprof::VP_CLOUDBASE, tt->zUnit(), "");
  for (size_t k = 0; k < tt->length(); ++k) {
    if (tt->x(k) - 0.1 <= td->x(k)) {
      cloudbase->add(tt->z(k), tt->x(k));
    }
  }
  return cloudbase;
}

VprofGraphData_cp ducting(VprofGraphData_cp tt, VprofGraphData_cp td)
{
  METLIBS_LOG_SCOPE();

  if (!tt) {
    METLIBS_LOG_DEBUG("missing tt");
    return VprofGraphData_cp();
  }
  if (!td) {
    METLIBS_LOG_DEBUG("missing td");
    return VprofGraphData_cp();
  }
  if (!check_same_z(tt, td)) {
    METLIBS_LOG_DEBUG("z mismatch tt - td");
    return VprofGraphData_cp();
  }

  // if (!vcross::util::unitsConvertible(tt->zUnit(), "hPa"))
  if (tt->zUnit() != "hPa") { // FIXME
    METLIBS_LOG_DEBUG("bad tt z unit '" << tt->zUnit() << "'");
    return VprofGraphData_cp();
  }

  // p,t,td -> ducting index
  //
  // t and td in degrees celsius (and at the same levels)
  //
  // ducting = 77.6*(p/t)+373000.*(q*p)/(eps*t*t)
  //
  // q*p/eps = rh*qsat*p/eps = rh*(eps*e(t)/p)*p/eps
  //         = rh*e(t) = (e(td)/e(t))*e(t) = e(td)
  //
  //    =>  ducting = 77.6*(p/t)+373000.*e(td)/(t*t)
  //
  // output: duct(k) = (ducting(k+1)-ducting(k))/(dz*0.001) + 157.

  using namespace miutil::constants;
  const float ginv = 1. / g;
  const float rcp = r / cp;
  const float p0inv = 1. / p0;

  const size_t nlev = tt->length();
  if (nlev < 2)
    return VprofGraphData_cp();

  VprofSimpleData_p ductp = std::make_shared<VprofSimpleData>(VP_DUCTING_INDEX, tt->zUnit(), "1");
  VprofSimpleData& duct = *ductp;
  duct.reserve(nlev);
  float tk, pi1, pi2, th1, th2, dz;

  for (size_t k = 0; k < nlev; k++) {
    const miutil::constants::ewt_calculator ewt(td->x(k));
    float di = 0;
    if (ewt.defined()) {
      const float etd = ewt.value();
      tk = tt->x(k) + t0;
      ////duct[k]= 77.6*(pp[k]/tk) + 373000.*etd/(tk*tk);
      di = 77.6 * (tt->z(k) / tk) + 373256. * etd / miutil::square(tk);
    }
    duct.add(tt->z(k), di);
  }

  pi2 = cp * powf(tt->z(0) * p0inv, rcp);
  th2 = cp * (tt->x(0) + t0) / pi2;

  for (size_t k = 1; k < nlev; k++) {
    pi1 = pi2;
    th1 = th2;
    pi2 = cp * powf(tt->z(k) * p0inv, rcp);
    th2 = cp * (tt->x(k) + t0) / pi2;
    dz = (th1 + th2) * 0.5 * (pi1 - pi2) * ginv;
    ////duct[k-1]= (duct[k]-duct[k-1])/(dz*0.001);
    duct.setX(k - 1, (duct.x(k) - duct.x(k - 1)) / (dz * 0.001) + 157.);
  }

  if (tt->z(0) > tt->z(nlev - 1)) {
    duct.setX(nlev - 1, duct.x(nlev - 2));
  } else {
    for (size_t k = nlev - 1; k > 0; k--)
      duct.setX(k, duct.x(k - 1));
    duct.setX(0, duct.x(1));
  }
  return ductp;
}

// convert to east/west and north/south component
std::tuple<VprofGraphData_cp, VprofGraphData_cp> wind_uv(VprofGraphData_cp wind_dd, VprofGraphData_cp wind_ff)
{
  VprofSimpleData_p wind_u = std::make_shared<VprofSimpleData>(vprof::VP_WIND_X, wind_dd->zUnit(), "m/s");
  VprofSimpleData_p wind_v = std::make_shared<VprofSimpleData>(vprof::VP_WIND_Y, wind_dd->zUnit(), "m/s");

  const size_t nz = wind_dd->length();
  wind_u->reserve(nz);
  wind_v->reserve(nz);

  for (size_t k = 0; k < nz; k++) {
    const float p = wind_dd->z(k);
    const float fff = wind_ff->x(k);
    const float ddd = (wind_dd->x(k) + 90) / RAD_TO_DEG; // convert compass degrees to math angle
    wind_u->add(p, fff * cosf(ddd));
    wind_v->add(p, -fff * sinf(ddd));
  }

  return std::make_tuple(wind_u, wind_v);
}

std::tuple<VprofGraphData_cp, VprofGraphData_cp> wind_dd_ff(VprofGraphData_cp wind_u, VprofGraphData_cp wind_v)
{
  VprofSimpleData_p wind_dd = std::make_shared<VprofSimpleData>(vprof::VP_WIND_DD, wind_u->zUnit(), vprof::VP_UNIT_COMPASS_DEGREES);
  VprofSimpleData_p wind_ff = std::make_shared<VprofSimpleData>(vprof::VP_WIND_FF, wind_u->zUnit(), "m/s");

  const size_t nz = wind_u->length();
  wind_dd->reserve(nz);
  wind_ff->reserve(nz);

  for (size_t k = 0; k < nz; k++) {
    const float z = wind_u->z(k);
    const float uew = wind_u->x(k);
    const float vns = wind_v->x(k);
    const float ff = miutil::absval(uew, vns);
    int dd = diutil::float2int(270. - RAD_TO_DEG * atan2f(vns, uew)); // convert math angle to compass degrees
    if (dd > 360)
      dd -= 360;
    if (dd <= 0)
      dd += 360;
    if (std::abs(ff) < 0.01)
      dd = 0;
    wind_dd->add(z, dd);
    wind_ff->add(z, ff);
  }

  return std::make_tuple(wind_dd, wind_ff);
}

VprofGraphData_cp wind_sig(VprofGraphData_cp wind_ff)
{
  const size_t nz = wind_ff->length();

  // dd,ff and significant levels (as in temp observation...)
  size_t kmax = 0;
  for (size_t k = 0; k < nz; k++) {
    const float ff = wind_ff->x(k);
    if (ff > wind_ff->x(kmax))
      kmax = k;
  }

  VprofSimpleData_p wind_sig = std::make_shared<VprofSimpleData>(vprof::VP_WIND_SIG, wind_ff->zUnit(), "");
  wind_sig->reserve(nz);
  if (nz >= 2) {
    wind_sig->add(wind_ff->z(0), 0);
    for (size_t k = 1; k < wind_ff->length() - 1; k++) {
      float ws = 0;
      if (k == kmax) {
        ws = 3;
      } else {
        float ffk = wind_ff->x(k);
        float ffb = wind_ff->x(k - 1);
        float ffa = wind_ff->x(k + 1);
        if (ffk < ffb && ffk < ffa)
          ws = 1; // local ff minimum
        if (ffk > ffb && ffk > ffa)
          ws = 2; // local ff maximum
      }
      wind_sig->add(wind_ff->z(k), ws);
    }
    wind_sig->add(wind_ff->z(nz - 1), 0);
  }

  return wind_sig;
}

float kindex(VprofGraphData_cp tt, VprofGraphData_cp td)
{
  METLIBS_LOG_SCOPE();

  // K-index = (t+td)850 - (t-td)700 - (t)500

  if (!all_valid(tt)) {
    METLIBS_LOG_DEBUG("invalid tt");
    return vprof::UNDEF;
  }
  if (!all_valid(td)) {
    METLIBS_LOG_DEBUG("invalid td");
    return vprof::UNDEF;
  }
  if (!check_same_z(tt, td)) {
    METLIBS_LOG_DEBUG("z mismatch tt - td");
    return vprof::UNDEF;
  }

  // if (!vcross::util::unitsConvertible(tt->zUnit(), "hPa"))
  if (tt->zUnit() != "hPa") { // FIXME
    METLIBS_LOG_DEBUG("bad tt z unit '" << tt->zUnit() << "'");
    return vprof::UNDEF;
  }

  const size_t nlev = tt->length();
  if (nlev < 2)
    return vprof::UNDEF;

  const float ttp0 = tt->z(0);
  const float ttpL = tt->z(nlev - 1);
  const bool pIncreasing = (ttp0 < ttpL);
  if (pIncreasing) {
    if (ttp0 > 500. || ttpL < 850.)
      return vprof::UNDEF;
  } else {
    if (ttp0 < 850. || ttpL > 500.)
      return vprof::UNDEF;
  }

  const int NP = 3;
  const float pfind[NP] = {850., 700., 500.};
  float tfind[NP], tdfind[NP];

  for (int n = 0; n < NP; n++) {
    size_t k = 1;
    if (pIncreasing)
      while (k < nlev && pfind[n] > tt->z(k))
        k++;
    else
      while (k < nlev && pfind[n] < tt->z(k))
        k++;
    if (k == nlev)
      k--;

    // linear interpolation in exner function
    const float pi1 = vcross::util::exnerFunction(tt->z(k - 1));
    const float pi2 = vcross::util::exnerFunction(tt->z(k));
    const float pi = vcross::util::exnerFunction(pfind[n]);
    const float f = (pi - pi1) / (pi2 - pi1);

    tfind[n] = tt->x(k - 1) + (tt->x(k) - tt->x(k - 1)) * f;
    tdfind[n] = td->x(k - 1) + (td->x(k) - td->x(k - 1)) * f;
  }

  // K-index = (t+td)850 - (t-td)700 - (t)500

  return (tfind[0] + tdfind[0]) - (tfind[1] - tdfind[1]) - tfind[2];
}

bool is_empty(VprofGraphData_cp data)
{
  return !data || data->empty();
}

bool valid_content(VprofGraphData_cp data)
{
  return !is_empty(data) && (data->defined() == miutil::ALL_DEFINED);
}

} // namespace vprof
