#ifndef DIPLOTCOMMAND_H
#define DIPLOTCOMMAND_H

#include <memory>
#include <string>
#include <vector>

class PlotCommand
{
public:
  PlotCommand();
  virtual ~PlotCommand();

  virtual const std::string& commandKey() const = 0;
  virtual const std::string toString() const = 0;
};

typedef std::shared_ptr<PlotCommand> PlotCommand_p;
typedef std::shared_ptr<const PlotCommand> PlotCommand_cp;

typedef std::vector<PlotCommand_p> PlotCommand_pv;
typedef std::vector<PlotCommand_cp> PlotCommand_cpv;

extern PlotCommand_cp makeCommand(const std::string& text);
extern PlotCommand_cpv makeCommands(const std::vector<std::string>& text, bool vcross=false);

#endif // DIPLOTCOMMAND_H
