/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "diSatPlotCommand.h"

#include "util/time_util.h"

#include <puTools/miStringFunctions.h>

#include <sstream>

static const std::string SAT = "SAT";

SatPlotCommand::SatPlotCommand()
    : KVListPlotCommand(SAT)
    , mosaic(false)
    , timediff(defaultTimediff)
    , cut(defaultCut)
    , alphacut(defaultAlphacut)
    , alpha(defaultAlpha/255)
    , classtable(defaultClasstable)
{
}

std::string SatPlotCommand::toString() const
{
  std::ostringstream s;
  s << SAT << ' ' << image_name() // may contain spaces -- no quotes here!
    << ' ' << subtype_name() << ' ' << plotChannels;
  if (hasFileName())
    s << ' ' << miutil::kv("file", filename);
  else if (hasFileTime())
    s << ' ' << miutil::kv("time", filetime.isoTime());

  s << ' ' << miutil::kv("timediff", timediff);
  s << ' ' << miutil::kv("mosaic", mosaic);
  s << ' ' << miutil::kv("cut", cut);
  s << ' ' << miutil::kv("alphacut", alphacut);
  s << ' ' << miutil::kv("alpha", alpha);
  s << ' ' << miutil::kv("table", classtable);
  if (!coloursToHideInLegend.empty()) {
    std::ostringstream ostr;
    bool first = true;
    for (const auto& cv : coloursToHideInLegend) {
      if (!first)
        ostr << ',';
      ostr << cv.first;
      if (cv.second != 0)
        ostr << ':' << cv.second;
      first = false;
    }
    s << ' ' << miutil::kv("hide", ostr.str());
  }

  if (!all().empty())
    s << ' ' << all();
  return s.str();
}

// static
SatPlotCommand_cp SatPlotCommand::fromString(const std::string& line)
{
  miutil::KeyValue_v pin = miutil::splitKeyValue(line);
  if (pin.size() < 3)
    return SatPlotCommand_p();

  SatPlotCommand_p cmd = std::make_shared<SatPlotCommand>();

  miutil::KeyValue_v::const_iterator it = pin.begin();

  // satellite name
  if (it->hasValue())
    return SatPlotCommand_cp();
  cmd->sist.image_name = it->key();
  ++it;

  // satellite area / subproduct / filetype
  if (it->hasValue())
    return SatPlotCommand_cp();
  cmd->sist.subtype_name = it->key();
  ++it;

  if (cmd->image_name().empty() || cmd->subtype_name().empty())
    return SatPlotCommand_cp();

  // channels
  if (it->hasValue())
    return SatPlotCommand_cp();
  cmd->plotChannels = it->key();
  ++it;

  if (it != pin.end() && it->key() == "file") {
    cmd->filename = it->value();
    ++it;
  }

  for (; it != pin.end(); ++it) {
    const std::string& key = it->key();
    const std::string& value = it->value();
    if (key == "time") {
      cmd->filetime = miutil::timeFromString(value);
    } else if (key == "timediff") {
      cmd->timediff = it->toInt();
    } else if (key == "mosaic") {
      cmd->mosaic = it->toBool();
    } else if (key == "cut") {
      cmd->cut = it->toFloat();
    } else if (key == "alphacut" | key == "alfacut") {
      cmd->alphacut = it->toFloat();
    } else if (key == "alpha" | key == "alfa") {
      cmd->alpha = it->toFloat();
    } else if (key == "table") {
      cmd->classtable = it->toBool();
    } else if (key == "hide") {
      cmd->coloursToHideInLegend.clear();
      const std::vector<std::string> stokens = miutil::split(value, 0, ",");
      for (const std::string& tok : stokens) {
        const std::vector<std::string> sstokens = miutil::split(tok, 0, ":");
        const int c = miutil::to_int(sstokens[0]);
        const int v = (sstokens.size() > 1) ? miutil::to_int(sstokens[1]) : 0;
        cmd->coloursToHideInLegend[c] = v;
      }
    } else if (key != "font" && key != "face") { // ignore keys "font" and "face"
      cmd->add(*it);
    }
  }

  return cmd;
}
