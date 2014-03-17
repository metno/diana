
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

model_values_m vc_fetch_crossection(Collector_p manager, const std::string& user_crossection, const Time& user_time)
{
  METLIBS_LOG_SCOPE(LOGVAL(user_crossection) << LOGVAL(user_time.unit) << LOGVAL(user_time.value));
  const model_required_m& mr = manager->getRequired();
  model_values_m model_values;
  for (model_required_m::const_iterator it=mr.begin(); it != mr.end(); ++it) {
    const std::string& model = it->first;
    
    Source_p src = manager->getSetup()->findSource(model);
    if (not src)
      continue;
    Inventory_cp inv = src->getInventory();
    if (not inv)
      continue;
    Crossection_cp cs = inv->findCrossectionByLabel(user_crossection);
    if (not cs)
      continue;

    name2value_t& n2v = model_values[model];
    const Time ref_time(src->getBestReferenceTime(user_time));
    vcross::evaluateCrossection(cs, n2v);
    src->getCrossectionValues(ref_time, cs, user_time, it->second, n2v);
  }
  return model_values;
}

model_values_m vc_fetch_pointValues(Collector_p manager, const LonLat& user_crossection, const Time& user_time)
{
  //METLIBS_LOG_SCOPE(LOGVAL(user_crossection) << LOGVAL(user_time.unit) << LOGVAL(user_time.value));
  const model_required_m& mr = manager->getRequired();
  model_values_m model_values;
  for (model_required_m::const_iterator it=mr.begin(); it != mr.end(); ++it) {
    const std::string& model = it->first;
    Source_p src = manager->getSetup()->findSource(model);
    if (not src)
      continue;
    Inventory_cp inv = src->getInventory();
    if (not inv)
      continue;
    size_t index;
    Crossection_cp cs = inv->findCrossectionPoint(user_crossection,index);
    if (not cs)
      continue;
    name2value_t& n2v = model_values[model];
    const Time ref_time(src->getBestReferenceTime(user_time));
    vcross::evaluateCrossection(cs, n2v);
    src->getPointValues(ref_time, cs, 0,user_time, it->second, n2v);
  }
  return model_values;
}

// ########################################################################

model_values_m vc_fetch_timegraph(Collector_p manager, const LonLat& position)
{
  METLIBS_LOG_SCOPE();
  const model_required_m& mr = manager->getRequired();
  model_values_m model_values;
  for (model_required_m::const_iterator it=mr.begin(); it != mr.end(); ++it) {
    const std::string& model = it->first;
    
    Source_p src = manager->getSetup()->findSource(model);
    if (not src)
      continue;
    Inventory_cp inv = src->getInventory();
    if (not inv)
      continue;
    size_t index = 0;
    Crossection_cp cs = inv->findCrossectionPoint(position, index);
    if (not cs)
      continue;

    name2value_t& n2v = model_values[model];

    // TODO how to get a reference time for a time graph?
    const Time ref_time(src->getDefaultReferenceTime());
    vcross::evaluateCrossection4TimeGraph(cs, index, inv->times.npoint(), n2v);
    src->getTimegraphValues(ref_time, cs, index, it->second, n2v);
  }
  return model_values;
}

// ########################################################################

Values_cp vc_evaluate_field(model_values_m& model_values,
    const std::string& model, InventoryBase_cp field)
{
  METLIBS_LOG_SCOPE();
  const model_values_m::iterator it_m = model_values.find(model);
  if (it_m == model_values.end())
    return Values_cp();

  name2value_t& n2v = it_m->second;
  const name2value_t::const_iterator it_f = n2v.find(VC_SURFACE_PRESSURE);
  if (it_f != n2v.end())
    return it_f->second;

  return vc_evaluate_field(field, n2v);
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
    const std::string& model, const InventoryBase_cpv& fields)
{
  METLIBS_LOG_SCOPE();

  const model_values_m::iterator it_m = model_values.find(model);
  if (it_m == model_values.end())
    return Values_cpv();
  return vc_evaluate_fields(it_m->second, fields);
}

//########################################################################

Values_cpv vc_evaluate_fields(Collector_p collector, model_values_m& model_values,
    const std::string& model, const string_v& field_ids)
{
  METLIBS_LOG_SCOPE();

  InventoryBase_cpv fields;
  BOOST_FOREACH(const std::string& f_id, field_ids)
      fields.push_back(collector->getResolvedField(model, f_id));
  return vc_evaluate_fields(model_values, model, fields);
}

//########################################################################

