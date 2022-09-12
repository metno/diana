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

#include "VcrossResolver.h"

#include "VcrossComputer.h"
#include "diField/VcrossUtil.h"
#include "util/diUnitsConverter.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "vcross.Resolver"
#include "miLogger/miLogging.h"

namespace vcross {

bool ModelReftime::valid() const
{
  return (!model.empty()) && reftime.valid();
}

bool ModelReftime::operator==(const ModelReftime& other) const
{
  return model == other.model && reftime == other.reftime;
}

bool lt_ModelReftime::operator()(const ModelReftime& mr1, const ModelReftime& mr2) const
{
  const int cmp = mr1.model.compare(mr2.model);
  if (cmp < 0)
    return true;
  if (cmp > 0)
    return false;
  return mr1.reftime < mr2.reftime;
};

std::ostream& operator<<(std::ostream& out, const ModelReftime& mr)
{
  out << '[' << mr.model << '@' << util::to_miTime(mr.reftime) << ']';
  return out;
}

//########################################################################

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

Source_p Resolver::getSource(const std::string& modelName)
{
  return mSetup->findSource(modelName);
}

// ------------------------------------------------------------------------

Inventory_cp Resolver::getInventory(const ModelReftime& model)
{
  return getModelData(model).inventory;
}

// ------------------------------------------------------------------------

const ResolvedPlot_cpv& Resolver::getAllResolvedPlots(const ModelReftime& model)
{
  return getModelData(model).resolved_plots;
}

// ------------------------------------------------------------------------

ResolvedPlot_cp Resolver::getResolvedPlot(const ModelReftime& model, const std::string& plot)
{
  const ResolvedPlot_cpv& rpv = getAllResolvedPlots(model);
  for (ResolvedPlot_cp rp : rpv) {
    if (rp->name() == plot)
      return rp;
  }
  return ResolvedPlot_cp();
}

// ------------------------------------------------------------------------

InventoryBase_cp Resolver::getResolvedField(const ModelReftime& model, const std::string& field_id)
{
  return findItemById(getModelData(model).resolved_fields, field_id);
}

// ------------------------------------------------------------------------

Resolver::model_data& Resolver::getModelData(const ModelReftime& mr)
{
  METLIBS_LOG_SCOPE();

  model_data_m::iterator it = mModelData.find(mr);
  if (it != mModelData.end())
    return it->second;

  METLIBS_LOG_DEBUG("new model_data for '" << mr.model << "'");
  model_data& md = mModelData[mr];
  md.source = mSetup->findSource(mr.model);
  if (md.source)
    md.inventory = md.source->getInventory(mr.reftime);
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
  for (ConfiguredPlot_cp cp : mSetup->getPlots()) {
    bool argumentsOk = true;
    ResolvedPlot_p rp = std::make_shared<ResolvedPlot>(cp);
    for (const std::string& arg : cp->arguments) {
      InventoryBase_cp p = findItemById(md.resolved_fields, arg);
      FieldData_cp fp = std::dynamic_pointer_cast<const FieldData>(p);
      if (not fp) {
        METLIBS_LOG_DEBUG("plot '" << cp->name << "' argument '" << arg << "' unknown or not a field");
        argumentsOk = false;
        break;
      } else if (not rp->arguments.empty() and rp->arguments.back()->zaxis() != fp->zaxis())
      {
        METLIBS_LOG_DEBUG("plot '" << cp->name << "' arguments '" << arg << "' have different z axes");
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

InventoryBase_cp vc_resolve_unit(Resolver_p resolver, const ModelReftime& model, const std::string& field_id, const std::string& unit)
{
  InventoryBase_cp field = resolver->getResolvedField(model, field_id);
  if (field && diutil::unitsConvertible(field->unit(), unit))
    return field;
  return InventoryBase_cp();
}

bool vc_resolve_surface(Resolver_p resolver, const ModelReftime& model)
{
  METLIBS_LOG_SCOPE();
  return vc_resolve_unit(resolver, model, VC_SURFACE_PRESSURE, "hPa")
      and vc_resolve_unit(resolver, model, VC_SURFACE_ALTITUDE, "m");
}

} // namespace vcross
