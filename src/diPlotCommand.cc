#include "diPlotCommand.h"

#include "diKVListPlotCommand.h"
#include "diLabelPlotCommand.h"
#include "diStringPlotCommand.h"
#include "diStationPlotCommand.h"
#include "vcross_v2/VcrossPlotCommand.h"

#include "util/string_util.h"

#include <puTools/miStringFunctions.h>

#include "config.h"

#define MILOGGER_CATEGORY "diana.PlotCommand"
#include <miLogger/miLogging.h>

PlotCommand::PlotCommand()
{
}

PlotCommand::~PlotCommand()
{
}

namespace {
//! \return position after "word" (or "word" + 1 whitespace) if text equals to or begins with "word", else 0
size_t matchFirstWord(const std::string& word, const std::string& text)
{
  const size_t len_word = word.size();
  if (text == word)
    return len_word;
  if (text.length() > len_word && std::iswspace(text[len_word]) && diutil::startswith(text, word))
    return len_word+1;
  return 0;
}

size_t identify(const std::string& commandKey, const std::string& text)
{
  if (size_t t = matchFirstWord(commandKey, text))
      return t;

#define NO_UPPER_MAJOR 3
#define NO_UPPER_MINOR 44
#if PVERSION_MAJOR == NO_UPPER_MAJOR && PVERSION_MINOR < NO_UPPER_MINOR
  if (size_t t = matchFirstWord(commandKey, miutil::to_upper(text))) {
    METLIBS_LOG_WARN("command keys must use upper case starting with diana "
                     << NO_UPPER_MAJOR << '.' << NO_UPPER_MINOR
                     << ", please use '" << commandKey << "' in '" << text << "'");
    return t;
  }
#endif

  return 0;
}

const std::vector<std::string> commandKeysKV = {
  "MAP", "AREA", "DRAWING", "FIELD", "EDITFIELD", "SAT", "OBJECTS", "OBS", "WEBMAP"
};

PlotCommand_cp identifyKeyValue(const std::string& commandKey, const std::string& text)
{
  if (size_t start = identify(commandKey, text))
      return std::make_shared<const KVListPlotCommand>(commandKey, text.substr(start));
  else
    return PlotCommand_cp();
}

PlotCommand_cp identifyLabel(const std::string& text)
{
  if (size_t start = identify("LABEL", text))
      return std::make_shared<LabelPlotCommand>(text.substr(start));
  else
    return PlotCommand_cp();
}
} // namespace

PlotCommand_cp makeCommand(const std::string& text)
{
  for (const std::string& ck : commandKeysKV) {
    if (PlotCommand_cp c = identifyKeyValue(ck, text))
      return c;
  }

  if (PlotCommand_cp c = identifyLabel(text))
    return c;

  if (identify("STATION", text))
    return StationPlotCommand::parseLine(text);

  return std::make_shared<StringPlotCommand>(text);
}

PlotCommand_cpv makeCommands(const std::vector<std::string>& text, bool vcross)
{
  PlotCommand_cpv cmds;
  cmds.reserve(text.size());

  // FIXME handle vcross properly, or change VCROSS commands

  for (const std::string& t : text) {
    const std::string tt = miutil::trimmed(t);
    if (t.empty() || tt[0] == '#')
      continue;

    if (t == "VCROSS")
      vcross = true; // remaining commands are vcross, too

    if (vcross)
      cmds.push_back(VcrossPlotCommand::fromString(tt));
    else
      cmds.push_back(makeCommand(tt));
  }
  return cmds;
}
