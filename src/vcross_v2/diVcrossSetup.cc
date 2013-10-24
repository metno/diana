/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2013 met.no

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

/*
 NOTES: Heavily based on old fortran code (1987-2001, A.Foss)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diVcrossSetup.h"

#include "diVcrossComputer.h"
#include "diVcrossUtil.h"
#include "diCommandParser.h"

#include <puTools/miSetupParser.h>
#include <puTools/miStringBuilder.h>

#include <boost/foreach.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>

#include <set>

#define MILOGGER_CATEGORY "diana.VcrossSetup"
#include <miLogger/miLogging.h>

using namespace miutil;

namespace /* anonymous*/ {

const char SECTION_FILES[] = "VERTICAL_CROSSECTION_FILES";
const char SECTION_PARAMETERS[] = "VERTICAL_CROSSECTION_PARAMETERS";
const char SECTION_COMPUTATIONS[] = "VERTICAL_CROSSECTION_COMPUTATIONS";
const char SECTION_PLOTS[] = "VERTICAL_CROSSECTION_PLOTS";

const std::string EMPTY_STRING;
const std::set<std::string> EMPTY_STRING_SET;

const VcrossComputer::FunctionLike vcrossPlotDefs[vcpt_no_plot] = {
  { "contour",  1, vcpt_contour  },
  { "wind",     2, vcpt_wind     },
  { "vector",   2, vcpt_vector   },
  { "line",     1, vcpt_line   },
};

} // namespace anonymous

// ########################################################################

void VcrossSetup::Parameter::addId(const std::string& id, const string_s& models)
{
  METLIBS_LOG_SCOPE();
  if (models.empty()) {
    globalid = id;
    METLIBS_LOG_DEBUG(LOGVAL(globalid));
  } else {
    BOOST_FOREACH(ParameterModelId& pmi, modelids) {
      if (pmi.id == id) {
        METLIBS_LOG_DEBUG(id << " -> '" << *models.begin() << "', ...");
        pmi.models.insert(models.begin(), models.end());
        return;
      }
    }
    ParameterModelId pmi;
    pmi.id = id;
    pmi.models = models;
    modelids.push_back(pmi);
  }
}

void VcrossSetup::Parameter::addId(const std::string& id, const std::string& model)
{
  METLIBS_LOG_SCOPE();
  if (model.empty()) {
    globalid = id;
    METLIBS_LOG_DEBUG(LOGVAL(globalid));
  } else {
    BOOST_FOREACH(ParameterModelId& pmi, modelids) {
      if (pmi.id == id) {
        METLIBS_LOG_DEBUG(id << " -> '" << model << "'");
        pmi.models.insert(model);
        return;
      }
    }
    ParameterModelId pmi;
    pmi.id = id;
    pmi.models.insert(model);
    modelids.push_back(pmi);
  }
}

VcrossSetup::string_s VcrossSetup::Parameter::allModels() const
{
  string_s all;
  BOOST_FOREACH(const ParameterModelId& pmi, modelids) {
    all.insert(pmi.models.begin(), pmi.models.end());
  }
  return all;
}

const std::string& VcrossSetup::Parameter::idForModel(const std::string& model) const
{
  BOOST_FOREACH(const ParameterModelId& pmi, modelids) {
    if (pmi.models.find(model) != pmi.models.end())
      return pmi.id;
  }
  return globalid;
}

// ########################################################################

VcrossSetup::VcrossSetup()
{
}

VcrossSetup::~VcrossSetup()
{
  METLIBS_LOG_SCOPE();
}

bool VcrossSetup::isDefinedName(const std::string& name) const
{
  if (mParameters.find(name) != mParameters.end())
    return true;
  
  if (vcFunctions.find(name) != vcFunctions.end())
    return true;

  return false;
}

const std::string& VcrossSetup::findParameterId(const std::string& model, const std::string& name) const
{
  parameters_t::const_iterator it = mParameters.find(name);
  if (it != mParameters.end())
    return it->second.idForModel(model);
  return EMPTY_STRING;
}

const std::string& VcrossSetup::findParameterName(const std::string& model, const std::string& id) const
{
  BOOST_FOREACH(const parameters_t::value_type& p, mParameters) {
    if (p.second.idForModel(model) == id)
      return p.first;
  }
  return EMPTY_STRING;
}

bool VcrossSetup::parseSetup()
{
  METLIBS_LOG_SCOPE();

  bool ok = true;
  ok &= parseModels();
  ok &= parseParameters();
  ok &= parseComputations();
  ok &= parsePlots();
  return ok;
}

