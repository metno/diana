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

#include "diObsDataRotated.h"

#include "util/misc_util.h"

ObsDataRotated::ObsDataRotated(ObsDataContainer_cp c)
    : container_(c)
{
  stringdata_plot_.resize(size());
  fdata_plot_.resize(size());
  fdata_rotated_.resize(size());
}

ObsDataRef ObsDataRotated::at(size_t idx) const
{
  if (stringdata_plot_[idx].empty() && fdata_plot_[idx].empty() && fdata_rotated_[idx].empty())
    return container_->at(idx);
  else
    return ObsDataRef(*this, idx);
}

std::vector<std::string> ObsDataRotated::get_keys() const
{
  std::set<std::string> key_names;
  diutil::insert_all(key_names, container_->get_keys());
  for (size_t i=0; i<size(); ++i) {
    for (const auto& kv : stringdata_plot_[i])
      key_names.insert(kv.first);
    for (const auto& kv : fdata_plot_[i])
      key_names.insert(kv.first);
    for (const auto& kv : fdata_rotated_[i])
      key_names.insert(kv.first);
  }
  return std::vector<std::string>(key_names.begin(), key_names.end());
}

const std::string* ObsDataRotated::get_string(size_t i, const std::string& key) const
{
  const auto it = stringdata_plot_[i].find(key);
  if (it != stringdata_plot_[i].end())
    return &it->second;

  return container_->get_string(i, key);
}

const float* ObsDataRotated::get_float(size_t i, const std::string& key) const
{
  const auto it = fdata_rotated_[i].find(key);
  if (it != fdata_rotated_[i].end())
    return &it->second;

  return get_unrotated_float(i, key);
}

const float* ObsDataRotated::get_unrotated_float(size_t i, const std::string& key) const
{
  const auto it = fdata_plot_[i].find(key);
  if (it != fdata_plot_[i].end())
    return &it->second;

  return container_->get_float(i, key);
}

void ObsDataRotated::put_rotated_float(size_t i, const std::string& key, float value)
{
  fdata_rotated_[i][key] = value;
}

void ObsDataRotated::put_float(size_t i, const std::string& key, float value)
{
  fdata_plot_[i][key] = value;
}

void ObsDataRotated::put_string(size_t i, const std::string& key, const std::string& value)
{
  stringdata_plot_[i][key] = value;
}
