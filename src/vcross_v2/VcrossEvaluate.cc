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

#include "VcrossEvaluate.h"
#include "VcrossComputer.h"
#include "diField/VcrossUtil.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "vcross.Evaluate"
#include "miLogger/miLogging.h"

namespace vcross {

model_values_m vc_fetch_crossection(Collector_p collector, const std::string& user_crossection, const Time& user_time, int realization)
{
  METLIBS_LOG_SCOPE(LOGVAL(user_crossection) << LOGVAL(user_time.unit) << LOGVAL(user_time.value));
  const model_required_m& mr = collector->getRequired();
  model_values_m model_values;
  for (model_required_m::const_iterator it=mr.begin(); it != mr.end(); ++it) {
    const ModelReftime& mr = it->first;

    Source_p src = collector->getSetup()->findSource(mr.model);
    if (not src)
      continue;
    Inventory_cp inv = src->getInventory(mr.reftime);
    if (not inv)
      continue;
    Crossection_cp cs = inv->findCrossectionByLabel(user_crossection);
    if (not cs)
      continue;

    name2value_t& n2v = model_values[mr];
    vcross::evaluateCrossection(cs, n2v);
    try {
      src->getCrossectionValues(mr.reftime, cs, user_time, it->second, n2v, realization);
    } catch (std::exception& ex) {
      METLIBS_LOG_WARN(ex.what());
    }
  }
  return model_values;
}

model_values_m vc_fetch_pointValues(Collector_p collector, const LonLat& user_crossection, const Time& user_time, int realization)
{
  //METLIBS_LOG_SCOPE(LOGVAL(user_crossection) << LOGVAL(user_time.unit) << LOGVAL(user_time.value));
  const model_required_m& mr = collector->getRequired();
  model_values_m model_values;
  for (model_required_m::const_iterator it=mr.begin(); it != mr.end(); ++it) {
    const ModelReftime& mr = it->first;
    Source_p src = collector->getSetup()->findSource(mr.model);
    if (not src)
      continue;
    Inventory_cp inv = src->getInventory(mr.reftime);
    if (not inv)
      continue;
    Crossection_cp cs = inv->findCrossectionPoint(user_crossection);
    if (not cs)
      continue;
    name2value_t& n2v = model_values[mr];
    vcross::evaluateCrossection(cs, n2v);
    try {
      src->getPointValues(mr.reftime, cs, 0, user_time, it->second, n2v, realization);
    } catch (std::exception& ex) {
      METLIBS_LOG_WARN(ex.what());
    }
  }
  return model_values;
}

// ########################################################################

model_values_m vc_fetch_timegraph(Collector_p collector, const LonLat& position, int realization)
{
  METLIBS_LOG_SCOPE();
  const model_required_m& mr = collector->getRequired();
  model_values_m model_values;
  for (model_required_m::const_iterator it=mr.begin(); it != mr.end(); ++it) {
    const ModelReftime& mr = it->first;

    Source_p src = collector->getSetup()->findSource(mr.model);
    if (not src)
      continue;
    Inventory_cp inv = src->getInventory(mr.reftime);
    if (not inv)
      continue;
    Crossection_cp cs = inv->findCrossectionPoint(position);
    if (not cs)
      continue;

    name2value_t& n2v = model_values[mr];

    vcross::evaluateCrossection4TimeGraph(cs, 0, inv->times.npoint(), n2v);
    try {
      src->getTimegraphValues(mr.reftime, cs, 0, it->second, n2v, realization);
    } catch (std::exception& ex) {
      METLIBS_LOG_WARN(ex.what());
    }
  }
  return model_values;
}

// ########################################################################

diutil::Values_cp vc_evaluate_field(model_values_m& model_values, const ModelReftime& model, InventoryBase_cp field)
{
  METLIBS_LOG_SCOPE();
  const model_values_m::iterator it_m = model_values.find(model);
  if (it_m == model_values.end())
    return diutil::Values_cp();

  return vc_evaluate_field(field, it_m->second);
}

//########################################################################

diutil::Values_cpv vc_evaluate_fields(name2value_t& n2v, const InventoryBase_cpv& fields)
{
  METLIBS_LOG_SCOPE();

  diutil::Values_cpv values;
  for (InventoryBase_cp f : fields)
      values.push_back(vc_evaluate_field(f, n2v));
  return values;
}

//########################################################################

diutil::Values_cpv vc_evaluate_fields(name2value_t& n2v, const FieldData_cpv& fields)
{
  return vc_evaluate_fields(n2v, InventoryBase_cpv(fields.begin(), fields.end()));
}

//########################################################################

diutil::Values_cpv vc_evaluate_fields(model_values_m& model_values, const ModelReftime& model, const InventoryBase_cpv& fields)
{
  METLIBS_LOG_SCOPE();

  const model_values_m::iterator it_m = model_values.find(model);
  if (it_m == model_values.end())
    return diutil::Values_cpv();
  return vc_evaluate_fields(it_m->second, fields);
}

//########################################################################

diutil::Values_cpv vc_evaluate_fields(Collector_p collector, model_values_m& model_values, const ModelReftime& model, const string_v& field_ids)
{
  METLIBS_LOG_SCOPE();

  InventoryBase_cpv fields;
  for (const std::string& f_id : field_ids)
      fields.push_back(collector->getResolvedField(model, f_id));
  return vc_evaluate_fields(model_values, model, fields);
}

//########################################################################

diutil::Values_cpv vc_evaluate_fields(Collector_p collector, model_values_m& model_values, const ModelReftime& model, const char** field_ids)
{
  METLIBS_LOG_SCOPE();

  InventoryBase_cpv fields;
  for (; *field_ids; ++field_ids)
    fields.push_back(collector->getResolvedField(model, *field_ids));
  return vc_evaluate_fields(model_values, model, fields);
}

//########################################################################

void vc_evaluate_surface(Collector_p collector, model_values_m& model_values, const ModelReftime& model)
{
  METLIBS_LOG_SCOPE();
  static const char* surface_field_ids[] = { VC_SURFACE_PRESSURE, VC_SURFACE_ALTITUDE, 0 };
  // FIXME need to convert to expected units
  vc_evaluate_fields(collector, model_values, model, surface_field_ids);
}

// ########################################################################

diutil::Values_cp vc_evaluate_z(ZAxisData_cp zaxis, Z_AXIS_TYPE z_type, name2value_t& n2v)
{
  if (InventoryBase_cp zfield = zaxis->getField(z_type)) {
    return vc_evaluate_field(zfield, n2v);
  }
  return diutil::Values_cp();
}

EvaluatedPlot_cpv vc_evaluate_plots(Collector_p collector, model_values_m& model_values, Z_AXIS_TYPE z_type)
{
  METLIBS_LOG_SCOPE();
  
  EvaluatedPlot_cpv evaluated_plots;
  for (SelectedPlot_cp sp : collector->getSelectedPlots()) {
    if (!sp->visible)
      continue;
    if (sp->resolved->arguments.empty())
      continue;

    model_values_m::iterator it = model_values.find(sp->model);
    if (it == model_values.end())
      continue;
    name2value_t& n2v = it->second;

    EvaluatedPlot_p ep = std::make_shared<EvaluatedPlot>(sp);
    FieldData_cp arg0 = sp->resolved->arguments.at(0);
    if (ZAxisData_cp zaxis = arg0->zaxis()) {
      // FIXME these values do not match zaxis' units
      ep->z_values = vc_evaluate_z(zaxis, z_type, n2v);
    }

    ep->argument_values = vc_evaluate_fields(n2v, sp->resolved->arguments);

    const size_t n_null = std::count(ep->argument_values.begin(), ep->argument_values.end(), diutil::Values_cp());
    if (n_null == 0)
      evaluated_plots.push_back(ep);
  }
  return evaluated_plots;
}

} // namespace vcross
