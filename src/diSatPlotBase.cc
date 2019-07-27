
#include "diSatPlotBase.h"

#include "diSatPlotCommand.h"

#include <sstream>

SatPlotBase::SatPlotBase(SatPlotCommand_cp cmd)
    : command_(cmd)
{
}

void SatPlotBase::setCommand(SatPlotCommand_cp cmd)
{
  command_ = cmd;
  setPlotInfo(command_->all());
}

std::string SatPlotBase::getEnabledStateKey() const
{
  if (!command_)
    return "ARGHHH!";

  std::ostringstream oks;
  oks << command_->image_name() << command_->subtype_name() << command_->plotChannels << command_->filename;
  return oks.str();
}
