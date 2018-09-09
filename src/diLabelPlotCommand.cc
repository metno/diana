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

#include "diLabelPlotCommand.h"

namespace {
const std::string LABEL = "LABEL";
} // namespace

LabelPlotCommand::LabelPlotCommand()
    : KVListPlotCommand(LABEL)
{
}

// static
LabelPlotCommand_p LabelPlotCommand::fromString(const std::string& text)
{
  LabelPlotCommand_p cmd = std::make_shared<LabelPlotCommand>();
  cmd->add(miutil::splitKeyValue(text, true)); // keep quotes
  return cmd;
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
