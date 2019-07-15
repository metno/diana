/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2019 met.no

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

#include "diObjectsPlotCommand.h"

#include "util/misc_util.h"
#include "util/time_util.h"

#include <puTools/miStringFunctions.h>

#include <sstream>

static const std::string OBJECTS = "OBJECTS";

ObjectsPlotCommand::ObjectsPlotCommand()
    : timeDiff(0)
    , alpha(255)
    , newfrontlinewidth(0)
    , fixedsymbolsize(0)
{
}

miutil::KeyValue_v ObjectsPlotCommand::toKV() const
{
  miutil::KeyValue_v kvs;
  add(kvs, "name", objectname);
  add(kvs, "types", objecttypes);
  if (!symbolfilter.empty())
    add(kvs, "symbolfilter", symbolfilter);
  if (!file.empty())
    add(kvs, "file", file);
  if (!time.undef())
    add(kvs, "time", miutil::stringFromTime(time, true));
  if (timeDiff > 0)
    add(kvs, "timediff", timeDiff);
  if (alpha != 255)
    add(kvs, "alpha", alpha / 255.0f);
  if (newfrontlinewidth != 0)
    add(kvs, "frontlinewidth", newfrontlinewidth);
  if (fixedsymbolsize != 0)
    add(kvs, "fixedsymbolsize", fixedsymbolsize);

  return kvs;
}

const std::string& ObjectsPlotCommand::commandKey() const
{
  return OBJECTS;
}

std::string ObjectsPlotCommand::toString() const
{
  std::ostringstream s;
  s << OBJECTS << ' ' << toKV();
  return s.str();
}

// static
ObjectsPlotCommand_cp ObjectsPlotCommand::fromString(const std::string& line)
{
  ObjectsPlotCommand_p cmd = std::make_shared<ObjectsPlotCommand>();
  for (const miutil::KeyValue& kv : miutil::splitKeyValue(line)) {
    const std::string& key = kv.key();
    const std::string& value = kv.value();
    if (kv.key() == "types") {
      cmd->objecttypes = miutil::split(value, ",");
    } else if (key == "file") {
      cmd->file = value;
    } else if (key == "name") {
      cmd->objectname = value;
    } else if (key == "time") {
      cmd->time = miutil::timeFromString(value);
    } else if (key == "timediff") {
      cmd->timeDiff = kv.toInt();
    } else if (key == "alpha" || key == "alfa") {
      cmd->alpha = int(kv.toFloat() * 255);
    } else if (key == "frontlinewidth") {
      cmd->newfrontlinewidth = kv.toInt();
    } else if (key == "fixedsymbolsize") {
      cmd->fixedsymbolsize = kv.toInt();
    } else if (key == "symbolfilter") {
      diutil::insert_all(cmd->symbolfilter, miutil::split(value, ","));
    }
  }
  return cmd;
}