bool VcrossSetup::parseModels()
{
  METLIBS_LOG_SCOPE();
  mModels.clear();
  std::vector<std::string> vstr;
  if (not miutil::SetupParser::getSection(SECTION_FILES, vstr))
    return true;

  bool ok = true;
  int l = -1;
  BOOST_FOREACH(const std::string& mdlLine, vstr) {
    l += 1;
    std::string msg = parseOneModel(mdlLine);
    if (not msg.empty()) {
      SetupParser::errorMsg(SECTION_PARAMETERS, l, msg);
      ok = false;
    }
  }
  return ok;
}

std::string VcrossSetup::parseOneModel(const std::string& mdlLine)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(mdlLine));

  const std::vector<miutil::KeyValue> kvs = miutil::SetupParser::splitManyKeyValue(mdlLine, true);
  if (kvs.size() !=2 or kvs[0].key != "m" or (kvs[1].key != "t" and kvs[1].key != "f"))
    return "need exactly m=... and f/t=...";

  const miutil::KeyValue& modelKV = kvs[0];
  const miutil::KeyValue& fileKV = kvs[1];

  Model m;
  const std::string& model = modelKV.value;

  if (fileKV.key == "f") {
    m.type = VCFILE;
    m.filename = fileKV.value;
  } else if (fileKV.value == "GribFile")
    m.type = VCFIELD;
  else
    return "unknown type '" + fileKV.value + "' for model '" + model + "'";

  mModels.insert(std::make_pair(model, m));
  METLIBS_LOG_DEBUG(LOGVAL(model) << LOGVAL(m.type));
  return "";
}

bool VcrossSetup::parseParameters()
{
  mParameters.clear();
  std::vector<std::string> vstr;
  if (not miutil::SetupParser::getSection(SECTION_PARAMETERS, vstr))
    return true;

  METLIBS_LOG_SCOPE();
  bool ok = true;
  int l = -1;
  BOOST_FOREACH(const std::string& parLine, vstr) {
    l += 1;
    const std::vector<miutil::KeyValue> kvs = miutil::SetupParser::splitManyKeyValue(parLine, true);
    BOOST_FOREACH(const miutil::KeyValue& kv, kvs) {
      std::string msg = parseOneParameter(kv);
      if (not msg.empty()) {
        SetupParser::errorMsg(SECTION_PARAMETERS, l, msg);
        ok = false;
      }
    }
  }
  return ok;
}

std::string VcrossSetup::parseOneParameter(const KeyValue& kv)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(kv.key) << LOGVAL(kv.value));

  std::string parname = kv.key;
  string_s models;
  {
    const size_t open = kv.key.find('[');
    if (open != std::string::npos) {
      if (kv.key.at(kv.key.size()-1) != ']') // cannot be empty if it contains '['
        return "model specification for parameter must end with ']'";
      VcrossUtil::set_insert(models, miutil::split(kv.key.substr(open+1, kv.key.size()-open-2), ","));
      BOOST_FOREACH(const std::string& m, models) {
        if (m.empty())
          return "empty model name specified for parameter";
        if (mModels.find(m) == mModels.end())
          return "unknown model name '" + m + "' specified for parameter";
      }
      parname = parname.substr(0, open);
    }
  }

  if (parname.empty() or kv.value.empty())
    return "Empty parameter definition";

  METLIBS_LOG_DEBUG(LOGVAL(parname));
  mParameters[parname].addId(kv.value, models);
  return "";
}

bool VcrossSetup::parseComputations()
{
  METLIBS_LOG_SCOPE();

  vcFunctions.clear();
  std::vector<std::string> vstr;
  if (not SetupParser::getSection(SECTION_COMPUTATIONS, vstr))
    return true;

  bool ok = true;
  int l = -1;
  BOOST_FOREACH(const std::string& compLine, vstr) {
    l += 1;
    const std::vector<KeyValue> kvs = miutil::SetupParser::splitManyKeyValue(compLine);
    BOOST_FOREACH(const KeyValue& kv, kvs) {
      std::string msg = parseOneComputation(kv);
      if (not msg.empty()) {
        SetupParser::errorMsg(SECTION_COMPUTATIONS, l, msg);
        ok = false;
      }
    }
  }
  return ok;
}