Values_cpv vc_evaluate_fields(Collector_p collector, model_values_m& model_values,
    const std::string& model, const char** field_ids)
{
  METLIBS_LOG_SCOPE();

  InventoryBase_cpv fields;
  for (; *field_ids; ++field_ids)
    fields.push_back(collector->getResolvedField(model, *field_ids));
  return vc_evaluate_fields(model_values, model, fields);
}

//########################################################################

void vc_evaluate_surface(Collector_p collector, model_values_m& model_values, const std::string& model)
{
  METLIBS_LOG_SCOPE();
  static const char* surface_field_ids[] = { VC_SURFACE_PRESSURE, VC_SURFACE_HEIGHT, 0 };
  // FIXME need to convert to expected units
  vc_evaluate_fields(collector, model_values, model, surface_field_ids);
}

// ########################################################################

Values_cpv vc_evaluate_pressure_height(Collector_p collector, model_values_m& model_values, const std::string& model)
{
  METLIBS_LOG_SCOPE();
  if (not vc_resolve_pressure_height(collector->getResolver(), model))
    return Values_cpv();

  static const char* conversion_field_ids[] = {
    VC_SPECIFIC_HUMIDITY, VC_AIR_TEMPERATURE, VC_SURFACE_PRESSURE, VC_SURFACE_HEIGHT, 0 
  };
  // FIXME need to convert to expected units
  return vc_evaluate_fields(collector, model_values, model, conversion_field_ids);
}

// ########################################################################

Values_cp vc_converted_z_axis(ZAxisData_cp zaxis, Values_cp z_values, Z_AXIS_TYPE z_type,
    Collector_p collector, model_values_m& model_values, const std::string& model)
{
  z_values = util::unitConversion(z_values, zaxis->unit(), "hPa");
  if (not z_values)
    return Values_cp();

  const Z_AXIS_TYPE zaxis_type = zaxis->zType();
  if (zaxis_type == z_type)
    return z_values;
  
  if (zaxis_type == Z_TYPE_PRESSURE and z_type == Z_TYPE_HEIGHT) {
    const Values_cpv conversion_values = vc_evaluate_pressure_height(collector, model_values, model);
    if (conversion_values.empty())
      return Values_cp();

    return heightFromPressure(z_values, zaxis->zdirection() == Z_DIRECTION_UP,
        conversion_values[0], conversion_values[1],
        conversion_values[2], conversion_values[3]);
  }
  return Values_cp();
}

// ------------------------------------------------------------------------

EvaluatedPlot_cpv vc_evaluate_plots(Collector_p collector, model_values_m& model_values, Z_AXIS_TYPE z_type)
{
  METLIBS_LOG_SCOPE();
  typedef std::pair<Values_cp, Values_cp> z_hpa_conv_t;
  typedef std::map<std::string, z_hpa_conv_t> z_name_values_t;
  typedef std::map<std::string, z_name_values_t> model_z_t;
  model_z_t model_z;
  
  EvaluatedPlot_cpv evaluated_plots;
  BOOST_FOREACH(SelectedPlot_cp sp, collector->getSelectedPlots()) {
    if (sp->resolved->arguments.empty())
      continue;

    model_values_m::iterator it = model_values.find(sp->model);
    if (it == model_values.end())
      continue;

    name2value_t& n2v = it->second;
    z_name_values_t& z_name_values = model_z[sp->model];

    EvaluatedPlot_p ep(new EvaluatedPlot(sp));

    FieldData_cp arg0 = sp->resolved->arguments.at(0);
    if (ZAxisData_cp zaxis = arg0->zaxis()) {
      z_name_values_t::iterator it_z = z_name_values.find(zaxis->id());
      if (it_z == z_name_values.end()) {
        z_hpa_conv_t z;
        z.first  = util::unitConversion(vc_evaluate_field(zaxis, n2v), zaxis->unit(), "hPa");
        z.second = vc_converted_z_axis(zaxis, z.first, z_type,
            collector, model_values, sp->model);
        it_z = z_name_values.insert(std::make_pair(zaxis->id(), z)).first;
      }
      n2v[VC_PRESSURE] = it_z->second.first;
      ep->z_values     = it_z->second.second; // FIXME these values do not match zaxis' units
    } else {
      n2v.erase(VC_PRESSURE);
    }

    ep->argument_values = vc_evaluate_fields(n2v, sp->resolved->arguments);

    const size_t n_null = std::count(ep->argument_values.begin(), ep->argument_values.end(), Values_cp());
    if (n_null == 0)
      evaluated_plots.push_back(ep);
  }
  return evaluated_plots;
}

} // namespace vcross