
#include "diSatManagerBase.h"

SatManagerBase::SatManagerBase() {}

SatManagerBase::~SatManagerBase() {}

void SatManagerBase::getCapabilitiesTime(plottimes_t& normalTimes, int& timediff, const PlotCommand_cp& pinfo)
{
  timediff=0;

  SatPlotCommand_cp cmd = std::dynamic_pointer_cast<const SatPlotCommand>(pinfo);
  if (!cmd)
    return;

  timediff = cmd->timediff;

  if (!cmd->hasFileName()) { // FIXME should this not be cmd->isAuto()?
    for (const auto& fi : getFiles(cmd->sist, true))
      normalTimes.insert(fi.time);
#if 0 // TODO use constant time if not cmd->isAuto()?
  } else if (cmd->hasFileTime()) {
    normalTimes.insert(cmd->filetime);
#endif
  }
}