std::string VcrossSetup::parseOneComputation(const KeyValue& kv)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(kv.key) << LOGVAL(kv.value));

  if (kv.value.empty())
    return "Empty computation definition";

  const std::string& name = kv.key;
  if (isDefinedName(name))
    return "Name '" + name + "' already defined";

  std::string fname;
  std::vector<std::string> fargs;
  if (not VcrossUtil::parseNameAndArgs(miutil::to_lower(kv.value), fname, fargs))
    return "Error parsing computation definition '" + kv.value + "'";

  const VcrossComputer::FunctionLike* cf = VcrossComputer::findFunction(fname);
  if (not cf or (cf->id < VcrossComputer::vcf_add) or (cf->id >= VcrossComputer::vcf_no_function))
    return "Function '" + fname + "' not known";

  if (cf->argc != fargs.size())
    return (miutil::StringBuilder() << "Function '" <<  fname << "' expects "
        << cf->argc << " arguments, not " << fargs.size()).str();

  vcFunction::parameters_t fparameters;
  BOOST_FOREACH(const std::string& a, fargs) {
    const parameters_t::const_iterator p_it = mParameters.find(a);
    if (p_it != mParameters.end()) {
      fparameters.insert(a);
      continue;
    }
    const vf_t::const_iterator f_it = vcFunctions.find(a);
    if (f_it != vcFunctions.end()) {
      fparameters.insert(f_it->second.parameters.begin(), f_it->second.parameters.end());
      continue;
    }
    
    return "Argument '" + a + "' not known";
  }

  vcFunction vcfunc;
  vcfunc.function = static_cast<VcrossComputer::VcrossFunction>(cf->id);
  vcfunc.vars = fargs;
  vcfunc.parameters = fparameters;
  vcFunctions.insert(std::make_pair(name, vcfunc));
  return "";
}

bool VcrossSetup::parsePlots()
{
  METLIBS_LOG_SCOPE();

  mPlots.clear();
  std::vector<std::string> vstr;
  if (not SetupParser::getSection(SECTION_PLOTS, vstr))
    return true;

  bool ok = true;
  int l = -1;
  BOOST_FOREACH(const std::string& plotLine, vstr) {
    l += 1;
    std::string msg = parseOnePlot(plotLine);
    if (not msg.empty()) {
      SetupParser::errorMsg(SECTION_PLOTS, l, msg);
      ok = false;
    }
  }
  return ok;
}

std::string VcrossSetup::parseOnePlot(const std::string& plotLine)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(plotLine));

  vcPlot vcp;

  enum { NOTHING = 0, HAVE_NAME = 1<<0, HAVE_PLOTTYPE = 1<<1 };
  int have = NOTHING;

  const std::vector<KeyValue> kvs = miutil::SetupParser::splitManyKeyValue(plotLine);
  BOOST_FOREACH(const KeyValue& kv, kvs) {
    const std::string value = miutil::to_lower(kv.value);
    if (not kv.value.empty()) {
      if (kv.key == "name") {
        if (mPlots.find(value) != mPlots.end())
          return "Plot name '" + value + "' already used";
        
        vcp.name = kv.value; // not in lower case
        have = HAVE_NAME;
      } else if (kv.key == "plot") {
        std::string ptype;
        std::vector<std::string> pargs;
        if (not VcrossUtil::parseNameAndArgs(value, ptype, pargs))
          return "Error parsing plot type '" + value + "'";
        METLIBS_LOG_DEBUG(LOGVAL(ptype));

        const VcrossComputer::FunctionLike* pt = 0;
        for (int i=0; i<vcpt_no_plot; ++i) {
          if (ptype == vcrossPlotDefs[i].name) {
            pt = &vcrossPlotDefs[i];
            break;
          }
        }
        if (not pt)
          return "Plot type '" + ptype + "' not known";

        if (pt->argc != pargs.size())
          return (miutil::StringBuilder() << "Plot type '" << ptype << "' expects "
              << pt->argc << " arguments, not " << pargs.size()).str();
        
        vcPlot::parameters_t pparameters;
        BOOST_FOREACH(const std::string& a, pargs) {
          const parameters_t::const_iterator p_it = mParameters.find(a);
          if (p_it != mParameters.end()) {
            pparameters.insert(a);
            continue;
          }
          const vf_t::const_iterator f_it = vcFunctions.find(a);
          if (f_it != vcFunctions.end()) {
            pparameters.insert(f_it->second.parameters.begin(), f_it->second.parameters.end());
            continue;
          }
          
          return "Plot argument '" + a + "' not known";
        }
        vcp.vars = pargs;
        vcp.parameters = pparameters;
        vcp.plotType = static_cast<VCPlotType>(pt->id);
        METLIBS_LOG_DEBUG(LOGVAL(pt->id));
        have |= HAVE_PLOTTYPE;
      } else {
        // can only hope this is a sensible plot option...
        miutil::appendTo(vcp.plotOpts, " ", kv.key);
      }
    } else {
      // any plot options without key=value syntax ?????
      miutil::appendTo(vcp.plotOpts, " ", kv.key);
    }
  }

  if (have != (HAVE_NAME | HAVE_PLOTTYPE))
    return "Plot definition without name and/or plot type";

  mPlots[vcp.name] = vcp;
  METLIBS_LOG_DEBUG("plot '" << vcp.name << "'");

  return "";
}

