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

#include "diFieldDefined.h"

namespace difield {

const float UNDEF = 1.0e35f;

inline bool is_defined(float f)
{ return (f < UNDEF); }

ValuesDefined checkDefined(const float* data, size_t n)
{
  // check undef
  bool has_undefined = false, has_defined = false;
  for (const float* f = data; f != data + n; ++f) {
    bool defined = is_defined(*f);
    if (defined)
      has_defined = true;
    else
      has_undefined = true;
    if (has_undefined && has_defined)
      break;
  }
  if (has_undefined && has_defined)
    return SOME_DEFINED;
  else if (has_defined)
    return ALL_DEFINED;
  else
    return NONE_DEFINED;
}

ValuesDefined checkDefined(size_t n_undefined, size_t n)
{
  if (n_undefined == 0)
    return ALL_DEFINED;
  else if (n_undefined == n)
    return NONE_DEFINED;
  else
    return SOME_DEFINED;
}

ValuesDefined combineDefined(ValuesDefined a, ValuesDefined b)
{
  switch (a) {
  case ALL_DEFINED:
    return b;
  case NONE_DEFINED:
    return NONE_DEFINED;
  case SOME_DEFINED:
    return b != ALL_DEFINED ? b : SOME_DEFINED;
  }
}

} // namespace difield

const float fieldUndef = difield::UNDEF;
