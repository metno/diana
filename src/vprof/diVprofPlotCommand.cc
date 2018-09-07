/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018 met.no

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

#include "diVprofPlotCommand.h"

#include "util/string_util.h"

#include <puTools/miStringFunctions.h>

#include <sstream>

namespace {
VprofPlotCommand_p prefixCommandFromString(const std::string& typetext, VprofPlotCommand::Type type, const std::string& text)
{
  if (!diutil::startswith(text, typetext + "="))
    return VprofPlotCommand_p();

  VprofPlotCommand_p cmd = std::make_shared<VprofPlotCommand>(type);
  cmd->setItems(miutil::split(miutil::trimmed(text.substr(typetext.length() + 1)), ","));
  return cmd;
}

VprofPlotCommand_p optionCommandFromString(const std::string& typetext, VprofPlotCommand::Type type, const std::string& text)
{
  if (diutil::startswith(text, typetext + " "))
    return std::make_shared<VprofPlotCommand>(type, miutil::trimmed(text.substr(typetext.length() + 1)));
  else
    return VprofPlotCommand_p();
}

const std::string t_VPROF = "VPROF";

const std::string k_STATION = "STATION";
const std::string k_MODEL = "MODEL";
const std::string k_MODELS = k_MODEL + "S";

const std::string k_DIAGRAM = "VPROF_DIAGRAM";
const std::string k_BOX = "VPROF_BOX";
const std::string k_GRAPH = "VPROF_GRAPH";
const std::string k_DATA = "VPROF_DATA";
} // namespace

VprofPlotCommand::VprofPlotCommand(Type type)
    : KVListPlotCommand(t_VPROF)
    , type_(type)
{
}

VprofPlotCommand::VprofPlotCommand(Type type, const std::string& command)
    : KVListPlotCommand(t_VPROF, command)
    , type_(type)
{
}

std::string VprofPlotCommand::toString() const
{
  std::ostringstream out;
  if (type() == DIAGRAM) {
    out << k_DIAGRAM << ' ' << all();
  } else if (type() == BOX) {
    out << k_BOX << ' ' << all();
  } else if (type() == GRAPH) {
    out << k_GRAPH << ' ' << all();
  } else if (type() == DATA) {
    out << k_DATA << ' ' << all();
  } else if (type() == OPTIONS) {
    out << all();
  } else {
    if (type() == MODELS) {
      out << k_MODELS;
    } else {
      out << k_STATION;
    }
    out << '=';
    bool first = true;
    for (const std::string& i : items_) {
      if (!first)
        out << ',';
      first = false;
      out << i;
    }
  }
  return out.str();
}

// static
VprofPlotCommand_cp VprofPlotCommand::fromString(const std::string& text)
{
  if (text.empty() || text == t_VPROF)
    return VprofPlotCommand_cp();

  if (text == "OBSERVATION.ON" || text == "OBSERVATION.OFF") {
    // ignore
    return VprofPlotCommand_cp();
  }

  if (VprofPlotCommand_cp cmd = optionCommandFromString(k_DIAGRAM, VprofPlotCommand::DIAGRAM, text))
    return cmd;
  if (VprofPlotCommand_cp cmd = optionCommandFromString(k_BOX, VprofPlotCommand::BOX, text))
    return cmd;
  if (VprofPlotCommand_cp cmd = optionCommandFromString(k_GRAPH, VprofPlotCommand::GRAPH, text))
    return cmd;
  if (VprofPlotCommand_cp cmd = optionCommandFromString(k_DATA, VprofPlotCommand::DATA, text))
    return cmd;

  if (VprofPlotCommand_cp cmd = prefixCommandFromString(k_STATION, VprofPlotCommand::STATION, text))
    return cmd;
  if (VprofPlotCommand_cp cmd = prefixCommandFromString(k_MODEL, VprofPlotCommand::MODELS, text))
    return cmd;
  if (VprofPlotCommand_cp cmd = prefixCommandFromString(k_MODELS, VprofPlotCommand::MODELS, text))
    return cmd;

  VprofPlotCommand_p cmd = std::make_shared<VprofPlotCommand>(VprofPlotCommand::OPTIONS);
  // there is no prefix for this command, split the complete text
  cmd->add(miutil::splitKeyValue(text));
  return cmd;
}
