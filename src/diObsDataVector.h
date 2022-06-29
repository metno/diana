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

#ifndef diObsDataVector_h
#define diObsDataVector_h

#include "diObsDataContainer.h"

#include <vector>

/*! ObsDataContainer implementation based on a `std::vector`.
  */
class ObsDataVector : public ObsDataContainer
{
public:
  ~ObsDataVector();

  void clear();

  //! add one element
  void push_back();

  //! remove last element
  void pop_back();

  //! add observation data from ObsData (not very efficient)
  void add(const ObsData& od);

  //! preallocate memory to avoid reallocations if the number of observations is known
  void reserve(size_t size) { data_.reserve(size); }

  //! add a key, and return its index
  size_t add_key(const std::string& key);

  //! set float value, overwriting
  void put_float(size_t obs_index, size_t key_index, float value);

  //! set float value, not overwriting
  void put_new_float(size_t obs_index, size_t key_index, float value);

  //! set a string value, overwriting
  void put_string(size_t obs_index, size_t key_index, std::string&& value);

  //! set a string value, overwriting
  void put_string(size_t obs_index, size_t key_index, const std::string& value);

  // implementation of ObsDataContainer

  bool empty() const override { return data_.empty(); }
  size_t size() const override { return data_.size(); }
  ObsDataRef at(size_t idx) const override { return ObsDataRef(*this, idx); }

  const ObsDataBasic& basic(size_t i) const override { return data_[i].basic; }
  ObsDataBasic& basic(size_t i) { grow(i); return data_[i].basic; }
  const ObsDataMetar& metar(size_t i) const override { return data_[i].metar; }
  ObsDataMetar& metar(size_t i) { grow(i); return data_[i].metar; }

  std::vector<std::string> get_keys() const override;
  const std::string* get_string(size_t i, const std::string& key) const override;
  const float* get_float(size_t i, const std::string& key) const override;
  const float* get_float(size_t i, size_t key_index) const;

private:
  //! ensure that index `i` is valid
  void grow(size_t i);

private:
  typedef std::string const* string_cx;

  /*! Comparator for string pointers, used in `ObsDataX::fdata` and `ObsDataX::stringdata`
   * to copy string pointers instead of complete strings.
   */
  struct CompareStringPointers
  {
    bool operator()(string_cx a, string_cx b) const { return *a < *b; }
  };

  typedef std::map<string_cx, float, CompareStringPointers> fdata_t;
  typedef std::map<string_cx, std::string, CompareStringPointers> stringdata_t;

  /*! Stores for values for a single observation.
   *
   * Corresponds to "ObsData" with string keys in `fdata` and `stringdata` replaced by
   * pointers to strings to avoid repetition of identical strings.
   */
  struct ObsDataX
  {
    ObsDataBasic basic;
    ObsDataMetar metar;
    fdata_t fdata;
    stringdata_t stringdata;
  };

  typedef std::map<std::string, size_t> keys_t;

  //! keys used for observation values, mapping from key string to index
  keys_t keys_;

  //! pointers to key strings in `keys_`, used for access by key index in `get_float(..., key_index)`
  std::vector<string_cx> key_pointers_;

  //! actual observation data
  std::vector<ObsDataX> data_;
};

typedef std::shared_ptr<ObsDataVector> ObsDataVector_p;
typedef std::shared_ptr<const ObsDataVector> ObsDataVector_cp;

#endif // diObsDataVector_h