const VcrossSetup::string_s& VcrossSetup::getPlotParameterNames(const std::string& plotname) const
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(plotname));

  const Plots_t::const_iterator p_it = mPlots.find(plotname);
  if (p_it == mPlots.end()) {
    METLIBS_LOG_DEBUG("unknown plot");
    return EMPTY_STRING_SET;
  }

  return p_it->second.parameters;
}

FileContents VcrossSetup::makeContents(const std::string& model, const string_s& paramIds) const
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(model));

  FileContents fco;

  std::set<std::string> definedNames;
  std::set<std::string> paramNames;
  BOOST_FOREACH(const std::string& id, paramIds) {
    const std::string& pname = findParameterName(model, id);
    METLIBS_LOG_DEBUG(LOGVAL(id) << LOGVAL(pname));
    if (not pname.empty()) {
      paramNames.insert(pname);
      definedNames.insert(pname);
    }
  }

  while (true) {
    std::set<std::string> definedTmp;

    BOOST_FOREACH(const vf_t::value_type& vf, vcFunctions) {
      const std::string& name = vf.first;
      METLIBS_LOG_DEBUG("computation '" << name << "'");

      if (definedNames.find(name) != definedNames.end()) {
        METLIBS_LOG_DEBUG(" name '" << name << "' already defined");
        continue;
      }

      std::set<std::string> functionParams;
      bool allVarsFound = true;
      BOOST_FOREACH(const std::string& var, vf.second.vars) {
        if (paramNames.find(var) != paramNames.end()) {
          METLIBS_LOG_DEBUG("  function uses parameter '" << var << "' as argument");
          functionParams.insert(name);
        } else if (definedNames.find(var) == definedNames.end()) {
          METLIBS_LOG_DEBUG("  function uses undefined name '" << var << "', stop");
          allVarsFound = false;
          break;
        }
      }
      if (allVarsFound) {
        METLIBS_LOG_DEBUG("  adding defined name '" << name << "' for this function");
        definedTmp.insert(name);
        BOOST_FOREACH(const std::string& pname, functionParams) {
          const std::string pid = findParameterId(model, pname);
          if (not pid.empty())
            fco.useParams.insert(pid);
        }
        fco.useFunctions[name] = vf.second;
      }
    }

    if (not definedTmp.empty()) {
      definedNames.insert(definedTmp.begin(), definedTmp.end());

      // stay in while loop until no more new functions appear
    } else {
      break;
    }
  }

  BOOST_FOREACH(const vcPlot& vcf, boost::adaptors::values(mPlots)) {
    METLIBS_LOG_DEBUG("plot '" << vcf.name << "'");
    bool allVarsFound = true;
    BOOST_FOREACH(const std::string& var, vcf.vars) {
      if (definedNames.find(var) == definedNames.end()) {
        METLIBS_LOG_DEBUG("  plot uses undefined name '" << var << "', stop");
        allVarsFound = false;
        break;
      }
    }
    if (allVarsFound) {
      METLIBS_LOG_DEBUG("  adding plot '" << vcf.name << "' for model '" << model << "'");
      fco.plotNames.push_back(vcf.name);
    }
  }

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("  no. of output fields: "<<fco.fieldNames.size());
  for (int j=0; j<fco.fieldNames.size(); j++)
    METLIBS_LOG_DEBUG(setw(5)<<j<<": "<<fco.fieldNames[j]);
#endif

  return fco;
}

