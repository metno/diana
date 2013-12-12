
#include "diVcrossComputer.h"

#include "diVcrossUtil.h"

#include <diField/diFieldFunctions.h>
#include <diField/diMetConstants.h>
#include <puTools/miStringFunctions.h>

#include <boost/foreach.hpp>

#define MILOGGER_CATEGORY "diana.VcrossComputer"
#include <miLogger/miLogging.h>

namespace /* anonymous */ {

using namespace VcrossComputer;

const VcrossComputer::FunctionLike vcrossFunctionDefs[VcrossComputer::vcf_no_function] = {
  { "add", 2, vcf_add },
  { "subtract", 2, vcf_subtract },
  { "multiply", 2, vcf_multiply },
  { "divide", 2, vcf_divide },
  { "tc_from_tk", 1, vcf_tc_from_tk },
  { "tk_from_tc", 1, vcf_tk_from_tc },
  { "tc_from_th", 1, vcf_tc_from_th },
  { "tk_from_th", 1, vcf_tk_from_th },
  { "th_from_tk", 1, vcf_th_from_tk },
  { "thesat_from_tk", 1, vcf_thesat_from_tk },
  { "thesat_from_th", 1, vcf_thesat_from_th },
  { "the_from_tk_q", 2, vcf_the_from_tk_q },
  { "the_from_th_q", 2, vcf_the_from_th_q },
  { "rh_from_tk_q", 2, vcf_rh_from_tk_q },
  { "rh_from_th_q", 2, vcf_rh_from_th_q },
  { "q_from_tk_rh", 2, vcf_q_from_tk_rh },
  { "q_from_th_rh", 2, vcf_q_from_th_rh },
  { "tdc_from_tk_q", 2, vcf_tdc_from_tk_q },
  { "tdc_from_th_q", 2, vcf_tdc_from_th_q },
  { "tdc_from_tk_rh", 2, vcf_tdc_from_tk_rh },
  { "tdc_from_th_rh", 2, vcf_tdc_from_th_rh },
  { "tdk_from_tk_q", 2, vcf_tdk_from_tk_q },
  { "tdk_from_th_q", 2, vcf_tdk_from_th_q },
  { "tdk_from_tk_rh", 2, vcf_tdk_from_tk_rh },
  { "tdk_from_th_rh", 2, vcf_tdk_from_th_rh },
  { "ducting_from_tk_q", 2, vcf_ducting_from_tk_q },
  { "ducting_from_th_q", 2, vcf_ducting_from_th_q },
  { "ducting_from_tk_rh", 2, vcf_ducting_from_tk_rh },
  { "ducting_from_th_rh", 2, vcf_ducting_from_th_rh },
  { "d_ducting_dz_from_tk_q", 2, vcf_d_ducting_dz_from_tk_q },
  { "d_ducting_dz_from_th_q", 2, vcf_d_ducting_dz_from_th_q },
  { "d_ducting_dz_from_tk_rh", 2, vcf_d_ducting_dz_from_tk_rh },
  { "d_ducting_dz_from_th_rh", 2, vcf_d_ducting_dz_from_th_rh },
  { "ff_total", 2, vcf_ff_total },
  { "ff_normal", 2, vcf_ff_normal },
  { "ff_tangential", 2, vcf_ff_tangential },
//  { "ff_north_south", 2, vcf_ff_north_south },
//  { "ff_east_west", 2, vcf_ff_east_west },
  { "knots_from_ms", 1, vcf_knots_from_ms },
  { "momentum_vn_fs", 1, vcf_momentum_vn_fs },
//  { "height_above_msl_from_th", 1, vcf_height_above_msl_from_th },
//  { "height_above_surface_from_th", 1, vcf_height_above_surface_from_th }
  { "height_above_msl_from_surface_geopotential", 1, vcf_height_above_msl_from_surface_geopotential }
};

static FieldFunctions ffunc;     // Container for Field Functions

const VcrossData::ParameterData* par_data(const VcrossData::Parameters_t& csPar, const std::string& name)
{
  const VcrossData::Parameters_t::const_iterator it_pp = csPar.find(name);
  if (it_pp != csPar.end()) 
    return &it_pp->second;
  else
    return 0;
}

float* par_floats(const VcrossData::Parameters_t& csPar, const std::string& name)
{
  const VcrossData::ParameterData* d = par_data(csPar, name);
  if (d) 
    return d->values.get();
  else
    return 0;
}

} // namespace anonymous

