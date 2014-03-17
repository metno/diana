
#include "VcrossCollector.h"
#include "VcrossComputer.h"
#include <diField/VcrossUtil.h>
#include <puTools/mi_boost_compatibility.hh>
#include <puTools/miStringFunctions.h>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

#define MILOGGER_CATEGORY "vcross.Collector"
#include "miLogger/miLogging.h"

namespace vcross {

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

void Collector::clear()
{
  mModelRequired.clear();
  mSelectedPlots.clear();
}

// ------------------------------------------------------------------------

void Collector::requireField(const std::string& model, InventoryBase_cp field)
{
  METLIBS_LOG_SCOPE();
  if (not field)
    return;

  InventoryBase_cps& required = mModelRequired[model];
  collectRequired(required, field);
}

// ------------------------------------------------------------------------

bool Collector::selectPlot(const std::string& model, const std::string& plot, const string_v& options)
{
  METLIBS_LOG_SCOPE(LOGVAL(model) << LOGVAL(plot));
  ResolvedPlot_cp rp = mResolver->getResolvedPlot(model, plot);
  if (not rp)
    return false;

  SelectedPlot_p sp(new SelectedPlot);
  sp->model = model;
  sp->resolved = rp;
  sp->options = options;
  mSelectedPlots.push_back(sp);

  InventoryBase_cps& required = mModelRequired[model];
  BOOST_FOREACH(const FieldData_cp& a, rp->arguments) {
    collectRequired(required, a);
    if (a->hasZAxis())
      required.insert(a->zaxis());
  }

  return true;
}

//########################################################################

void vc_select_plots(Collector_p collector, const string_v& to_plot)
{
  METLIBS_LOG_SCOPE();

  collector->clear();
  BOOST_FOREACH(const std::string& line, to_plot) {
    if (util::isCommentLine(line))
      continue;

    string_v m_p_o = miutil::split(line, 2);
    if (m_p_o.size() < 2) {
      METLIBS_LOG_DEBUG(LOGVAL(line) << " split to " << m_p_o.size());
      continue;
    }

    string_v::iterator it = m_p_o.begin();
    const std::string model = *it++, plotname = *it++;
    m_p_o.erase(m_p_o.begin(), it);
    
    if (not collector->selectPlot(model, plotname, m_p_o))
      METLIBS_LOG_WARN("could not select plot '" << plotname << "' for model '" << model << "'");
  }
}

//########################################################################

bool vc_require_unit(Collector_p collector, const std::string& model, const std::string& field_id, const std::string& unit)
{
  METLIBS_LOG_SCOPE(LOGVAL(model) << LOGVAL(field_id) << LOGVAL(unit));
  InventoryBase_cp field = vc_resolve_unit(collector->getResolver(), model, field_id, unit);
  if (not field)
    return false;
  collector->requireField(model, field);
  return true;
}

bool vc_require_surface(Collector_p collector, const std::string& model)
{
  METLIBS_LOG_SCOPE();
  bool ok = vc_require_unit(collector, model, VC_SURFACE_PRESSURE, "hPa");
  ok     &= vc_require_unit(collector, model, VC_SURFACE_HEIGHT,   "m");
  return ok;
}

bool vc_require_pressure_height(Collector_p collector, const std::string& model)
{
  METLIBS_LOG_SCOPE();
  bool ok = vc_require_surface(collector, model);
  ok &= vc_require_unit(collector, model, VC_SPECIFIC_HUMIDITY, "1");
  ok &= vc_require_unit(collector, model, VC_AIR_TEMPERATURE,   "K");
  return ok;
}

} // namespace vcross
