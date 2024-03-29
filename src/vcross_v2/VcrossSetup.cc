/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2014-2022 met.no

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

#include "VcrossSetup.h"

#include "diPlotOptions.h"
#include "diUtilities.h"
#include "miSetupParser.h"

#include "diField/FimexSource.h"
#include "util/string_util.h"

#include <puTools/miStringFunctions.h>

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/map.hpp>

#include <sstream>

#define MILOGGER_CATEGORY "vcross.Setup"
#include "miLogger/miLogging.h"

namespace vcross {

namespace /* anonymous */ {
bool parseFunctionWithArgs(std::string function, NameItem& ni, bool need_parens=false)
{
  const std::string::size_type pos_open = function.find_first_of("(");
  const std::string::size_type pos_close = function.find_last_of(")");

  const bool have_open = (pos_open != std::string::npos),
      have_close = (pos_close != std::string::npos);
  if (have_open or have_close) {
    if (have_open != have_close)
      return false;

    if (not miutil::trimmed(function.substr(pos_close+1)).empty())
      return false;

    ni.arguments = miutil::split(function.substr(pos_open+1, pos_close-pos_open-1), ",");
    function = function.substr(0, pos_open);
  } else if (need_parens) {
    return false;
  }
  ni.function = miutil::trimmed(function);
  return true;
}

// ========================================================================

const vcross::Setup::string_string_m EMPTY_STRING_STRING_MAP;

} // anonymous namespace

// ========================================================================

NameItem parseComputationLine(const std::string& line)
{
  METLIBS_LOG_SCOPE(LOGVAL(line));
  const std::string::size_type pos_eq = line.find('=');
  if (pos_eq == std::string::npos)
    return NameItem();

  NameItem ni;
  ni.name = miutil::trimmed(line.substr(0, pos_eq));
  if (diutil::startswith(ni.name, "__"))
    return NameItem();

  if (parseFunctionWithArgs(line.substr(pos_eq+1), ni))
    return ni;
  else
    return NameItem();
}

// ========================================================================

ConfiguredPlot_cp parsePlotLine(const std::string& line)
{
  METLIBS_LOG_SCOPE();
  ConfiguredPlot_p pc(new ConfiguredPlot);

  const miutil::KeyValue_v kvs = miutil::SetupParser::splitManyKeyValue(line, true);
  for (const miutil::KeyValue& kv : kvs) {
    METLIBS_LOG_DEBUG(LOGVAL(kv.key()) << LOGVAL(kv.value()));
    if (kv.key() == "name") {
      pc->name = kv.value();
    } else if (kv.key() == "plot") {
      NameItem ni;
      if (not parseFunctionWithArgs(kv.value(), ni, true))
        return ConfiguredPlot_cp();

      pc->arguments = ni.arguments;
      if ((ni.function == "contour" or ni.function == "CONTOUR") and ni.arguments.size() == 1) {
        pc->type = ConfiguredPlot::T_CONTOUR;
      } else if ((ni.function == "wind" or ni.function == "WIND") and ni.arguments.size() == 2) {
        pc->type = ConfiguredPlot::T_WIND;
      } else if ((ni.function == "vector" or ni.function == "VECTOR") and ni.arguments.size() == 2) {
        pc->type = ConfiguredPlot::T_VECTOR;
      } else {
        return ConfiguredPlot_cp();
      }
    } else {
      pc->options.push_back(kv);
    }
  }

  return pc;
}

// ========================================================================

SyntaxError_v Setup::configureSources(const string_v& lines)
{
  METLIBS_LOG_SCOPE();
  SyntaxError_v errors;

  mSources.clear();
  for (size_t l=0; l<lines.size(); ++l) {
    if (lines[l].empty() or lines[l][0] == '#')
      continue;

    METLIBS_LOG_DEBUG(LOGVAL(lines[l]));
    const miutil::KeyValue_v kvs = miutil::SetupParser::splitManyKeyValue(lines[l], true);
    std::string name, filename, filetype, fileconfig, cs_name_charset = diutil::CHARSET_READ();
    string_string_m options;
    for (const miutil::KeyValue& kv : kvs) {
      if (kv.key() == "m")
        name = kv.value();
      else if (kv.key() == "f")
        filename = kv.value();
      else if (kv.key() == "t") {
        if (kv.value() != "fimex")
          filetype = kv.value();
        else
          METLIBS_LOG_INFO("ignoring filetype 't=fimex', it is not valid for vertical cross-sections");
      } else if (kv.key() == "format")
        filetype = kv.value();
      else if (kv.key() == "c" or kv.key() == "config")
        fileconfig = kv.value();
      else if (kv.key() == "cs_name_charset")
        cs_name_charset = kv.value();
      else  {
        METLIBS_LOG_DEBUG(LOGVAL(kv.key()) << LOGVAL(kv.value()));
        options[kv.key()] = kv.value();
      }
    }
    if (name.empty() or filename.empty()) {
      errors.push_back(SyntaxError(l, "name and filename required"));
    } else if (mSources.find(name) != mSources.end()) {
      errors.push_back(SyntaxError(l, "name '" + name + "' already used"));
    } else {
      diutil::CharsetConverter_p charsetConverter = diutil::findConverter(cs_name_charset, diutil::CHARSET_INTERNAL());
      Source_p src = std::make_shared<FimexSource>(filename, filetype, fileconfig, charsetConverter);
      mSources.insert(std::make_pair(name, src));
      mModelOptions[name] = options;
    }
  }

  return errors;
}

SyntaxError_v Setup::configureComputations(const string_v& lines)
{
  METLIBS_LOG_SCOPE();
  SyntaxError_v errors;

  mComputations.clear();
  for (size_t l=0; l<lines.size(); ++l) {
    if (lines[l].empty() or lines[l][0] == '#')
      continue;

    const NameItem ni = parseComputationLine(lines[l]);
    if (ni.valid())
      mComputations.push_back(ni);
    else
      errors.push_back(SyntaxError(l, "parse error for computation"));
  }

  return errors;
}

SyntaxError_v Setup::configurePlots(const string_v& lines)
{
  METLIBS_LOG_SCOPE();
  SyntaxError_v errors;

  mPlots.clear();
  for (size_t l=0; l<lines.size(); ++l) {
    if (lines[l].empty() or lines[l][0] == '#')
      continue;

    METLIBS_LOG_DEBUG(LOGVAL(lines[l]));
    ConfiguredPlot_cp ps = parsePlotLine(lines[l]);
    if (ps and ps->valid())
      mPlots.push_back(ps);
    else
      errors.push_back(SyntaxError(l, "parse error for plot"));
  }

  return errors;
}

string_v Setup::getAllModelNames() const
{
  return string_v(boost::adaptors::keys(mSources).begin(), boost::adaptors::keys(mSources).end());
}

Source_p Setup::findSource(const std::string& name) const
{
  Source_p_m::const_iterator it = mSources.find(name);
  if (it != mSources.end())
    return it->second;
  else
    return Source_p();
}

ConfiguredPlot_cp Setup::findPlot(const std::string& name) const
{
  for (ConfiguredPlot_cp cp : mPlots) {
    if (cp->name == name)
      return cp;
  }
  
  return ConfiguredPlot_cp();
}

miutil::KeyValue_v Setup::getPlotOptions(ConfiguredPlot_cp cp) const
{
  METLIBS_LOG_SCOPE();

  if (!cp)
    return miutil::KeyValue_v();

  PlotOptions po;
  PlotOptions::parsePlotOption(cp->options, po);
  
  if (po.linetype.name.length() == 0)
    po.linetype = Linetype::getDefaultLinetype();
  if (po.undefLinetype.name.length() == 0)
    po.undefLinetype = Linetype::getDefaultLinetype();

  miutil::KeyValue_v opts;

  if (cp->type == ConfiguredPlot::T_CONTOUR) {
    if (po.colours.size() < 2 || po.colours.size() > 3) {
      miutil::add(opts, PlotOptions::key_colour, po.linecolour.Name());
    } else {
      std::ostringstream ostr;
      ostr << po.colours[0].Name();
      for (unsigned int j = 1; j < po.colours.size(); j++)
        ostr << "," << po.colours[j].Name();
      miutil::add(opts, PlotOptions::key_colours, ostr.str());
    }
    miutil::add(opts, PlotOptions::key_linetype, po.linetype.name);
    miutil::add(opts, PlotOptions::key_linewidth, po.linewidth);
    miutil::add(opts, PlotOptions::key_lineinterval, po.lineinterval);
    if (po.zeroLine)
      miutil::add(opts, PlotOptions::key_zeroLine, po.zeroLine);
    miutil::add(opts, PlotOptions::key_lineSmooth, po.lineSmooth);
    miutil::add(opts, PlotOptions::key_valueLabel, po.valueLabel);
    miutil::add(opts, PlotOptions::key_labelSize, po.labelSize);
    miutil::add(opts, PlotOptions::key_basevalue, po.base);
    if (po.minvalue > -1.e+35)
      miutil::add(opts, PlotOptions::key_minvalue, po.minvalue);
    else
      miutil::add(opts, PlotOptions::key_minvalue, "off");
    if (po.maxvalue < 1.e+35)
      miutil::add(opts, PlotOptions::key_maxvalue, po.maxvalue);
    else
      miutil::add(opts, PlotOptions::key_maxvalue, "off");

    miutil::add(opts, PlotOptions::key_palettecolours, po.palettename);
    miutil::add(opts, PlotOptions::key_table, po.table);
    miutil::add(opts, PlotOptions::key_repeat, po.repeat);

  } else if (cp->type == ConfiguredPlot::T_WIND) {
    miutil::add(opts, PlotOptions::key_colour, po.linecolour.Name());
    miutil::add(opts, PlotOptions::key_linewidth, po.linewidth);
    miutil::add(opts, PlotOptions::key_density, po.density);
  } else if (cp->type == ConfiguredPlot::T_VECTOR) {
    miutil::add(opts, PlotOptions::key_colour, po.linecolour.Name());
    miutil::add(opts, PlotOptions::key_linewidth, po.linewidth);
    miutil::add(opts, PlotOptions::key_density, po.density);
    miutil::add(opts, PlotOptions::key_vectorunit, po.vectorunit);
    miutil::add(opts, PlotOptions::key_vectorscale_x, po.vectorscale_x);
    miutil::add(opts, PlotOptions::key_vectorscale_y, po.vectorscale_y);
    miutil::add(opts, PlotOptions::key_vectorthickness, po.vectorthickness);
  } else {
    miutil::add(opts, PlotOptions::key_colour, po.linecolour.Name());
  }

  return opts;
}

const Setup::string_string_m& Setup::getModelOptions(const std::string& name) const
{
  ModelOptions_m::const_iterator it = mModelOptions.find(name);
  if (it != mModelOptions.end())
    return it->second;
  else
    return EMPTY_STRING_STRING_MAP;
}

// ########################################################################

bool vc_configure(Setup_p setup, const string_v& sources,
    const string_v& computations, const string_v& plots)
{
  const SyntaxError_v errS = setup->configureSources(sources),
      errC = setup->configureComputations(computations),
      errP = setup->configurePlots(plots);
  for (const SyntaxError& e : errS)
      METLIBS_LOG_ERROR("source " << e.line << ":" << e.message);
  for (const SyntaxError& e : errC)
      METLIBS_LOG_ERROR("config " << e.line << ":" << e.message);
  for (const SyntaxError& e : errP)
      METLIBS_LOG_ERROR("plot   " << e.line << ":" << e.message);

  return errS.empty() and errC.empty() and errP.empty();
}


} // namespace vcross
