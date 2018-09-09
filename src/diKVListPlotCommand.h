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

#ifndef DIKVLISTPLOTCOMMAND_H
#define DIKVLISTPLOTCOMMAND_H

#include "diPlotCommand.h"

#include "util/diKeyValue.h"

class KVListPlotCommand;
typedef std::shared_ptr<KVListPlotCommand> KVListPlotCommand_p;
typedef std::shared_ptr<const KVListPlotCommand> KVListPlotCommand_cp;

class KVListPlotCommand : public PlotCommand
{
public:
  KVListPlotCommand(const std::string& commandKey);

  // FIXME parsing should be done somewhere else
  KVListPlotCommand(const std::string& commandKey, const std::string& command);

  static KVListPlotCommand_p fromString(const std::string& text, const std::string& commandKey);

  const std::string& commandKey() const override
    { return commandKey_; }

  std::string toString() const override;

  virtual KVListPlotCommand& add(const std::string& key, const std::string& value);

  virtual KVListPlotCommand& add(const miutil::KeyValue& kv);

  virtual KVListPlotCommand& add(const miutil::KeyValue_v& kvs);

  size_t size() const
    { return keyValueList_.size(); }

  size_t find(const std::string& key, size_t start=0) const;

  size_t rfind(const std::string& key) const;

  size_t rfind(const std::string& key, size_t start) const;

  const std::string& value(size_t idx) const
    { return get(idx).value(); }

  const miutil::KeyValue& get(size_t idx) const
    { return keyValueList_.at(idx); }

  const miutil::KeyValue_v& all() const
    { return keyValueList_; }

  static const size_t npos = static_cast<size_t>(-1);

private:
  std::string commandKey_;

  miutil::KeyValue_v keyValueList_;
};

#endif // DIKVLISTPLOTCOMMAND_H
