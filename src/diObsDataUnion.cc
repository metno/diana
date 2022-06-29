/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2022 met.no

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

#include "diObsDataUnion.h"

#include "util/misc_util.h"

#include <algorithm>
#include <set>

#define MILOGGER_CATEGORY "diana.ObsDataUnion"
#include <miLogger/miLogging.h>

ObsDataUnion::ObsDataUnion() {}

void ObsDataUnion::add(ObsDataContainer_cp c)
{
  if (c && !c->empty()) {
    containers_.push_back(c);
    sizes_.push_back(size() + c->size());
  }
}

ObsDataContainer_cp ObsDataUnion::single() const
{
  if (containers_.size() == 1)
    return containers_.front();
  else
    return nullptr;
}

size_t ObsDataUnion::size() const
{
  return sizes_.empty() ? 0 : sizes_.back();
}

std::pair<ObsDataContainer_cp, size_t> ObsDataUnion::find(size_t index) const
{
  const auto it = std::upper_bound(sizes_.begin(), sizes_.end(), index);
  if (it == sizes_.end())
    throw std::runtime_error("index out of bounds");
  const size_t ci = std::distance(sizes_.begin(), it);

  const size_t before = ci > 0 ? sizes_.at(ci - 1) : 0;
  const size_t after = index - before;
  return std::make_pair(containers_.at(ci), after);
}

ObsDataRef ObsDataUnion::at(size_t idx) const
{
  const auto ci = find(idx);
  return ci.first->at(ci.second);
}

const ObsDataBasic& ObsDataUnion::basic(size_t i) const
{
  const auto ci = find(i);
  return ci.first->basic(ci.second);
}

const ObsDataMetar& ObsDataUnion::metar(size_t i) const
{
  const auto ci = find(i);
  return ci.first->metar(ci.second);
}

const float* ObsDataUnion::get_float(size_t i, const std::string& key) const
{
  const auto ci = find(i);
  return ci.first->get_float(ci.second, key);
}

const std::string* ObsDataUnion::get_string(size_t i, const std::string& key) const
{
  const auto ci = find(i);
  return ci.first->get_string(ci.second, key);
}

std::vector<std::string> ObsDataUnion::get_keys() const
{
  std::set<std::string> key_names;
  for (const auto c : containers_) {
    diutil::insert_all(key_names, c->get_keys());
  }
  return std::vector<std::string>(key_names.begin(), key_names.end());
}
