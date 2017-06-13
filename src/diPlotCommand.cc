#include "diPlotCommand.h"

#include "diStringPlotCommand.h"
#include "util/string_util.h"

#include <puTools/miStringFunctions.h>

PlotCommand::PlotCommand()
{
}

PlotCommand::~PlotCommand()
{
}


PlotCommand_cp makeCommandVcross(const std::string& text)
{
  return std::make_shared<StringPlotCommand>("VCROSS", text);
}

PlotCommand_cp makeCommand(const std::string& text)
{
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
