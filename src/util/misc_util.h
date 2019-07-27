/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2019 met.no

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

#ifndef MISC_UTIL_H
#define MISC_UTIL_H

#include <set>
#include <vector>

namespace diutil {
template <typename T> void insert_all(std::vector<T>& v, const std::vector<T>& a)
{
  v.insert(v.end(), a.begin(), a.end());
}
template <typename T> void insert_all(std::vector<T>& v, const std::set<T>& a)
{
  v.insert(v.end(), a.begin(), a.end());
}
template <typename T> void insert_all(std::set<T>& v, const std::vector<T>& a)
{
  v.insert(a.begin(), a.end());
}
template <typename T> void insert_all(std::set<T>& v, const std::set<T>& a)
{
  v.insert(a.begin(), a.end());
}

template <class T, class F>
struct static_caster
{
  T operator()(F f) { return static_cast<T>(f); }
};

template <class T, class F>
struct dynamic_caster
{
  T operator()(F f) { return dynamic_cast<T>(f); }
};

template <class T, class I, class C = static_caster<T, typename I::value_type>>
class caster_iterator : public std::iterator<std::bidirectional_iterator_tag, T>
{
public:
  caster_iterator(I iter)
      : iter_(iter)
  {
  }
  bool operator==(const caster_iterator& other) const { return iter_ == other.iter_; }
  bool operator!=(const caster_iterator& other) const { return iter_ != other.iter_; }

  caster_iterator& operator++()
  {
    iter_++;
    return *this;
  }
  caster_iterator& operator--()
  {
    iter_--;
    return *this;
  }

  T operator*() const { return C()(iter_.operator*()); }
  T* operator->() const { return C()(iter_.operator->()); }

private:
  I iter_;
};

template <class T, class C, class Cast>
class cast_adaptor
{
  typedef caster_iterator<T, typename C::iterator, Cast> iterator;

public:
  cast_adaptor(C& container)
      : container_(container)
  {
  }

  iterator begin() { return iterator(container_.begin()); }
  iterator end() { return iterator(container_.end()); }

private:
  C& container_;
};

template <class T, class C, class Cast>
class cast_adaptor_const
{
  typedef caster_iterator<const T, typename C::const_iterator, Cast> const_iterator;

public:
  cast_adaptor_const(const C& container)
      : container_(container)
  {
  }

  const_iterator begin() const { return const_iterator(container_.begin()); }
  const_iterator end() const { return const_iterator(container_.end()); }

private:
  const C& container_;
};

template <class T, class C>
cast_adaptor<T, C, static_caster<T, typename C::value_type>> static_content_cast(C& container)
{
  return cast_adaptor<T, C, static_caster<T, typename C::value_type>>(container);
}

template <class T, class C>
cast_adaptor_const<T, C, static_caster<T, typename C::value_type>> static_content_cast(const C& container)
{
  return cast_adaptor_const<T, C, static_caster<T, typename C::value_type>>(container);
}

template <class T, class C>
cast_adaptor<T, C, dynamic_caster<T, typename C::value_type>> dynamic_content_cast(C& container)
{
  return cast_adaptor<T, C, dynamic_caster<T, typename C::value_type>>(container);
}

template <class T, class C>
cast_adaptor_const<T, C, dynamic_caster<T, typename C::value_type>> dynamic_content_cast(const C& container)
{
  return cast_adaptor_const<T, C, dynamic_caster<T, typename C::value_type>>(container);
}
} // namespace diutil

#endif // MISC_UTIL_H
