#include "diKVListPlotCommand.h"

#include <sstream>

const size_t KVListPlotCommand::npos;

KVListPlotCommand::KVListPlotCommand(const std::string& commandKey)
  : commandKey_(commandKey)
{
}

KVListPlotCommand::KVListPlotCommand(const std::string& commandKey, const std::string& command)
  : commandKey_(commandKey)
{
  add(miutil::splitKeyValue(command));
}

const std::string KVListPlotCommand::toString() const
{
  std::ostringstream out;
  out << commandKey_;
  if (!keyValueList_.empty())
    out << ' ';
  out << keyValueList_;
  return out.str();
}

size_t KVListPlotCommand::find(const std::string& key, size_t start) const
{
  return miutil::find(keyValueList_, key, start);
}

size_t KVListPlotCommand::rfind(const std::string& key) const
{
  return miutil::rfind(keyValueList_, key);
}

size_t KVListPlotCommand::rfind(const std::string& key, size_t start) const
{
  return miutil::rfind(keyValueList_, key, start);
}
