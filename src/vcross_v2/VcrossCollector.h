
#ifndef VCROSSCOLLECTOR_H
#define VCROSSCOLLECTOR_H 1

#include <diField/VcrossData.h>
#include "VcrossResolver.h"
#include "VcrossSetup.h"

namespace vcross {

struct SelectedPlot {
  ResolvedPlot_cp resolved;

  std::string model;
  string_v options;
  bool visible;

  std::string name() const
    { return resolved->name(); }

  std::string optionString() const;

  SelectedPlot() : visible(true) { }
};
typedef boost::shared_ptr<SelectedPlot> SelectedPlot_p;
typedef boost::shared_ptr<const SelectedPlot> SelectedPlot_cp;
typedef std::vector<SelectedPlot_p> SelectedPlot_pv;
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

  //! must be called when the setup changes
  void setupChanged();

  bool clear();

  Resolver_p getResolver() const
    { return mResolver; }

  const SelectedPlot_pv& getSelectedPlots() const
    { return mSelectedPlots; }

  InventoryBase_cp getResolvedField(const std::string& model, const std::string& field)
    { return mResolver->getResolvedField(model, field); }

  const ResolvedPlot_cpv& getAllResolvedPlots(const std::string& model)
    { return mResolver->getAllResolvedPlots(model); }

  bool updateRequired();
  void setUpdateRequiredNeeded()
    { mModelRequired.clear(); }
  const model_required_m& getRequired() const
    { return mModelRequired; }

  void requireVertical(Z_AXIS_TYPE zType);
  void requireField(const std::string& model, const std::string& field_id)
    { requireField(model, getResolvedField(model, field_id)); }
  void requireField(const std::string& model, InventoryBase_cp field);

  int selectPlot(const std::string& model, const std::string& plot, const string_v& options);
  int insertPlot(const std::string& model, const std::string& plot, const string_v& options, int index);
  bool removePlot(int index);

  //! returns the first model that has a visible SelectedPlot
  std::string getFirstModel() const;

private:
  void requirePlots();
  void requirePlot(SelectedPlot_p sp);

private:
  Resolver_p mResolver;
  SelectedPlot_pv mSelectedPlots;
  model_required_m mModelRequired;
};

typedef boost::shared_ptr<Collector> Collector_p;

// ########################################################################

bool vc_require_unit(Collector_p collector, const std::string& model, const std::string& field_id, const std::string& unit);
bool vc_require_surface(Collector_p collector, const std::string& model);

} // namespace vcross

#endif // VCROSSCOLLECTOR_H
