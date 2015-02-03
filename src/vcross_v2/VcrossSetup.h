
#ifndef VcrossSetup_h
#define VcrossSetup_h 1

#include <diField/VcrossData.h>
#include <diField/VcrossSource.h>

#include <set>
#include <string>
#include <vector>

namespace vcross {

typedef std::set<std::string> string_s;
typedef std::vector<std::string> string_v;

// ========================================================================

struct NameItem {
  std::string name;
  std::string function;
  string_v arguments;

  NameItem()
    { }

  NameItem(const std::string& n, const std::string& f)
    : name(n), function(f) { }

  bool valid() const
    { return (not name.empty()); }

  operator bool() const
    { return valid(); }
};
typedef std::vector<NameItem> NameItem_v;

// ========================================================================

struct ConfiguredPlot {
  enum Type {
    T_CONTOUR, //! 2D contour
    T_WIND,    //! 2D wind arrows (fixed length, flags)
    T_VECTOR,  //! 2D arrows with length ~ magnitude
    T_NONE
  };

  std::string name;
  Type type;
  string_v arguments;
  string_v options;

  ConfiguredPlot() : type(T_NONE) { }
  ConfiguredPlot(std::string n, Type t) : name(n), type(t) { };

  bool valid() const
    { return (type != T_NONE) and (not name.empty()); }

  operator bool() const
    { return valid(); }
};
typedef boost::shared_ptr<ConfiguredPlot> ConfiguredPlot_p;
typedef boost::shared_ptr<const ConfiguredPlot> ConfiguredPlot_cp;
typedef std::vector<ConfiguredPlot_p> ConfiguredPlot_pv;
typedef std::vector<ConfiguredPlot_cp> ConfiguredPlot_cpv;
typedef std::vector<ConfiguredPlot> ConfiguredPlot_v;

// ========================================================================

struct SyntaxError {
  int line;
  std::string message;
  SyntaxError(int l, std::string m) : line(l), message(m) { }
};
typedef std::vector<SyntaxError> SyntaxError_v;

// ========================================================================

NameItem parseComputationLine(const std::string& line);
ConfiguredPlot_cp parsePlotLine(const std::string& line);

class Setup {
public:
  typedef std::map<std::string,std::string> string_string_m;
  
  SyntaxError_v configureSources(const string_v& lines);
  SyntaxError_v configureComputations(const string_v& lines);
  SyntaxError_v configurePlots(const string_v& lines);

  string_v getAllModelNames() const;

  Source_p findSource(const std::string& name) const;
  ConfiguredPlot_cp findPlot(const std::string& name) const;

  const NameItem_v& getComputations() const
    { return mComputations; }
  const ConfiguredPlot_cpv& getPlots() const
    { return mPlots; }


  std::string getPlotOptions(const std::string& name) const
    { return getPlotOptions(findPlot(name)); }
  std::string getPlotOptions(ConfiguredPlot_cp cp) const;

  const string_string_m& getModelOptions(const std::string& name) const;

private:
  typedef std::map<std::string, Source_p> Source_p_m;
  Source_p_m mSources;

  typedef std::map<std::string, string_string_m> ModelOptions_m;
  ModelOptions_m mModelOptions;

  NameItem_v mComputations;
  ConfiguredPlot_cpv mPlots;
};

typedef boost::shared_ptr<Setup> Setup_p;

// ########################################################################

bool vc_configure(Setup_p setup, const string_v& sources,
    const string_v& computations, const string_v& plots);

} // namespace vcross

#endif // VcrossSetup_h