namespace VcrossComputer {

const FunctionLike* findFunction(const std::string& name)
{
  const VcrossComputer::FunctionLike *begin = vcrossFunctionDefs, *end = &vcrossFunctionDefs[VcrossComputer::vcf_no_function];
  for (const VcrossComputer::FunctionLike *f = begin; f != end; ++f) {
    if (name == f->name)
      return f;
  }
  return 0;
}

VcrossData::ParameterData compute(const VcrossData::Parameters_t& csPar, int vcfunc,
    const std::vector<VcrossData::ParameterData>& params)
{
  METLIBS_LOG_SCOPE();

  int compute = 0;
  std::string unit;

  VcrossData::ParameterData out;
  out.mPoints = params.front().mPoints;
  out.mLevels = params.front().mLevels;
  out.zAxis = params.front().zAxis;
  out.alloc();

  bool allDefined = false;
  float undefValue = params.front().undefValue;
  const float* param0 = (params.size() > 0) ? params[0].values.get() : 0;
  const float* param1 = (params.size() > 1) ? params[1].values.get() : 0;

  const float* par_pp = par_floats(csPar, "__PRESSURE");

  switch ((VcrossFunction)vcfunc) {

  case vcf_add:
    if (compute == 0)
      compute = 1;
  case vcf_subtract:
    if (compute == 0)
      compute = 2;
  case vcf_multiply:
    if (compute == 0)
      compute = 3;
  case vcf_divide:
    if (compute == 0)
      compute = 4;
    if (!ffunc.fieldOPERfield(compute, out.mPoints, out.mLevels, param0, param1, out.values.get(), allDefined, undefValue))
      return VcrossData::ParameterData();
    break;

  case vcf_tc_from_tk:
    if (compute == 0)
      compute = 1;
  case vcf_tk_from_tc:
    if (compute == 0)
      compute = 2;
    if (!ffunc.cvtemp(compute, out.mPoints, out.mLevels, param0, out.values.get(), allDefined, undefValue))
      return VcrossData::ParameterData();
    break;

  case vcf_tc_from_th:
    if (compute == 0){
      compute = 1;
      unit = "celsius";
    }
  case vcf_tk_from_th:
    if (compute == 0) {
      compute = 2;
      unit = "kelvin";
    }
  case vcf_th_from_tk:
    if (compute == 0)
      compute = 3;
  case vcf_thesat_from_tk:
    if (compute == 0)
      compute = 4;
  case vcf_thesat_from_th: {
    if (compute == 0)
      compute = 5;
    if (not par_pp)
      return VcrossData::ParameterData();
    if (!ffunc.aleveltemp(compute, out.mPoints, out.mLevels, param0, par_pp, out.values.get(), allDefined, undefValue, unit))
      return VcrossData::ParameterData();
    break; }

  case vcf_the_from_tk_q:
    if (compute == 0)
      compute = 1;
  case vcf_the_from_th_q: {
    if (compute == 0)
      compute = 2;
    if (not par_pp)
      return VcrossData::ParameterData();
    if (!ffunc.alevelthe(compute, out.mPoints, out.mLevels, param0, param1, par_pp, out.values.get(), allDefined, undefValue))
      return VcrossData::ParameterData();
    break; }

  case vcf_rh_from_tk_q:
    if (compute == 0)
      compute = 1;
  case vcf_rh_from_th_q:
    if (compute == 0)
      compute = 2;
  case vcf_q_from_tk_rh:
    if (compute == 0)
      compute = 3;
  case vcf_q_from_th_rh:
    if (compute == 0)
      compute = 4;
  case vcf_tdc_from_tk_q:
    if (compute == 0) {
      compute = 5;
      unit = "celsius";
    }
  case vcf_tdc_from_th_q:
    if (compute == 0) {
      compute = 6;
      unit = "celsius";
    }
  case vcf_tdc_from_tk_rh:
    if (compute == 0) {
      compute = 7;
      unit = "celsius";
    }
  case vcf_tdc_from_th_rh:
    if (compute == 0) {
      compute = 8;
      unit = "celsius";
    }
  case vcf_tdk_from_tk_q:
    if (compute == 0) {
      compute = 9;
      unit = "kelvin";
    }
  case vcf_tdk_from_th_q:
    if (compute == 0) {
      compute = 10;
      unit = "kelvin";
    }
  case vcf_tdk_from_tk_rh:
    if (compute == 0){
      compute = 11;
      unit = "kelvin";
    }
  case vcf_tdk_from_th_rh: {
    if (compute == 0) {
      compute = 12;
      unit = "kelvin";
    }
    if (not par_pp)
      return VcrossData::ParameterData();
    if (!ffunc.alevelhum(compute, out.mPoints, out.mLevels, param0, param1, par_pp, out.values.get(), allDefined, undefValue, unit))
      return VcrossData::ParameterData();
    break; }

  case vcf_ducting_from_tk_q:
    if (compute == 0)
      compute = 1;
  case vcf_ducting_from_th_q:
    if (compute == 0)
      compute = 2;
  case vcf_ducting_from_tk_rh:
    if (compute == 0)
      compute = 3;
  case vcf_ducting_from_th_rh:
    if (compute == 0)
      compute = 4;
  case vcf_d_ducting_dz_from_tk_q:
    if (compute == 0)
      compute = 5;
  case vcf_d_ducting_dz_from_th_q:
    if (compute == 0)
      compute = 6;
  case vcf_d_ducting_dz_from_tk_rh:
    if (compute == 0)
      compute = 7;
  case vcf_d_ducting_dz_from_th_rh: {
    if (compute == 0)
      compute = 8;
    if (not par_pp)
      return VcrossData::ParameterData();
    const int compddz = compute;
    if (compute > 4)
      compute -= 4;
    if (!ffunc.alevelducting(compute, out.mPoints, out.mLevels, param0, param1, par_pp, out.values.get(), allDefined, undefValue))
      return VcrossData::ParameterData();
    if (compddz > 4) {
      if (compddz > 8)
        return VcrossData::ParameterData();
      //...................................................................
      //       t in unit Kelvin, p in unit hPa ???
      //       duct=77.6*(p/t)+373000.*(q*p)/(eps*t*t)
      //       q*p/eps = rh*qsat*p/eps = rh*(eps*e(t)/p)*p/eps
      //               = rh*e(t) = (e(td)/e(t))*e(t) = e(td)
      //       => duct = 77.6*(p/t)+373000.*e(td)/(t*t)
      //
      //       As i Vertical Profiles:
      //       D(ducting)/Dz =
      //       duct(k) = (duct(k+1)-duct(k))/(dz*0.001) !!!
      //...................................................................
      const float* par_tk_th = param0;
      for (int k = 0; k < out.mLevels - 1; k++) {
        for (int i = 0, n = out.mPoints * k; i < out.mPoints; i++, n++) {
          if (par_tk_th[n] != undefValue && par_tk_th[n + out.mPoints] != undefValue
              && out.values.get()[n] != undefValue && out.values.get()[n + out.mPoints] != undefValue)
          {
            const float pi1 = VcrossUtil::exnerFunction(par_pp[n]);
            const float pi2 = VcrossUtil::exnerFunction(par_pp[n + out.mPoints]);
            float th1 = par_tk_th[n], th2 = par_tk_th[n + out.mPoints];
            if (compddz == 5 || compddz == 7) {
              th1 *= MetNo::Constants::cp / pi1;
              th2 *= MetNo::Constants::cp / pi2;
            }
            const float dz = (th1 + th2) * 0.5 * (pi1 - pi2) * MetNo::Constants::ginv;
            const float fv1 = out.values.get()[n];
            const float fv2 = out.values.get()[n + out.mPoints];
            out.values.get()[n] = (fv2 - fv1) / (dz * 0.001);
          } else {
            out.values.get()[n] = undefValue;
          }
        }
      }
    }
    // upper level equal the level below
    const int n1 = (out.mLevels - 1) * out.mPoints;
    const int n2 = (out.mLevels - 2) * out.mPoints;
    for (int i = 0; i < out.mPoints; i++)
      out.values.get()[n1 + i] = out.values.get()[n2 + i];
    break; }

  case vcf_ff_total:
    for (int n = 0; n < out.mLevels * out.mPoints; n++) {
      if (param0[n] != undefValue and param1[n] != undefValue) {
        const float u = param0[n];
        const float v = param1[n];
        out.values.get()[n] = sqrtf(u * u + v * v);
      } else {
        out.values.get()[n] = undefValue;
      }
    }
    break;

  case vcf_ff_normal:
    if (compute == 0)
      compute = 1;
  case vcf_ff_tangential: {
    if (compute == 0)
      compute = 2;
    const float *par_bearing = par_floats(csPar, "__BEARING");
    if (not par_bearing or (compute != 1 and compute != 2))
      return VcrossData::ParameterData();

    const float b0 = 90 * DEG_TO_RAD;
    const float b_offset = b0 * (2-compute); // TODO correct?

    for (int p = 0; p < out.mPoints; p++) {
      const float b = (b0-par_bearing[p]) + b_offset, bsin = std::sin(b), bcos = std::cos(b);
      for (int l = 0, n = 0; l < out.mLevels; l++, n += out.mPoints) {
        const float pX = param0[n], pY = param1[n];
        const float v = (pX != undefValue and pY != undefValue) ? (bcos * pX + bsin * pY): undefValue;
        out.setValue(l, p, v);
      }
    }
    break; }

  case vcf_knots_from_ms: {
    const float unitscale = 3600. / 1852.;
    compute = 3;
    if (!ffunc.fieldOPERconstant(compute, out.mPoints, out.mLevels, param0, unitscale, out.values.get(), allDefined, undefValue))
      return VcrossData::ParameterData();
    break; }

  case vcf_momentum_vn_fs: { // m = FFnormal + f*s,  f=coriolis, s=length(incl. map.ratio)
    const float* par_cor = par_floats(csPar, "__CORIOLIS"), *par_xs = par_floats(csPar, "__STEP");
    if (not par_cor or not par_xs)
      return VcrossData::ParameterData();
    for (int k = 0; k < out.mLevels; k++) {
      for (int i = 0, n = out.mPoints * k; i < out.mPoints; i++, n++) {
        if (param0[n] != undefValue)
          out.values.get()[n] = param0[n] + par_cor[i] * par_xs[i];
        else
          out.values.get()[n] = undefValue;
      }
    }
    break; }

#if 0
  case vcf_height_above_msl_from_th:
    if (compute == 0)
      compute = 1; // height above msl
  case vcf_height_above_surface_from_th: {
    if (compute == 0)
      compute = 2; // height above surface
    const float* par_th = param0;
    if (ci->vcoord != 2 && ci->vcoord != 10)
      return VcrossData::ParameterData();
    const float* par_ps = ci->values(Info::SURFACE_PRESSURE);
    if (not par_ps)
      return VcrossData::ParameterData();
    const float* par_topo = ci->values(Info::TOPOGRAPHY);
    if (compute == 1 && not par_topo)
      return VcrossData::ParameterData();
    float *cwork1 = new float[out.mPoints], *cwork2 = new float[out.mPoints];
    if (compute == 1) {
      for (int i = 0; i < out.mPoints; i++)
        cwork1[i] = par_topo[i];
    } else {
      for (int i = 0; i < out.mPoints; i++)
        cwork1[i] = 0.;
    }
    // height computed in sigma1/eta_half levels (plot in sigma2/eta_full)
    const float* alevel = ci->values(Info::ALEVEL), *blevel = ci->values(Info::BLEVEL);
    for (int i = 0; i < out.mPoints; i++)
      cwork2[i] = VcrossUtil::exnerFunction(par_ps[i]);
    float alvl2 = 0., blvl2 = 1.;
    for (int k = 0; k < out.mLevels; k++) {
      const float alvl1 = alvl2, blvl1 = blvl2;
      alvl2 = std::max(alvl1 - (alvl1 - alevel[k]) * 2., 0.);
      blvl2 = std::max(blvl1 - (blvl1 - blevel[k]) * 2., 0.);
      const int km = (k > 0) ? k - 1 : 0;
      const int kp = (k < out.mLevels - 1) ? k + 1 : out.mLevels - 1;
      for (int i = 0, n = out.mPoints * k; i < out.mPoints; i++, n++) {
        // pressure and exner function (pi) at sigma1 level above
        const float p2 = alvl2 + blvl2 * par_ps[i];
        const float pi2 = VcrossUtil::exnerFunction(p2);
        const float dz = par_th[n] * (cwork2[i] - pi2) * ginv;
        const float z1 = out.values.get()[i];
        const float pi1 = cwork2[i];
        cwork1[i] += dz;
        cwork2[i] = pi2;

        // linear interpolation of height is not good (=> T=const. in layer),
        // so we make a first guess of a temperature profile to comp. height
        const float pim1 = VcrossUtil::exnerFunction(alevel[km] + blevel[km] * par_ps[i]);
        const float pip1 = VcrossUtil::exnerFunction(alevel[kp] + blevel[kp] * par_ps[i]);
        const float dthdpi = (par_th[km * out.mPoints + i] - par_th[kp * out.mPoints + i]) / (pim1 - pip1);
        // get temperature at half level (bottom of layer)
        const float px = alevel[k] + blevel[k] * par_ps[i];
        const float pifull = VcrossUtil::exnerFunction(px);
        const float thhalf = par_th[n] + dthdpi * (pi1 - pifull);
        // thickness from half level to full level
        const float dz2 = (thhalf + par_th[n]) * 0.5 * (pi1 - pifull) * ginv;
        // height at full level
        out.values.get()[n] = z1 + dz2;
      }
    }
    delete[] cwork1;
    delete[] cwork2;
    break; }
#endif

  case vcf_height_above_msl_from_surface_geopotential: {
    const float factor = 1/9.81;
    compute = 3;
    if (!ffunc.fieldOPERconstant(compute, out.mPoints, out.mLevels, param0, factor, out.values.get(), allDefined, undefValue))
      return VcrossData::ParameterData();
    out.unit = "m";
    break; }

  case vcf_no_function:
    METLIBS_LOG_ERROR("unknown function '" << vcfunc << "'");
    return VcrossData::ParameterData();
  }
  
  return out;
}

} // namespace VcrossComputer
