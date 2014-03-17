
#ifndef VCROSSEVALUATE_H
#define VCROSSEVALUATE_H 1

#include "VcrossCollector.h"

namespace vcross {

struct EvaluatedPlot {
  SelectedPlot_cp selected;
  Values_cpv argument_values;
  Values_cp z_values;
  
  EvaluatedPlot(SelectedPlot_cp sp)
    : selected(sp) { }
  std::string name() const
    { return selected->name(); }
  std::string model() const
    { return selected->model; }
  ConfiguredPlot::Type type() const
    { return selected->resolved->configured->type; }
  FieldData_cp argument(size_t idx) const
    { return selected->resolved->arguments.at(idx); }
  Values_cp values(size_t idx) const
    { return argument_values.at(idx); }
};
typedef boost::shared_ptr<EvaluatedPlot> EvaluatedPlot_p;
typedef boost::shared_ptr<const EvaluatedPlot> EvaluatedPlot_cp;
typedef std::vector<EvaluatedPlot_p> EvaluatedPlot_pv;
typedef std::vector<EvaluatedPlot_cp> EvaluatedPlot_cpv;
typedef std::vector<EvaluatedPlot> EvaluatedPlot_v;

// ========================================================================

typedef std::map<std::string, name2value_t> model_values_m;

model_values_m vc_fetch_crossection(Collector_p manager, const std::string& user_crossection, const Time& user_time);
model_values_m vc_fetch_pointValues(Collector_p manager, const LonLat& user_crossection, const Time& user_time);
model_values_m vc_fetch_timegraph(Collector_p manager, const LonLat& position);

EvaluatedPlot_cpv vc_evaluate_plots(Collector_p manager, model_values_m& model_values, Z_AXIS_TYPE z_type=Z_TYPE_PRESSURE);

void vc_evaluate_surface(Collector_p manager, model_values_m& model_values, const std::string& model);

Values_cp vc_evaluate_field(model_values_m& model_values,
    const std::string& model, InventoryBase_cp field);
Values_cpv vc_evaluate_fields(model_values_m& model_values,
    const std::string& model, const InventoryBase_cpv& fields);
Values_cpv vc_evaluate_fields(Collector_p manager, model_values_m& model_values,
    const std::string& model, const string_v& field_ids);
Values_cpv vc_evaluate_fields(Collector_p manager, model_values_m& model_values,
    const std::string& model, const char** field_ids);

} // namespace vcross

#endif // VCROSSEVALUATE_H
