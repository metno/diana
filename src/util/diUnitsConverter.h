/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2022 met.no

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

#ifndef DIANA_UNITS_H
#define DIANA_UNITS_H 1

#include "diField/diValues.h"

namespace diutil {

enum UnitConvertibility {
  UNITS_MISMATCH = 0, //! no unit conversion known
  UNITS_CONVERTIBLE,  //! units are convertible with a function
  UNITS_LINEAR,       //! units are convertible with a linear transformation
  UNITS_IDENTICAL     //! units are the same
};

class UnitsConverter
{
public:
  virtual ~UnitsConverter();
  virtual UnitConvertibility convertibility() const = 0;

  virtual void convert(const double* from_begin, const double* from_end, double* to_begin) const = 0;
  virtual void convert(const float* from_begin, const float* from_end, float* to_begin) const = 0;

  double convert1(double in) const { convert(&in, &in + 1, &in); return in; }
  float convert1(float in) const { convert(&in, &in + 1, &in); return in; }
};
typedef std::shared_ptr<UnitsConverter> UnitsConverter_p;

UnitConvertibility unitConvertibility(const std::string& ua, const std::string& ub);

inline bool unitsIdentical(const std::string& ua, const std::string& ub)
{
  return unitConvertibility(ua, ub) == UNITS_IDENTICAL;
}

inline bool unitsConvertible(const std::string& ua, const std::string& ub)
{
  return unitConvertibility(ua, ub) != UNITS_MISMATCH;
}

//! result unit after multiplication or division
std::string unitsMultiplyDivide(const std::string& ua, const std::string& ub, bool multiply);

//! result unit after taking root
std::string unitsRoot(const std::string& u, int root = 2);

UnitsConverter_p unitConverter(const std::string& ua, const std::string& ub);

Values_cp unitConversion(Values_cp valuesIn, const std::string& unitIn, const std::string& unitOut);
float unitConversion(float valueIn, const std::string& unitIn, const std::string& unitOut);
bool unitConversion(const std::string& unitIn, const std::string& unitOut, size_t volume, float undefval, const float* vIn, float* vOut);

} // namespace diutil

#endif // DIANA_UNITS_H
