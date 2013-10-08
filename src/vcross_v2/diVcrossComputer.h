
#ifndef diVcrossComputer_h
#define diVcrossComputer_h 1

#include <diField/diVcrossData.h>
#include <string>

namespace VcrossComputer {

struct FunctionLike {
  const char* name;
  unsigned int argc;
  int id;
};

/// functions for computations of vertical crossection data
enum VcrossFunction {
  vcf_add,
  vcf_subtract,
  vcf_multiply,
  vcf_divide,
  vcf_tc_from_tk,
  vcf_tk_from_tc,
  vcf_tc_from_th,
  vcf_tk_from_th,
  vcf_th_from_tk,
  vcf_thesat_from_tk,
  vcf_thesat_from_th,
  vcf_the_from_tk_q,
  vcf_the_from_th_q,
  vcf_rh_from_tk_q,
  vcf_rh_from_th_q,
  vcf_q_from_tk_rh,
  vcf_q_from_th_rh,
  vcf_tdc_from_tk_q,
  vcf_tdc_from_th_q,
  vcf_tdc_from_tk_rh,
  vcf_tdc_from_th_rh,
  vcf_tdk_from_tk_q,
  vcf_tdk_from_th_q,
  vcf_tdk_from_tk_rh,
  vcf_tdk_from_th_rh,
  vcf_ducting_from_tk_q,
  vcf_ducting_from_th_q,
  vcf_ducting_from_tk_rh,
  vcf_ducting_from_th_rh,
  vcf_d_ducting_dz_from_tk_q,
  vcf_d_ducting_dz_from_th_q,
  vcf_d_ducting_dz_from_tk_rh,
  vcf_d_ducting_dz_from_th_rh,
  vcf_ff_total,
  vcf_ff_normal,
  vcf_ff_tangential,
//  vcf_ff_north_south,
//  vcf_ff_east_west,
  vcf_knots_from_ms,
  vcf_momentum_vn_fs,
//  vcf_height_above_msl_from_th,
//  vcf_height_above_surface_from_th,
  vcf_no_function
};

VcrossData::ParameterData compute(const VcrossData::Parameters_t& csPar, int vcfunc,
    const std::vector<VcrossData::ParameterData>& arguments);

const FunctionLike* findFunction(const std::string& name);

} // namespace VcrossComputer

#endif // diVcrossComputer_h
