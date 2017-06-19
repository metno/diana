#include "diLabelPlotCommand.h"

LabelPlotCommand::LabelPlotCommand()
  : KVListPlotCommand("LABEL")
{
}

LabelPlotCommand::LabelPlotCommand(const std::string& text)
  : KVListPlotCommand("LABEL")
{
  add(miutil::splitKeyValue(text, true)); // keep quotes
}

LabelPlotCommand& LabelPlotCommand::add(const std::string& key, const std::string& value)
{
  return add(miutil::KeyValue(key, value, true));
}

LabelPlotCommand& LabelPlotCommand::add(const miutil::KeyValue& kv)
{
  if (!kv.keptQuotes())
    KVListPlotCommand::add(kv);
  else
    KVListPlotCommand::add(miutil::KeyValue(kv.key(), kv.value(), true));
  return *this;
}

LabelPlotCommand& LabelPlotCommand::add(const miutil::KeyValue_v& kvs)
{
  for (const miutil::KeyValue& kv : kvs)
    add(kv);
  return *this;
}
