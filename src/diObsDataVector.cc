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

#include "diObsDataVector.h"

#define MILOGGER_CATEGORY "diana.ObsDataVector"
#include <miLogger/miLogging.h>

ObsDataVector::~ObsDataVector() {}

void ObsDataVector::add(const ObsData& od)
{
  const auto obs_idx = data_.size();
  basic(obs_idx) = od.basic();
  metar(obs_idx) = od.metar();
  for (const auto& fkv : od.fdata)
    put_float(obs_idx, add_key(fkv.first), fkv.second);
  for (const auto& skv : od.stringdata)
    put_string(obs_idx, add_key(skv.first), skv.second);
  if (obs_idx >= data_.size())
    throw std::runtime_error("no size increase");
}

size_t ObsDataVector::add_key(const std::string& key)
{
  const auto ins = keys_.insert(std::make_pair(key, keys_.size()));
  if (ins.second)
    key_pointers_.push_back(&ins.first->first);
  return ins.first->second;
}

void ObsDataVector::put_float(size_t obs_index, size_t key_index, float value)
{
  grow(obs_index);
  data_[obs_index].fdata[key_pointers_[key_index]] = value;
}

void ObsDataVector::put_new_float(size_t obs_index, size_t key_index, float value)
{
  grow(obs_index);
  data_[obs_index].fdata.insert(std::make_pair(key_pointers_[key_index], value));
}

void ObsDataVector::put_string(size_t obs_index, size_t key_index, std::string&& value)
{
  grow(obs_index);
  data_[obs_index].stringdata[key_pointers_[key_index]] = std::move(value);
}

void ObsDataVector::put_string(size_t obs_index, size_t key_index, const std::string& value)
{
  grow(obs_index);
  data_[obs_index].stringdata[key_pointers_[key_index]] = value;
}

void ObsDataVector::grow(size_t i)
{
  if (data_.size() < i + 1)
    data_.resize(i + 1);
}

void ObsDataVector::push_back()
{
  data_.push_back(ObsDataX());
}

void ObsDataVector::pop_back()
{
  if (!data_.empty())
    data_.pop_back();
}

void ObsDataVector::clear()
{
  keys_.clear();
  data_.clear();
}

std::vector<std::string> ObsDataVector::get_keys() const
{
  std::vector<std::string> key_names;
  key_names.reserve(keys_.size());
  for (const auto& ki : keys_)
    key_names.push_back(ki.first);
  return key_names;
}

const std::string* ObsDataVector::get_string(size_t i, const std::string& key) const
{
  if (i >= data_.size())
    return nullptr;

  const auto& sd = data_[i].stringdata;
  const auto iti = sd.find(&key);
  if (iti == sd.end())
    return nullptr;

  return &iti->second;
}

const float* ObsDataVector::get_float(size_t i, const std::string& key) const
{
  if (i >= data_.size())
    return nullptr;

  const auto& fd = data_[i].fdata;
  const auto iti = fd.find(&key);
  if (iti == fd.end())
    return nullptr;

  return &iti->second;
}

const float* ObsDataVector::get_float(size_t i, size_t key_index) const
{
  if (i >= data_.size())
    return nullptr;

  const auto& fd = data_[i].fdata;
  const auto iti = fd.find(key_pointers_[key_index]);
  if (iti == fd.end())
    return nullptr;

  return &iti->second;
}
