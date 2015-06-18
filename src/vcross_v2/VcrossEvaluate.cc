
#include "VcrossEvaluate.h"
#include "VcrossComputer.h"
#include <diField/VcrossUtil.h>
#include <puTools/mi_boost_compatibility.hh>
#include <puTools/miStringFunctions.h>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

#define MILOGGER_CATEGORY "vcross.Evaluate"
#include "miLogger/miLogging.h"

namespace vcross {

model_values_m vc_fetch_crossection(Collector_p collector, const std::string& user_crossection, const Time& user_time)
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
      src->getCrossectionValues(mr.reftime, cs, user_time, it->second, n2v);
    } catch (std::exception& ex) {
      METLIBS_LOG_WARN(ex.what());
    }
  }
  return model_values;
}

model_values_m vc_fetch_pointValues(Collector_p collector, const LonLat& user_crossection, const Time& user_time)
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
    size_t index;
    Crossection_cp cs = inv->findCrossectionPoint(user_crossection);
    if (not cs)
      continue;
    name2value_t& n2v = model_values[mr];
    vcross::evaluateCrossection(cs, n2v);
    try {
      src->getPointValues(mr.reftime, cs, 0, user_time, it->second, n2v);
    } catch (std::exception& ex) {
      METLIBS_LOG_WARN(ex.what());
    }
  }
  return model_values;
}

// ########################################################################

model_values_m vc_fetch_timegraph(Collector_p collector, const LonLat& position)
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
      src->getTimegraphValues(mr.reftime, cs, 0, it->second, n2v);
    } catch (std::exception& ex) {
      METLIBS_LOG_WARN(ex.what());
    }
  }
  return model_values;
}

// ########################################################################

Values_cp vc_evaluate_field(model_values_m& model_values,
    const ModelReftime& model, InventoryBase_cp field)
{
  METLIBS_LOG_SCOPE();
  const model_values_m::iterator it_m = model_values.find(model);
  if (it_m == model_values.end())
    return Values_cp();

  return vc_evaluate_field(field, it_m->second);
}

//########################################################################

Values_cpv vc_evaluate_fields(name2value_t& n2v, const InventoryBase_cpv& fields)
{
  METLIBS_LOG_SCOPE();

  Values_cpv values;
  BOOST_FOREACH(InventoryBase_cp f, fields)
      values.push_back(vc_evaluate_field(f, n2v));
  return values;
}

//########################################################################

Values_cpv vc_evaluate_fields(name2value_t& n2v, const FieldData_cpv& fields)
{
  return vc_evaluate_fields(n2v, InventoryBase_cpv(fields.begin(), fields.end()));
}

//########################################################################

Values_cpv vc_evaluate_fields(model_values_m& model_values,
    const ModelReftime& model, const InventoryBase_cpv& fields)
{
  METLIBS_LOG_SCOPE();

  const model_values_m::iterator it_m = model_values.find(model);
  if (it_m == model_values.end())
    return Values_cpv();
  return vc_evaluate_fields(it_m->second, fields);
}

//########################################################################

Values_cpv vc_evaluate_fields(Collector_p collector, model_values_m& model_values,
    const ModelReftime& model, const string_v& field_ids)
{
  METLIBS_LOG_SCOPE();

  InventoryBase_cpv fields;
  BOOST_FOREACH(const std::string& f_id, field_ids)
      fields.push_back(collector->getResolvedField(model, f_id));
  return vc_evaluate_fields(model_values, model, fields);
}

//########################################################################

Values_cpv vc_evaluate_fields(Collector_p collector, model_values_m& model_values,
    const ModelReftime& model, const char** field_ids)
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

EvaluatedPlot_cpv vc_evaluate_plots(Collector_p collector, model_values_m& model_values, Z_AXIS_TYPE z_type)
{
  METLIBS_LOG_SCOPE();
  
  EvaluatedPlot_cpv evaluated_plots;
  BOOST_FOREACH(SelectedPlot_cp sp, collector->getSelectedPlots()) {
    if (!sp->visible)
      continue;
    if (sp->resolved->arguments.empty())
      continue;

    model_values_m::iterator it = model_values.find(sp->model);
    if (it == model_values.end())
      continue;
    name2value_t& n2v = it->second;

    EvaluatedPlot_p ep(new EvaluatedPlot(sp));
    FieldData_cp arg0 = sp->resolved->arguments.at(0);
    if (ZAxisData_cp zaxis = arg0->zaxis()) {
      Values_cp z_values;
      if (z_type == Z_TYPE_PRESSURE) {
        if (util::unitsConvertible(zaxis->unit(), "hPa"))
          z_values = util::unitConversion(vc_evaluate_field(zaxis, n2v), zaxis->unit(), "hPa");
        else if (InventoryBase_cp pfield = zaxis->pressureField())
          z_values = vc_evaluate_field(pfield, n2v);
      } else if (z_type == Z_TYPE_ALTITUDE) {
        if (util::unitsConvertible(zaxis->unit(), "m"))
          z_values = util::unitConversion(vc_evaluate_field(zaxis, n2v), zaxis->unit(), "m");
        else if (InventoryBase_cp afield = zaxis->altitudeField())
          z_values = vc_evaluate_field(afield, n2v);
      }
      ep->z_values = z_values; // FIXME these values do not match zaxis' units
    }

    ep->argument_values = vc_evaluate_fields(n2v, sp->resolved->arguments);

    const size_t n_null = std::count(ep->argument_values.begin(), ep->argument_values.end(), Values_cp());
    if (n_null == 0)
      evaluated_plots.push_back(ep);
  }
  return evaluated_plots;
}

} // namespace vcross
