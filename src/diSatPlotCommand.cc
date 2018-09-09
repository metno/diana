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

#include <sstream>

static const std::string SAT = "SAT";

SatPlotCommand::SatPlotCommand()
    : KVListPlotCommand(SAT)
{
}

std::string SatPlotCommand::toString() const
{
  std::ostringstream s;
  s << SAT << ' ' << satellite // may contain spaces -- no quotes here!
    << ' ' << filetype << ' ' << plotChannels;
  if (!filename.empty())
    s << ' ' << miutil::kv("file", filename);
  if (!all().empty())
    s << ' ' << all();
  return s.str();
}

// static
SatPlotCommand_cp SatPlotCommand::fromString(const std::string& line)
{
  SatPlotCommand_p cmd;

  miutil::KeyValue_v pin = miutil::splitKeyValue(line);
  if (pin.size() >= 3) {
    cmd = std::make_shared<SatPlotCommand>();

    cmd->satellite = pin[0].key();
    cmd->filetype = pin[1].key();
    cmd->plotChannels = pin[2].key();

    size_t skip = 3;
    if (pin.size() > skip && pin[skip].key() == "file") {
      cmd->filename = pin[skip].value();
      skip += 1;
    }

    pin.erase(pin.begin(), pin.begin() + skip);
    cmd->add(pin);
  }

  return cmd;
}
