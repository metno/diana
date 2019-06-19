
#include "VcrossCollector.h"
#include "VcrossComputer.h"
#include <diField/VcrossUtil.h>
#include <puTools/miStringFunctions.h>

#include <boost/algorithm/string/join.hpp>

#define MILOGGER_CATEGORY "vcross.Collector"
#include "miLogger/miLogging.h"

namespace vcross {

Collector::Collector(Setup_p setup)
  : mResolver(std::make_shared<Resolver>(setup))
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

int Collector::selectPlot(const ModelReftime& model, const std::string& plot, const miutil::KeyValue_v& options)
{
  return insertPlot(model, plot, options, -1);
}


int Collector::insertPlot(const ModelReftime& model, const std::string& plot, const miutil::KeyValue_v& options, int index)
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


bool Collector::hasVisiblePlot() const
{
  for (const SelectedPlot_p sp : mSelectedPlots)
    if (sp->visible)
      return true;
  return false;
}


ModelReftime Collector::getFirstModel() const
{
  for (const SelectedPlot_p sp : mSelectedPlots)
    if (sp->visible)
      return sp->model;
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
  for (auto& mr : mModelRequired) {
    InventoryBase_cps& required = mr.second;
    const InventoryBase_cps fields = mr.second; // take copy, not reference, as we modify it->second in the following loop
    for (InventoryBase_cp f : fields)
      vcross::collectRequiredVertical(required, f, zType);
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
  for (FieldData_cp fd : sp->resolved->arguments)
    collectRequired(required, fd);
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
