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

#ifndef UTIL_LRU_CACHE_H
#define UTIL_LRU_CACHE_H 1

#include <cassert>
#include <list>
#include <unordered_map>

template <class K, class V, class KH = std::hash<K>, class KE = std::equal_to<K>>
class lru_cache
{
private:
  typedef std::list<std::pair<K, V>> items_t;
  typedef typename items_t::iterator items_it;
  typedef std::unordered_map<K, items_it, KH, KE> keys_t;

public:
  lru_cache(size_t cache_size)
    : cache_size_(cache_size) { assert(cache_size>0); }

  void put(const K& key, const V& value);

  bool has(const K& key) const { return (keys_.find(key) != keys_.end()); }

  const V& get(const K& key)
  {
    auto kit = keys_.find(key);
    assert(kit != keys_.end());
    items_.splice(items_.begin(), items_, kit->second);
    return items_.front().second;
  }

private:
  items_t items_;
  keys_t keys_;
  size_t cache_size_;
};

template <class K, class V, class KH, class KE>
void lru_cache<K, V, KH, KE>::put(const K& key, const V& value)
{
  auto kit = keys_.find(key);
  if (kit != keys_.end()) {
    items_.splice(items_.begin(), items_, kit->second);
    items_.front().second = value;
  } else {
    items_.push_front(std::make_pair(key, value));
    keys_.insert(std::make_pair(key, items_.begin()));
    if (items_.size() > cache_size_) {
      keys_.erase(items_.back().first);
      items_.pop_back();
    }
  }
}

#endif // UTIL_LRU_CACHE_H
