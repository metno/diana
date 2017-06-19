#ifndef DISIMPLEPLOTCOMMAND_H
#define DISIMPLEPLOTCOMMAND_H

#include "diPlotCommand.h"

class StringPlotCommand : public PlotCommand
{
public:
  StringPlotCommand(const std::string& command);
  StringPlotCommand(const std::string& commandKey, const std::string& command);

  const std::string& commandKey() const override
    { return commandKey_; }

  const std::string& command() const
    { return command_; }

  const std::string toString() const override
    { return command_; }

private:
  std::string commandKey_;
  std::string command_;
};

typedef std::shared_ptr<StringPlotCommand> StringPlotCommand_p;
typedef std::shared_ptr<const StringPlotCommand> StringPlotCommand_cp;


#endif // DISIMPLEPLOTCOMMAND_H
