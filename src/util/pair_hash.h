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

#ifndef DIANA_UTIL_PAIR_HASH
#define DIANA_UTIL_PAIR_HASH 1

#include <functional>
#include <utility>

namespace diutil {

template <class T1, class T2>
struct pair_hash
{
  std::size_t operator()(const std::pair<T1, T2>& p) const { return std::hash<T1>()(p.first) ^ std::hash<T2>()(p.second); }
};

} // namespace  diutil

#endif // DIANA_UTIL_PAIR_HASH
