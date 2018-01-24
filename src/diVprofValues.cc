/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

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

#include "diana_config.h"

#include "diVprofValues.h"

#include "diField/VcrossUtil.h"
#include "diField/diMetConstants.h"
#include "util/math_util.h"

#include <algorithm>
#include <cmath>

#define MILOGGER_CATEGORY "diana.VprofValues"
#include <miLogger/miLogging.h>

using namespace std;

namespace {
bool is_invalid(float f)
{
  return isnan(f) || abs(f) >= 1e20;
}

size_t count_invalid(const std::vector<float>& data)
{
  return std::count_if(data.begin(), data.end(), is_invalid);
}

difield::ValuesDefined check_valid(const std::vector<float>& data)
{
  const size_t n_undefined = count_invalid(data), n = data.size();
  return difield::checkDefined(n_undefined, n);
}

bool all_valid(const std::vector<float>& data)
{
  return check_valid(data) == difield::ALL_DEFINED;
}
} // namespace

VprofValues::VprofValues()
    : windInKnots(true)
{
  METLIBS_LOG_SCOPE();
}

void VprofValues::calculate()
{
  difield::ValuesDefined defined_t, defined_p;
  if (prognostic) {
    defined_t = check_valid(tt);
    defined_p = check_valid(ptt);

    relhum(tt, td);
    ducting(ptt, tt, td);
    kindex(ptt, tt, td);
  } else {
    defined_t = check_valid(tcom);
    defined_p = check_valid(pcom);

    relhum(tcom, tdcom);
    ducting(pcom, tcom, tdcom);
    kindex(pcom, tcom, tdcom);
  }
  defined_ = difield::combineDefined(defined_t, defined_p);
}

void VprofValues::relhum(const vector<float>& tt, const vector<float>& td)
{
  METLIBS_LOG_SCOPE();
  using namespace MetNo::Constants;

  if (tt.size() == td.size() && all_valid(tt) && all_valid(td)) {
    int nlev = tt.size();
    rhum.resize(nlev);
    for (int k = 0; k < nlev; k++) {
      const ewt_calculator ewt(tt[k]), ewt2(td[k]);
      float rhx = 0;
      if (ewt.defined() and ewt2.defined()) {
        const float et = ewt.value();
        const float etd = ewt2.value();
        rhx = 100. * etd / et;
      }
      if (rhx < 0)
        rhx = 0;
      if (rhx > 100)
        rhx = 100;
      rhum[k] = rhx;
    }
  } else {
    rhum.clear();
  }
}

void VprofValues::ducting(const vector<float>& pp, const vector<float>& tt, const vector<float>& td)
{
  METLIBS_LOG_SCOPE();

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

  const float g = 9.8;
  const float r = 287.;
  const float cp = 1004.;
  const float p0 = 1000.;
  const float t0 = 273.15;
  const float ginv = 1. / g;
  const float rcp = r / cp;
  const float p0inv = 1. / p0;

  int nlev = pp.size();
  if (nlev < 2 || td.size() != pp.size() || tt.size() != pp.size() || !all_valid(td) || !all_valid(pp) || !all_valid(tt)) {
    duct.clear();
    return;
  }

#if 1
  cloudbase_p = -1;
  { // cloud base
    for (size_t i = 0; i < ptt.size(); ++i) {
      if (tt[i] - 0.1 <= td[i]) {
        cloudbase_p = ptt[i];
        cloudbase_t = tt[i];
      }
    }
  }
#endif

  duct.resize(nlev);
  int k;
  float tk, pi1, pi2, th1, th2, dz;

  for (k = 0; k < nlev; k++) {
    const MetNo::Constants::ewt_calculator ewt(td[k]);
    if (ewt.defined()) {
      const float etd = ewt.value();
      tk = tt[k] + t0;
      ////duct[k]= 77.6*(pp[k]/tk) + 373000.*etd/(tk*tk);
      duct[k] = 77.6 * (pp[k] / tk) + 373256. * etd / diutil::square(tk);
    } else {
      duct[k] = 0;
    }
  }

  pi2 = cp * powf(pp[0] * p0inv, rcp);
  th2 = cp * (tt[0] + t0) / pi2;

  for (k = 1; k < nlev; k++) {
    pi1 = pi2;
    th1 = th2;
    pi2 = cp * powf(pp[k] * p0inv, rcp);
    th2 = cp * (tt[k] + t0) / pi2;
    dz = (th1 + th2) * 0.5 * (pi1 - pi2) * ginv;
    ////duct[k-1]= (duct[k]-duct[k-1])/(dz*0.001);
    duct[k - 1] = (duct[k] - duct[k - 1]) / (dz * 0.001) + 157.;
  }

  if (pp[0] > pp[nlev - 1]) {
    duct[nlev - 1] = duct[nlev - 2];
  } else {
    for (k = nlev - 1; k > 0; k--)
      duct[k] = duct[k - 1];
    duct[0] = duct[1];
  }
}

void VprofValues::kindex(const vector<float>& pp, const vector<float>& tt, const vector<float>& td)
{
  METLIBS_LOG_SCOPE();

  // K-index = (t+td)850 - (t-td)700 - (t)500

  text.kindexFound = false;

  const int nlev = pp.size();
  if (nlev < 2 || td.size() != pp.size() || tt.size() != pp.size() || !all_valid(td) || !all_valid(pp) || !all_valid(tt))
    return;

  const bool pIncreasing = (pp[0] < pp[nlev - 1]);
  if (pIncreasing) {
    if (pp[0] > 500. || pp[nlev - 1] < 850.)
      return;
  } else {
    if (pp[0] < 850. || pp[nlev - 1] > 500.)
      return;
  }

  const int NP = 3;
  const float pfind[NP] = {850., 700., 500.};
  float tfind[NP], tdfind[NP];

  for (int n = 0; n < NP; n++) {
    int k = 1;
    if (pIncreasing)
      while (k < nlev && pfind[n] > pp[k])
        k++;
    else
      while (k < nlev && pfind[n] < pp[k])
        k++;
    if (k == nlev)
      k--;

    // linear interpolation in exner function
    const float pi1 = vcross::util::exnerFunction(pp[k - 1]);
    const float pi2 = vcross::util::exnerFunction(pp[k]);
    const float pi = vcross::util::exnerFunction(pfind[n]);
    const float f = (pi - pi1) / (pi2 - pi1);

    tfind[n] = tt[k - 1] + (tt[k] - tt[k - 1]) * f;
    tdfind[n] = td[k - 1] + (td[k] - td[k - 1]) * f;
  }

  // K-index = (t+td)850 - (t-td)700 - (t)500

  text.kindexValue = (tfind[0] + tdfind[0]) - (tfind[1] - tdfind[1]) - tfind[2];
  text.kindexFound = true;
}