VcrossSetup::PlotOptions_t VcrossSetup::getAllPlotOptions()
{
  METLIBS_LOG_SCOPE();

  PlotOptions_t plotopts;

  BOOST_FOREACH(vcPlot& vcf, boost::adaptors::values(mPlots)) {
    PlotOptions po;
    PlotOptions::parsePlotOption(vcf.plotOpts, po);

    if (po.linetype.name.length() == 0)
      po.linetype = Linetype::getDefaultLinetype();
    if (po.undefLinetype.name.length() == 0)
      po.undefLinetype = Linetype::getDefaultLinetype();

    std::ostringstream ostr;

    if (vcf.plotType == vcpt_contour) {
      bool usebase = false;
      if (po.colours.size() < 2 || po.colours.size() > 3) {
        ostr << "colour=" << po.linecolour.Name();
      } else {
        ostr << "colours=" << po.colours[0].Name();
        for (unsigned int j = 1; j < po.colours.size(); j++)
          ostr << "," << po.colours[j].Name();
        usebase = true;
      }
      if (po.linetypes.size() < 2 || po.linetypes.size() > 3) {
        ostr << " linetype=" << po.linetype.name;
      } else {
        ostr << " linetypes=" << po.linetypes[0].name;
        for (unsigned int j = 1; j < po.linetypes.size(); j++)
          ostr << "," << po.linetypes[j].name;
        usebase = true;
      }
      if (po.linewidths.size() < 2 || po.linewidths.size() > 3) {
        ostr << " linewidth=" << po.linewidth;
      } else {
        ostr << " linewidths=" << po.linewidths[0];
        for (unsigned int j = 1; j < po.linewidths.size(); j++)
          ostr << "," << po.linewidths[j];
        usebase = true;
      }
      if (po.linevalues.size() == 0 && po.loglinevalues.size() == 0) {
        ostr << " line.interval=" << po.lineinterval;
        if (po.zeroLine >= 0)
          ostr << " zero.line=" << po.zeroLine;
      }
      ostr << " line.smooth=" << po.lineSmooth << " value.label="
          << po.valueLabel << " label.size=" << po.labelSize;
      ostr << " base=" << po.base;
      if (po.minvalue > -fieldUndef)
        ostr << " minvalue=" << po.minvalue;
      else
        ostr << " minvalue=off";
      if (po.maxvalue < fieldUndef)
        ostr << " maxvalue=" << po.maxvalue;
      else
        ostr << " maxvalue=off";
      if (!usebase) {
        int n = po.palettecolours.size();
        if (n > 0) {
          ostr << " palettecolours=" << po.palettecolours[0];
          for (int j = 1; j < n; j++)
            ostr << "," << po.palettecolours[j].Name();
        } else {
          ostr << " palettecolours=off";
        }
        ostr << " table=" << po.table;
        ostr << " repeat=" << po.repeat;
      }
    } else if (vcf.plotType == vcpt_wind) {
      ostr << "colour=" << po.linecolour.Name() << " linewidth="
          << po.linewidth << " density=" << po.density;
    } else if (vcf.plotType == vcpt_vector) {
      ostr << "colour=" << po.linecolour.Name() << " linewidth="
          << po.linewidth << " density=" << po.density << " vector.unit="
          << po.vectorunit;
    } else {
      ostr << "colour=" << po.linecolour.Name();
    }

    plotopts[vcf.name] = ostr.str();
  }

  return plotopts;
}

std::string VcrossSetup::getPlotOptions(const std::string& plotname)
{
  METLIBS_LOG_SCOPE();

  const Plots_t::const_iterator vcf = mPlots.find(miutil::to_lower(plotname));
  if (vcf == mPlots.end())
    return std::string();
  
  return vcf->second.plotOpts;
}


VcrossSetup::string_v VcrossSetup::getAllModelNames() const
{
  METLIBS_LOG_SCOPE();

  std::vector<std::string> modelnames;
  boost::copy(boost::adaptors::keys(mModels), std::back_inserter(modelnames));
  return modelnames;
}

VcrossSetup::FileType VcrossSetup::getModelFileType(const std::string& model) const
{
  METLIBS_LOG_SCOPE();

  const models_t::const_iterator it = mModels.find(model);
  if (it == mModels.end())
    return VCUNKNOWN;

  return it->second.type;
}

const std::string& VcrossSetup::getModelFileName(const std::string& model) const
{
  METLIBS_LOG_SCOPE();

  const models_t::const_iterator it = mModels.find(model);
  if (it == mModels.end())
    return EMPTY_STRING;

  return it->second.filename;
}
