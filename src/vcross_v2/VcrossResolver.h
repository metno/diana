
#ifndef VCROSSRESOLVER_H
#define VCROSSRESOLVER_H 1

#include "VcrossSetup.h"
#include <diField/VcrossData.h>
#include <iosfwd>

namespace vcross {

struct ModelReftime {
  std::string model;
  Time reftime;
  ModelReftime(const std::string& m, const Time& rt)
    : model(m), reftime(rt) { }
  ModelReftime() { }

  bool valid() const;

  bool operator==(const ModelReftime& other) const;

  bool operator!=(const ModelReftime& other) const
    { return !(*this == other); }
};

struct lt_ModelReftime : public std::binary_function<bool, ModelReftime, ModelReftime> {
  bool operator()(const ModelReftime& mr1, const ModelReftime& mr2) const;
};

std::ostream& operator<<(std::ostream& out, const ModelReftime& mr);

// ========================================================================

struct ResolvedPlot {
  ConfiguredPlot_cp configured;
  FieldData_cpv arguments;

  ResolvedPlot(ConfiguredPlot_cp c)
    : configured(c) { }
  const std::string& name() const
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
  typedef std::map<ModelReftime, model_data, lt_ModelReftime> model_data_m;

public:
  Resolver(Setup_p setup);
  ~Resolver();

  Setup_p getSetup() const
    { return mSetup; }
  void setupChanged();

  Source_p getSource(const std::string& modelName);
  Inventory_cp getInventory(const ModelReftime& model);
  const ResolvedPlot_cpv& getAllResolvedPlots(const ModelReftime& model);
  ResolvedPlot_cp getResolvedPlot(const ModelReftime& model, const std::string& plot);
  const InventoryBase_cpv& getAllResolvedFields(const ModelReftime& model);
  InventoryBase_cp getResolvedField(const ModelReftime& model, const std::string& field_id);

private:
  void resolveAllFields(model_data& md);
  void resolveAllPlots(model_data& md);
  model_data& getModelData(const ModelReftime& model);

private:
  Setup_p mSetup;
  model_data_m mModelData;
};

typedef boost::shared_ptr<Resolver> Resolver_p;

// ########################################################################

InventoryBase_cp vc_resolve_unit(Resolver_p resolver, const ModelReftime& model,
    const std::string& field_id, const std::string& unit);

bool vc_resolve_surface(Resolver_p resolver, const ModelReftime& model);

Crossection_cp vc_add_dynamic_point(Source_p sp, const std::string& pointName, const LonLat& pos);

} // namespace vcross

#endif // VCROSSRESOLVER_H
