#include "diStationPlotCommand.h"

#include <puTools/miStringFunctions.h>
#include <boost/algorithm/string/join.hpp>
#include <sstream>

static const std::string STATION = "STATION";

StationPlotCommand::StationPlotCommand()
{
}

const std::string& StationPlotCommand::commandKey() const
{
  return STATION;
}

const std::string StationPlotCommand::toString() const
{
  std::ostringstream s;
  s << STATION
    << ' ' << name // may contain spaces -- no quotes here!
    << ' ' << url
    << ' ' << select;
  return s.str();
}

// static
StationPlotCommand_cp StationPlotCommand::parseLine(const std::string& line)
{
  StationPlotCommand_p cmd = std::make_shared<StationPlotCommand>();
  std::vector<std::string> pieces = miutil::split(line, " ");
  pieces.erase(pieces.begin()); // STATION

  cmd->select = pieces.back();
  pieces.pop_back();

  cmd->url = pieces.back();
  pieces.pop_back();

  cmd->name = boost::algorithm::join(pieces, " ");
  return cmd;
}
