
#include "VcrossCollector.h"
#include "VcrossComputer.h"
#include <diField/VcrossUtil.h>
#include <puTools/mi_boost_compatibility.hh>
#include <puTools/miStringFunctions.h>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

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
    const InventoryBase_cps fields = it->second; // take copy, not reference
    for (InventoryBase_cps::const_iterator it = fields.begin(); it != fields.end(); ++it)
      vcross::collectRequiredVertical(required, *it, zType);
  }
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
    METLIBS_LOG_DEBUG(LOGVAL(line) );

    string_v m_p_o = miutil::split(line);
    string_v poptions;
    std::string model, plotname;

    BOOST_FOREACH(const std::string& token, m_p_o) {
      string_v stoken = miutil::split(token,"=");
      if ( stoken.size() != 2 )
        continue;
      std::string key = boost::algorithm::to_lower_copy(stoken[0]);
      if (key == "model" ) {
        model = stoken[1];
      } else if ( key == "field" ) {
        plotname = stoken[1];
      } else {
        poptions.push_back(token);
      }
    }
    
    if (not collector->selectPlot(model, plotname, poptions))
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
