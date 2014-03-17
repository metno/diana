
#ifndef VCROSSCOLLECTOR_H
#define VCROSSCOLLECTOR_H 1

#include <diField/VcrossData.h>
#include "VcrossResolver.h"
#include "VcrossSetup.h"

namespace vcross {

struct SelectedPlot {
  ResolvedPlot_cp resolved;
  string_v options;

  std::string name() const
    { return resolved->name(); }
  std::string model;
};
typedef boost::shared_ptr<SelectedPlot> SelectedPlot_p;
typedef boost::shared_ptr<const SelectedPlot> SelectedPlot_cp;
typedef std::vector<SelectedPlot_p> SelectedPlot_pv;
typedef std::vector<SelectedPlot_cp> SelectedPlot_cpv;
typedef std::vector<SelectedPlot> SelectedPlot_v;

// ========================================================================

typedef std::map<std::string, InventoryBase_cps> model_required_m;

// ========================================================================

class Collector {
public:
  Collector(Setup_p setup);
  ~Collector();

  Setup_p getSetup() const
    { return mResolver->getSetup(); }
  void setupChanged();
  void clear();

  Resolver_p getResolver() const
    { return mResolver; }

  const model_required_m& getRequired() const
    { return mModelRequired; }

  const SelectedPlot_cpv& getSelectedPlots() const
    { return mSelectedPlots; }

  InventoryBase_cp getResolvedField(const std::string& model, const std::string& field)
    { return mResolver->getResolvedField(model, field); }

  const ResolvedPlot_cpv& getAllResolvedPlots(const std::string& model)
    { return mResolver->getAllResolvedPlots(model); }

  void requireField(const std::string& model, const std::string& field_id)
    { requireField(model, getResolvedField(model, field_id)); }

  void requireField(const std::string& model, InventoryBase_cp field);

  bool selectPlot(const std::string& model, const std::string& plot, const string_v& options);

private:
  Resolver_p mResolver;
  model_required_m mModelRequired;
  SelectedPlot_cpv mSelectedPlots;
};

typedef boost::shared_ptr<Collector> Collector_p;

// ########################################################################

void vc_select_plots(Collector_p collector, const string_v& to_plot);

bool vc_require_surface(Collector_p collector, const std::string& model);
bool vc_require_pressure_height(Collector_p collector, const std::string& model);

} // namespace vcross

#endif // VCROSSCOLLECTOR_H
