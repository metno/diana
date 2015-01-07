
#ifndef VCROSSCOMPUTER_H
#define VCROSSCOMPUTER_H 1

#include <diField/VcrossData.h>
#include "VcrossSetup.h"

#include <map>

namespace vcross {

// ================================================================================

/// functions for computations of vertical crossection data
enum VcrossFunction {
  vcf_identity,

  vcf_add,
  vcf_subtract,
  vcf_multiply,
  vcf_divide,

  vcf_convert_unit,
  vcf_impose_unit,

  vcf_total,
  vcf_normal,
  vcf_tangential,
  vcf_sqrt,

  vcf_tk_from_th,
  vcf_th_from_tk,
  vcf_thesat_from_tk,
  vcf_thesat_from_th,

  vcf_the_from_tk_q,
  vcf_the_from_th_q,
  vcf_tdk_from_tk_q,
  vcf_tdk_from_th_q,

  vcf_tdk_from_tk_rh,
  vcf_tdk_from_th_rh,

  vcf_rh_from_tk_q,
  vcf_rh_from_th_q,
  vcf_q_from_tk_rh,
  vcf_q_from_th_rh,

//  vcf_ducting_from_tk_q,
//  vcf_ducting_from_th_q,
//  vcf_ducting_from_tk_rh,
//  vcf_ducting_from_th_rh,
//  vcf_d_ducting_dz_from_tk_q,
//  vcf_d_ducting_dz_from_th_q,
//  vcf_d_ducting_dz_from_tk_rh,
//  vcf_d_ducting_dz_from_th_rh,

//  vcf_momentum_vn_fs,
//  vcf_height_above_msl_from_th,
//  vcf_height_above_surface_from_th,
  vcf_height_above_msl_from_surface_geopotential,
  vcf_no_function
};

struct FunctionSpec {
  const char* name;
  unsigned int argc;
  VcrossFunction id;
  const char* unit_out;
  const char* unit_args[2];
};

// ================================================================================

// data with horizontal and vertical dimension
class FunctionData : public FieldData
{
public:
  FunctionData(std::string id, const FunctionSpec* function);

  VcrossFunction function() const
    { return mFunction->id; }

  virtual std::string dataType() const
    { return DATA_TYPE(); }
  static std::string DATA_TYPE();

  InventoryBase_cpv arguments() const
    { return mArguments; }
  size_t nargument() const
    { return mArguments.size(); }
  InventoryBase_cp argument(size_t a) const
    { return mArguments.at(a); }
  void addArgument(InventoryBase_cp a)
    { mArguments.push_back(a); }

  bool setArguments(const string_v& arguments, const InventoryBase_cps& contentSoFar);
  Values_cp evaluate(name2value_t& n2v) const;
  void collectRequired(InventoryBase_cps& required) const;

private:
  bool hasNumericArg() const
    { return mNumericArgIndex >= 0; }

private:
  const FunctionSpec* mFunction;
  int mNumericArgIndex;
  double mNumericArg;
  InventoryBase_cpv mArguments;
};
typedef boost::shared_ptr<FunctionData> FunctionData_p;
typedef boost::shared_ptr<const FunctionData> FunctionData_cp;

// ================================================================================

void resolve(InventoryBase_cps& content, const NameItem_v& nameItems);
void collectRequired(InventoryBase_cps& required, InventoryBase_cp item);
void collectRequiredVertical(InventoryBase_cps& required, InventoryBase_cp item, Z_AXIS_TYPE zType);
Values_cp vc_evaluate_field(InventoryBase_cp item, name2value_t& n2v);

extern const char VC_LONGITUDE[];
extern const char VC_LATITUDE[];
extern const char VC_BEARING[];
extern const char VC_STEP[];
extern const char VC_CORIOLIS[];

extern const char VC_SURFACE_PRESSURE[];
extern const char VC_SURFACE_ALTITUDE[];
extern const char VC_INFLIGHT_PRESSURE[];
extern const char VC_INFLIGHT_ALTITUDE[];

void resolveCrossection(InventoryBase_cps& inv);
void evaluateCrossection(Crossection_cp cs, name2value_t& n2v);
void evaluateCrossection4TimeGraph(Crossection_cp cs, size_t cs_index, size_t ntimes, name2value_t& n2v);

// ================================================================================

namespace detail {
class NormalTangential {
public:
  NormalTangential(bool normal, float b);
  float operator()(float x, float y) const
    { return bcos*x + bsin*y; }
private:
  float bsin, bcos;
};

inline NormalTangential func_ff_normal(float bearing)
{ return NormalTangential(true, bearing); }

inline NormalTangential func_ff_tangential(float bearing)
{ return NormalTangential(false, bearing); }

} // namespace computer_detail

} // namespace vcross

#endif // VCROSSCOMPUTER_H
