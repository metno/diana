#ifndef DIPLOTCOMMANDFACTORY_H
#define DIPLOTCOMMANDFACTORY_H

#include "diPlotCommand.h"

extern PlotCommand_cp makeCommand(const std::string& text);

enum PlotCommandMode {
  PLOTCOMMANDS_FIELD,
  PLOTCOMMANDS_VCROSS,
  PLOTCOMMANDS_VPROF
};

extern PlotCommand_cpv makeCommands(const std::vector<std::string>& text, PlotCommandMode mode = PLOTCOMMANDS_FIELD);

#endif // DIPLOTCOMMANDFACTORY_H
