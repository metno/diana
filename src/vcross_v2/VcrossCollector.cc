
#include "VcrossCollector.h"
#include "VcrossComputer.h"
#include <diField/VcrossUtil.h>
#include <puTools/mi_boost_compatibility.hh>
#include <puTools/miStringFunctions.h>

#include <boost/algorithm/string/join.hpp>
#include <boost/make_shared.hpp>

#define MILOGGER_CATEGORY "vcross.Collector"
#include "miLogger/miLogging.h"

namespace vcross {

std::string SelectedPlot::optionString() const
{
  return boost::algorithm::join(options, " ");
}

// ########################################################################

Collector::Collector(Setup_p setup)
  : mResolver(miutil::make_shared<Resolver>(setup))
{
}

// ------------------------------------------------------------------------

Collector::~Collector()
{
}

// ------------------------------------------------------------------------

void Collector::setupChanged()
{
  mResolver->setupChanged();
  clear();
}

// ------------------------------------------------------------------------

bool Collector::clear()
{
  if (mSelectedPlots.empty())
    return false;

  mSelectedPlots.clear();
  setUpdateRequiredNeeded();
  mModelRequired.clear();

  return true;
}

// ------------------------------------------------------------------------

int Collector::selectPlot(const ModelReftime& model, const std::string& plot, const string_v& options)
{
  return insertPlot(model, plot, options, -1);
}


int Collector::insertPlot(const ModelReftime& model, const std::string& plot, const string_v& options, int index)
{
  METLIBS_LOG_SCOPE(LOGVAL(model) << LOGVAL(plot));
  ResolvedPlot_cp rp = mResolver->getResolvedPlot(model, plot);
  if (not rp)
    return -1;

  SelectedPlot_p sp(new SelectedPlot(model));
  sp->resolved = rp;
  sp->options = options;

  if (index < 0 || index >= (int)mSelectedPlots.size()) {
    index = mSelectedPlots.size();
    mSelectedPlots.push_back(sp);
  } else {
    mSelectedPlots.insert(mSelectedPlots.begin() + index, sp);
  }

  setUpdateRequiredNeeded();
  return index;
}


bool Collector::removePlot(int index)
{
  if (index < 0 || index >= (int)mSelectedPlots.size())
    return false;

  mSelectedPlots.erase(mSelectedPlots.begin() + index);
  setUpdateRequiredNeeded();

  return true;
}


ModelReftime Collector::getFirstModel() const
{
  for (SelectedPlot_pv::const_iterator itSP = mSelectedPlots.begin(); itSP != mSelectedPlots.end(); ++itSP)
    if ((*itSP)->visible)
      return (*itSP)->model;
  return ModelReftime();
}


bool Collector::updateRequired()
{
  if (mModelRequired.empty())
    requirePlots();

  return !mModelRequired.empty();
}

void Collector::requireField(const ModelReftime& model, InventoryBase_cp field)
{
  METLIBS_LOG_SCOPE(LOGVAL(model));
  if (not field)
    return;

  METLIBS_LOG_DEBUG(LOGVAL(field->id()));
  InventoryBase_cps& required = mModelRequired[model];
  collectRequired(required, field);
}


void Collector::requireVertical(Z_AXIS_TYPE zType)
{
  for (model_required_m::iterator it = mModelRequired.begin(); it != mModelRequired.end(); ++it) {
    InventoryBase_cps& required = it->second;
    const InventoryBase_cps fields = it->second; // take copy, not reference, as we modify it->second in the following loop
    for (InventoryBase_cps::const_iterator it = fields.begin(); it != fields.end(); ++it)
      vcross::collectRequiredVertical(required, *it, zType);
  }
}


void Collector::requirePlots()
{
  for (SelectedPlot_pv::iterator it = mSelectedPlots.begin(); it != mSelectedPlots.end(); ++it)
    requirePlot(*it);
}


void Collector::requirePlot(SelectedPlot_p sp)
{
  if (!sp->visible)
    return;

  InventoryBase_cps& required = mModelRequired[sp->model];
  const FieldData_cpv& args = sp->resolved->arguments;
  for (FieldData_cpv::const_iterator it = args.begin(); it != args.end(); ++it)
    collectRequired(required, *it);
}


//########################################################################

bool vc_require_unit(Collector_p collector, const ModelReftime& model, const std::string& field_id, const std::string& unit)
{
  METLIBS_LOG_SCOPE(LOGVAL(model) << LOGVAL(field_id) << LOGVAL(unit));
  InventoryBase_cp field = vc_resolve_unit(collector->getResolver(), model, field_id, unit);
  if (not field)
    return false;
  collector->requireField(model, field);
  return true;
}

bool vc_require_surface(Collector_p collector, const ModelReftime& model)
{
  METLIBS_LOG_SCOPE();
  bool ok = vc_require_unit(collector, model, VC_SURFACE_PRESSURE, "hPa");
  ok     &= vc_require_unit(collector, model, VC_SURFACE_ALTITUDE, "m");
  return ok;
}

} // namespace vcross
