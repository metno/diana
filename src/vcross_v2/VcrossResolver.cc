
#include "VcrossResolver.h"
#include "VcrossComputer.h"
#include <diField/VcrossUtil.h>
#include <puTools/mi_boost_compatibility.hh>
#include <puTools/miStringFunctions.h>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

#define MILOGGER_CATEGORY "vcross.Resolver"
#include "miLogger/miLogging.h"

namespace vcross {

Resolver::Resolver(Setup_p setup)
  : mSetup(setup)
{
}

// ------------------------------------------------------------------------

Resolver::~Resolver()
{
}

// ------------------------------------------------------------------------

void Resolver::setupChanged()
{
  mModelData.clear();
}

// ------------------------------------------------------------------------

Source_p Resolver::getSource(const std::string& model)
{
  return getModelData(model).source;
}

// ------------------------------------------------------------------------

Inventory_cp Resolver::getInventory(const std::string& model)
{
  return getModelData(model).inventory;
}

void Resolver::addDynamicPointValue(std::string model, std::string name, LonLat pos)
{
  Source_p sp= getModelData(model).source;
  LonLat_v positions;
  positions.push_back(pos);
  sp->addDynamicCrossection(name,positions);
}

// ------------------------------------------------------------------------

const ResolvedPlot_cpv& Resolver::getAllResolvedPlots(const std::string& model)
{
  return getModelData(model).resolved_plots;
}

// ------------------------------------------------------------------------

ResolvedPlot_cp Resolver::getResolvedPlot(const std::string& model, const std::string& plot)
{
  const ResolvedPlot_cpv& rpv = getAllResolvedPlots(model);
  BOOST_FOREACH(ResolvedPlot_cp rp, rpv) {
    if (rp->name() == plot)
      return rp;
  }
  return ResolvedPlot_cp();
}

// ------------------------------------------------------------------------

InventoryBase_cp Resolver::getResolvedField(const std::string& model, const std::string& field_id)
{
  return findItemById(getModelData(model).resolved_fields, field_id);
}

// ------------------------------------------------------------------------

Resolver::model_data& Resolver::getModelData(const std::string& model)
{
  METLIBS_LOG_SCOPE();

  model_data_m::iterator it = mModelData.find(model);
  if (it != mModelData.end())
    return it->second;

  METLIBS_LOG_DEBUG("new model_data for '" << model << "'");
  model_data& md = mModelData[model];
  md.source = mSetup->findSource(model);
  if (md.source)
    md.inventory = md.source->getInventory();
  resolveAllFields(md);
  resolveAllPlots(md);
  return md;
}

// ------------------------------------------------------------------------

void Resolver::resolveAllFields(model_data& md)
{
  if (md.inventory) {
    md.resolved_fields = InventoryBase_cps(md.inventory->fields.begin(), md.inventory->fields.end());
    vcross::resolveCrossection(md.resolved_fields);
    // FIXME __PRESSURE is missing here as it depends on the z axis of the first selected plot, or other things
    vcross::resolve(md.resolved_fields, mSetup->getComputations());
  }
}

// ------------------------------------------------------------------------

void Resolver::resolveAllPlots(model_data& md)
{
  if (not md.inventory)
    return;
  BOOST_FOREACH(ConfiguredPlot_cp cp, mSetup->getPlots()) {
    bool argumentsOk = true;
    ResolvedPlot_p rp = miutil::make_shared<ResolvedPlot>(cp);
    BOOST_FOREACH(const std::string& arg, cp->arguments) {
      InventoryBase_cp p = findItemById(md.resolved_fields, arg);
      FieldData_cp fp = boost::dynamic_pointer_cast<const FieldData>(p);
      if (not fp) {
        METLIBS_LOG_WARN("plot '" << cp->name << "' argument '" << arg << "' unknown or not a field");
        argumentsOk = false;
        break;
      } else if (not rp->arguments.empty() and rp->arguments.back()->zaxis() != fp->zaxis())
      {
        METLIBS_LOG_WARN("plot '" << cp->name << "' arguments '" << arg << "' have different z axes");
        argumentsOk = false;
        break;
      } else {
        rp->arguments.push_back(fp);
      }
    }
    if (argumentsOk)
      md.resolved_plots.push_back(rp);
  }
}

// ########################################################################

InventoryBase_cp vc_resolve_unit(Resolver_p resolver, const std::string& model, const std::string& field_id, const std::string& unit)
{
  InventoryBase_cp field = resolver->getResolvedField(model, field_id);
  if (field and util::unitsConvertible(field->unit(), unit))
    return field;
  return InventoryBase_cp();
}

bool vc_resolve_surface(Resolver_p resolver, const std::string& model)
{
  METLIBS_LOG_SCOPE();
  return vc_resolve_unit(resolver, model, VC_SURFACE_PRESSURE, "hPa")
      and vc_resolve_unit(resolver, model, VC_SURFACE_HEIGHT,   "m");
}

bool vc_resolve_pressure_height(Resolver_p resolver, const std::string& model)
{
  METLIBS_LOG_SCOPE();
  return vc_resolve_surface(resolver, model)
      and vc_resolve_unit(resolver, model, VC_SPECIFIC_HUMIDITY, "1")
      and vc_resolve_unit(resolver, model, VC_AIR_TEMPERATURE,   "K");
}

} // namespace vcross
