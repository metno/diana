/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2014-2022 met.no

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

#ifndef VCROSSEVALUATE_H
#define VCROSSEVALUATE_H 1

#include "VcrossCollector.h"
#include "diField/diValues.h"

namespace vcross {

struct EvaluatedPlot {
  SelectedPlot_cp selected;
  diutil::Values_cpv argument_values;
  diutil::Values_cp z_values;

  EvaluatedPlot(SelectedPlot_cp sp)
    : selected(sp) { }
  const std::string& name() const
    { return selected->name(); }
  const ModelReftime& model() const
    { return selected->model; }
  ConfiguredPlot::Type type() const
    { return selected->resolved->configured->type; }
  FieldData_cp argument(size_t idx) const
    { return selected->resolved->arguments.at(idx); }
  diutil::Values_cp values(size_t idx) const { return argument_values.at(idx); }
};
typedef std::shared_ptr<EvaluatedPlot> EvaluatedPlot_p;
typedef std::shared_ptr<const EvaluatedPlot> EvaluatedPlot_cp;
typedef std::vector<EvaluatedPlot_p> EvaluatedPlot_pv;
typedef std::vector<EvaluatedPlot_cp> EvaluatedPlot_cpv;
typedef std::vector<EvaluatedPlot> EvaluatedPlot_v;

// ========================================================================

typedef std::map<ModelReftime, name2value_t, lt_ModelReftime> model_values_m;

model_values_m vc_fetch_crossection(Collector_p manager, const std::string& user_crossection, const Time& user_time, int realization);
model_values_m vc_fetch_pointValues(Collector_p manager, const LonLat& user_crossection, const Time& user_time, int realization);
model_values_m vc_fetch_timegraph(Collector_p manager, const LonLat& position, int realization);

EvaluatedPlot_cpv vc_evaluate_plots(Collector_p manager, model_values_m& model_values, Z_AXIS_TYPE z_type=Z_TYPE_PRESSURE);

diutil::Values_cp vc_evaluate_z(ZAxisData_cp zaxis, Z_AXIS_TYPE z_type, name2value_t& n2v);

void vc_evaluate_surface(Collector_p manager, model_values_m& model_values, const ModelReftime& model);

diutil::Values_cp vc_evaluate_field(model_values_m& model_values, const ModelReftime& model, InventoryBase_cp field);
diutil::Values_cpv vc_evaluate_fields(model_values_m& model_values, const ModelReftime& model, const InventoryBase_cpv& fields);
diutil::Values_cpv vc_evaluate_fields(Collector_p manager, model_values_m& model_values, const ModelReftime& model, const string_v& field_ids);
diutil::Values_cpv vc_evaluate_fields(Collector_p manager, model_values_m& model_values, const ModelReftime& model, const char** field_ids);

} // namespace vcross

#endif // VCROSSEVALUATE_H
