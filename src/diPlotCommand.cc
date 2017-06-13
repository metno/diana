#include "diPlotCommand.h"

#include "diKVListPlotCommand.h"
#include "diStringPlotCommand.h"
#include "diStationPlotCommand.h"
#include "util/string_util.h"

#include <puTools/miStringFunctions.h>

PlotCommand::PlotCommand()
{
}

PlotCommand::~PlotCommand()
{
}

namespace {
size_t identify(const std::string& commandKey, const std::string& text)
{
  if (text == commandKey)
    return commandKey.size()+1;
  else if (diutil::startswith(text, commandKey + " "))
    return commandKey.size()+1;
  else
    return 0;
}

PlotCommand_cp identifyKeyValue(const std::string& commandKey, const std::string& text)
{
  if (size_t start = identify(commandKey, text))
      return std::make_shared<const KVListPlotCommand>(commandKey, text.substr(start));
  else
    return PlotCommand_cp();
}
} // namespace

PlotCommand_cp makeCommandVcross(const std::string& text)
{
  return std::make_shared<StringPlotCommand>("VCROSS", text);
}

PlotCommand_cp makeCommand(const std::string& text)
{
  if (PlotCommand_cp c = identifyKeyValue("MAP", text))
    return c;
  if (PlotCommand_cp c = identifyKeyValue("AREA", text))
    return c;
  if (PlotCommand_cp c = identifyKeyValue("DRAWING", text))
    return c;

  if (identify("STATION", text))
    return StationPlotCommand::parseLine(text);

  return std::make_shared<StringPlotCommand>(text);
}

PlotCommand_cpv makeCommands(const std::vector<std::string>& text)
{
  PlotCommand_cpv cmds;
  cmds.reserve(text.size());

  // FIXME handle this properly, or change VCROSS commands
  bool vcross = false;

  for (const std::string& t : text) {
    const std::string tt = miutil::trimmed(t);
    if (t.empty() || tt[0] == '#')
      continue;

    if (t == "VCROSS")
      vcross = true; // remaining commands are vcross, too

    PlotCommand_cp cmd = vcross ? makeCommandVcross(tt) : makeCommand(tt);
    if (cmd->commandKey() == "VCROSS")
      vcross = true; // remaining commands are vcross, too

    cmds.push_back(cmd);
  }
  return cmds;
}
