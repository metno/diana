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

#include "diFieldPlotCommand.h"

#include "diCommandParser.h"
#include "diFieldUtil.h"
#include "util/misc_util.h"

#include <sstream>

namespace {
const std::string FIELD = "FIELD";
const std::string EDITFIELD = "EDITFIELD";

void extractFieldSpec(const miutil::KeyValue_v& kvs, FieldPlotCommand::FieldSpec& fs, miutil::KeyValue_v& options)
{
  for (const miutil::KeyValue& kv : kvs) {
    if (kv.key().empty())
      continue;
    if (kv.key() == "model") {
      fs.model = kv.value();
    } else if (kv.key() == "reftime") {
      fs.reftime = kv.value();
    } else if (kv.key() == "refoffset") {
      if (CommandParser::isInt(kv.value()))
        fs.refoffset = kv.toInt();
    } else if (kv.key() == "refhour") {
      if (CommandParser::isInt(kv.value()))
        fs.refhour = kv.toInt();
    } else if (kv.key() == "plot") {
      fs.plot = kv.value();
    } else if (kv.key() == "parameter") {
      fs.parameters.push_back(kv.value());
    } else if (kv.key() == "vcoord" || kv.key() == "vccor") {
      fs.vcoord = kv.value();
    } else if (kv.key() == "level" || kv.key() == "vlevel") {
      fs.vlevel = kv.value();
    } else if (kv.key() == "ecoord") {
      fs.ecoord = kv.value();
    } else if (kv.key() == "elevel") {
      fs.elevel = kv.value();
    } else if (kv.key() == "alltimesteps") {
      fs.allTimeSteps = kv.toBool();
    } else if (kv.key() == "hour.offset") {
      if (CommandParser::isInt(kv.value()))
        fs.hourOffset = kv.toInt();
    } else if (kv.key() == "hour.diff") {
      if (CommandParser::isInt(kv.value()))
        fs.hourDiff = kv.toInt();
    } else if (kv.key() != "unknown") {
      options << kv;
    }
  }
}
} // namespace

FieldPlotCommand::FieldSpec::FieldSpec()
    : refoffset(0)
    , refhour(-1)
    , allTimeSteps(true)
    , hourOffset(0)
    , hourDiff(0)
{
}

miutil::KeyValue_v FieldPlotCommand::FieldSpec::toKV() const
{
  miutil::KeyValue_v ostr;
  miutil::add(ostr, "model", model);
  if (!reftime.empty())
    miutil::add(ostr, "reftime", reftime);
  if (isPredefinedPlot()) {
    miutil::add(ostr, "plot", plot);
  } else {
    for (const std::string& p : parameters)
      miutil::add(ostr, "parameter", p);
  }

  if (!vlevel.empty()) {
    if (!vcoord.empty())
      miutil::add(ostr, "vcoord", vcoord);
    miutil::add(ostr, "vlevel", vlevel);
  }
  if (!elevel.empty()) {
    if (!ecoord.empty())
      miutil::add(ostr, "ecoord", vcoord);
    miutil::add(ostr, "elevel", elevel);
  }
  if (!allTimeSteps)
    miutil::add(ostr, "alltimesteps", allTimeSteps);

  if (hourOffset != 0)
    miutil::add(ostr, "hour.offset", hourOffset);
  if (hourDiff != 0)
    miutil::add(ostr, "hour.diff", hourDiff);
  return ostr;
}

const std::string& FieldPlotCommand::FieldSpec::name() const
{
  if (isPredefinedPlot())
    return plot;
  else
    return parameters.front();
}

// ========================================================================

FieldPlotCommand::FieldPlotCommand(bool edit)
    : isEdit(edit)
{
}

const std::string& FieldPlotCommand::commandKey() const
{
  return isEdit ? EDITFIELD : FIELD;
}

void FieldPlotCommand::clearOptions()
{
  options_.clear();
}

void FieldPlotCommand::addOptions(const miutil::KeyValue_v& opts)
{
  extractFieldSpec(opts, field, options_);
  if (hasMinusField()) {
    miutil::KeyValue_v dummy_options;
    extractFieldSpec(opts, minus, dummy_options);
  }
}

miutil::KeyValue_v FieldPlotCommand::toKV() const
{
  miutil::KeyValue_v kv;
  const bool has_minus = hasMinusField();
  if (has_minus)
    kv << miutil::KeyValue("(");
  kv << field.toKV();
  if (has_minus) {
    kv << miutil::KeyValue("-");
    kv << minus.toKV();
    kv << miutil::KeyValue(")");
  }
  diutil::insert_all(kv, options_);
  if (!time.empty())
    kv << miutil::KeyValue("time", time);
  return kv;
}

std::string FieldPlotCommand::toString() const
{
  std::ostringstream out;
  out << (isEdit ? EDITFIELD : FIELD);
  out << ' ' << toKV();
  return out.str();
}

// static
FieldPlotCommand_cp FieldPlotCommand::fromKV(const miutil::KeyValue_v& kvs, bool edit)
{
  FieldPlotCommand_p cmd = std::make_shared<FieldPlotCommand>(edit);
  miutil::KeyValue_v field, minus;
  if (splitDifferenceCommandString(kvs, field, minus)) {
    extractFieldSpec(field, cmd->field, cmd->options_);
    miutil::KeyValue_v dummy_options;
    extractFieldSpec(minus, cmd->minus, dummy_options);
  } else {
    extractFieldSpec(kvs, cmd->field, cmd->options_);
  }
  return cmd;
}

// static
FieldPlotCommand_cp FieldPlotCommand::fromString(const std::string& text, bool edit)
{
  return fromKV(miutil::splitKeyValue(text), edit);
}
