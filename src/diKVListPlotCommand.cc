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

#include "diKVListPlotCommand.h"
#include "util/misc_util.h"

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

// static
KVListPlotCommand_p KVListPlotCommand::fromString(const std::string& text, const std::string& commandKey)
{
  return std::make_shared<KVListPlotCommand>(commandKey, text);
}

std::string KVListPlotCommand::toString() const
{
  std::ostringstream out;
  out << commandKey_;
  if (!keyValueList_.empty())
    out << ' ';
  out << keyValueList_;
  return out.str();
}

KVListPlotCommand& KVListPlotCommand::add(const std::string& key, const std::string& value)
{
  return add(miutil::KeyValue(key, value));
}

KVListPlotCommand& KVListPlotCommand::add(const miutil::KeyValue& kv)
{
  keyValueList_.push_back(kv);
  return *this;
}

KVListPlotCommand& KVListPlotCommand::add(const miutil::KeyValue_v& kvs)
{
  diutil::insert_all(keyValueList_, kvs);
  return *this;
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
