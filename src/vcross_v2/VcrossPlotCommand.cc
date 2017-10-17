#include "VcrossPlotCommand.h"

#include "util/string_util.h"

#include <puTools/miStringFunctions.h>

#include <sstream>

VcrossPlotCommand::VcrossPlotCommand(Type type)
  : KVListPlotCommand("VCROSS")
  , type_(type)
{
}

const std::string VcrossPlotCommand::toString() const
{
  std::ostringstream out;
  if (type() == FIELD) {
    out << "VCROSS";
    if (!all().empty())
      out << ' ' << all();
  } else if (type() == CROSSECTION || type() == CROSSECTION_LONLAT || type() == TIMEGRAPH || type() == TIMEGRAPH_LONLAT) {
    if (!all().empty()) {
      const miutil::KeyValue& kv = get(0);
      out << kv.key() << '=' << kv.value(); // no quotes
    }
  } else {
    // no command key!
    out << all();
  }
  return out.str();
}

namespace {
VcrossPlotCommand_p prefixCommandFromString(const std::string& typetext, VcrossPlotCommand::Type type, const std::string& text)
{
  if (!diutil::startswith(text, typetext + "="))
    return VcrossPlotCommand_p();

  VcrossPlotCommand_p cmd = std::make_shared<VcrossPlotCommand>(type);
  cmd->add(typetext, miutil::trimmed(text.substr(typetext.length()+1)));
  return cmd;
}
} // namespace

// static
VcrossPlotCommand_cp VcrossPlotCommand::fromString(const std::string& text)
{
  if (text.empty())
    return VcrossPlotCommand_cp();

  if (VcrossPlotCommand_cp cmd = prefixCommandFromString("CROSSECTION", VcrossPlotCommand::CROSSECTION, text))
    return cmd;
  if (VcrossPlotCommand_cp cmd = prefixCommandFromString("CROSSECTION_LONLAT_DEG", VcrossPlotCommand::CROSSECTION_LONLAT, text))
    return cmd;
  if (VcrossPlotCommand_cp cmd = prefixCommandFromString("TIMEGRAPH", VcrossPlotCommand::TIMEGRAPH, text))
    return cmd;
  if (VcrossPlotCommand_cp cmd = prefixCommandFromString("TIMEGRAPH_LONLAT_DEG", VcrossPlotCommand::TIMEGRAPH_LONLAT, text))
    return cmd;

  miutil::KeyValue_v kvs = miutil::splitKeyValue(text);
  VcrossPlotCommand::Type type;
  if (diutil::startswith(text, "VCROSS ")) {
    type = VcrossPlotCommand::FIELD;
    kvs.erase(kvs.begin()); // erase "VCROSS"
  } else {
    type = VcrossPlotCommand::OPTIONS;
  }
  VcrossPlotCommand_p cmd = std::make_shared<VcrossPlotCommand>(type);
  cmd->add(kvs);
  return cmd;
}

