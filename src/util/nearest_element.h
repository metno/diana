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

#ifndef DIANA_UTIL_NEAREST_ELEMENT_H
#define DIANA_UTIL_NEAREST_ELEMENT_H 1

#include <functional>
#include <map>
#include <set>

namespace diutil {

namespace detail {

template <class C>
struct nearest_element_helper
{
};

template <class K, class C, class A>
struct nearest_element_helper<std::set<K, C, A>>
{
  typedef std::set<K, C, A> container_type;
  typedef typename container_type::key_type element_value_type;
  typedef typename container_type::const_iterator const_iterator;
  element_value_type key(const_iterator it) const { return *it; }
};

template <class K, class T, class C, class A>
struct nearest_element_helper<std::map<K, T, C, A>>
{
  typedef std::map<K, T, C, A> container_type;
  typedef typename container_type::key_type element_value_type;
  typedef typename container_type::const_iterator const_iterator;
  element_value_type key(const_iterator it) const { return it->first; }
};

} // namespace detail

template <class C, class V, class D = std::minus<V>>
typename C::const_iterator nearest_element(const C& data, const V& value, const D& diff = D())
{
  const detail::nearest_element_helper<C> helper;
  typename C::const_iterator itBest = data.lower_bound(value);
  if (itBest == data.end()) { // after last, nearest must be last unless empty
    if (itBest != data.begin())
      --itBest;
  } else if (itBest != data.begin() && helper.key(itBest) != value) {
    // not first and not equal: need to compare with difference for previous element
    typename C::const_iterator itPrev = itBest;
    --itPrev;
    if (diff(value, helper.key(itPrev)) <= diff(helper.key(itBest), value))
      itBest = itPrev;
  }
  return itBest;
}

} // namespace diutil

#endif // DIANA_UTIL_NEAREST_ELEMENT_H
