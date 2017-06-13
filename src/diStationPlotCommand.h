#ifndef DISTATIONPLOTCOMMAND_H
#define DISTATIONPLOTCOMMAND_H

#include "diPlotCommand.h"

class StationPlotCommand : public PlotCommand
{
public:
  StationPlotCommand();

  const std::string& commandKey() const override;
  const std::string toString() const override;

  std::string name;
  std::string url;
  std::string select;

  static std::shared_ptr<const StationPlotCommand> parseLine(const std::string& line);
};

typedef std::shared_ptr<StationPlotCommand> StationPlotCommand_p;
typedef std::shared_ptr<const StationPlotCommand> StationPlotCommand_cp;

#endif // DISTATIONPLOTCOMMAND_H
