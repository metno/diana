/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013 met.no

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
#ifndef diVcrossSetup_h
#define diVcrossSetup_h

#include "diColour.h"
#include "diPlotOptions.h"
#include "diPrintOptions.h"
#include "diVcrossComputer.h"

#include <diField/diField.h>
#include <puTools/miTime.h>

#include <boost/bimap/bimap.hpp>

#include <map>
#include <set>
#include <vector>

namespace miutil {
class KeyValue;
}

enum VCPlotType {
  vcpt_contour, //! 2D contour
  vcpt_wind,    //! 2D wind arrows (fixed length, flags)
  vcpt_vector,  //! 2D arrows with length ~ magnitude
  vcpt_line,    //! 1D line (topography etc.)
  vcpt_no_plot
};

/// Vertical Crossection computations from setup file
struct vcFunction {
  typedef std::vector<std::string> vars_t;
  typedef std::set<std::string> parameters_t;

  VcrossComputer::VcrossFunction function;
  vars_t vars;
  parameters_t parameters; //! parameters required to calculate this function; includes parameters needed to calculate functions used as input
};

/// Vertical Crossection plottable fields from setup file
struct vcPlot {
  typedef std::vector<std::string> vars_t;
  typedef std::set<std::string> parameters_t;

  std::string name;
  vars_t vars;
  parameters_t parameters; //! parameters required to calculate this plot; includes parameters needed to calculate functions used as input
  VCPlotType plotType;
  std::string plotOpts;
};

/// Vertical Crossection data contents incl. possible computations
struct FileContents {
  std::vector<std::string> plotNames;
  std::set<std::string> useParams;
  std::map<std::string,vcFunction> useFunctions;
};
//----------------------------------------------------

/**
   \brief Configuration for vertical cross sections from diana.setup file
*/
class VcrossSetup
{
public:
  enum FileType { VCUNKNOWN, VCFILE, VCFIELD };

private:
  typedef std::vector<std::string> string_v;
  typedef std::set<std::string> string_s;
  typedef std::map<std::string, std::string> string2string_t;

  struct Model {
    FileType type;
    std::string filename; // only used for VCFILE
    string2string_t options;
  };

  struct ParameterModelId {
    std::string id; //! id is the name used in the model (may be an integer)
    string_s models; //! models for which this id applies
  };

  struct Parameter {
    std::string globalid; //! id is the name used in all models which are not explicitly specified
    std::vector<ParameterModelId> modelids;

    /*! add an id for the this parameter in the given models, or globally if the model set is empty */
    void addId(const std::string& id, const string_s& models);

    /*! add an id for the this parameter in the given model, or globally if the model name is empty */
    void addId(const std::string& id, const std::string& model);

    /*! @return all models for which the parameter is defined */
    string_s allModels() const;

    /*! @return the id for the given model, or empty if not defined */
    const std::string& idForModel(const std::string& model) const;

    /*! @return true iff the parameter has an id for the given model */
    bool hasIdForModel(const std::string& model) const
      { return not idForModel(model).empty(); }
  };

public:
  VcrossSetup();
  ~VcrossSetup();

  /** Fetch vcross contiguration from SetupParser and define parameters, computations, and plots.
   * @return true if no error
   */
  bool parseSetup();

  /** Find a parameter id as defined in the setup
   * @return parameter id, or an empty string if not found
   */
  const std::string& findParameterId(const std::string& model, const std::string& name) const;

  /** Find a parameter's name as defined in the setup
   * @return parameter name, or an empty string if not found
   */
  const std::string& findParameterName(const std::string& model, const std::string& id) const;

  /** Get plotoptions, as parsed in parseSetup, for a given plot name
   * @return plot options, or an empty string if plot is not known*/
  std::string getPlotOptions(const std::string& plotname);
  
  typedef string2string_t PlotOptions_t;
  PlotOptions_t getAllPlotOptions();

  bool isFunction(const std::string& resultname) const
    { return vcFunctions.find(resultname) != vcFunctions.end(); }
  VcrossComputer::VcrossFunction getFunctionType(const std::string& resultname) const
    { return vcFunctions.find(resultname)->second.function; }
  const string_v& getFunctionArguments(const std::string& resultname) const
    { return vcFunctions.find(resultname)->second.vars; }

  const string_s& getPlotParameterNames(const std::string& plotname) const;
  const string_v& getPlotArguments(const std::string& plotname) const
    { return mPlots.find(plotname)->second.vars; }

  VCPlotType getPlotType(const std::string& plotname) const
    { return mPlots.find(plotname)->second.plotType; }
  const std::string& getPlotOptions(const std::string& plotname) const
    { return mPlots.find(plotname)->second.plotOpts; }

  FileContents makeContents(const std::string& model, const string_s& parameterIds) const;

  string_v getAllModelNames() const;
  FileType getModelFileType(const std::string& model) const;
  const std::string& getModelFileName(const std::string& model) const;

private:
  bool parseModels();
  std::string parseOneModel(const std::string& mdlLine);
  bool parseParameters();
  std::string parseOneParameter(const miutil::KeyValue& kv);
  bool parseComputations();
  std::string parseOneComputation(const miutil::KeyValue& kv);
  bool parsePlots();
  std::string parseOnePlot(const std::string& plotLine);

  int findParam(const std::string& var);
  int computer(const std::string& var, int vcfunc,
	       const std::vector<int>& parloc);

  /*! @return true iff a model by this name is defined */
  bool hasModel(const std::string& name) const
    { return mModels.find(name) != mModels.end(); }

  bool isDefinedName(const std::string& name) const;

private:
  typedef std::map<std::string, Parameter> parameters_t;
  typedef std::map<std::string, Model> models_t;

  parameters_t mParameters;
  models_t mModels;

  typedef std::multimap<std::string, vcFunction> vf_t;
  vf_t vcFunctions;

  typedef std::map<std::string, vcPlot> Plots_t;
  Plots_t mPlots;
};

#endif // diVcrossSetup_h
