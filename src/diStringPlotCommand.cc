#include "diStringPlotCommand.h"

#include <puTools/miStringFunctions.h>

StringPlotCommand::StringPlotCommand(const std::string& commandKey, const std::string& command)
  : commandKey_(commandKey)
  , command_(command)
{
  if (commandKey_.empty()) {
    std::vector<std::string> vs = miutil::split(command_, " ");
    commandKey_ = miutil::to_upper(vs[0]);
  }
}

StringPlotCommand::StringPlotCommand(const std::string& command)
  : command_(command)
{
  std::vector<std::string> vs = miutil::split(command_, " ");
  commandKey_ = miutil::to_upper(vs[0]);
}
