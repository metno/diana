
#ifndef VCROSSRESOLVER_H
#define VCROSSRESOLVER_H 1

#include <diField/VcrossData.h>
#include "VcrossSetup.h"

namespace vcross {

struct ResolvedPlot {
  ConfiguredPlot_cp configured;
  FieldData_cpv arguments;

  ResolvedPlot(ConfiguredPlot_cp c)
    : configured(c) { }
  std::string name() const
    { return configured->name; }
};

typedef boost::shared_ptr<ResolvedPlot> ResolvedPlot_p;
typedef boost::shared_ptr<const ResolvedPlot> ResolvedPlot_cp;
typedef std::vector<ResolvedPlot_p> ResolvedPlot_pv;
typedef std::vector<ResolvedPlot_cp> ResolvedPlot_cpv;
typedef std::vector<ResolvedPlot> ResolvedPlot_v;

// ========================================================================

class Resolver {
private:
  struct model_data {
    Source_p source;
    Inventory_cp inventory;
    InventoryBase_cps resolved_fields;
    ResolvedPlot_cpv resolved_plots;
  };
  typedef std::map<std::string, model_data> model_data_m;

public:
  Resolver(Setup_p setup);
  ~Resolver();

  Setup_p getSetup() const
    { return mSetup; }
  void setupChanged();

  Source_p getSource(const std::string& model);
  Inventory_cp getInventory(const std::string& model);
  const ResolvedPlot_cpv& getAllResolvedPlots(const std::string& model);
  ResolvedPlot_cp getResolvedPlot(const std::string& model, const std::string& plot);
  const InventoryBase_cpv& getAllResolvedFields(const std::string& model);
  InventoryBase_cp getResolvedField(const std::string& model, const std::string& field_id);
  
private:
  void resolveAllFields(model_data& md);
  void resolveAllPlots(model_data& md);
  model_data& getModelData(const std::string& model);

private:
  Setup_p mSetup;
  model_data_m mModelData;
};

typedef boost::shared_ptr<Resolver>       Resolver_p;

// ########################################################################

InventoryBase_cp vc_resolve_unit(Resolver_p resolver, const std::string& model,
    const std::string& field_id, const std::string& unit);

bool vc_resolve_surface(Resolver_p resolver, const std::string& model);
bool vc_resolve_pressure_height(Resolver_p resolver, const std::string& model);

} // namespace vcross

#endif // VCROSSRESOLVER_H
