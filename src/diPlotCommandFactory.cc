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

#include "diPlotCommandFactory.h"

#include "diFieldPlotCommand.h"
#include "diKVListPlotCommand.h"
#include "diLabelPlotCommand.h"
#include "diStationPlotCommand.h"
#include "diStringPlotCommand.h"
#include "vcross_v2/VcrossPlotCommand.h"
#include "vprof/diVprofPlotCommand.h"

#include "util/string_util.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.PlotCommandFactory"
#include <miLogger/miLogging.h>

namespace {
//! \return position after "word" (or "word" + 1 whitespace) if text equals to or begins with "word", else 0
size_t matchFirstWord(const std::string& word, const std::string& text)
{
  const size_t len_word = word.size();
  if (text == word)
    return len_word;
  if (text.length() > len_word && std::iswspace(text[len_word]) && diutil::startswith(text, word))
    return len_word + 1;
  return 0;
}

size_t identify(const std::string& commandKey, const std::string& text)
{
  if (size_t t = matchFirstWord(commandKey, text))
    return t;

#define NO_UPPER_MAJOR 3
#define NO_UPPER_MINOR 45
  if (size_t t = matchFirstWord(commandKey, miutil::to_upper(text))) {
    METLIBS_LOG_WARN("command keys must use upper case starting with diana " << NO_UPPER_MAJOR << '.' << NO_UPPER_MINOR << ", please use '" << commandKey
                                                                             << "' in '" << text << "'");
    return t;
  }

  return 0;
}

const std::vector<std::string> commandKeysKV = {"MAP", "AREA", "DRAWING", "SAT", "OBJECTS", "OBS", "WEBMAP"};

PlotCommand_cp identifyKeyValue(const std::string& commandKey, const std::string& text)
{
  if (size_t start = identify(commandKey, text))
    return std::make_shared<const KVListPlotCommand>(commandKey, text.substr(start));
  else
    return PlotCommand_cp();
}

PlotCommand_cp identifyLabel(const std::string& text)
{
  if (size_t start = identify("LABEL", text))
    return std::make_shared<LabelPlotCommand>(text.substr(start));
  else
    return PlotCommand_cp();
}

PlotCommand_cp identifyField(const std::string& text)
{
  if (size_t start = identify("FIELD", text))
    return FieldPlotCommand::fromString(false, text.substr(start));
  if (size_t start = identify("EDITFIELD", text))
    return FieldPlotCommand::fromString(true, text.substr(start));
  return PlotCommand_cp();
}
} // namespace

PlotCommand_cp makeCommand(const std::string& text)
{
  for (const std::string& ck : commandKeysKV) {
    if (PlotCommand_cp c = identifyKeyValue(ck, text))
      return c;
  }

  if (PlotCommand_cp c = identifyField(text))
    return c;

  if (PlotCommand_cp c = identifyLabel(text))
    return c;

  if (identify("STATION", text))
    return StationPlotCommand::parseLine(text);

  return std::make_shared<StringPlotCommand>(text);
}

PlotCommand_cpv makeCommands(const std::vector<std::string>& text, PlotCommandMode mode)
{
  PlotCommand_cpv cmds;
  cmds.reserve(text.size());

  // FIXME handle vcross properly, or change VCROSS commands

  for (const std::string& t : text) {
    const std::string tt = miutil::trimmed(t);
    if (t.empty() || tt[0] == '#')
      continue;

    if (t == "VCROSS")
      mode = PLOTCOMMANDS_VCROSS; // remaining commands are vcross, too
    else if (t == "VPROF")
      mode = PLOTCOMMANDS_VPROF; // remaining commands are vprof, too

    PlotCommand_cp cmd;
    if (mode == PLOTCOMMANDS_VCROSS)
      cmd = VcrossPlotCommand::fromString(tt);
    else if (mode == PLOTCOMMANDS_VPROF)
      cmd = VprofPlotCommand::fromString(tt);
    else
      cmd = makeCommand(tt);
    if (cmd)
      cmds.push_back(cmd);
  }
  return cmds;
}
